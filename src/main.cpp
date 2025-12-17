#include <headers.h>

class MainWindow: public Gtk::Window {
	public:
	MainWindow();
	private:
	Gtk::Notebook notebook;
	// code tab widgets
	Gtk::Paned code_tab;
	Gtk::Box compilation_options;
	Gtk::CheckButton c_button;
	Gtk::CheckButton cpp_button;
	Gtk::CheckButton asm_button;
	Gtk::CheckButton standard_library_box;
	Gtk::Button compile_button;
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
	// variable tab widgets
	Gtk::Paned variable_tab;
	Gtk::Frame variable_list;
	Gtk::Grid variables;
	Gtk::Box variable_buttons;
	Gtk::Button add_variable_button;
	Gtk::Button remove_variable_button;
};

MainWindow::MainWindow():
	notebook(),
	code_tab(),
	compilation_options(Gtk::Orientation::VERTICAL),
	c_button("C"),
	cpp_button("C++"),
	asm_button("Assembly"),
	standard_library_box("Include standard library"),
	compile_button("Compile"),
	source_code_view(),
	vm_tab(),
	registers_frame("Registers"),
	registers(),
	vm_buttons(Gtk::Orientation::VERTICAL),
	start_button("Start"),
	stop_button("Stop"),
	step_button("Step"),
	vm_code_frame("Code"),
	vm_code(),
	variable_tab(),
	variable_list(),
	variables(),
	variable_buttons(Gtk::Orientation::VERTICAL),
	add_variable_button("Add Variable"),
	remove_variable_button("Remove Varaiable")
{
	// set up code tab
	notebook.append_page(code_tab, "Code");
	code_tab.set_start_child(source_code_view);
	code_tab.set_end_child(compilation_options);
	compilation_options.append(c_button);
	compilation_options.append(cpp_button);
	compilation_options.append(asm_button);
	compilation_options.append(standard_library_box);
	compilation_options.append(compile_button);
	compilation_options.set_homogeneous();
	cpp_button.set_group(c_button);
	asm_button.set_group(c_button);
	compilation_options.set_margin(50);
	source_code_view.set_monospace();
	code_tab.set_position(500);
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
	vm_code.get_buffer()->set_text("print(\"Hello world\");)");
	// set up variables tab
	notebook.append_page(variable_tab, "Variables");
	variable_tab.set_start_child(variable_list);
	variable_tab.set_end_child(variable_buttons);
	variable_list.set_child(variables);
	variable_buttons.append(add_variable_button);
	variable_buttons.append(remove_variable_button);
	variable_buttons.set_margin(50);
	variable_buttons.set_homogeneous();
	variable_tab.set_position(500);
	// set up window
	set_title("Virtual machine");
	set_default_size(800, 600);
	set_child(notebook);
}

int main(int argc, char* argv[]) {
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create("org.nea.vm");
	return app->make_window_and_run<MainWindow>(argc, argv);
}
