#pragma once

#include "vulkan_extension_management.hpp"
#include "vulkan_memory.hpp"

#ifndef _WIN32
    #define HANDLE int
#endif



class VulkanExternalMemoryExtension final : public VulkanDeviceExtension
{
public:
    static VulkanExternalMemoryExtension* get(const VulkanDevice& p_Device);
    static VulkanExternalMemoryExtension* get(ResourceID p_DeviceID);

    explicit VulkanExternalMemoryExtension(ResourceID p_DeviceID);

    [[nodiscard]] VkBaseInStructure* getExtensionStruct() const override { return nullptr; }
    [[nodiscard]] VkStructureType getExtensionStructType() const override { return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR; }

    void free() override {}

    [[nodiscard]] ResourceID createExternalImage(VkImageType p_Type, VkFormat p_Format, VkExtent3D p_Extent, VkImageUsageFlags p_Usage, VkImageCreateFlags p_Flags, VkImageTiling p_Tiling = VK_IMAGE_TILING_OPTIMAL) const;

    void allocateExportFromIndex(ResourceID p_Resource, uint32_t p_MemoryIndex) const;
    void allocateExportFromFlags(ResourceID p_Resource, VulkanMemoryAllocator::MemoryPropertyPreferences p_MemoryProperties) const;

    HANDLE getResourceOpaqueHandle(ResourceID p_Resource) const;

    std::string getMainExtensionName() override { return VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME; }
    std::vector<std::string> getExtraExtensionNames() override;
};
