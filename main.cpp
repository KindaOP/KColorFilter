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

#ifdef NDEBUG
const char* VERTEX_SHADER_PATH = (
	"./KCF/shader/src/default_vert.vert"
);
const char* FRAGMENT_SHADER_PATH = (
	"./KCF/shader/src/default_frag.frag"
);
#else
const char* VERTEX_SHADER_PATH = (
	"./resource/shader/src/default_vert.vert"
);
const char* FRAGMENT_SHADER_PATH = (
	"./resource/shader/src/default_frag.frag"
);
#endif

#include "application.h"


int main() {
	kop::__KOP_BACKEND_TYPE__ renderer(
		VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH,
		12, 12
	);
	kop::Application app(renderer);
	app.run();
	return 0;
}