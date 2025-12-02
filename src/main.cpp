#include <headers.h>

class MainWindow: public Gtk::Window {
	public:
	MainWindow();
	private:
	Gtk::Notebook notebook;
	Gtk::Box code_tab;
	Gtk::Box vm_tab;
	Gtk::Box variable_tab;
	Gtk::TextView source_code_view;
	Gtk::Frame vm_registers;
	Gtk::Frame vm_code;
	Gtk::Frame variable_list;
	Gtk::Grid variables;
};

MainWindow::MainWindow():
	notebook(),
	code_tab(),
	vm_tab(),
	variable_tab(),
	source_code_view(),
	vm_registers(),
	vm_code(),
	variable_list(),
	variables()
{
	notebook.append_page(code_tab, "Code");
	notebook.append_page(vm_tab, "Virtual Machine");
	notebook.append_page(variable_tab, "Variables");
	code_tab.append(source_code_view);
	vm_tab.append(vm_registers);
	vm_tab.append(vm_code);
	variable_tab.append(variable_list);
	variable_list.set_child(variables);
	set_title("Virtual machine");
	set_default_size(800, 600);
	set_child(notebook);
}

int main(int argc, char* argv[]) {
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create("org.nea.vm");
	return app->make_window_and_run<MainWindow>(argc, argv);
}
