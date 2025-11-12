#include <headers.h>

class MainWindow: public Gtk::Window {
	public:
	MainWindow();
	private:
	Gtk::Notebook notebook;
	Gtk::Frame tab1;
	Gtk::Frame tab2;
};

MainWindow::MainWindow(): notebook(), tab1("Tab 1"), tab2("Tab 2") {
	notebook.append_page(tab1, "Tab 1");
	notebook.append_page(tab2, "Tab 2");
	set_title("Virtual machine");
	set_default_size(800, 600);
	set_child(notebook);
}

int main(int argc, char* argv[]) {
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create("org.nea.vm");
	return app->make_window_and_run<MainWindow>(argc, argv);
}
