#include "vulkan_context.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_device.hpp"
#include "vulkan_gpu.hpp"
#include "vulkan_queues.hpp"
#include "utils/logger.hpp"

std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

bool g_assertOnError = false;

VkResult CreateDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    const PFN_vkCreateDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    const PFN_vkDestroyDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';

    assert(!g_assertOnError || (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) == 0);

    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void VulkanContext::setupDebugMessenger() {
    if (!m_validationLayersEnabled) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (const VkResult ret = CreateDebugUtilsMessengerEXT(m_vkHandle, &createInfo, nullptr, &m_debugMessenger); ret != VK_SUCCESS) {
        throw std::runtime_error(std::string("Failed to set up debug messenger, error: ") + string_VkResult(ret));
    }
    Logger::print("Created debug messenger for vulkan context", Logger::LevelBits::INFO);
}

void VulkanContext::init(const uint32_t vulkanApiVersion, const bool enableValidationLayers, const bool assertOnError, std::vector<const char*> extensions)
{
    g_assertOnError = assertOnError;
	m_validationLayersEnabled = enableValidationLayers;

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = vulkanApiVersion;

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers)
	{
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        populateDebugMessengerCreateInfo(debugCreateInfo);
	    instanceCreateInfo.pNext = &debugCreateInfo;
	}
	else
	{
		instanceCreateInfo.enabledLayerCount = 0;
	}
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

	if (const VkResult ret = vkCreateInstance(&instanceCreateInfo, nullptr, &m_vkHandle); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create Vulkan instance, error:") + string_VkResult(ret));
	}

    if (m_validationLayersEnabled)
        setupDebugMessenger();
}

std::vector<VulkanGPU> VulkanContext::getGPUs()
{
	uint32_t gpuCount = 0;
	vkEnumeratePhysicalDevices(m_vkHandle, &gpuCount, nullptr);
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	vkEnumeratePhysicalDevices(m_vkHandle, &gpuCount, physicalDevices.data());

	std::vector<VulkanGPU> gpus;
	gpus.reserve(physicalDevices.size());
	for (const auto& physicalDevice : physicalDevices)
	{
		gpus.push_back(VulkanGPU(physicalDevice));
	}
	return gpus;
}

uint32_t VulkanContext::createDevice(const VulkanGPU gpu, const QueueFamilySelector& queues, const std::vector<const char*>& extensions, const VkPhysicalDeviceFeatures& features)
{
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	if (m_validationLayersEnabled)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}
	if (!extensions.empty())
	{
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = extensions.data();
	}
	else
	{
		deviceCreateInfo.enabledExtensionCount = 0;
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	for (const uint32_t index : queues.getUniqueIndices())
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = index;
		queueCreateInfo.queueCount = static_cast<uint32_t>(queues.m_selections[index].priorities.size());
		queueCreateInfo.pQueuePriorities = queues.m_selections[index].priorities.data();
		if (queues.m_selections[index].familyFlags & QueueFamilyTypeBits::PROTECTED)
		{
			queueCreateInfo.flags = VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT;
		}
		queueCreateInfos.push_back(queueCreateInfo);
	}
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

	deviceCreateInfo.pEnabledFeatures = &features;

	VkDevice device;
	if (const VkResult ret = vkCreateDevice(gpu.m_vkHandle, &deviceCreateInfo, nullptr, &device); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create logical device, error: ") + string_VkResult(ret));
	}

	m_devices.push_back({gpu, device});
	return m_devices.back().getID();
}

VulkanDevice& VulkanContext::getDevice(const uint32_t index)
{
	for (auto& device : m_devices)
	{
		if (device.getID() == index)
		{
			return device;
		}
	}

    Logger::print("Device search failed out of " + std::to_string(m_devices.size()) + " devices", Logger::LevelBits::DEBUG);
	throw std::runtime_error("Device (ID:" + std::to_string(index) + ") not found");
}

void VulkanContext::freeDevice(const uint32_t index)
{
	for (auto it = m_devices.begin(); it != m_devices.end(); ++it)
	{
		if (it->getID() == index)
		{
            it->free();
			m_devices.erase(it);
			break;
		}
	}
}

void VulkanContext::freeDevice(const VulkanDevice& device)
{
	freeDevice(device.getID());
}

void VulkanContext::free()
{
	for (auto& device : m_devices)
	{
		device.free();
	}
	m_devices.clear();

    if (m_validationLayersEnabled)
    {
        DestroyDebugUtilsMessengerEXT(m_vkHandle, m_debugMessenger, nullptr);
        Logger::print("Destroyed debug messenger", Logger::LevelBits::DEBUG);
    }

	vkDestroyInstance(m_vkHandle, nullptr);
    Logger::print("Destroyed vulkan context", Logger::LevelBits::INFO);
	m_vkHandle = VK_NULL_HANDLE;
}

VkInstance VulkanContext::getHandle()
{
	return m_vkHandle;
}
