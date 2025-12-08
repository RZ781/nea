#include <headers.h>

class MainWindow: public Gtk::Window {
	public:
	MainWindow();
	private:
	Gtk::Notebook notebook;
	Gtk::Paned code_tab;
	Gtk::Box compilation_options;
	Gtk::CheckButton c_button;
	Gtk::CheckButton cpp_button;
	Gtk::CheckButton asm_button;
	Gtk::CheckButton standard_library_box;
	Gtk::Button compile_button;
	Gtk::TextView source_code_view;
	Gtk::Box vm_tab;
	Gtk::Frame vm_registers;
	Gtk::Frame vm_code;
	Gtk::Box variable_tab;
	Gtk::Frame variable_list;
	Gtk::Grid variables;
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
	vm_registers(),
	vm_code(),
	variable_tab(),
	variable_list(),
	variables()
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
	// set up vm tab
	notebook.append_page(vm_tab, "Virtual Machine");
	vm_tab.append(vm_registers);
	vm_tab.append(vm_code);
	// set up variables tab
	notebook.append_page(variable_tab, "Variables");
	variable_tab.append(variable_list);
	variable_list.set_child(variables);
	// set up window
	set_title("Virtual machine");
	set_default_size(800, 600);
	set_child(notebook);
}

int main(int argc, char* argv[]) {
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create("org.nea.vm");
	return app->make_window_and_run<MainWindow>(argc, argv);
}
