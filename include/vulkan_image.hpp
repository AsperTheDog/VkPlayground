#pragma once
#include "utils/identifiable.hpp"
#include "vulkan_memory.hpp"

class VulkanDevice;

class VulkanImage final : public VulkanDeviceSubresource
{
public:
	[[nodiscard]] VkMemoryRequirements getMemoryRequirements() const;
	
	void allocateFromIndex(uint32_t p_MemoryIndex);
	void allocateFromFlags(VulkanMemoryAllocator::MemoryPropertyPreferences p_MemoryProperties);

	VkImageView createImageView(VkFormat p_Format, VkImageAspectFlags p_AspectFlags);
	void freeImageView(VkImageView p_ImageView);

    VkSampler createSampler(VkFilter p_Filter, VkSamplerAddressMode p_SamplerAddressMode);
    void freeSampler(VkSampler p_Sampler);

	void transitionLayout(VkImageLayout p_Layout, uint32_t p_ThreadID);

	[[nodiscard]] VkExtent3D getSize() const;
	[[nodiscard]] VkImageType getType() const;
	[[nodiscard]] VkImageLayout getLayout() const;

	VkImage operator*() const;

private:
	void free() override;

	VulkanImage(ResourceID p_Device, VkImage p_VkHandle, VkExtent3D p_Size, VkImageType p_Type, VkImageLayout p_Layout);

	void setBoundMemory(const MemoryChunk::MemoryBlock& p_MemoryRegion);

	MemoryChunk::MemoryBlock m_MemoryRegion;
	VkExtent3D m_Size{};
	VkImageType m_Type;
	VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
	
	VkImage m_VkHandle = VK_NULL_HANDLE;

	std::vector<VkImageView> m_ImageViews;
    std::vector<VkSampler> m_Samplers;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
	friend class VulkanSwapchain;
};