#pragma once
#include "ext/vulkan_extension_management.hpp"

class VulkanDeferredHostOperationsExtension final : public VulkanDeviceExtension
{
public:
    static VulkanDeferredHostOperationsExtension* get(const VulkanDevice& p_Device);
    static VulkanDeferredHostOperationsExtension* get(ResourceID p_DeviceID);

    explicit VulkanDeferredHostOperationsExtension(const ResourceID p_ID) : VulkanDeviceExtension(p_ID) {}

    [[nodiscard]] VkBaseInStructure* getExtensionStruct() const override { return nullptr; }
    [[nodiscard]] VkStructureType getExtensionStructType() const override { return VK_STRUCTURE_TYPE_MAX_ENUM; }

    void free() override {}

    std::string getMainExtensionName() override { return "VK_KHR_deferred_host_operations"; }
};
