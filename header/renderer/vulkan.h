#pragma once
#include "renderer.h"


namespace kop {

	class Vulkan : public Renderer {
	public:
		Vulkan(
			const char* vertexShaderPath,
			const char* fragmentShaderPath,
			size_t maxVertices,
			size_t maxElements
		);
		~Vulkan() override;
		const char* getRendererName() const override;
	private:
		void createWindow() override;
		void createShaderProgram() override;
	};

}