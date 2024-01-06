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
		template <typename T>
		static bool allAreSame(T* elements, size_t count, T value) {
			bool result = true;
			for (size_t i = 0; i < count; i++) {
				if (elements[i] != value) {
					result = false;
					break;
				}
			}
			return result;
		}
	public:
		static constexpr std::array<const char*, 1> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};
		static constexpr size_t queueRequirementCount = 2;	// Graphics, Present
	private:
		void createWindow() override;
		void createSurface();
		void selectPhysicalDevice();
		void selectQueueFamilies();
		void createLogicalDevice();
		void createShaderProgram() override;
		void createVertexBuffers() override;
		void createTextures() override;
	private:
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties physicalDeviceProperties{};
		VkPhysicalDeviceFeatures physicalDeviceFeatures{};
		std::array<uint32_t, queueRequirementCount> queueFamilyIndices = { NULL };
		bool allQueueFamiliesAreSame = false;
		VkDevice logicalDevice = VK_NULL_HANDLE;
		VkDeviceCreateInfo logicalDeviceInfo{};
		std::array<VkQueue, queueRequirementCount> queues = { VK_NULL_HANDLE };
		std::array<VkDeviceQueueCreateInfo, queueRequirementCount> queueInfos = { {} };
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