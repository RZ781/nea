#include <headers.h>
#include "cpu.h"

struct Variable {
	// name of the variable
	std::string name;
	// whether the variable is in memory or a register
	bool memory;
	// memory address or variable name
	std::string location;
	// get the value of the variable given a cpu
	std::string get_value(CPU& cpu) {
		if (memory) {
			uint32_t address;
			// attempt to convert string to an integer
			bool valid_address = true;
			try {
				address = std::stoul(location, nullptr, 0);
			} catch (std::exception& e) {
				valid_address = false;
			}
			if (valid_address) {
				return std::to_string(cpu.get(address));
			} else {
				return "Invalid address";
			}
		} else {
			std::map<std::string, int> registers = cpu.get_registers();
			if (registers.count(location) > 0) {
				return std::to_string(registers[location]);
			} else {
				return "Invalid register";
			}
		}
	}
};

class MainWindow: public Gtk::Window {
	public:
	MainWindow();
	private:
	std::unique_ptr<CPU> cpu;
	// whether or not the user has started the vm
	bool started;
	// callback for each button
	void compile(void);
	void load(void);
	void file_loaded(const Glib::RefPtr<Gio::AsyncResult>& result);
	void start(void);
	void stop(void);
	void step(void);
	void add_variable_clicked(void);
	void add_variable_confirmed(int);
	void remove_variable_clicked(void);
	void remove_variable_confirmed(int);
	// runs while vm is started 100 times per second
	bool timeout(void);
	Gtk::Notebook notebook;
	// code tab widgets
	Gtk::Paned code_tab;
	Gtk::Box compilation_options;
	Gtk::CheckButton arm_button;
	Gtk::CheckButton riscv_button;
	Gtk::CheckButton c_button;
	Gtk::CheckButton cpp_button;
	Gtk::CheckButton asm_button;
	Gtk::CheckButton standard_library_box;
	Gtk::Button compile_button;
	Gtk::Button load_button;
	Glib::RefPtr<Gtk::AlertDialog> alert;
	Glib::RefPtr<Gtk::FileDialog> file_dialogue;
	Gtk::TextView source_code_view;
	// virtual machine tab widgets
	Gtk::Box vm_tab;
	Gtk::Frame registers_frame;
	Gtk::Grid registers;
	Gtk::Box vm_buttons;
	Gtk::Button start_button;
	Gtk::Button stop_button;
	Gtk::Button step_button;
	Gtk::Frame vm_code_frame;
	Gtk::TextView vm_code;
	std::vector<Gtk::Label> register_names;
	std::vector<Gtk::Label> register_values;
	// variable tab widgets
	Gtk::Paned variable_tab;
	Gtk::Grid variables_grid;
	std::vector<Gtk::Label> variable_names;
	std::vector<Gtk::Label> variable_locations;
	std::vector<Gtk::Label> variable_values;
	std::vector<Variable> variables;
	Gtk::Box variable_buttons;
	Gtk::Button add_variable_button;
	Gtk::Button remove_variable_button;
	// add and remove variable dialogue widgets
	Gtk::Dialog add_variable_dialogue;
	Gtk::Dialog remove_variable_dialogue;
	Gtk::Label add_name_label;
	Gtk::Entry add_name_entry;
	Gtk::Label remove_name_label;
	Gtk::Entry remove_name_entry;
	Gtk::CheckButton memory_button;
	Gtk::CheckButton register_button;
	Gtk::Label location_label;
	Gtk::Entry location_entry;
	
};

MainWindow::MainWindow():
	compilation_options(Gtk::Orientation::VERTICAL),
	arm_button("Arm"),
	riscv_button("RISC-V"),
	c_button("C"),
	cpp_button("C++"),
	asm_button("Assembly"),
	standard_library_box("Include standard library"),
	compile_button("Compile"),
	load_button("Load Executable"),
	registers_frame("Registers"),
	vm_buttons(Gtk::Orientation::VERTICAL),
	start_button("Start"),
	stop_button("Stop"),
	step_button("Step"),
	vm_code_frame("Code"),
	variable_buttons(Gtk::Orientation::VERTICAL),
	add_variable_button("Add Variable"),
	remove_variable_button("Remove Varaiable"),
	add_variable_dialogue("Add Variable", *this),
	remove_variable_dialogue("Remove Variable", *this),
	add_name_label("Variable name:"),
	remove_name_label("Variable name:"),
	memory_button("Memory"),
	register_button("Register"),
	location_label("Variable location:")

{
	started = false;
	cpu = std::make_unique<ExampleCPU>();

	// set up code tab widgets
	notebook.append_page(code_tab, "Code");
	code_tab.set_start_child(source_code_view);
	code_tab.set_end_child(compilation_options);
	compilation_options.append(arm_button);
	compilation_options.append(riscv_button);
	compilation_options.append(c_button);
	compilation_options.append(cpp_button);
	compilation_options.append(asm_button);
	compilation_options.append(standard_library_box);
	compilation_options.append(compile_button);
	compilation_options.append(load_button);
	compilation_options.set_homogeneous();
	cpp_button.set_group(c_button);
	asm_button.set_group(c_button);
	riscv_button.set_group(arm_button);
	compilation_options.set_margin(50);
	source_code_view.set_monospace();
	code_tab.set_position(500);
	compile_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::compile));
	load_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::load));

	// set up vm tab
	notebook.append_page(vm_tab, "Virtual Machine");
	vm_tab.append(registers_frame);
	vm_tab.append(vm_code_frame);
	vm_tab.append(vm_buttons);
	vm_tab.set_homogeneous();
	vm_buttons.append(start_button);
	vm_buttons.append(stop_button);
	vm_buttons.append(step_button);
	vm_buttons.set_margin(50);
	vm_buttons.set_homogeneous();
	registers_frame.set_child(registers);
	vm_code_frame.set_child(vm_code);
	vm_code.set_monospace();
	vm_code.set_editable(false);
	vm_code.get_buffer()->set_text("No program loaded");
	start_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::start));
	stop_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::stop));
	step_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::step));

	// set up variables tab
	notebook.append_page(variable_tab, "Variables");
	variable_tab.set_start_child(variables_grid);
	variable_tab.set_end_child(variable_buttons);
	variable_buttons.append(add_variable_button);
	variable_buttons.append(remove_variable_button);
	variable_buttons.set_margin(50);
	variable_buttons.set_homogeneous();
	variable_tab.set_position(500);
	add_variable_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::add_variable_clicked));
	remove_variable_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::remove_variable_clicked));

	// set up add variable popup
	register_button.set_group(memory_button);
	add_variable_dialogue.add_button("Cancel", Gtk::ResponseType::CANCEL);
	add_variable_dialogue.add_button("OK", Gtk::ResponseType::OK);
	add_variable_dialogue.set_modal(true);
	add_variable_dialogue.signal_response().connect(sigc::mem_fun(*this, &MainWindow::add_variable_confirmed));
	Gtk::Box* add_variable_content = add_variable_dialogue.get_content_area();
	add_variable_content->append(add_name_label);
	add_variable_content->append(add_name_entry);
	add_variable_content->append(memory_button);
	add_variable_content->append(register_button);
	add_variable_content->append(location_label);
	add_variable_content->append(location_entry);
	remove_variable_dialogue.add_button("Cancel", Gtk::ResponseType::CANCEL);
	remove_variable_dialogue.add_button("OK", Gtk::ResponseType::OK);
	remove_variable_dialogue.set_modal(true);
	remove_variable_dialogue.signal_response().connect(sigc::mem_fun(*this, &MainWindow::remove_variable_confirmed));
	Gtk::Box* remove_variable_content = remove_variable_dialogue.get_content_area();
	remove_variable_content->append(remove_name_label);
	remove_variable_content->append(remove_name_entry);

	// set up window
	set_title("Virtual machine");
	set_default_size(800, 600);
	set_child(notebook);
}

// run when compile button is pressed
void MainWindow::compile(void) {
	// save code to file
	std::string source_code = this->source_code_view.get_buffer()->get_text();
	std::ofstream file("source");
	file << source_code;
	file.close();
	// generate compilation command
	std::string command;
	if (riscv_button.get_active()) {
		command = "riscv32-unknown-linux-musl-gcc";
	} else {
		command = "arm-none-eabi-gcc -mcpu=cortex-m0";
	}
	if (c_button.get_active()) {
		command += " -x c";
	} else if (cpp_button.get_active()) {
		command += " -x c++";
	} else {
		command += " -x assembler-with-cpp";
	}
	command += " -static source";
	if (!this->standard_library_box.get_active()) {
		command += " -nostdlib -ffreestanding -lgcc";
	}
	// run the command and get output
	std::string errors;
	int status;
	Glib::spawn_command_line_sync(command, nullptr, &errors, &status);
	bool success = WEXITSTATUS(status) == 0;
	// display output
	std::string output;
	if (success) {
		if (errors.length() == 0) {
			output = "Compilation succeeded without warnings";
		} else {
			output = "Compilation succeeded with the following warnings:\n";
		}
		// load the cpu if compilation was successful
		if (riscv_button.get_active()) {
			cpu = std::make_unique<RiscVCPU>("./a.out");
		} else {
			cpu = std::make_unique<ArmCPU>("./a.out");
		}
	} else {
		output = "Compilation failed with the following errors:\n";
	}
	if (errors.length() > 1000) {
		errors.resize(1000);
		errors += "\n(truncated to 1000 characters)";
	}
	output += errors;
	alert = Gtk::AlertDialog::create("Compilation finished");
	alert->set_message(output);
	alert->show();
}

// run when load button is pressed
void MainWindow::load(void) {
	// create file selection dialogue
	file_dialogue = Gtk::FileDialog::create();
	// attach callback to dialogue
	file_dialogue->open(*this, sigc::mem_fun(*this, &MainWindow::file_loaded));
}

// run when file is chosen
void MainWindow::file_loaded(const Glib::RefPtr<Gio::AsyncResult>& result) {
	// attempt to get file, null if cancelled
	Glib::RefPtr<Gio::File> file;
	try {
		file = file_dialogue->open_finish(result);
	} catch (std::exception& e) {
		file = NULL;
	}
	if (file) {
		// load cpu if file selector was not cancelled
		std::string filename = file->get_path();
		if (riscv_button.get_active()) {
			cpu = std::make_unique<RiscVCPU>(filename);
		} else {
			cpu = std::make_unique<ArmCPU>(filename);
		}
	}
}

// run when start button is pressed
void MainWindow::start(void) {
	if (!started) {
		// create timeout to run 100 times per second
		Glib::signal_timeout().connect(sigc::mem_fun(*this, &MainWindow::timeout), 10);
		started = true;
	}
}

// run when stop button is pressed
void MainWindow::stop(void) {
	started = false;
}

// run when step button is pressed
void MainWindow::step(void) {
	// run one instruction
	cpu->step();
	// redraw disassembly
	std::map<std::string, int> registers = cpu->get_registers();
	int pc = registers["pc"];
	std::string code;
	std::vector<std::string> lines = cpu->disassemble(pc - 4*10, pc + 4*10);
	for (size_t i = 0; i < lines.size(); i++) {
		code += lines[i];
		code += "\n";
	}
	vm_code.get_buffer()->set_text(code);
	// delete old register widgets
	for (Gtk::Label& label: register_names) {
		label.unparent();
	}
	for (Gtk::Label& label: register_values) {
		label.unparent();
	}
	register_names.clear();
	register_values.clear();
	// create new register widgets
	int i = 0;
	for (std::pair<std::string, int> r: registers) {
		register_names.push_back(Gtk::Label(r.first));
		register_names[i].set_hexpand();
		register_values.push_back(Gtk::Label(std::to_string(r.second)));
		register_values[i].set_hexpand();
		this->registers.attach(register_names[i], 0, i);
		this->registers.attach(register_values[i], 1, i);
		i++;
	}
	// update variable values
	for (size_t i =0; i < variables.size(); i++) {
		std::string value = variables[i].get_value(*cpu);
		variable_values[i].set_label(value);
	}
}

// run 100 times per second
bool MainWindow::timeout(void) {
	// run 1000 instructions without updating ui
	for (int i = 0; i < 1000; i++) {
		cpu->step();
	}
	// run an instruction and update ui
	step();
	return started;
}

// run when add variable button is clicked
void MainWindow::add_variable_clicked(void) {
	add_variable_dialogue.show();
}

// run when add variable dialogue is confirmed
void MainWindow::add_variable_confirmed(int result) {
	// do nothing if cancelled
	if (result != Gtk::ResponseType::OK) {
		add_variable_dialogue.hide();
		return;
	}
	// check if a name was entered
	if (add_name_entry.get_text() == "") {
		alert = Gtk::AlertDialog::create();
		alert->set_message("No variable name provided.");
		alert->show();
		return;
	}
	// check if a location was entered
	if (location_entry.get_text() == "") {
		alert = Gtk::AlertDialog::create();
		alert->set_message("No location provided.");
		alert->show();
		return;
	}
	// close dialogue
	add_variable_dialogue.hide();
	// create variable and add to the list
	std::string name = add_name_entry.get_text();
	std::string location = location_entry.get_text();
	bool memory = memory_button.get_active();
	Variable variable(name, memory, location);
	variables.push_back(variable);
	// create widgets
	variable_names.push_back(Gtk::Label(name));
	variable_locations.push_back(Gtk::Label(location));
	variable_values.push_back(Gtk::Label(variable.get_value(*cpu)));
	int index = variable_names.size() - 1;
	variable_names[index].set_hexpand();
	variable_locations[index].set_hexpand();
	variable_values[index].set_hexpand();
	variables_grid.attach(variable_names[index], 0, index);
	variables_grid.attach(variable_locations[index], 1, index);
	variables_grid.attach(variable_values[index], 2, index);
}

// run when remove variable button is clicked
void MainWindow::remove_variable_clicked(void) {
	remove_variable_dialogue.show();
}

// run when remove variable dialogue is confirmed
void MainWindow::remove_variable_confirmed(int result) {
	// do nothing if cancelled
	remove_variable_dialogue.hide();
	if (result != Gtk::ResponseType::OK) {
		return;
	}
	// remove old variable widgets
	for (size_t i = 0; i < variables.size(); i++) {
		variable_names[i].unparent();
		variable_locations[i].unparent();
		variable_values[i].unparent();
	}
	variable_names.clear();
	variable_locations.clear();
	variable_values.clear();
	// loop through each variable
	std::string name = remove_name_entry.get_text();
	size_t i = 0;
	while (i < variables.size()) {
		if (variables[i].name == name) {
			// delete variable from list if it has the same name
			variables.erase(variables.begin() + i);
		} else {
			// recreate widgets for this variable
			variable_names.push_back(Gtk::Label(variables[i].name));
			variable_names[i].set_hexpand();
			variables_grid.attach(variable_names[i], 0, i);
			variable_locations.push_back(Gtk::Label(variables[i].location));
			variable_locations[i].set_hexpand();
			variables_grid.attach(variable_locations[i], 1, i);
			variable_values.push_back(Gtk::Label(variables[i].get_value(*cpu)));
			variable_values[i].set_hexpand();
			variables_grid.attach(variable_values[i], 2, i);
			i++;
		}
	}
}

int main(int argc, char* argv[]) {
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create("org.nea.vm");
	return app->make_window_and_run<MainWindow>(argc, argv);
}
