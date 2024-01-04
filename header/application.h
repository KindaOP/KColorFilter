#pragma once
#include "renderer/opengl.h"
// #include "renderer/vulkan.h"
// #include "renderer/directx12.h"


namespace kop {

	class Application {
	public:
		Application();
		~Application();
		void run() const;
	private:
		__KOP_RENDERER_TYPE__ backend;
		Renderer* const renderer = &backend;
	};

}