#pragma once
#include <map>
#include <span>

#include "vulkan_memory.hpp"
#include "ext/vulkan_extension_management.hpp"

class VulkanAccelerationStructure;

class VulkanAccelerationStructureExtension final : public VulkanDeviceExtension
{
public:
    struct ModelData
    {
        struct VertexData
        {
            ResourceID buffer;
            VkFormat format;
            VkDeviceSize stride;
        };

        struct IndexData
        {
            ResourceID buffer;
            VkIndexType format;
        };

        VertexData vertexBuffer;
        VkDeviceSize vertexOffset = 0;
        IndexData indexBuffer;
        VkDeviceSize indexByteOffset = 0;
        VkDeviceSize indexCount = VK_WHOLE_SIZE;
        ResourceID transformBuffer = UINT32_MAX;
    };

    static VulkanAccelerationStructureExtension* get(const VulkanDevice& p_Device);
    static VulkanAccelerationStructureExtension* get(ResourceID p_DeviceID);

    VulkanAccelerationStructureExtension(ResourceID p_DeviceID, bool p_EnableStructure, bool p_EnableIndirectBuild, bool p_EnableCaptureReplay, bool p_EnableHostCommands, bool p_EnableUpdateAfterBuild);

    [[nodiscard]] VkBaseInStructure* getExtensionStruct() const override;
    [[nodiscard]] VkStructureType getExtensionStructType() const override { return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR; }

    ResourceID createBLASFromModels(std::span<const ModelData> p_Models, uint32_t p_BufferQueueFamilyIndex);

    void free() override {}

    std::string getMainExtensionName() override { return "VK_KHR_acceleration_structure"; }

private:
    bool m_EnableStructure;
    bool m_EnableIndirectBuild;
    bool m_EnableCaptureReplay;
    bool m_EnableHostCommands;
    bool m_EnableUpdateAfterBuild;

    std::map<ResourceID, VulkanAccelerationStructure*> m_AccStructures;
};

class VulkanAccelerationStructure final : public VulkanDeviceSubresource
{
public:
    VkAccelerationStructureKHR operator*() const { return m_VkHandle; }

    void allocateFromIndex(uint32_t p_MemoryIndex) const;
    void allocateFromFlags(VulkanMemoryAllocator::MemoryPropertyPreferences p_MemoryProperties) const;

private:
    void free() override;

    VulkanAccelerationStructure(ResourceID p_Device, VkAccelerationStructureKHR p_VkHandle, ResourceID p_Buffer);

    VkAccelerationStructureKHR m_VkHandle = VK_NULL_HANDLE;
    ResourceID m_Buffer;
    MemoryChunk::MemoryBlock m_MemoryRegion;

    friend class VulkanAccelerationStructureExtension;
};
