#pragma once
#include <vulkan/vulkan.h>
#include "renderer.h"
#include <array>
#include <vector>


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
		void setViewport(int width, int height) override;
		void clear() override;
		bool add(const Object& obj) override;
		bool updateTexture(const void* data, size_t index) override;
		void render() override;
		void present() override;
	public:
		static constexpr std::array<const char*, 1> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};
	private:
		void createWindow() override;
		void createShaderProgram() override;
		void createVertexBuffers() override;
		void createTextures() override;
	private:

	private:
		static size_t numInstances;
		static VkInstance instance;
		static VkApplicationInfo appInfo;
		static VkInstanceCreateInfo instanceInfo;
	private:
		static void createInstance();
		static void checkExtensionSupport();
		static void checkValidationLayers();
	};

}