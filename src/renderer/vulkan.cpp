#include "renderer/vulkan.h"
#include <backends/imgui_impl_vulkan.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#ifdef NDEBUG
static const bool IS_DEBUG = false;
static const char* SHADER_COMPILER_PATH = "./glslc";
#else
static const bool IS_DEBUG = true;
static const char* SHADER_COMPILER_PATH = "./external/glslc";
#endif

#ifdef _WIN32
static const bool IS_UNIX = false;
#else
static const bool IS_UNIX = true;
#endif

using namespace kop;


Vulkan::Vulkan(
	const char* vertexShaderPath,
	const char* fragmentShaderPath,
	size_t maxVertices,
	size_t maxElements,
	int textureWidth,
	int textureHeight
) 
	: Renderer(
		vertexShaderPath, fragmentShaderPath, maxVertices, maxElements,
		textureWidth, textureHeight
	  )
{
	this->createWindow();
	if (Vulkan::numInstances == 0) {
		Vulkan::createInstance();
		ImGui_ImplGlfw_InitForVulkan(this->window, true);
	}
	this->createSurface();
	this->selectPhysicalDevice();
	this->selectCapabilities();
	this->selectQueueFamilies();
	this->createLogicalDevice();
	this->createSwapchain();
	this->createImageViews();
	this->createPipelineLayout();
	this->createRenderPass();
	this->createGraphicsPipeline();
	this->createFrameBuffers();
	this->createCommandPool();
	this->createCommandBuffers();
	this->setSubmissionPresentationInfo();
	// ImGui_ImplVulkan_Init(,);
	Vulkan::numInstances += 1;
}


Vulkan::~Vulkan() {
	for (const VulkanCommandBuffer& commandBuffer : this->commandBuffers) {
		vkDestroyFence(
			this->logicalDevice, commandBuffer.isRenderingFence, nullptr
		);
		vkDestroySemaphore(
			this->logicalDevice, commandBuffer.isReadyForRenderingSemaphore, nullptr
		);
		vkDestroySemaphore(
			this->logicalDevice, commandBuffer.isReadyForPresentingSemaphore, nullptr
		);
	}
	vkDestroyCommandPool(this->logicalDevice, this->commandPool, nullptr);
	for (const VkFramebuffer& frameBuffer : this->frameBuffers) {
		vkDestroyFramebuffer(this->logicalDevice, frameBuffer, nullptr);
	}
	vkDestroyPipeline(this->logicalDevice, this->pipeline, nullptr);
	vkDestroyRenderPass(this->logicalDevice, this->renderPass, nullptr);
	vkDestroyPipelineLayout(this->logicalDevice, this->pipelineLayout, nullptr);
	for (const VkImageView& imageView : this->imageViews) {
		vkDestroyImageView(this->logicalDevice, imageView, nullptr);
	}
	vkDestroySwapchainKHR(this->logicalDevice, this->swapchain, nullptr);
	vkDestroyDevice(this->logicalDevice, nullptr);
	vkDestroySurfaceKHR(Vulkan::instance, this->surface, nullptr);
	Vulkan::numInstances -= 1;
	if (Vulkan::numInstances == 0) {
		vkDestroyInstance(Vulkan::instance, nullptr);
	}
}


const char* Vulkan::getWindowName() const {
	return "KCF-Vulkan";
}


void Vulkan::clear() {
	const VulkanCommandBuffer& currentCommandBuffer = this->commandBuffers[this->frameIndex];
	this->viewport.width = static_cast<float>(this->swapchainInfo.imageExtent.width);
	this->viewport.height = static_cast<float>(this->swapchainInfo.imageExtent.height);
	this->scissor.extent = this->swapchainInfo.imageExtent;
	this->renderPassBeginInfo.renderArea.extent = this->swapchainInfo.imageExtent;
	this->submissionInfo.pCommandBuffers = &currentCommandBuffer.handle;
	this->submissionInfo.pWaitSemaphores = &currentCommandBuffer.isReadyForRenderingSemaphore;
	this->submissionInfo.pSignalSemaphores = &currentCommandBuffer.isReadyForPresentingSemaphore;
	this->presentationInfo.pWaitSemaphores = &currentCommandBuffer.isReadyForPresentingSemaphore;
	
	vkWaitForFences(
		this->logicalDevice, 1, &currentCommandBuffer.isRenderingFence, VK_TRUE, UINT64_MAX
	);
	vkResetFences(this->logicalDevice, 1, &currentCommandBuffer.isRenderingFence);
	vkAcquireNextImageKHR(
		this->logicalDevice, this->swapchain, UINT64_MAX,
		currentCommandBuffer.isReadyForRenderingSemaphore, VK_NULL_HANDLE, &this->imageIndex
	);
	this->renderPassBeginInfo.framebuffer = this->frameBuffers[this->imageIndex];
	this->presentationInfo.pImageIndices = &this->imageIndex;

	vkResetCommandBuffer(currentCommandBuffer.handle, NULL);
	VkResult result = vkBeginCommandBuffer(
		currentCommandBuffer.handle, &currentCommandBuffer.beginInfo
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot begin recording for command buffer.");
	}
	vkCmdBeginRenderPass(
		currentCommandBuffer.handle, &this->renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE
	);
	vkCmdBindPipeline(
		currentCommandBuffer.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline
	);
}


bool Vulkan::add(const Object& obj) {
	return true;
}


bool Vulkan::updateTexture(const void* data, size_t index) {
	return true;
}


void Vulkan::render() {
	const VulkanCommandBuffer& currentCommandBuffer = this->commandBuffers[this->frameIndex];
	vkCmdSetViewport(currentCommandBuffer.handle, 0, 1, &this->viewport);
	vkCmdSetScissor(currentCommandBuffer.handle, 0, 1, &this->scissor);
	vkCmdDraw(currentCommandBuffer.handle, 3, 1, 0, 0);
	vkCmdEndRenderPass(currentCommandBuffer.handle);
	
	VkResult result = vkEndCommandBuffer(currentCommandBuffer.handle);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot record command buffer.");
	}
	result = vkQueueSubmit(
		this->queues[0], 1, &this->submissionInfo, currentCommandBuffer.isRenderingFence
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Failed to submit command buffer to queue.");
	}
}


void Vulkan::present() {
	vkQueuePresentKHR(this->queues[1], &this->presentationInfo);
	glfwPollEvents();
	this->frameIndex = (this->frameIndex + 1) % Vulkan::numRenderingFrames;
}


void Vulkan::endLoop() {
	vkDeviceWaitIdle(this->logicalDevice);
}


Vulkan::VulkanShaderModule::VulkanShaderModule(
	const VkDevice& logicalDevice, const char* sourcePath
) 
	: logicalDevice(logicalDevice)
{
	std::vector<char> binaryBuffer = {};
	if (IS_DEBUG) {
		const std::string binaryPath = this->compileSource(sourcePath);
		this->loadBinary(binaryPath.c_str(), binaryBuffer);
	}
	else {
		this->loadBinary(sourcePath, binaryBuffer);
	}

	this->moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	this->moduleInfo.codeSize = binaryBuffer.size();
	this->moduleInfo.pCode = reinterpret_cast<const uint32_t*>(binaryBuffer.data());
	
	VkResult result = vkCreateShaderModule(
		logicalDevice, &this->moduleInfo, nullptr, &this->handle
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot create shader module.");
	}
}


Vulkan::VulkanShaderModule::~VulkanShaderModule() {
	vkDestroyShaderModule(this->logicalDevice, this->handle, nullptr);
}


std::string Vulkan::VulkanShaderModule::compileSource(const char* sourcePath) {
	std::filesystem::path fCompilerPath(SHADER_COMPILER_PATH);
	if (!IS_UNIX) {
		fCompilerPath.concat(".exe");
	}
	std::filesystem::path fSourcePath(sourcePath);
	fCompilerPath.make_preferred();
	fSourcePath.make_preferred();
	const std::string cPath = fCompilerPath.string();
	const std::string sPath = fSourcePath.string();
	const std::string bPath = fSourcePath.replace_extension().string() + ".spv";
	const std::string compileCommand = cPath + ' ' + sPath + " -o " + bPath;
	int result = std::system(compileCommand.c_str());
	if (result != 0) {
		throw std::runtime_error(
			"Vulkan: Cannot compile shader from " + sPath + " using " + cPath + '.'
		);
	}
	return bPath;
}


void Vulkan::VulkanShaderModule::loadBinary(
	const char* binaryPath, std::vector<char>& buffer
) {
	std::filesystem::path fBinaryPath(binaryPath);
	fBinaryPath.make_preferred();
	const std::string bPath = fBinaryPath.string();
	std::ifstream file(bPath, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Vulkan: Cannot open binary at " + bPath + '.');
	}
	const size_t fileSize = file.tellg();
	buffer.clear();
	buffer.resize(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
}


void Vulkan::VulkanCommandBuffer::createSyncObjects() {
	this->fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	this->fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	VkResult result = vkCreateFence(
		*this->logicalDevice, &this->fenceInfo, nullptr, &this->isRenderingFence
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot create rendering fence.");
	}

	this->semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	result = vkCreateSemaphore(
		*this->logicalDevice, &this->semaphoreInfo, nullptr, &this->isReadyForRenderingSemaphore
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot create semaphore for rendering.");
	}
	result = vkCreateSemaphore(
		*this->logicalDevice, &this->semaphoreInfo, nullptr, &this->isReadyForPresentingSemaphore
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot create semaphore for presenting.");
	}
}


void Vulkan::createWindow() {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	this->window = glfwCreateWindow(
		this->windowWidth, this->windowHeight,
		this->getWindowName(), nullptr, nullptr
	);
	if (this->window == nullptr) {
		throw std::runtime_error("GLFW: Cannot create window.");
	}
}


void Vulkan::createSurface() {
	VkResult result = glfwCreateWindowSurface(Vulkan::instance, this->window, nullptr, &this->surface);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot create surface.");
	}
}


void Vulkan::selectPhysicalDevice() {
	uint32_t physicalDeviceCount = NULL;
	vkEnumeratePhysicalDevices(this->instance, &physicalDeviceCount, nullptr);
	if (physicalDeviceCount == 0) {
		throw std::runtime_error("Vulkan: Cannot find existing GPU.");
	}
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(this->instance, &physicalDeviceCount, physicalDevices.data());

	bool suitablePhysicalDeviceIsFound = false;
	for (const VkPhysicalDevice& physicalDevice : physicalDevices) {
		vkGetPhysicalDeviceProperties(physicalDevice, &this->physicalDeviceProperties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &this->physicalDeviceFeatures);
		if (
			Vulkan::checkLogicalDeviceExtensions(physicalDevice) &&
			this->checkFormat(physicalDevice) &&
			this->checkPresentMode(physicalDevice) &&
			this->physicalDeviceFeatures.geometryShader
		) {
			suitablePhysicalDeviceIsFound = true;
			this->physicalDevice = physicalDevice;
			break;
		}
	}
	if (!suitablePhysicalDeviceIsFound) {
		throw std::runtime_error("Vulkan: Cannot find suitable GPU.");
	}
}


bool Vulkan::checkFormat(const VkPhysicalDevice& physicalDevice) {
	uint32_t formatCount = NULL;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, this->surface, &formatCount, nullptr);
	if (formatCount == 0) {
		return false;
	}
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, this->surface, &formatCount, formats.data());

	this->format = formats[0];
	for (const VkSurfaceFormatKHR& format : formats) {
		if (
			format.format == VK_FORMAT_B8G8R8A8_SRGB &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		) {
			this->format = format;
			break;
		}
	}
	return true;
}


bool Vulkan::checkPresentMode(const VkPhysicalDevice& physicalDevice) {
	uint32_t modeCount = NULL;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, this->surface, &modeCount, nullptr);
	if (modeCount == 0) {
		return false;
	}
	std::vector<VkPresentModeKHR> modes(modeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, this->surface, &modeCount, modes.data());

	for (const VkPresentModeKHR mode : modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			this->presentMode = mode;
			break;
		}
	}
	return true;
}


void Vulkan::selectCapabilities() {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->physicalDevice, this->surface, &this->capabilities);
	if (
		this->capabilities.currentExtent.width != UINT32_MAX ||
		this->capabilities.currentExtent.height != UINT32_MAX
	) {
		return;
	}
	int width = NULL;
	int height = NULL;
	glfwGetFramebufferSize(this->window, &width, &height);
	this->capabilities.currentExtent.width = Vulkan::clamp<uint32_t>(
		this->capabilities.currentExtent.width,
		this->capabilities.minImageExtent.width,
		this->capabilities.maxImageExtent.width
	);
	this->capabilities.currentExtent.height = Vulkan::clamp<uint32_t>(
		this->capabilities.currentExtent.height,
		this->capabilities.minImageExtent.height,
		this->capabilities.maxImageExtent.height
	);
}


void Vulkan::selectQueueFamilies() {
	uint32_t queueFamilyCount = NULL;
	vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyCount, queueFamilies.data());

	const size_t& indexCount = Vulkan::queueRequirementCount;
	bool suitableQueueFamilyIsFounds[indexCount] = { false };
	bool allSuitableQueueFamiliesAreFound = false;
	for (size_t i = 0; i < indexCount; i++) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			suitableQueueFamilyIsFounds[0] = true;
			this->queueFamilyIndices[0] = i;
		}
		VkBool32 hasPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, this->surface, &hasPresentSupport);
		if (hasPresentSupport) {
			suitableQueueFamilyIsFounds[1] = true;
			this->queueFamilyIndices[1] = i;
		}
		if (!allSuitableQueueFamiliesAreFound) {
			allSuitableQueueFamiliesAreFound = Vulkan::allAreSame<bool>(
				suitableQueueFamilyIsFounds, indexCount, true
			);
		}
		else {
			this->allQueueFamiliesAreSame = Vulkan::allAreSame<uint32_t>(
				this->queueFamilyIndices.data(), this->queueFamilyIndices.size(), this->queueFamilyIndices[0]
			);
			if (this->allQueueFamiliesAreSame) {
				break;
			}
		}
	}
	if (!allSuitableQueueFamiliesAreFound) {
		throw std::runtime_error("Vulkan: Cannot find all necessary queue families.");
	}
}


void Vulkan::createLogicalDevice() {
	const float queuePriority = 1.0f;
	for (size_t i = 0; i < Vulkan::queueRequirementCount; i++) {
		this->queueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		this->queueInfos[i].queueFamilyIndex = this->queueFamilyIndices[i];
		this->queueInfos[i].queueCount = 1;
		this->queueInfos[i].pQueuePriorities = &queuePriority;
	}

	this->logicalDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	this->logicalDeviceInfo.pQueueCreateInfos = this->queueInfos.data();
	this->logicalDeviceInfo.queueCreateInfoCount = this->queueInfos.size();
	this->logicalDeviceInfo.pEnabledFeatures = &this->physicalDeviceFeatures;
	this->logicalDeviceInfo.enabledExtensionCount = Vulkan::logicalDeviceExtensions.size();
	this->logicalDeviceInfo.ppEnabledExtensionNames = Vulkan::logicalDeviceExtensions.data();

	VkResult result = vkCreateDevice(
		this->physicalDevice, &this->logicalDeviceInfo, nullptr, &this->logicalDevice
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot create logical device.");
	}
	for (size_t i = 0; i < Vulkan::queueRequirementCount; i++) {
		vkGetDeviceQueue(this->logicalDevice, this->queueFamilyIndices[i], 0, &this->queues[i]);
	}
}


void Vulkan::createSwapchain() {
	uint32_t imageCount = this->capabilities.minImageCount + 1;
	const uint32_t& maxImageCount = this->capabilities.maxImageCount;
	if (maxImageCount != NULL && imageCount > maxImageCount) {
		imageCount = maxImageCount;
	}
	this->setSwapchain(imageCount);

	VkResult result = vkCreateSwapchainKHR(
		this->logicalDevice, &this->swapchainInfo, nullptr, &this->swapchain
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot create swapchain.");
	}

	uint32_t actualImageCount = NULL;
	vkGetSwapchainImagesKHR(this->logicalDevice, this->swapchain, &actualImageCount, nullptr);
	this->images.resize(imageCount);
	vkGetSwapchainImagesKHR(this->logicalDevice, this->swapchain, &actualImageCount, this->images.data());
	this->setSwapchainViewport();
}


void Vulkan::setSwapchain(uint32_t imageCount) {
	this->swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	this->swapchainInfo.surface = this->surface;
	this->swapchainInfo.minImageCount = imageCount;
	this->swapchainInfo.imageFormat = this->format.format;
	this->swapchainInfo.imageColorSpace = this->format.colorSpace;
	this->swapchainInfo.imageExtent = this->capabilities.currentExtent;
	this->swapchainInfo.imageArrayLayers = 1;
	this->swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (this->allQueueFamiliesAreSame) {
		this->swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		this->swapchainInfo.queueFamilyIndexCount = NULL;
		this->swapchainInfo.pQueueFamilyIndices = nullptr;
	}
	else {
		this->swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		this->swapchainInfo.queueFamilyIndexCount = this->queueFamilyIndices.size();
		this->swapchainInfo.pQueueFamilyIndices = this->queueFamilyIndices.data();
	}
	this->swapchainInfo.preTransform = this->capabilities.currentTransform;
	this->swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	this->swapchainInfo.presentMode = this->presentMode;
	this->swapchainInfo.clipped = VK_TRUE;
	this->swapchainInfo.oldSwapchain = VK_NULL_HANDLE;
}


void Vulkan::setSwapchainViewport() {
	this->viewport.x = 0.0f;
	this->viewport.y = 0.0f;
	this->viewport.minDepth = 0.0f;
	this->viewport.maxDepth = 1.0f;

	this->scissor.offset = { 0,0 };
}


void Vulkan::createImageViews() {
	const size_t imageCount = this->images.size();
	this->imageViews.resize(imageCount);
	this->imageViewInfos.resize(imageCount);
	for (size_t i = 0; i < imageCount; i++) {
		this->imageViewInfos[i].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		this->imageViewInfos[i].image = this->images[i];
		this->imageViewInfos[i].viewType = VK_IMAGE_VIEW_TYPE_2D;
		this->imageViewInfos[i].format = this->format.format;
		this->imageViewInfos[i].components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		this->imageViewInfos[i].components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		this->imageViewInfos[i].components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		this->imageViewInfos[i].components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		this->imageViewInfos[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		this->imageViewInfos[i].subresourceRange.baseMipLevel = 0;
		this->imageViewInfos[i].subresourceRange.levelCount = 1;
		this->imageViewInfos[i].subresourceRange.baseArrayLayer = 0;
		this->imageViewInfos[i].subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(
			this->logicalDevice, &this->imageViewInfos[i], nullptr, &this->imageViews[i]
		);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Vulkan: Cannot create image view.");
		}
	}
}


void Vulkan::createPipelineLayout() {
	this->pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	this->pipelineLayoutInfo.setLayoutCount = 0;
	this->pipelineLayoutInfo.pSetLayouts = nullptr;
	this->pipelineLayoutInfo.pushConstantRangeCount = 0;
	this->pipelineLayoutInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(
		this->logicalDevice, &this->pipelineLayoutInfo, nullptr, &this->pipelineLayout
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot create pipeline layout.");
	}
}


void Vulkan::createRenderPass() {
	this->setRenderPassAttachments();
	this->renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	this->renderPassInfo.attachmentCount = 1;
	this->renderPassInfo.pAttachments = &this->colorAttachment;
	this->renderPassInfo.subpassCount = 1;
	this->renderPassInfo.pSubpasses = &this->subpass;
	this->renderPassInfo.dependencyCount = 1;
	this->renderPassInfo.pDependencies = &this->subpassDependency;

	VkResult result = vkCreateRenderPass(
		this->logicalDevice, &this->renderPassInfo, nullptr, &this->renderPass
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot create render pass.");
	}

	this->renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	this->renderPassBeginInfo.renderPass = this->renderPass;
	this->renderPassBeginInfo.renderArea.offset = { 0, 0 };
	this->renderPassBeginInfo.clearValueCount = 1;
	this->renderPassBeginInfo.pClearValues = &Vulkan::clearColor;
}


void Vulkan::setRenderPassAttachments() {
	this->colorAttachment.format = this->swapchainInfo.imageFormat;
	this->colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	this->colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	this->colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	this->colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	this->colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	this->colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	this->colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
	this->colorAttachmentReference.attachment = 0;
	this->colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	this->subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	this->subpass.colorAttachmentCount = 1;
	this->subpass.pColorAttachments = &this->colorAttachmentReference;

	this->subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	this->subpassDependency.dstSubpass = 0;
	this->subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	this->subpassDependency.srcAccessMask = NULL;
	this->subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	this->subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
}


void Vulkan::createGraphicsPipeline() {
	this->pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	this->pipelineInfo.layout = this->pipelineLayout;
	this->pipelineInfo.renderPass = this->renderPass;
	this->pipelineInfo.subpass = 0;
	this->pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	this->pipelineInfo.basePipelineIndex = -1;

	VulkanShaderModule vertexShader(this->logicalDevice, this->vertexShaderPath);
	VulkanShaderModule fragmentShader(this->logicalDevice, this->fragmentShaderPath);
	this->setPipelineShaders(vertexShader.handle, fragmentShader.handle);
	this->setPipelineDynamicStates();
	this->setPipelineVertexInput();
	this->setPipelineInputAssembly();
	this->setPipelineViewports();
	this->setPipelineRasterizer();
	this->setPipelineMultisampling();
	this->setPipelineColorBlending();
	this->setPipelineDepthAndStencilTesting();

	VkResult result = vkCreateGraphicsPipelines(
		this->logicalDevice, VK_NULL_HANDLE, 1, &this->pipelineInfo, nullptr, &this->pipeline
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot create graphics pipeline.");
	}
}


void Vulkan::setPipelineShaders(
	const VkShaderModule& vertexShader, const VkShaderModule& fragmentShader
) {
	this->shaderInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	this->shaderInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	this->shaderInfos[0].module = vertexShader;
	this->shaderInfos[0].pName = "main";
	this->shaderInfos[0].pSpecializationInfo = nullptr;
	this->shaderInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	this->shaderInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	this->shaderInfos[1].module = fragmentShader;
	this->shaderInfos[1].pName = "main";
	this->shaderInfos[1].pSpecializationInfo = nullptr;
	
	this->pipelineInfo.stageCount = this->shaderInfos.size();
	this->pipelineInfo.pStages = this->shaderInfos.data();
}


void Vulkan::setPipelineDynamicStates() {
	this->dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	this->dynamicStateInfo.dynamicStateCount = Vulkan::dynamicStates.size();
	this->dynamicStateInfo.pDynamicStates = Vulkan::dynamicStates.data();

this->pipelineInfo.pDynamicState = &this->dynamicStateInfo;
}


void Vulkan::setPipelineVertexInput() {
	this->vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	this->vertexInputInfo.vertexBindingDescriptionCount = 0;
	this->vertexInputInfo.pVertexBindingDescriptions = nullptr;
	this->vertexInputInfo.vertexAttributeDescriptionCount = 0;
	this->vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	this->pipelineInfo.pVertexInputState = &this->vertexInputInfo;
}


void Vulkan::setPipelineInputAssembly() {
	this->inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	this->inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	this->inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	this->pipelineInfo.pInputAssemblyState = &this->inputAssemblyInfo;
}


void Vulkan::setPipelineViewports() {
	this->viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	this->viewportInfo.viewportCount = 1;
	this->viewportInfo.scissorCount = 1;

	this->pipelineInfo.pViewportState = &this->viewportInfo;
}


void Vulkan::setPipelineRasterizer() {
	this->rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	this->rasterizerInfo.depthClampEnable = VK_FALSE;
	this->rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
	this->rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
	this->rasterizerInfo.lineWidth - 1.0f;
	this->rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	this->rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	this->rasterizerInfo.depthBiasEnable = VK_FALSE;
	this->rasterizerInfo.depthBiasConstantFactor = 0.0f;
	this->rasterizerInfo.depthBiasClamp = 0.0f;
	this->rasterizerInfo.depthBiasSlopeFactor = 0.0f;

	this->pipelineInfo.pRasterizationState = &this->rasterizerInfo;
}


void Vulkan::setPipelineMultisampling() {
	this->multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	this->multisamplingInfo.sampleShadingEnable = VK_FALSE;
	this->multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	this->multisamplingInfo.minSampleShading = 1.0f;
	this->multisamplingInfo.pSampleMask = nullptr;
	this->multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
	this->multisamplingInfo.alphaToOneEnable = VK_FALSE;

	this->pipelineInfo.pMultisampleState = &this->multisamplingInfo;
}


void Vulkan::setPipelineColorBlending() {
	this->colorBlendingAttachment.colorWriteMask = (
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		);
	this->colorBlendingAttachment.blendEnable = VK_TRUE;
	this->colorBlendingAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	this->colorBlendingAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	this->colorBlendingAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	this->colorBlendingAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	this->colorBlendingAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	this->colorBlendingAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	this->colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	this->colorBlendingInfo.logicOpEnable = VK_FALSE;
	this->colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
	this->colorBlendingInfo.attachmentCount = 1;
	this->colorBlendingInfo.pAttachments = &this->colorBlendingAttachment;
	this->colorBlendingInfo.blendConstants[0] = 0.0f;
	this->colorBlendingInfo.blendConstants[1] = 0.0f;
	this->colorBlendingInfo.blendConstants[2] = 0.0f;
	this->colorBlendingInfo.blendConstants[3] = 0.0f;

	this->pipelineInfo.pColorBlendState = &this->colorBlendingInfo;
}


void Vulkan::setPipelineDepthAndStencilTesting() {
	this->pipelineInfo.pDepthStencilState = nullptr;
}


void Vulkan::createFrameBuffers() {
	const size_t numImageViews = this->imageViews.size();
	this->frameBuffers.resize(numImageViews);
	this->frameBufferInfos.resize(numImageViews);
	for (size_t i = 0; i < numImageViews; i++) {
		this->frameBufferInfos[i].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		this->frameBufferInfos[i].renderPass = this->renderPass;
		this->frameBufferInfos[i].attachmentCount = 1;
		this->frameBufferInfos[i].pAttachments = &this->imageViews[i];
		this->frameBufferInfos[i].width = this->swapchainInfo.imageExtent.width;
		this->frameBufferInfos[i].height = this->swapchainInfo.imageExtent.height;
		this->frameBufferInfos[i].layers = 1;

		VkResult result = vkCreateFramebuffer(
			this->logicalDevice, &this->frameBufferInfos[i], nullptr, &this->frameBuffers[i]
		);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Vulkan: Cannot create frame buffer.");
		}
	}
}


void Vulkan::createCommandPool() {
	this->commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	this->commandPoolInfo.queueFamilyIndex = this->queueFamilyIndices[0];
	this->commandPoolInfo.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	
	VkResult result = vkCreateCommandPool(
		this->logicalDevice, &this->commandPoolInfo, nullptr, &this->commandPool
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot create command pool");
	}
}


void Vulkan::createCommandBuffers() {
	for (VulkanCommandBuffer& commandBuffer : this->commandBuffers) {
		commandBuffer.logicalDevice = &this->logicalDevice;
		commandBuffer.allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBuffer.allocateInfo.commandPool = commandPool;
		commandBuffer.allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBuffer.allocateInfo.commandBufferCount = 1;

		VkResult result = vkAllocateCommandBuffers(
			this->logicalDevice, &commandBuffer.allocateInfo, &commandBuffer.handle
		);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Vulkan: Cannot create commmand buffer.");
		}

		commandBuffer.createSyncObjects();
		commandBuffer.beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBuffer.beginInfo.flags = NULL;
		commandBuffer.beginInfo.pInheritanceInfo = nullptr;
	}
}


void Vulkan::setSubmissionPresentationInfo() {
	this->submissionInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	this->submissionInfo.pWaitDstStageMask = Vulkan::waitStages.data();
	this->submissionInfo.commandBufferCount = 1;
	this->submissionInfo.waitSemaphoreCount = 1;
	this->submissionInfo.signalSemaphoreCount = 1;
	
	this->presentationInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	this->presentationInfo.waitSemaphoreCount = 1;
	this->presentationInfo.swapchainCount = 1;
	this->presentationInfo.pSwapchains = &this->swapchain;
	this->presentationInfo.pResults = nullptr;
}


void Vulkan::createVertexBuffers() {

}


void Vulkan::createTextures() {

}


size_t Vulkan::numInstances = 0;


VkInstance Vulkan::instance = VK_NULL_HANDLE;


VkApplicationInfo Vulkan::appInfo{};


VkInstanceCreateInfo Vulkan::instanceInfo{};


void Vulkan::createInstance() {
	Vulkan::appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	Vulkan::appInfo.pApplicationName = "KCF";
	Vulkan::appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	Vulkan::appInfo.pEngineName = "No Engine";
	Vulkan::appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	Vulkan::appInfo.apiVersion = VK_API_VERSION_1_3;

	Vulkan::instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	Vulkan::instanceInfo.pApplicationInfo = &Vulkan::appInfo;
	Vulkan::checkInstanceExtensions();
	if (IS_DEBUG) {
		Vulkan::checkValidationLayers();
	}
	else {
		Vulkan::instanceInfo.enabledLayerCount = 0;
		Vulkan::instanceInfo.ppEnabledLayerNames = nullptr;
	}

	VkResult result = vkCreateInstance(&Vulkan::instanceInfo, nullptr, &Vulkan::instance);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Vulkan: Cannot create instance.");
	}
}


void Vulkan::checkInstanceExtensions() {
	uint32_t extensionCount = NULL;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	uint32_t requiredExtensionCount = NULL;
	const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);

	for (size_t i = 0; i < requiredExtensionCount; i++) {
		const char* requiredExtensionName = requiredExtensions[i];
		bool requiredExtensionIsFound = false;
		for (const VkExtensionProperties& extension : extensions) {
			const char* extensionName = extension.extensionName;
			if (strcmp(requiredExtensionName, extensionName) == 0) {
				requiredExtensionIsFound = true;
				break;
			}
		}
		if (!requiredExtensionIsFound) {
			throw std::runtime_error(
				"Vulkan: Cannot find all required extensions for creating instance."
			);
		}
	}
	Vulkan::instanceInfo.enabledExtensionCount = requiredExtensionCount;
	Vulkan::instanceInfo.ppEnabledExtensionNames = requiredExtensions;
}


void Vulkan::checkValidationLayers() {
	uint32_t layerCount = NULL;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> layers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

	for (const char* validationLayerName : Vulkan::validationLayers) {
		bool validationLayerIsFound = false;
		for (const VkLayerProperties& layer : layers) {
			const char* layerName = layer.layerName;
			if (strcmp(validationLayerName, layerName) == 0) {
				validationLayerIsFound = true;
				break;
			}
		}
		if (!validationLayerIsFound) {
			throw std::runtime_error(
				"Vulkan: Cannot find all validation layers for creating instance."
			);
		}
	}
	Vulkan::instanceInfo.enabledLayerCount = validationLayers.size();
	Vulkan::instanceInfo.ppEnabledLayerNames = validationLayers.data();
}


bool Vulkan::checkLogicalDeviceExtensions(const VkPhysicalDevice& physicalDevice) {
	uint32_t extensionCount = NULL;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
	
	for (const char* requiredExtensionName : Vulkan::logicalDeviceExtensions) {
		bool requiredExtensionIsFound = false;
		for (const VkExtensionProperties& extension : extensions) {
			const char* extensionName = extension.extensionName;
			if (strcmp(requiredExtensionName, extensionName) == 0) {
				requiredExtensionIsFound = true;
				break;
			}
		}
		if (!requiredExtensionIsFound) {
			return false;
		}
	}
	return true;
}