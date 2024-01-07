#pragma once
#include <vulkan/vulkan.h>
#include "renderer.h"
#include <array>
#include <string>
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
		template <typename T>
		static T clamp(T value, T vmin, T vmax) {
			if (value < vmin) {
				return vmin;
			}
			else if (value > vmax) {
				return vmax;
			}
			else {
				return value;
			}
		}
	public:
		static constexpr std::array<const char*, 1> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};
		static constexpr std::array<const char*, 1> logicalDeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		static constexpr size_t queueRequirementCount = 2;	// Graphics, Present
		static constexpr std::array<VkDynamicState, 2> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};
		static constexpr VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	private:
		void createWindow() override;
		void createSurface();
		void selectPhysicalDevice();
		bool checkFormat(const VkPhysicalDevice& physicalDevice);
		bool checkPresentMode(const VkPhysicalDevice& physicalDevice);
		void selectCapabilities();
		void selectQueueFamilies();
		void createLogicalDevice();
		void createSwapchain();
		void setSwapchain(uint32_t imageCount);
		void setSwapchainViewport();
		void createImageViews();
		void createPipelineLayout();
		void createRenderPass();
		void setColorAttachments();
		void createGraphicsPipeline() override;
		void setPipelineShaders(
			const VkShaderModule& vertexShader, 
			const VkShaderModule& fragmentShader
		);
		void setPipelineDynamicStates();
		void setPipelineVertexInput();
		void setPipelineInputAssembly();
		void setPipelineViewports();
		void setPipelineRasterizer();
		void setPipelineMultisampling();
		void setPipelineColorBlending();
		void setPipelineDepthAndStencilTesting();
		void createFrameBuffers();
		void createCommandPool();
		void createCommandBuffer();
		void createSyncObjects();
		void createVertexBuffers() override;
		void createTextures() override;
	private:
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties physicalDeviceProperties{};
		VkPhysicalDeviceFeatures physicalDeviceFeatures{};
		VkSurfaceFormatKHR format{};
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		VkSurfaceCapabilitiesKHR capabilities{};
		std::array<uint32_t, queueRequirementCount> queueFamilyIndices = { NULL };
		bool allQueueFamiliesAreSame = false;
		VkDevice logicalDevice = VK_NULL_HANDLE;
		VkDeviceCreateInfo logicalDeviceInfo{};
		std::array<VkQueue, queueRequirementCount> queues = { VK_NULL_HANDLE };
		std::array<VkDeviceQueueCreateInfo, queueRequirementCount> queueInfos = { {} };
		VkSwapchainKHR swapchain = VK_NULL_HANDLE;
		VkSwapchainCreateInfoKHR swapchainInfo{};
		VkViewport viewport{};
		VkRect2D scissor{};
		std::vector<VkImage> images = { VK_NULL_HANDLE };
		std::vector<VkImageView> imageViews = { VK_NULL_HANDLE };
		std::vector<VkImageViewCreateInfo> imageViewInfos = {};
		uint32_t imageIndex = NULL;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		VkRenderPass renderPass = VK_NULL_HANDLE;
		VkRenderPassCreateInfo renderPassInfo{};
		VkAttachmentDescription colorAttachment{};
		VkSubpassDescription subpass{};
		VkAttachmentReference colorAttachmentReference{};
		VkRenderPassBeginInfo renderPassBeginInfo{};
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderInfos = { {} }; // Vertex, Fragment
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		VkPipelineViewportStateCreateInfo viewportInfo{};
		VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
		VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
		VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
		VkPipelineColorBlendAttachmentState colorBlendingAttachment{};
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
		std::vector<VkFramebuffer> frameBuffers = { VK_NULL_HANDLE };
		std::vector<VkFramebufferCreateInfo> frameBufferInfos = {};
		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkCommandPoolCreateInfo commandPoolInfo{};
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		VkCommandBufferAllocateInfo commandBufferInfo{};
		VkCommandBufferBeginInfo commandBufferBeginInfo{};
		std::array<VkSemaphore, 2> semaphores = {
			VK_NULL_HANDLE,	// Image available for rendering
			VK_NULL_HANDLE, // Image available for presenting
		};
		std::array<VkSemaphoreCreateInfo, 2> semaphoreInfos = { {} };
		std::array<VkFence, 1> fences = {
			VK_NULL_HANDLE, // Image is being rendered
		};
		std::array<VkFenceCreateInfo, 1> fenceInfos = { {} };
	private:
		class VulkanShaderModule {
		public:
			VulkanShaderModule(const VkDevice& logicalDevice, const char* sourcePath);
			~VulkanShaderModule();
		public:
			const VkDevice& logicalDevice;
			VkShaderModule handle = VK_NULL_HANDLE;
			VkShaderModuleCreateInfo moduleInfo{};
		private:
			static std::string compileSource(const char* sourcePath);
			static void loadBinary(const char* binaryPath, std::vector<char>& buffer);
		};
	private:
		static size_t numInstances;
		static VkInstance instance;
		static VkApplicationInfo appInfo;
		static VkInstanceCreateInfo instanceInfo;
	private:
		static void createInstance();
		static void checkInstanceExtensions();
		static void checkValidationLayers();
		static bool checkLogicalDeviceExtensions(const VkPhysicalDevice & physicalDevice);
	};

}