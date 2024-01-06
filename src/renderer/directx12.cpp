#include "renderer/directx12.h"

using namespace kop;


DirectX12::DirectX12(
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


DirectX12::~DirectX12() {

}


const char* DirectX12::getWindowName() const {
	return "KCF-DirectX12";
}


void DirectX12::createWindow() {

}


void DirectX12::createGraphicsPipeline() {

}