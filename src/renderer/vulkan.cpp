#include "renderer/vulkan.h"

using namespace kop;


Vulkan::Vulkan(
	const char* vertexShaderPath,
	const char* fragmentShaderPath,
	size_t maxVertices,
	size_t maxElements
) 
	: Renderer(vertexShaderPath, fragmentShaderPath, maxVertices, maxElements)
{

}


Vulkan::~Vulkan() {

}


const char* Vulkan::getRendererName() const {
	return "Vulkan";
}


void Vulkan::createWindow() {

}


void Vulkan::createShaderProgram() {

}