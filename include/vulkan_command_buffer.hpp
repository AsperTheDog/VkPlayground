#pragma once
#include <map>
#include <span>
#include <vector>
#include <Volk/volk.h>

#include "vulkan_context.hpp"
#include "utils/identifiable.hpp"


class VulkanImage;
class VulkanBuffer;
class VulkanQueue;
class VulkanFence;
class VulkanRenderPass;
class VulkanDevice;
class VulkanDevice;

class VulkanMemoryBarrierBuilder
{
public:
    VulkanMemoryBarrierBuilder(ResourceID p_Device, VkPipelineStageFlags p_SrcStageMask, VkPipelineStageFlags p_DstStageMask, VkDependencyFlags p_DependencyFlags);

    void addAbsoluteMemoryBarrier();

    void addMemoryBarrier(VkAccessFlags p_SrcAccessMask, VkAccessFlags p_DstAccessMask);
    void addBufferMemoryBarrier(ResourceID p_Buffer, VkDeviceSize p_Offset, VkDeviceSize p_Size, VkAccessFlags p_SrcAccessMask, VkAccessFlags p_DstAccessMask, uint32_t p_DstQueueFamily = VK_QUEUE_FAMILY_IGNORED);
    void addImageMemoryBarrier(ResourceID p_Image, VkImageLayout p_NewLayout, uint32_t p_DstQueueFamily = VK_QUEUE_FAMILY_IGNORED, VkAccessFlags p_SrcAccessMask = VK_ACCESS_FLAG_BITS_MAX_ENUM, VkAccessFlags p_DstAccessMask = VK_ACCESS_FLAG_BITS_MAX_ENUM);
    void addImageMemoryBarrier(const VulkanImage& p_Image, VkImageLayout p_NewLayout, uint32_t p_DstQueueFamily = VK_QUEUE_FAMILY_IGNORED, VkAccessFlags p_SrcAccessMask = VK_ACCESS_FLAG_BITS_MAX_ENUM, VkAccessFlags p_DstAccessMask = VK_ACCESS_FLAG_BITS_MAX_ENUM);

private:
    ResourceID m_Device;

    VkPipelineStageFlags m_SrcStageMask;
    VkPipelineStageFlags m_DstStageMask;
    VkDependencyFlags m_DependencyFlags;
    TRANS_VECTOR(m_MemoryBarriers, VkMemoryBarrier);
    TRANS_VECTOR(m_BufferMemoryBarriers, VkBufferMemoryBarrier);
    TRANS_VECTOR(m_ImageMemoryBarriers, VkImageMemoryBarrier);

    struct AccessData
    {
        VkAccessFlags srcAccessMask;
        VkPipelineStageFlags srcStageMask;
        VkAccessFlags dstAccessMask;
        VkPipelineStageFlags dstStageMask;
    };
    static std::map<VkImageLayout, AccessData> s_TransitionMapping;
    friend class VulkanCommandBuffer;
};

class VulkanCommandBuffer final : public VulkanDeviceSubresource
{
public:
    struct WaitSemaphoreData
    {
        ResourceID semaphore;
        VkPipelineStageFlags stages;
    };

	void beginRecording(VkCommandBufferUsageFlags p_Flags = 0);
	void endRecording();
	void submit(const VulkanQueue& p_Queue, std::span<const WaitSemaphoreData> p_WaitSemaphoreData, std::span<const ResourceID> p_SignalSemaphores, ResourceID p_Fence = UINT32_MAX);
	void reset() const;

	void cmdBeginRenderPass(ResourceID p_RenderPass, ResourceID p_FrameBuffer, VkExtent2D p_Extent, std::span<VkClearValue> p_ClearValues) const;
	void cmdEndRenderPass() const;
	void cmdBindPipeline(VkPipelineBindPoint p_BindPoint, ResourceID p_Pipeline) const;
	void cmdNextSubpass() const;
	void cmdPipelineBarrier(const VulkanMemoryBarrierBuilder& p_Builder) const;
	
	void cmdBindVertexBuffer(ResourceID p_Buffer, VkDeviceSize p_Offset) const;
	void cmdBindVertexBuffers(std::span<const ResourceID> p_BufferIDs, std::span<const VkDeviceSize> p_Offsets) const;
	void cmdBindIndexBuffer(ResourceID p_BufferID, VkDeviceSize p_Offset, VkIndexType p_IndexType) const;

	void cmdCopyBuffer(ResourceID p_Source, ResourceID p_Destination, std::span<const VkBufferCopy> p_CopyRegions) const;
    void cmdCopyBufferToImage(ResourceID p_Buffer, ResourceID p_Image, VkImageLayout p_ImageLayout, std::span<const VkBufferImageCopy> p_CopyRegions) const;
	void cmdBlitImage(ResourceID p_Source, ResourceID p_Destination, std::span<const VkImageBlit> p_Regions, VkFilter p_Filter) const;
    void cmdBlitImage(const VulkanImage& p_Source, const VulkanImage& p_Destination, std::span<const VkImageBlit> p_Regions, VkFilter p_Filter) const;
    void cmdSimpleBlitImage(ResourceID p_Source, ResourceID p_Destination, VkFilter p_Filter) const;
	void cmdSimpleBlitImage(const VulkanImage& p_Source, const VulkanImage& p_Destination, VkFilter p_Filter) const;
	void ecmdDumpStagingBuffer(ResourceID p_Buffer, VkDeviceSize p_Size, VkDeviceSize p_Offset) const;
	void ecmdDumpStagingBuffer(ResourceID p_Buffer, std::span<const VkBufferCopy> p_Regions) const;
    void ecmdDumpStagingBufferToImage(ResourceID p_Image, VkExtent3D p_Size, VkOffset3D p_Offset, bool p_KeepLayout = false) const;
    void ecmdDumpDataIntoBuffer(ResourceID p_DestBuffer, const uint8_t* p_Data, VkDeviceSize p_Size) const;
    void ecmdDumpDataIntoImage(ResourceID p_DestImage, const uint8_t* p_Data, VkExtent3D p_Extent, uint32_t p_BytesPerPixel, bool p_KeepLayout) const;

	void cmdPushConstant(ResourceID p_Layout, VkShaderStageFlags p_StageFlags, uint32_t p_Offset, uint32_t p_Size, const void* p_Values) const;
    void cmdBindDescriptorSet(VkPipelineBindPoint p_BindPoint, ResourceID p_Layout, ResourceID p_DescriptorSet) const;

	void cmdSetViewport(const VkViewport& p_Viewport) const;
	void cmdSetScissor(VkRect2D p_Scissor) const;

	void cmdDraw(uint32_t p_VertexCount, uint32_t p_FirstVertex, uint32_t p_InstanceCount = 1, uint32_t p_FirstInstance = 0) const;
	void cmdDrawIndexed(uint32_t p_IndexCount, uint32_t p_FirstIndex, int32_t p_VertexOffset, uint32_t p_InstanceCount = 1, uint32_t p_FirstInstance = 0) const;
    void cmdDispatch(uint32_t p_GroupCountX, uint32_t p_GroupCountY, uint32_t p_GroupCountZ) const;

	VkCommandBuffer operator*() const;

    bool isRecording() const { return m_IsRecording; }

private:
    enum TypeFlagBits : uint8_t
    {
        SECONDARY = 1,
        ONE_TIME = 2
    };

    typedef uint32_t TypeFlags;
    void free() override;

	VulkanCommandBuffer(ResourceID p_Device, VkCommandBuffer p_CommandBuffer, TypeFlags p_Flags, uint32_t p_FamilyIndex, uint32_t p_ThreadID);

	VkCommandBuffer m_VkHandle = VK_NULL_HANDLE;

	bool m_IsRecording = false;
    bool m_HasRecorded = false;
    bool m_HasSubmitted = false;
	TypeFlags m_Flags = false;
	uint32_t m_FamilyIndex = 0;
	uint32_t m_ThreadID = 0;

    bool m_CanBeReset = false;

	friend class VulkanDevice;
};

