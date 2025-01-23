#pragma once
#include "vulkan_memory.hpp"
#include "utils/identifiable.hpp"

class VulkanDevice;

class VulkanBuffer final : public VulkanDeviceSubresource
{
public:
	[[nodiscard]] VkMemoryRequirements getMemoryRequirements() const;
	
	void allocateFromIndex(uint32_t p_MemoryIndex);
	void allocateFromFlags(VulkanMemoryAllocator::MemoryPropertyPreferences p_MemoryProperties);

	void* map(VkDeviceSize p_Size, VkDeviceSize p_Offset);
	void unmap();

	[[nodiscard]] bool isMemoryMapped() const;
	[[nodiscard]] void* getMappedData() const;
	[[nodiscard]] VkDeviceSize getSize() const;

	VkBuffer operator*() const;

	[[nodiscard]] bool isMemoryBound() const;
	[[nodiscard]] uint32_t getBoundMemoryType() const;
    [[nodiscard]] VkDeviceAddress getDeviceAddress() const;

private:
	void free() override;

	VulkanBuffer(ResourceID p_Device, VkBuffer p_VkHandle, VkDeviceSize p_Size);

	void setBoundMemory(const MemoryChunk::MemoryBlock& p_MemoryRegion);

private:
    VkBuffer m_VkHandle = VK_NULL_HANDLE;

	MemoryChunk::MemoryBlock m_MemoryRegion;
	VkDeviceSize m_Size = 0;
	void* m_MappedData = nullptr;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
};

