#include "application.h"

using namespace kop;


Application::Application(Renderer& renderer) 
	: renderer(&renderer)
{

}


Application::~Application() {

}


void Application::run() const {
	GLFWwindow* window = this->renderer->getWindow();
	glfwShowWindow(window);
	while (!glfwWindowShouldClose(window)) {
		this->renderer->clear();
		this->renderer->render();
		this->renderer->present();
	}
	glfwHideWindow(window);
}