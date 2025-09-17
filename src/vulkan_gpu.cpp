#include "vulkan_gpu.hpp"

#include <span>
#include <stdexcept>

#include "vulkan_context.hpp"
#include "vulkan_queues.hpp"

VkPhysicalDeviceProperties VulkanGPU::getProperties() const
{
    VkPhysicalDeviceProperties l_Properties;
    vkGetPhysicalDeviceProperties(m_VkHandle, &l_Properties);
    return l_Properties;
}

VkPhysicalDeviceFeatures VulkanGPU::getFeatures() const
{
    VkPhysicalDeviceFeatures l_Features;
    vkGetPhysicalDeviceFeatures(m_VkHandle, &l_Features);
    return l_Features;
}

VkPhysicalDeviceMemoryProperties VulkanGPU::getMemoryProperties() const
{
    VkPhysicalDeviceMemoryProperties l_MemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_VkHandle, &l_MemoryProperties);
    return l_MemoryProperties;
}

VkSurfaceCapabilitiesKHR VulkanGPU::getCapabilities(const VkSurfaceKHR& p_Surface) const
{
    VkSurfaceCapabilitiesKHR l_Capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_VkHandle, p_Surface, &l_Capabilities);
    return l_Capabilities;
}

uint32_t VulkanGPU::getSupportedExtensionCount() const
{
    uint32_t l_ExtensionCount;
    vkEnumerateDeviceExtensionProperties(m_VkHandle, nullptr, &l_ExtensionCount, nullptr);
    return l_ExtensionCount;
}

void VulkanGPU::getSupportedExtensions(VkExtensionProperties p_Container[]) const
{
    uint32_t l_ExtensionCount = getSupportedExtensionCount();
    vkEnumerateDeviceExtensionProperties(m_VkHandle, nullptr, &l_ExtensionCount, p_Container);
}

bool VulkanGPU::supportsExtension(const std::string_view p_Extension) const
{
    const uint32_t l_ExtensionCount = getSupportedExtensionCount();
    std::vector<VkExtensionProperties> l_Extensions(l_ExtensionCount);
    getSupportedExtensions(l_Extensions.data());
    for (const VkExtensionProperties& l_Extension : l_Extensions)
    {
        if (p_Extension == l_Extension.extensionName)
        {
            return true;
        }
    }
    return false;
}

GPUQueueStructure VulkanGPU::getQueueFamilies() const
{
    return GPUQueueStructure(*this);
}

bool VulkanGPU::isFormatSupported(const VkSurfaceKHR p_Surface, const VkSurfaceFormatKHR p_Format) const
{
    uint32_t l_FormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_VkHandle, p_Surface, &l_FormatCount, nullptr);
    TRANS_VECTOR(l_Formats, VkSurfaceFormatKHR);
    l_Formats.resize(l_FormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_VkHandle, p_Surface, &l_FormatCount, l_Formats.data());
    for (const VkSurfaceFormatKHR& l_AvailableFormat : l_Formats)
    {
        if (l_AvailableFormat.colorSpace == p_Format.colorSpace && l_AvailableFormat.format == p_Format.format)
        {
            return true;
        }
    }
    return false;
}

VkSurfaceFormatKHR VulkanGPU::getClosestFormat(const VkSurfaceKHR& p_Surface, const VkSurfaceFormatKHR p_Format) const
{
    uint32_t l_FormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_VkHandle, p_Surface, &l_FormatCount, nullptr);
    TRANS_VECTOR(l_Formats, VkSurfaceFormatKHR);
    l_Formats.resize(l_FormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_VkHandle, p_Surface, &l_FormatCount, l_Formats.data());
    std::optional<VkSurfaceFormatKHR> l_FormatMatch = std::nullopt;
    std::optional<VkSurfaceFormatKHR> l_ColorMatch = std::nullopt;
    for (const VkSurfaceFormatKHR& l_AvailableFormat : l_Formats)
    {
        if (l_AvailableFormat.colorSpace == p_Format.colorSpace && l_AvailableFormat.format == p_Format.format)
        {
            return l_AvailableFormat;
        }

        if (l_AvailableFormat.format == p_Format.format && !l_FormatMatch.has_value())
        {
            l_FormatMatch = l_AvailableFormat;
        }

        else if (l_AvailableFormat.colorSpace == p_Format.colorSpace && !l_ColorMatch.has_value())
        {
            l_ColorMatch = l_AvailableFormat;
        }
    }
    if (l_FormatMatch.has_value())
    {
        return l_FormatMatch.value();
    }

    if (l_ColorMatch.has_value())
    {
        return l_ColorMatch.value();
    }

    return l_Formats[0];
}

VkSurfaceFormatKHR VulkanGPU::getFirstFormat(const VkSurfaceKHR& p_Surface) const
{
    uint32_t l_FormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_VkHandle, p_Surface, &l_FormatCount, nullptr);
    TRANS_VECTOR(l_Formats, VkSurfaceFormatKHR);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_VkHandle, p_Surface, &l_FormatCount, l_Formats.data());
    return l_Formats[0];
}

VkFormatProperties VulkanGPU::getFormatProperties(const VkFormat P_Format) const
{
    VkFormatProperties l_FormatProperties;
    vkGetPhysicalDeviceFormatProperties(m_VkHandle, P_Format, &l_FormatProperties);
    return l_FormatProperties;
}

VkFormat VulkanGPU::findSupportedFormat(const std::span<const VkFormat> P_Candidates, const VkImageTiling p_Tiling, const VkFormatFeatureFlags p_Features) const
{
    for (const VkFormat l_Format : P_Candidates)
    {
        const VkFormatProperties l_Props = getFormatProperties(l_Format);
        if (p_Tiling == VK_IMAGE_TILING_LINEAR && (l_Props.linearTilingFeatures & p_Features) == p_Features)
        {
            return l_Format;
        }
        if (p_Tiling == VK_IMAGE_TILING_OPTIMAL && (l_Props.optimalTilingFeatures & p_Features) == p_Features)
        {
            return l_Format;
        }
    }

    throw std::runtime_error("Failed to find supported format: " + std::to_string(P_Candidates.size()) + " candidates");
}

VkPhysicalDevice VulkanGPU::operator*() const
{
    return m_VkHandle;
}

VulkanGPU::VulkanGPU(const VkPhysicalDevice p_PhysicalDevice)
    : m_VkHandle(p_PhysicalDevice) {}
