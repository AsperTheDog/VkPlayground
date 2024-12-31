#pragma once
#include <vulkan/vulkan.h>

#include "utils/identifiable.hpp"
#include "vulkan_memory.hpp"

class VulkanDevice;

class VulkanImage final : public VulkanDeviceSubresource
{
public:
	[[nodiscard]] VkMemoryRequirements getMemoryRequirements() const;
	
	void allocateFromIndex(uint32_t memoryIndex);
	void allocateFromFlags(VulkanMemoryAllocator::MemoryPropertyPreferences memoryProperties);

	VkImageView createImageView(VkFormat format, VkImageAspectFlags aspectFlags);
	void freeImageView(VkImageView imageView);

    VkSampler createSampler(VkFilter filter, VkSamplerAddressMode samplerAddressMode);
    void freeSampler(VkSampler sampler);

	void transitionLayout(VkImageLayout layout, uint32_t threadID);

	[[nodiscard]] VkExtent3D getSize() const;
	[[nodiscard]] VkImageType getType() const;
	[[nodiscard]] VkImageLayout getLayout() const;

	VkImage operator*() const;

private:
	void free() override;

	VulkanImage(uint32_t device, VkImage vkHandle, VkExtent3D size, VkImageType type, VkImageLayout layout);

	void setBoundMemory(const MemoryChunk::MemoryBlock& memoryRegion);

	MemoryChunk::MemoryBlock m_memoryRegion;
	VkExtent3D m_size{};
	VkImageType m_type;
	VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	
	VkImage m_vkHandle = VK_NULL_HANDLE;

	std::vector<VkImageView> m_imageViews;
    std::vector<VkSampler> m_samplers;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
	friend class VulkanSwapchain;
};