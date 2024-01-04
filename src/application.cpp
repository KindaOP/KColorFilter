#include "application.h"
#include <iostream>

using namespace kop;


Application::Application() {

}


Application::~Application() {

}


void Application::run() const {
	std::cout << this->renderer->getNumber() << std::endl;
}