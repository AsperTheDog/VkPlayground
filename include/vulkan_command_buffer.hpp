#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "utils/identifiable.hpp"


class VulkanImage;
class VulkanBuffer;
class VulkanQueue;
class VulkanFence;
class VulkanRenderPass;
class VulkanDevice;

class VulkanCommandBuffer final : public VulkanDeviceSubresource
{
public:
	void beginRecording(VkCommandBufferUsageFlags flags = 0);
	void endRecording();
	void submit(const VulkanQueue& queue, const std::vector<std::pair<uint32_t, VkSemaphoreWaitFlags>>& waitSemaphoreData, const std::vector<uint32_t>& signalSemaphores, uint32_t fence = UINT32_MAX);
	void reset() const;

	void cmdBeginRenderPass(uint32_t renderPass, uint32_t frameBuffer, VkExtent2D extent, const std::vector<VkClearValue>& clearValues) const;
	void cmdEndRenderPass() const;
	void cmdBindPipeline(VkPipelineBindPoint bindPoint, uint32_t pipeline) const;
	void cmdNextSubpass() const;
	void cmdPipelineBarrier(
		VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, 
		const std::vector<VkMemoryBarrier>& memoryBarriers, 
		const std::vector<VkBufferMemoryBarrier>& bufferMemoryBarriers, 
		const std::vector<VkImageMemoryBarrier>& imageMemoryBarriers) const;
	void cmdSimpleTransitionImageLayout(uint32_t image, VkImageLayout newLayout, uint32_t srcQueueFamily = VK_QUEUE_FAMILY_IGNORED, uint32_t dstQueueFamily = VK_QUEUE_FAMILY_IGNORED) const;
	void cmdSimpleAbsoluteBarrier() const;

	void cmdBindVertexBuffer(uint32_t buffer, VkDeviceSize offset) const;
	void cmdBindVertexBuffers(const std::vector<uint32_t>& bufferIDs, const std::vector<VkDeviceSize>& offsets) const;
	void cmdBindIndexBuffer(uint32_t bufferID, VkDeviceSize offset, VkIndexType indexType) const;

	void cmdCopyBuffer(uint32_t source, uint32_t destination, const std::vector<VkBufferCopy>& copyRegions) const;
    void cmdCopyBufferToImage(uint32_t buffer, uint32_t image, VkImageLayout imageLayout, const std::vector<VkBufferImageCopy>& copyRegions) const;
	void cmdBlitImage(uint32_t source, uint32_t destination, const std::vector<VkImageBlit>& regions, VkFilter filter) const;
	void cmdSimpleBlitImage(uint32_t source, uint32_t destination, VkFilter filter) const;
	void cmdSimpleBlitImage(const VulkanImage& source, const VulkanImage& destination, VkFilter filter) const;

	void cmdPushConstant(uint32_t layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues) const;
    void cmdBindDescriptorSet(VkPipelineBindPoint bindPoint, uint32_t layout, uint32_t descriptorSet) const;

	void cmdSetViewport(const VkViewport& viewport) const;
	void cmdSetScissor(VkRect2D scissor) const;

	void cmdDraw(uint32_t vertexCount, uint32_t firstVertex) const;
	void cmdDrawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset) const;

	VkCommandBuffer operator*() const;

private:
    enum TypeFlagBits
    {
        SECONDARY = 1,
        ONE_TIME = 2
    };
    typedef uint32_t TypeFlags;
    void free() override;

	VulkanCommandBuffer(uint32_t device, VkCommandBuffer commandBuffer, TypeFlags flags, uint32_t familyIndex, uint32_t threadID);

	VkCommandBuffer m_vkHandle = VK_NULL_HANDLE;

	bool m_isRecording = false;
	TypeFlags m_flags = false;
	uint32_t m_familyIndex = 0;
	uint32_t m_threadID = 0;

	friend class VulkanDevice;
};

