#pragma once
#include <unordered_map>

#include "vulkan_context.hpp"
#include "utils/identifiable.hpp"
#include "vulkan_memory.hpp"

class VulkanDevice;
class VulkanMemoryBarrierBuilder;

class VulkanImageView final : public VulkanDeviceSubresource
{
public:
    [[nodiscard]] VkImageView operator*() const { return m_VkHandle; }

private:
    void free() override;

    VulkanImageView(ResourceID p_Device, VkImageView p_VkHandle);

    VkImageView m_VkHandle = VK_NULL_HANDLE;

    friend class VulkanImage;
    friend class VulkanDevice;
};

class VulkanImageSampler final : public VulkanDeviceSubresource
{
public:
    [[nodiscard]] VkSampler operator*() const { return m_VkHandle; }

private:
    void free() override;

    VulkanImageSampler(ResourceID p_Device, VkSampler p_VkHandle);

    VkSampler m_VkHandle = VK_NULL_HANDLE;

    friend class VulkanImage;
    friend class VulkanDevice;
};

class VulkanImage final : public VulkanDeviceSubresource
{
public:
	[[nodiscard]] VkMemoryRequirements getMemoryRequirements() const;
	
	void allocateFromIndex(uint32_t p_MemoryIndex);
	void allocateFromFlags(VulkanMemoryAllocator::MemoryPropertyPreferences p_MemoryProperties);

	ResourceID createImageView(VkFormat p_Format, VkImageAspectFlags p_AspectFlags);
    VulkanImageView& getImageView(ResourceID p_ImageView);
    [[nodiscard]] const VulkanImageView& getImageView(ResourceID p_ImageView) const;
	void freeImageView(ResourceID p_ImageView);
    void freeImageView(const VulkanImageView& p_ImageView);

    ResourceID createSampler(VkFilter p_Filter, VkSamplerAddressMode p_SamplerAddressMode);
    VulkanImageSampler& getSampler(ResourceID p_Sampler);
    [[nodiscard]] const VulkanImageSampler& getSampler(ResourceID p_Sampler) const;
    void freeSampler(ResourceID p_Sampler);
    void freeSampler(const VulkanImageSampler& p_Sampler);

	[[nodiscard]] VkExtent3D getSize() const;
	[[nodiscard]] VkImageType getType() const;
	[[nodiscard]] VkImageLayout getLayout() const;

	VkImage operator*() const;

private:
    [[nodiscard]] VulkanImageView* getImageViewPtr(ResourceID p_ImageView) const;
    [[nodiscard]] VulkanImageSampler* getSamplerPtr(ResourceID p_Sampler) const;

	void free() override;

	VulkanImage(ResourceID p_Device, VkImage p_VkHandle, VkExtent3D p_Size, VkImageType p_Type, VkImageLayout p_Layout);

	void setBoundMemory(const MemoryChunk::MemoryBlock& p_MemoryRegion);

	MemoryChunk::MemoryBlock m_MemoryRegion;
	VkExtent3D m_Size{};
	VkImageType m_Type;
	VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;

    uint32_t m_QueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	
	VkImage m_VkHandle = VK_NULL_HANDLE;

    ARENA_UMAP(m_ImageViews, ResourceID, VulkanImageView*);
    ARENA_UMAP(m_Samplers, ResourceID, VulkanImageSampler*);

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
	friend class VulkanSwapchain;
    friend class VulkanMemoryBarrierBuilder;
};