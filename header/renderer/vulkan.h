#pragma once
#include "renderer.h"


namespace kop {

	class Vulkan : public Renderer {
	public:
		Vulkan(
			const char* vertexShaderPath,
			const char* fragmentShaderPath,
			size_t maxVertices,
			size_t maxElements,
			int textureWidth,
			int textureHeight
		);
		~Vulkan() override;
		const char* getWindowName() const override;
	private:
		void createWindow() override;
		void createShaderProgram() override;
	};

}