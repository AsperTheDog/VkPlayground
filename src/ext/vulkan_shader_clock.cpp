#include "ext/vulkan_shader_clock.hpp"

VulkanShaderClockExtension::VulkanShaderClockExtension(const ResourceID p_DeviceID, const bool p_EnableDeviceClock, const bool p_EnableSubgroupClock)
    : VulkanDeviceExtension(p_DeviceID), m_EnableDeviceClock(p_EnableDeviceClock), m_EnableSubgroupClock(p_EnableSubgroupClock)
{
}

VulkanExtensionElem* VulkanShaderClockExtension::getExtensionStruct() const
{
    VkPhysicalDeviceShaderClockFeaturesKHR* l_Struct = new VkPhysicalDeviceShaderClockFeaturesKHR{};
    l_Struct->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR;
    l_Struct->pNext = nullptr;
    l_Struct->shaderSubgroupClock = m_EnableSubgroupClock;
    l_Struct->shaderDeviceClock = m_EnableDeviceClock;
    return reinterpret_cast<VulkanExtensionElem*>(l_Struct);
}
