#include "renderer/vulkan.h"
#include <backends/imgui_impl_vulkan.h>
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
	this->selectQueueFamilies();
	this->createLogicalDevice();

	// ImGui_ImplVulkan_Init(,);
	Vulkan::numInstances += 1;
}


Vulkan::~Vulkan() {
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
			this->physicalDeviceFeatures.geometryShader	&&
			Vulkan::checkLogicalDeviceExtensions(physicalDevice)
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