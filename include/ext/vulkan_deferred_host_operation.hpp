#pragma once
#include "ext/vulkan_extension_management.hpp"

class VulkanDeferredHostOperationsExtension : public VulkanDeviceExtension
{
public:
    static VulkanDeferredHostOperationsExtension* get(const VulkanDevice& p_Device);
    static VulkanDeferredHostOperationsExtension* get(ResourceID p_DeviceID);

    explicit VulkanDeferredHostOperationsExtension(const ResourceID p_ID) : VulkanDeviceExtension(p_ID) {}

    [[nodiscard]] VulkanExtensionElem* getExtensionStruct() const override { return nullptr; }
    [[nodiscard]] VkStructureType getExtensionStructType() const override { return VK_STRUCTURE_TYPE_MAX_ENUM; }

    void free() override {}
};

