#include "application.h"
#include <iostream>

using namespace kop;


Application::Application(Backend backend) {
	this->selectBackend(backend);
}


Application::~Application() {

}


void Application::run() const {
	std::cout << this->renderer->getNumber() << std::endl;
}


void Application::selectBackend(Backend backend) {
	switch (backend) {
	case Backend::OPENGL:
		this->openglRenderer = OpenGL();
		this->renderer = &this->openglRenderer;
		break;
	case Backend::VULKAN:
		this->vulkanRenderer = Vulkan();
		this->renderer = &this->vulkanRenderer;
		break;
	case Backend::DIRECTX12:
		this->directx12Renderer = DirectX12();
		this->renderer = &this->directx12Renderer;
		break;
	default:
		throw std::runtime_error("Application: Backend not found.");
	}
}