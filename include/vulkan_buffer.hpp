#pragma once
#include <vulkan/vulkan.h>

#include "vulkan_memory.hpp"
#include "utils/identifiable.hpp"

class VulkanDevice;

class VulkanBuffer final : public VulkanDeviceSubresource
{
public:
	[[nodiscard]] VkMemoryRequirements getMemoryRequirements() const;
	
	void allocateFromIndex(uint32_t memoryIndex);
	void allocateFromFlags(VulkanMemoryAllocator::MemoryPropertyPreferences memoryProperties);

	void* map(VkDeviceSize size, VkDeviceSize offset);
	void unmap();

	[[nodiscard]] bool isMemoryMapped() const;
	[[nodiscard]] void* getMappedData() const;
	[[nodiscard]] VkDeviceSize getSize() const;

	VkBuffer operator*() const;

	[[nodiscard]] bool isMemoryBound() const;
	[[nodiscard]] uint32_t getBoundMemoryType() const;

private:
	void free() override;

	VulkanBuffer(uint32_t device, VkBuffer vkHandle, VkDeviceSize size);

	void setBoundMemory(const MemoryChunk::MemoryBlock& memoryRegion);

private:
    VkBuffer m_vkHandle = VK_NULL_HANDLE;

	MemoryChunk::MemoryBlock m_memoryRegion;
	VkDeviceSize m_size = 0;
	void* m_mappedData = nullptr;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
};

