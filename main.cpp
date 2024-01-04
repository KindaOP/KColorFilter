#define __KOP_BACKEND_OPENGL__

#if defined(__KOP_BACKEND_OPENGL__)
#define __KOP_BACKEND_TYPE__ OpenGL
#include "renderer/opengl.h"
#elif defined(__KOP_BACKEND_VULKAN__)
#define __KOP_BACKEND_TYPE__ Vulkan
#include "renderer/vulkan.h"
#elif defined(__KOP_BACKEND_DIRECTX12__)
#define __KOP_BACKEND_TYPE__ DirectX12
#include "renderer/directx12.h"
#endif

#include "application.h"


int main() {
	kop::__KOP_BACKEND_TYPE__ renderer(
		"./resource/shader/src/default_vert.vert",
		"./resource/shader/src/default_frag.frag",
		12, 12
	);
	kop::Application app(renderer);
	app.run();
	return 0;
}