#include "application.h"

using namespace kop;


Application::Application(Renderer& renderer) 
	: renderer(&renderer)
{

}


Application::~Application() {

}


void Application::run() const {
	// Init
	Object rect;
	rect.vboData = {
		{{0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
		{{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
	};
	rect.eboData = {
		0, 1, 2,
		0, 3, 2,
	};
	rect.move({ -0.5f, 0.0f, 0.0f });
	rect.scale({ 1.0f, 2.0f, 1.0f });
	rect.applyTransform();

	// Loop
	GLFWwindow* window = this->renderer->getWindow();
	glfwShowWindow(window);
	while (!glfwWindowShouldClose(window)) {
		this->renderer->clear();
		this->renderer->add(rect);
		this->renderer->render();
		this->renderer->present();
	}
	
	// End
	glfwHideWindow(window);
}