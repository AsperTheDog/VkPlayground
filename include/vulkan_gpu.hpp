#pragma once
#include <span>
#include <Volk/volk.h>

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
	[[nodiscard]] VkSurfaceCapabilitiesKHR getCapabilities(const VkSurfaceKHR& p_Surface) const;

    [[nodiscard]] uint32_t getSupportedExtensionCount() const;
    [[nodiscard]] void getSupportedExtensions(VkExtensionProperties p_Container[]) const;

	[[nodiscard]] GPUQueueStructure getQueueFamilies() const;

	[[nodiscard]] bool isFormatSupported(VkSurfaceKHR p_Surface, VkSurfaceFormatKHR p_Format) const;
	[[nodiscard]] VkSurfaceFormatKHR getClosestFormat(const VkSurfaceKHR& p_Surface, VkSurfaceFormatKHR p_Format) const;
	[[nodiscard]] VkSurfaceFormatKHR getFirstFormat(const VkSurfaceKHR& p_Surface) const;
	[[nodiscard]] VkFormatProperties getFormatProperties(VkFormat P_Format) const;
	[[nodiscard]] VkFormat findSupportedFormat(std::span<const VkFormat> P_Candidates, VkImageTiling p_Tiling, VkFormatFeatureFlags p_Features) const;

    [[nodiscard]] VkPhysicalDevice getHandle() const { return m_VkHandle; }

	VkPhysicalDevice operator*() const;

private:
	explicit VulkanGPU(VkPhysicalDevice p_PhysicalDevice);

	VkPhysicalDevice m_VkHandle = VK_NULL_HANDLE;

	friend class VulkanContext;
	friend class GPUQueueStructure;
	friend class MemoryStructure;
};

