#pragma once
#include "ext/vulkan_extension_management.hpp"

class VulkanAccelerationStructureExtension : public VulkanDeviceExtension
{
public:
    static VulkanAccelerationStructureExtension* get(const VulkanDevice& p_Device);
    static VulkanAccelerationStructureExtension* get(ResourceID p_DeviceID);

    VulkanAccelerationStructureExtension(ResourceID p_DeviceID, bool p_EnableStructure, bool p_EnableIndirectBuild, bool p_EnableCaptureReplay, bool p_EnableHostCommands, bool p_EnableUpdateAfterBuild);
    
    [[nodiscard]] VulkanExtensionElem* getExtensionStruct() const override;
    [[nodiscard]] VkStructureType getExtensionStructType() const override { return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR; }

    void free() override {}

private:
    bool m_EnableStructure;
    bool m_EnableIndirectBuild;
    bool m_EnableCaptureReplay;
    bool m_EnableHostCommands;
    bool m_EnableUpdateAfterBuild;
};

