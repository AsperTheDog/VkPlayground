#include "ext/vulkan_shader_clock.hpp"

#include "vulkan_device.hpp"


VulkanShaderClockExtension* VulkanShaderClockExtension::get(const VulkanDevice& p_Device)
{
    return p_Device.getExtensionManager()->getExtension<VulkanShaderClockExtension>(VK_KHR_SHADER_CLOCK_EXTENSION_NAME);
}

VulkanShaderClockExtension* VulkanShaderClockExtension::get(const ResourceID p_DeviceID)
{
    return VulkanContext::getDevice(p_DeviceID).getExtensionManager()->getExtension<VulkanShaderClockExtension>(VK_KHR_SHADER_CLOCK_EXTENSION_NAME);
}

VulkanShaderClockExtension::VulkanShaderClockExtension(const ResourceID p_DeviceID, const bool p_EnableDeviceClock, const bool p_EnableSubgroupClock)
    : VulkanDeviceExtension(p_DeviceID), m_EnableDeviceClock(p_EnableDeviceClock), m_EnableSubgroupClock(p_EnableSubgroupClock) {}

VkBaseInStructure* VulkanShaderClockExtension::getExtensionStruct() const
{
    VkPhysicalDeviceShaderClockFeaturesKHR* l_Struct = TRANS_ALLOC(VkPhysicalDeviceShaderClockFeaturesKHR){};
    l_Struct->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR;
    l_Struct->pNext = nullptr;
    l_Struct->shaderSubgroupClock = m_EnableSubgroupClock;
    l_Struct->shaderDeviceClock = m_EnableDeviceClock;
    return reinterpret_cast<VkBaseInStructure*>(l_Struct);
}
