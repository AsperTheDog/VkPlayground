#pragma once
#include <map>
#include <vector>
#include <Volk/volk.h>

#include "utils/identifiable.hpp"


class VulkanImage;
class VulkanBuffer;
class VulkanQueue;
class VulkanFence;
class VulkanRenderPass;
class VulkanDevice;

//TODO: Make a proper memory barrier system (builder?)

class VulkanCommandBuffer final : public VulkanDeviceSubresource
{
public:
	void beginRecording(VkCommandBufferUsageFlags p_Flags = 0);
	void endRecording();
	void submit(const VulkanQueue& p_Queue, const std::vector<std::pair<ResourceID, VkSemaphoreWaitFlags>>& p_WaitSemaphoreData, const std::vector<ResourceID>& p_SignalSemaphores, ResourceID p_Fence = UINT32_MAX);
	void reset() const;

	void cmdBeginRenderPass(ResourceID p_RenderPass, ResourceID p_FrameBuffer, VkExtent2D p_Extent, const std::vector<VkClearValue>& p_ClearValues) const;
	void cmdEndRenderPass() const;
	void cmdBindPipeline(VkPipelineBindPoint p_BindPoint, ResourceID p_Pipeline) const;
	void cmdNextSubpass() const;
	void cmdPipelineBarrier(
        VkPipelineStageFlags p_SrcStageMask, VkPipelineStageFlags p_DstStageMask, VkDependencyFlags p_DependencyFlags,
        const std::vector<VkMemoryBarrier>& p_MemoryBarriers,
        const std::vector<VkBufferMemoryBarrier>& p_BufferMemoryBarriers,
        const std::vector<VkImageMemoryBarrier>& p_ImageMemoryBarriers) const;
	void cmdSimpleTransitionImageLayout(ResourceID p_Image, VkImageLayout p_NewLayout, uint32_t p_SrcQueueFamily = VK_QUEUE_FAMILY_IGNORED, uint32_t p_DstQueueFamily = VK_QUEUE_FAMILY_IGNORED) const;
	void cmdSimpleAbsoluteBarrier() const;

	void cmdBindVertexBuffer(ResourceID p_Buffer, VkDeviceSize p_Offset) const;
	void cmdBindVertexBuffers(const std::vector<ResourceID>& p_BufferIDs, const std::vector<VkDeviceSize>& p_Offsets) const;
	void cmdBindIndexBuffer(ResourceID p_BufferID, VkDeviceSize p_Offset, VkIndexType p_IndexType) const;

	void cmdCopyBuffer(ResourceID p_Source, ResourceID p_Destination, const std::vector<VkBufferCopy>& p_CopyRegions) const;
    void cmdCopyBufferToImage(ResourceID p_Buffer, ResourceID p_Image, VkImageLayout p_ImageLayout, const std::vector<VkBufferImageCopy>& p_CopyRegions) const;
	void cmdBlitImage(ResourceID p_Source, ResourceID p_Destination, const std::vector<VkImageBlit>& p_Regions, VkFilter p_Filter) const;
	void cmdSimpleBlitImage(ResourceID p_Source, ResourceID p_Destination, VkFilter p_Filter) const;
	void cmdSimpleBlitImage(const VulkanImage& p_Source, const VulkanImage& p_Destination, VkFilter p_Filter) const;

	void cmdPushConstant(ResourceID p_Layout, VkShaderStageFlags p_StageFlags, uint32_t p_Offset, uint32_t p_Size, const void* p_Values) const;
    void cmdBindDescriptorSet(VkPipelineBindPoint p_BindPoint, ResourceID p_Layout, ResourceID p_DescriptorSet) const;

	void cmdSetViewport(const VkViewport& p_Viewport) const;
	void cmdSetScissor(VkRect2D p_Scissor) const;

	void cmdDraw(uint32_t p_VertexCount, uint32_t p_FirstVertex) const;
	void cmdDrawIndexed(uint32_t p_IndexCount, uint32_t p_FirstIndex, int32_t p_VertexOffset) const;

	VkCommandBuffer operator*() const;

private:
    enum TypeFlagBits
    {
        SECONDARY = 1,
        ONE_TIME = 2
    };

    struct AccessData
    {
        VkAccessFlags srcAccessMask;
        VkPipelineStageFlags srcStageMask;
        VkAccessFlags dstAccessMask;
        VkPipelineStageFlags dstStageMask;
    };

    typedef uint32_t TypeFlags;
    void free() override;

	VulkanCommandBuffer(ResourceID p_Device, VkCommandBuffer p_CommandBuffer, TypeFlags p_Flags, uint32_t p_FamilyIndex, uint32_t p_ThreadID);

	VkCommandBuffer m_VkHandle = VK_NULL_HANDLE;

	bool m_IsRecording = false;
	TypeFlags m_Flags = false;
	uint32_t m_FamilyIndex = 0;
	uint32_t m_ThreadID = 0;

    bool m_CanBeReset = false;

    static std::map<VkImageLayout, ::VulkanCommandBuffer::AccessData> s_TransitionMapping;

	friend class VulkanDevice;
};

