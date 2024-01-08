#define __KOP_BACKEND_OPENGL__

#include <string>

#ifdef NDEBUG
static const std::string SHADER_ROOT = "./shader/";
#else
static const std::string SHADER_ROOT = "./resource/shader/";
#endif

#if defined(__KOP_BACKEND_OPENGL__)
#define __KOP_BACKEND_TYPE__ OpenGL
#include "renderer/opengl.h"
static const char* VERTEX_SHADER_NAME = "opengl_vert.vert";
static const char* FRAGMENT_SHADER_NAME = "opengl_frag.frag";

#elif defined(__KOP_BACKEND_VULKAN__)
#define __KOP_BACKEND_TYPE__ Vulkan
#include "renderer/vulkan.h"
#ifdef NDEBUG
static const char* VERTEX_SHADER_NAME = "vulkan_vert.spv";
static const char* FRAGMENT_SHADER_NAME = "vulkan_frag.spv";
#else
static const char* VERTEX_SHADER_NAME = "vulkan_vert.vert";
static const char* FRAGMENT_SHADER_NAME = "vulkan_frag.frag";
#endif

#elif defined(__KOP_BACKEND_DIRECTX12__)
#define __KOP_BACKEND_TYPE__ DirectX12
#include "renderer/directx12.h"
static const char* VERTEX_SHADER_NAME = "directx12_vert.vert";
static const char* FRAGMENT_SHADER_NAME = "directx12_frag.frag";
#endif

#include "application.h"


int main() {
	const std::string vertexShaderPath = SHADER_ROOT + VERTEX_SHADER_NAME;
	const std::string fragmentSahderPath = SHADER_ROOT + FRAGMENT_SHADER_NAME;
	kop::Webcam webcam(0, 720, 480);
	kop::__KOP_BACKEND_TYPE__ renderer(
		vertexShaderPath.c_str(), fragmentSahderPath.c_str(),
		12, 12, webcam.getWidth(), webcam.getHeight()
	);
	kop::Application app(webcam, renderer);
	app.run();
	return 0;
}