#include "renderer/directx12.h"

using namespace kop;


DirectX12::DirectX12(
	const char* vertexShaderPath,
	const char* fragmentShaderPath,
	size_t maxVertices,
	size_t maxElements
) 
	: Renderer(
		vertexShaderPath, fragmentShaderPath, maxVertices, maxElements
	  )
{

}


DirectX12::~DirectX12() {

}


const char* DirectX12::getRendererName() const {
	return "DirectX12";
}


void DirectX12::createWindow() {

}


void DirectX12::createShaderProgram() {

}