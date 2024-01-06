#include "renderer/vulkan.h"
#include <backends/imgui_impl_vulkan.h>
#include <limits>
#include <stdexcept>

#ifdef NDEBUG
const bool IS_DEBUG = false;
#else
const bool IS_DEBUG = true;
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

	// ImGui_ImplVulkan_Init(,);
	Vulkan::numInstances += 1;
}


Vulkan::~Vulkan() {
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


void Vulkan::setViewport(int width, int height) {

}


void Vulkan::clear() {

}


bool Vulkan::add(const Object& obj) {
	return true;
}


bool Vulkan::updateTexture(const void* data, size_t index) {
	return true;
}


void Vulkan::render() {

}


void Vulkan::present() {

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
		this->capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() ||
		this->capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max()
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


void Vulkan::createShaderProgram() {

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