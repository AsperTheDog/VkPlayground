#pragma once
#include <vector>
#include <vulkan/vulkan.h>

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
	[[nodiscard]] VkSurfaceFormatKHR getClosestFormat(const VkSurfaceKHR& surface, VkSurfaceFormatKHR format) const;
	[[nodiscard]] VkSurfaceFormatKHR getFirstFormat(const VkSurfaceKHR& surface) const;
	[[nodiscard]] VkFormatProperties getFormatProperties(VkFormat format) const;
	[[nodiscard]] VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

	[[nodiscard]] VkPhysicalDevice getHandle() const;
	[[nodiscard]]

	VkPhysicalDevice operator*() const;

private:
	explicit VulkanGPU(VkPhysicalDevice physicalDevice);

	VkPhysicalDevice m_vkHandle = VK_NULL_HANDLE;

	friend class VulkanContext;
	friend class GPUQueueStructure;
	friend class MemoryStructure;
};

