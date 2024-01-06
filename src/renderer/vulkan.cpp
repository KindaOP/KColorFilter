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

	// ImGui_ImplVulkan_Init(,);
	Vulkan::numInstances += 1;
}


Vulkan::~Vulkan() {
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
	Vulkan::checkExtensionSupport();
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


void Vulkan::checkExtensionSupport() {
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