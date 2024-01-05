#include "renderer/vulkan.h"

using namespace kop;


Vulkan::Vulkan(
	const char* vertexShaderPath,
	const char* fragmentShaderPath,
	size_t maxVertices,
	size_t maxElements,
	int textureWidth,
	int textureHeight
) 
	: Renderer(
		vertexShaderPath, fragmentShaderPath, maxVertices, maxElements,
		textureWidth, textureHeight
	  )
{

}


Vulkan::~Vulkan() {

}


const char* Vulkan::getWindowName() const {
	return "KCF-Vulkan";
}


void Vulkan::createWindow() {

}


void Vulkan::createShaderProgram() {

}