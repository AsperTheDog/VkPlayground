#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

class SDLWindow;
class VulkanContext;
class GPUQueueStructure;

class VulkanGPU
{
public:
	VulkanGPU() = default;

	[[nodiscard]] VkPhysicalDeviceProperties getProperties() const;
	[[nodiscard]] VkPhysicalDeviceFeatures getFeatures() const;
	[[nodiscard]] VkPhysicalDeviceMemoryProperties getMemoryProperties() const;
	[[nodiscard]] VkSurfaceCapabilitiesKHR getCapabilities(const VkSurfaceKHR& surface) const;

	[[nodiscard]] GPUQueueStructure getQueueFamilies() const;

	[[nodiscard]] bool isFormatSupported(VkSurfaceKHR surface, VkSurfaceFormatKHR format) const;
	[[nodiscard]] VkSurfaceFormatKHR getClosestFormat(const VkSurfaceKHR& surface, const VkSurfaceFormatKHR format) const;
	[[nodiscard]] VkFormatProperties getFormatProperties(VkFormat format) const;
	[[nodiscard]] VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling, const VkFormatFeatureFlags features) const;

	VkPhysicalDevice operator*() const;

private:
	explicit VulkanGPU(VkPhysicalDevice physicalDevice);

	VkPhysicalDevice m_vkHandle = VK_NULL_HANDLE;

	friend class VulkanContext;
	friend class GPUQueueStructure;
	friend class MemoryStructure;
};

