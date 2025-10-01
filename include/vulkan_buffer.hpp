#pragma once
#include "vulkan_memory.hpp"
#include "utils/identifiable.hpp"

class VulkanDevice;

class VulkanMemArray : public VulkanDeviceSubresource
{
public:
    using MemoryPreferences = VulkanMemoryAllocatorVMA::MemoryPreferences;

    [[nodiscard]] virtual VkMemoryRequirements getMemoryRequirements() const = 0;

    virtual void allocate(MemoryPreferences p_Preferences);

    [[nodiscard]] bool isMemoryBound() const;
    [[nodiscard]] uint32_t getBoundMemoryType() const;
    [[nodiscard]] VkDeviceSize getMemorySize() const;

    [[nodiscard]] VmaAllocation getAllocation() const;

    void* map(VkDeviceSize p_Size, VkDeviceSize p_Offset);
    void unmap();

    [[nodiscard]] bool isMemoryMapped() const;
    [[nodiscard]] void* getMappedData() const;

protected:
    explicit VulkanMemArray(const ResourceID p_ID) : VulkanDeviceSubresource(p_ID) {}

    virtual void setBoundMemory(VmaAllocation p_Allocation) = 0;
    
    VmaAllocation m_Allocation = VK_NULL_HANDLE;

    uint32_t m_QueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    void* m_MappedData = nullptr;

    friend class VulkanExternalMemoryExtension;
};

class VulkanBuffer final : public VulkanMemArray
{
public:
    struct Config
    {
        VkDeviceSize size;
        VkBufferUsageFlags usage;
        uint32_t ownerQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    };

    using MemoryPreferences = VulkanMemoryAllocatorVMA::MemoryPreferences;

    [[nodiscard]] VkMemoryRequirements getMemoryRequirements() const override;

    void allocate(MemoryPreferences p_Preferences) override;

    [[nodiscard]] VkDeviceSize getSize() const;
    [[nodiscard]] uint32_t getQueue() const;

    VkBuffer operator*() const;

    [[nodiscard]] VkDeviceAddress getDeviceAddress() const;

    void setQueue(uint32_t p_QueueFamilyIndex);

private:
    void free() override;

    VulkanBuffer(ResourceID p_Device, VkBuffer p_VkHandle, VkDeviceSize p_Size);
    
    void setBoundMemory(VmaAllocation p_Allocation) override;

    VkBuffer m_VkHandle = VK_NULL_HANDLE;

    VkDeviceSize m_Size = 0;

    friend class VulkanDevice;
    friend class VulkanCommandBuffer;
    friend class VulkanMemoryBarrierBuilder;
};
