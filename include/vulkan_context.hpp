#pragma once
#include <vulkan/vulkan.h>
#include <vector>

#include "vulkan_device.hpp"
#include "vulkan_queues.hpp"

class VulkanGPU;

class VulkanContext : public Identifiable
{
public:
	static void init(uint32_t vulkanApiVersion, bool enableValidationLayers, bool assertOnError, std::vector<const char*> extensions);

	static [[nodiscard]] uint32_t getGPUCount();
	static [[nodiscard]] std::vector<VulkanGPU> getGPUs();

	static uint32_t createDevice(VulkanGPU gpu, const QueueFamilySelector& queues, const std::vector<const char*>& extensions, const VkPhysicalDeviceFeatures& features, void* nextInfo = nullptr);
	static VulkanDevice& getDevice(uint32_t index);
	static void freeDevice(uint32_t index);
	static void freeDevice(const VulkanDevice& device);

	static void free();

	static VkInstance getHandle();

private:
	static bool checkValidationLayerSupport();
	static bool areExtensionsSupported(const std::vector<const char*>& extensions);
    static void setupDebugMessenger();

	inline static VkInstance m_vkHandle = VK_NULL_HANDLE;
	inline static bool m_validationLayersEnabled = false;

	inline static std::vector<VulkanDevice> m_devices{};

    inline static VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

	friend class SDLWindow;
};

