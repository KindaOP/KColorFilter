#pragma once
#include "renderer.h"
#include "renderer/opengl.h"
#include "renderer/vulkan.h"
#include "renderer/directx12.h"


namespace kop {

	class Application {
	public:
		enum struct Backend {
			OPENGL = 0,
			VULKAN = 1,
			DIRECTX12 = 2,
		};
	public:
		Application(Backend backend);
		~Application();
		void run() const;
	private:
		OpenGL openglRenderer;
		Vulkan vulkanRenderer;
		DirectX12 directx12Renderer;
		Renderer* renderer = nullptr;
	private:
		void selectBackend(Backend backend);
	};

}