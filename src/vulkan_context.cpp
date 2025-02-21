#include "vulkan_context.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_device.hpp"
#include "vulkan_gpu.hpp"
#include "vulkan_queues.hpp"
#include "ext/vulkan_extension_management.hpp"
#include "utils/logger.hpp"

std::array<const char*, 1> g_ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

bool g_assertOnError = false;

VkResult CreateDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_CreateInfo, const VkAllocationCallbacks* p_Allocator, VkDebugUtilsMessengerEXT* p_DebugMessenger) {
	const PFN_vkCreateDebugUtilsMessengerEXT l_Func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	if (l_Func != nullptr) {
		return l_Func(instance, p_CreateInfo, p_Allocator, p_DebugMessenger);
	}
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(const VkInstance p_Instance, const VkDebugUtilsMessengerEXT p_DebugMessenger, const VkAllocationCallbacks* p_Allocator) {
	const PFN_vkDestroyDebugUtilsMessengerEXT l_Func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(p_Instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (l_Func != nullptr) {
		l_Func(p_Instance, p_DebugMessenger, p_Allocator);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT p_MessageSeverity, VkDebugUtilsMessageTypeFlagsEXT p_MessageType, const VkDebugUtilsMessengerCallbackDataEXT* p_CallbackData, void* p_UserData) {
    LOG_ERR("validation layer: ", p_CallbackData->pMessage);

	assert(!g_assertOnError || (p_MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) == 0);

	return VK_FALSE;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& p_CreateInfo) {
	p_CreateInfo = {};
	p_CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	p_CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	p_CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	p_CreateInfo.pfnUserCallback = debugCallback;
}

void VulkanContext::setupDebugMessenger() {
	if (!m_ValidationLayersEnabled) return;

	VkDebugUtilsMessengerCreateInfoEXT l_CreateInfo;
	populateDebugMessengerCreateInfo(l_CreateInfo);

	if (const VkResult l_Ret = CreateDebugUtilsMessengerEXT(m_VkHandle, &l_CreateInfo, nullptr, &m_DebugMessenger); l_Ret != VK_SUCCESS) {
		throw std::runtime_error(std::string("Failed to set up debug messenger, error: ") + string_VkResult(l_Ret));
	}
    LOG_DEBUG("Created debug messenger for vulkan context");
}

void VulkanContext::init(const uint32_t p_VulkanApiVersion, const bool p_EnableValidationLayers, const bool p_AssertOnError, const std::span<const char*> p_Extensions)
{
    const VkResult ret = volkInitialize();
    if (ret != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to initialize volk");
    }

	g_assertOnError = p_AssertOnError;
	m_ValidationLayersEnabled = p_EnableValidationLayers;

#ifdef _DEBUG
	if (p_EnableValidationLayers && !checkValidationLayerSupport())
	{
		throw std::runtime_error("Validation layers requested, but not available");
	}
	if (!areExtensionsSupported(p_Extensions))
	{
		throw std::runtime_error("One or more requested instance extensions are not supported");
	}
#endif

	VkApplicationInfo l_AppInfo{};
	l_AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	l_AppInfo.pApplicationName = "Vulkan Application";
	l_AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	l_AppInfo.pEngineName = "No Engine";
	l_AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	l_AppInfo.apiVersion = p_VulkanApiVersion;

	VkInstanceCreateInfo l_InstanceCreateInfo{};
	l_InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	l_InstanceCreateInfo.pApplicationInfo = &l_AppInfo;

	VkDebugUtilsMessengerCreateInfoEXT l_DebugCreateInfo{};
    TRANS_VECTOR(l_Extensions, const char*);
    l_Extensions.reserve(p_Extensions.size() + 1);
    for (const char* l_Ext : p_Extensions)
        l_Extensions.push_back(l_Ext);
	if (p_EnableValidationLayers)
	{
		l_InstanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(g_ValidationLayers.size());
		l_InstanceCreateInfo.ppEnabledLayerNames = g_ValidationLayers.data();

        l_Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		populateDebugMessengerCreateInfo(l_DebugCreateInfo);
		l_InstanceCreateInfo.pNext = &l_DebugCreateInfo;
	}
	else
	{
		l_InstanceCreateInfo.enabledLayerCount = 0;
	}
	l_InstanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(l_Extensions.size());
	l_InstanceCreateInfo.ppEnabledExtensionNames = l_Extensions.data();

	if (const VkResult l_Ret = vkCreateInstance(&l_InstanceCreateInfo, nullptr, &m_VkHandle); l_Ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create Vulkan instance, error:") + string_VkResult(l_Ret));
	}

    LOG_DEBUG("Created vulkan context");

    volkLoadInstance(m_VkHandle);

	if (m_ValidationLayersEnabled)
		setupDebugMessenger();
}

void VulkanContext::initializeTransientMemory(const size_t p_Size)
{
    m_TransientAllocator.initialize(p_Size);
}

void VulkanContext::initializeTransientMemory(uint8_t* p_Container, const size_t p_Size, const bool p_ShouldDelete)
{
    m_TransientAllocator.initialize(p_Container, p_Size, p_ShouldDelete);
}

void VulkanContext::initializeArenaMemory(const size_t p_Size)
{
    m_ArenaAllocator.initialize(p_Size);
    ARENA_VECTOR(l_Devices, VulkanDevice*);
    l_Devices.reserve(m_Devices.size());
    std::ranges::move(m_Devices, std::back_inserter(l_Devices));
    m_Devices.swap(l_Devices);
}

uint32_t VulkanContext::getGPUCount()
{
	uint32_t l_GPUCount = 0;
	vkEnumeratePhysicalDevices(m_VkHandle, &l_GPUCount, nullptr);
	return l_GPUCount;
}

void VulkanContext::getGPUs(VulkanGPU p_Container[])
{
	uint32_t l_GPUCount = 0;
	vkEnumeratePhysicalDevices(m_VkHandle, &l_GPUCount, nullptr);
    TRANS_VECTOR(l_PhysicalDevices, VkPhysicalDevice);
    l_PhysicalDevices.resize(l_GPUCount);
	vkEnumeratePhysicalDevices(m_VkHandle, &l_GPUCount, l_PhysicalDevices.data());

    for (uint32_t i = 0; i < l_GPUCount; i++)
    {
        p_Container[i] = VulkanGPU{ l_PhysicalDevices[i] };
    }
}

uint32_t VulkanContext::createDevice(const VulkanGPU p_GPU, const QueueFamilySelector& p_Queues, const VulkanDeviceExtensionManager* p_Extensions, const VkPhysicalDeviceFeatures& p_Features)
{
	VkDeviceCreateInfo l_DeviceCreateInfo{};
	l_DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    VulkanExtensionChain l_Chain{};
    if (p_Extensions != nullptr)
    {
        p_Extensions->addExtensionsToChain(l_Chain);
    }
    l_DeviceCreateInfo.pNext = l_Chain.getChain();
    const void* pNext = l_DeviceCreateInfo.pNext;
    while (pNext != nullptr)
    {
        const VkBaseInStructure* l_Struct = static_cast<const VkBaseInStructure*>(pNext);
        pNext = l_Struct->pNext;
    }

	if (m_ValidationLayersEnabled)
	{
		l_DeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(g_ValidationLayers.size());
		l_DeviceCreateInfo.ppEnabledLayerNames = g_ValidationLayers.data();
	}
	else
	{
		l_DeviceCreateInfo.enabledLayerCount = 0;
	}
    TRANS_VECTOR(l_ExtensionNames, const char*);
    l_ExtensionNames.resize(p_Extensions->getExtensionCount());
	if (p_Extensions != nullptr && !p_Extensions->isEmpty())
	{
        p_Extensions->populateExtensionNames(l_ExtensionNames.data());
		l_DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(l_ExtensionNames.size());
		l_DeviceCreateInfo.ppEnabledExtensionNames = l_ExtensionNames.data();
	}
	else
	{
		l_DeviceCreateInfo.enabledExtensionCount = 0;
	}

    TRANS_VECTOR(l_QueueCreateInfos, VkDeviceQueueCreateInfo);
    const std::vector<uint32_t> l_QueueFamilyIndices = p_Queues.getUniqueIndices();
    l_QueueCreateInfos.reserve(l_QueueFamilyIndices.size());
	for (const uint32_t l_Index : l_QueueFamilyIndices)
	{
		VkDeviceQueueCreateInfo l_QueueCreateInfo{};
		l_QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		l_QueueCreateInfo.queueFamilyIndex = l_Index;
		l_QueueCreateInfo.queueCount = static_cast<uint32_t>(p_Queues.m_Selections[l_Index].priorities.size());
		l_QueueCreateInfo.pQueuePriorities = p_Queues.m_Selections[l_Index].priorities.data();
		if (p_Queues.m_Selections[l_Index].familyFlags & QueueFamilyTypeBits::PROTECTED)
		{
			l_QueueCreateInfo.flags = VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT;
		}
		l_QueueCreateInfos.push_back(l_QueueCreateInfo);
	}
	l_DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(l_QueueCreateInfos.size());
	l_DeviceCreateInfo.pQueueCreateInfos = l_QueueCreateInfos.data();

	l_DeviceCreateInfo.pEnabledFeatures = &p_Features;

	VkDevice l_Device;
	if (const VkResult L_Ret = vkCreateDevice(p_GPU.m_VkHandle, &l_DeviceCreateInfo, nullptr, &l_Device); L_Ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create logical device, error: ") + string_VkResult(L_Ret));
	}
    VulkanDeviceExtensionManager* l_ExtManager = nullptr;
    if (p_Extensions != nullptr)
        l_ExtManager = ARENA_ALLOC(VulkanDeviceExtensionManager){ *p_Extensions };
    m_Devices.push_back(ARENA_ALLOC(VulkanDevice){ p_GPU, l_Device, l_ExtManager });
    return m_Devices.back()->getID();
}

VulkanDevice& VulkanContext::getDevice(const ResourceID p_Index)
{
	for (VulkanDevice* l_Device : m_Devices)
	{
		if (l_Device->getID() == p_Index)
		{
			return *l_Device;
		}
	}

    LOG_DEBUG("Device search failed out of ", m_Devices.size(), " devices");
	throw std::runtime_error("Device (ID:" + std::to_string(p_Index) + ") not found");
}

void VulkanContext::freeDevice(const ResourceID p_Index)
{
	for (auto l_It = m_Devices.begin(); l_It != m_Devices.end(); ++l_It)
	{
		if ((*l_It)->getID() == p_Index)
		{
            VulkanDevice* l_Device = *l_It;
			l_Device->free();
			m_Devices.erase(l_It);
            ARENA_FREE(l_Device, sizeof(VulkanDevice));
			break;
		}
	}
}

void VulkanContext::freeDevice(const VulkanDevice& p_Device)
{
	freeDevice(p_Device.getID());
}

void VulkanContext::free()
{
	for (VulkanDevice* l_Device : m_Devices)
	{
		l_Device->free();
        ARENA_FREE(l_Device, sizeof(VulkanDevice));
	}
	m_Devices.clear();

	if (m_ValidationLayersEnabled)
	{
		DestroyDebugUtilsMessengerEXT(m_VkHandle, m_DebugMessenger, nullptr);
        LOG_DEBUG("Destroyed debug messenger");
	}

	vkDestroyInstance(m_VkHandle, nullptr);
    LOG_DEBUG("Destroyed vulkan context");
	m_VkHandle = VK_NULL_HANDLE;
}

VkInstance VulkanContext::getHandle()
{
	return m_VkHandle;
}

void VulkanContext::resetTransMemory()
{
    m_TransientAllocator.reset();
}

void VulkanContext::resetArenaMemory()
{
    m_ArenaAllocator.reset();
}

bool VulkanContext::checkValidationLayerSupport()
{
	uint32_t l_LayerCount;
	vkEnumerateInstanceLayerProperties(&l_LayerCount, nullptr);
    TRANS_VECTOR(l_AvailableLayers, VkLayerProperties);
    l_AvailableLayers.resize(l_LayerCount);
	vkEnumerateInstanceLayerProperties(&l_LayerCount, l_AvailableLayers.data());

	for (const char* l_LayerName : g_ValidationLayers)
	{
		bool layerFound = false;
		for (const VkLayerProperties& layerProperties : l_AvailableLayers)
		{
			if (strcmp(l_LayerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
		{
			return false;
		}
	}
	return true;
}

bool VulkanContext::areExtensionsSupported(const std::span<const char*> p_Extensions)
{
	uint32_t l_ExtensionCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &l_ExtensionCount, nullptr);
    TRANS_VECTOR(l_AvailableExtensions, VkExtensionProperties);
    l_AvailableExtensions.resize(l_ExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &l_ExtensionCount, l_AvailableExtensions.data());

	for (const char* l_ExtensionName : p_Extensions)
	{
		bool l_ExtensionFound = false;
		for (const VkExtensionProperties& l_ExtensionProperties : l_AvailableExtensions)
		{
			if (strcmp(l_ExtensionName, l_ExtensionProperties.extensionName) == 0)
			{
				l_ExtensionFound = true;
				break;
			}
		}
		if (!l_ExtensionFound)
		{
			return false;
		}
	}
	return true;
}
