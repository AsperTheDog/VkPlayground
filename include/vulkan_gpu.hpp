#pragma once
#include <span>
#include <string_view>
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
    void getSupportedExtensions(VkExtensionProperties p_Container[]) const;
    [[nodiscard]] bool supportsExtension(std::string_view p_Extension) const;

    [[nodiscard]] GPUQueueStructure getQueueFamilies() const;

    [[nodiscard]] bool isFormatSupported(VkSurfaceKHR p_Surface, VkSurfaceFormatKHR p_Format) const;
    [[nodiscard]] VkSurfaceFormatKHR getClosestFormat(const VkSurfaceKHR& p_Surface, VkSurfaceFormatKHR p_Format) const;
    [[nodiscard]] VkSurfaceFormatKHR getFirstFormat(const VkSurfaceKHR& p_Surface) const;
    [[nodiscard]] VkFormatProperties getFormatProperties(VkFormat p_Format) const;
    [[nodiscard]] VkFormat findSupportedFormat(std::span<const VkFormat> p_Candidates, VkImageTiling p_Tiling, VkFormatFeatureFlags p_Features) const;

    [[nodiscard]] VkPhysicalDevice getHandle() const { return m_VkHandle; }

    VkPhysicalDevice operator*() const;

    bool operator==(const VulkanGPU& p_Other) const { return m_VkHandle == p_Other.m_VkHandle; }

    bool isValid() const { return m_VkHandle != VK_NULL_HANDLE; }

private:
    explicit VulkanGPU(VkPhysicalDevice p_PhysicalDevice);

    VkPhysicalDevice m_VkHandle = VK_NULL_HANDLE;

    friend class VulkanContext;
    friend class GPUQueueStructure;
    friend class MemoryStructure;
};
