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
		/*void setViewport(int width, int height) override;
		void clear() override;
		bool add(const Object& obj) override;
		bool updateTexture(const void* data, size_t index) override;
		void render() override;
		void present() override;*/
	private:
		void createWindow() override;
		void createShaderProgram() override;
		/*void createVertexBuffers() override;
		void createTextures() override;*/
	};

}