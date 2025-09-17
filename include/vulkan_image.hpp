#pragma once
#include <unordered_map>

#include "vulkan_buffer.hpp"
#include "vulkan_context.hpp"
#include "utils/identifiable.hpp"

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

class VulkanImage final : public VulkanMemArray
{
public:
    [[nodiscard]] VkMemoryRequirements getMemoryRequirements() const override;

    void allocateFromIndex(uint32_t p_MemoryIndex) override;
    void allocateFromFlags(VulkanMemoryAllocator::MemoryPropertyPreferences p_MemoryProperties) override;

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
    [[nodiscard]] uint32_t getFlatSize() const;
    [[nodiscard]] VkImageType getType() const;
    [[nodiscard]] VkImageLayout getLayout() const;
    [[nodiscard]] uint32_t getQueue() const;

    void setLayout(VkImageLayout p_Layout);
    void setQueue(uint32_t p_QueueFamilyIndex);

    VkImage operator*() const;

private:
    [[nodiscard]] VulkanImageView* getImageViewPtr(ResourceID p_ImageView) const;
    [[nodiscard]] VulkanImageSampler* getSamplerPtr(ResourceID p_Sampler) const;

    void free() override;

    VulkanImage(ResourceID p_Device, VkImage p_VkHandle, VkExtent3D p_Size, VkImageType p_Type, VkImageLayout p_Layout);

    void setBoundMemory(const MemoryChunk::MemoryBlock& p_MemoryRegion) override;

    VkExtent3D m_Size{};
    VkImageType m_Type;
    VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage m_VkHandle = VK_NULL_HANDLE;

    ARENA_UMAP(m_ImageViews, ResourceID, VulkanImageView*);
    ARENA_UMAP(m_Samplers, ResourceID, VulkanImageSampler*);

    friend class VulkanDevice;
    friend class VulkanCommandBuffer;
    friend class VulkanSwapchain;
    friend class VulkanMemoryBarrierBuilder;
    friend class VulkanExternalMemoryExtension;
};
