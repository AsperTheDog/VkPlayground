#pragma once
#include <Volk/volk.h>
#include <vector>

#include "vulkan_device.hpp"
#include "vulkan_queues.hpp"

class VulkanGPU;
class VulkanDeviceExtensionManager;

class VulkanContext : public Identifiable
{
public:
	static void init(uint32_t p_VulkanApiVersion, bool p_EnableValidationLayers, bool p_AssertOnError, std::vector<const char*> p_Extensions);

	static [[nodiscard]] uint32_t getGPUCount();
	static [[nodiscard]] std::vector<VulkanGPU> getGPUs();

	static ResourceID createDevice(VulkanGPU p_GPU, const QueueFamilySelector& p_Queues, const VulkanDeviceExtensionManager* p_Extensions, const VkPhysicalDeviceFeatures& p_Features);
	static VulkanDevice& getDevice(ResourceID p_Index);
	static void freeDevice(ResourceID p_Index);
	static void freeDevice(const VulkanDevice& p_Device);

	static void free();

	static VkInstance getHandle();

private:
	static bool checkValidationLayerSupport();
	static bool areExtensionsSupported(const std::vector<const char*>& p_Extensions);
    static void setupDebugMessenger();

	inline static VkInstance m_VkHandle = VK_NULL_HANDLE;
	inline static bool m_ValidationLayersEnabled = false;

	inline static std::vector<VulkanDevice*> m_Devices{};

    inline static VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

	friend class SDLWindow;
};

