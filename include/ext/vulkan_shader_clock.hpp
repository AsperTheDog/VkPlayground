#pragma once
#include "vulkan_extension_management.hpp"

class VulkanShaderClockExtension final : public VulkanDeviceExtension
{
public:
    static VulkanShaderClockExtension* get(const VulkanDevice& p_Device);
    static VulkanShaderClockExtension* get(ResourceID p_DeviceID);

    explicit VulkanShaderClockExtension(ResourceID p_DeviceID, bool p_EnableDeviceClock, bool p_EnableSubgroupClock);

    [[nodiscard]] VkBaseInStructure* getExtensionStruct() const override;
    [[nodiscard]] VkStructureType getExtensionStructType() const override { return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR; }

    void free() override {}
    std::string getMainExtensionName() override { return VK_KHR_SHADER_CLOCK_EXTENSION_NAME; }

private:
    bool m_EnableDeviceClock;
    bool m_EnableSubgroupClock;
};

