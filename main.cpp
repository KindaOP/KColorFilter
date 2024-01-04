#define __KOP_BACKEND_OPENGL__

#if defined(__KOP_BACKEND_OPENGL__)
#include "renderer/opengl.h"
#elif defined(__KOP_BACKEND_VULKAN__)
#include "renderer/vulkan.h"
#elif defined(__KOP_BACKEND_DIRECTX12__)
#include "renderer/directx12.h"
#endif

#include "application.h"


int main() {
	kop::OpenGL renderer(
		"./resource/shader/src/default_vert.vert",
		"./resource/shader/src/default_frag.frag",
		12, 12
	);
	kop::Application app(renderer);
	app.run();
	return 0;
}