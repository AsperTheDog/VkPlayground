#include "ext/vulkan_acceleration_structure.hpp"

#include "vulkan_context.hpp"

VulkanAccelerationStructureExtension* VulkanAccelerationStructureExtension::get(const VulkanDevice& p_Device)
{
    return p_Device.getExtensionManager()->getExtension<VulkanAccelerationStructureExtension>(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
}

VulkanAccelerationStructureExtension* VulkanAccelerationStructureExtension::get(ResourceID p_DeviceID)
{
    return VulkanContext::getDevice(p_DeviceID).getExtensionManager()->getExtension<VulkanAccelerationStructureExtension>(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
}

VulkanAccelerationStructureExtension::VulkanAccelerationStructureExtension(const ResourceID p_DeviceID, const bool p_EnableStructure, const bool p_EnableIndirectBuild, const bool p_EnableCaptureReplay, const bool p_EnableHostCommands, const bool p_EnableUpdateAfterBuild)
    : VulkanDeviceExtension(p_DeviceID), m_EnableStructure(p_EnableStructure), m_EnableIndirectBuild(p_EnableIndirectBuild), m_EnableCaptureReplay(p_EnableCaptureReplay), m_EnableHostCommands(p_EnableHostCommands), m_EnableUpdateAfterBuild(p_EnableUpdateAfterBuild)
{
}

VulkanExtensionElem* VulkanAccelerationStructureExtension::getExtensionStruct() const
{
    VkPhysicalDeviceAccelerationStructureFeaturesKHR* l_Struct = new VkPhysicalDeviceAccelerationStructureFeaturesKHR{};
    l_Struct->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    l_Struct->pNext = nullptr;
    l_Struct->accelerationStructure = m_EnableStructure;
    l_Struct->accelerationStructureIndirectBuild = m_EnableIndirectBuild;
    l_Struct->accelerationStructureCaptureReplay = m_EnableCaptureReplay;
    l_Struct->accelerationStructureHostCommands = m_EnableHostCommands;
    l_Struct->descriptorBindingAccelerationStructureUpdateAfterBind = m_EnableUpdateAfterBuild;
    return reinterpret_cast<VulkanExtensionElem*>(l_Struct);
}
