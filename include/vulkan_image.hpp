#pragma once
#include <vulkan/vulkan.h>

#include "utils/identifiable.hpp"
#include "vulkan_memory.hpp"

class VulkanDevice;

class VulkanImage : public Identifiable
{
public:
	[[nodiscard]] VkMemoryRequirements getMemoryRequirements() const;
	
	void allocateFromIndex(uint32_t memoryIndex);
	void allocateFromFlags(VulkanMemoryAllocator::MemoryPropertyPreferences memoryProperties);

	VkImageView createImageView(VkFormat format, VkImageAspectFlags aspectFlags);
	void freeImageView(VkImageView imageView);

	void transitionLayout(VkImageLayout layout, VkImageAspectFlags aspectFlags, uint32_t srcQueueFamily, uint32_t dstQueueFamily, uint32_t threadID);

	[[nodiscard]] VkExtent3D getSize() const;

	VkImage operator*() const;

private:
	void free();

	VulkanImage(uint32_t device, VkImage vkHandle, VkExtent3D size, VkImageType type, VkImageLayout layout);

	void setBoundMemory(const MemoryChunk::MemoryBlock& memoryRegion);

	MemoryChunk::MemoryBlock m_memoryRegion;
	VkExtent3D m_size{};
	VkImageType m_type;
	VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	
	VkImage m_vkHandle = VK_NULL_HANDLE;
	uint32_t m_device;

	std::vector<VkImageView> m_imageViews;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
};

