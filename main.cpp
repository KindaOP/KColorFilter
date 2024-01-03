#include "application.h"


int main() {
	kop::Application app(
		kop::Application::Backend::OPENGL
	);
	app.run();
	return 0;
}