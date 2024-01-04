#include "renderer.h"
#include <stdexcept>

using namespace kop;


Renderer::Renderer(
	const char* vertexShaderPath,
	const char* fragmentShaderPath,
	size_t maxVertices,
	size_t maxElements
) 
	: vertexShaderPath(vertexShaderPath),
	  fragmentShaderPath(fragmentShaderPath),
	  maxVertices(maxVertices),
	  maxElements(maxElements)
{
	if (Renderer::numInstances == 0) {
		if (!glfwInit()) {
			throw std::runtime_error("GLFW: Cannot initialize GLFW.");
		}
	}
	Renderer::numInstances += 1;
}


Renderer::~Renderer() {
	Renderer::numInstances -= 1;
	if (Renderer::numInstances == 0) {
		glfwTerminate();
	}
}


GLFWwindow* Renderer::getWindow() const {
	return this->window;
}


size_t Renderer::numInstances = 0;
