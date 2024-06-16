#include "vulkan_command_buffer.hpp"

#include <array>
#include <stdexcept>
#include <vector>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_buffer.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"
#include "vulkan_sync.hpp"
#include "vulkan_framebuffer.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_queues.hpp"
#include "vulkan_render_pass.hpp"
#include "utils/logger.hpp"

void VulkanCommandBuffer::beginRecording(const VkCommandBufferUsageFlags flags)
{
#ifdef _DEBUG
	if (m_isRecording)
	{
		Logger::print("Tried to begin recording, but command buffer (ID:" + std::to_string(m_id) + ") is already recording", Logger::LevelBits::WARN);
		return;
	}
#endif

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = flags;

	vkBeginCommandBuffer(m_vkHandle, &beginInfo);

	m_isRecording = true;
}

void VulkanCommandBuffer::endRecording()
{
	if (!m_isRecording)
	{
		Logger::print("Tried to end recording, but command buffer (ID:" + std::to_string(m_id) + ") is not recording", Logger::LevelBits::WARN);
		return;
	}

	vkEndCommandBuffer(m_vkHandle);

	m_isRecording = false;
}

void VulkanCommandBuffer::cmdCopyBuffer(const uint32_t source, const uint32_t destination, const std::vector<VkBufferCopy>& copyRegions) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	VulkanDevice& device = VulkanContext::getDevice(m_device);
	vkCmdCopyBuffer(m_vkHandle, *device.getBuffer(source), *device.getBuffer(destination), static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
}

void VulkanCommandBuffer::cmdCopyBufferToImage(const uint32_t buffer, const uint32_t image, const VkImageLayout imageLayout, const std::vector<VkBufferImageCopy>& copyRegions) const
{
    if (!m_isRecording)
    {
        throw std::runtime_error("Command buffer (ID:" + std::to_string(m_id) + ") is not recording");
    }

    VulkanDevice& device = VulkanContext::getDevice(m_device);
    vkCmdCopyBufferToImage(m_vkHandle, *device.getBuffer(buffer), *device.getImage(image), imageLayout, copyRegions.size(), copyRegions.data());
}

void VulkanCommandBuffer::cmdBlitImage(const uint32_t source, const uint32_t destination, const std::vector<VkImageBlit>& regions, const VkFilter filter) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	VulkanDevice& device = VulkanContext::getDevice(m_device);
	const VulkanImage& srcImage = device.getImage(source);
	const VulkanImage& dstImage = device.getImage(destination);
	vkCmdBlitImage(m_vkHandle, *srcImage, srcImage.getLayout(), *dstImage, dstImage.getLayout(), static_cast<uint32_t>(regions.size()), regions.data(), filter);
}

void VulkanCommandBuffer::cmdSimpleBlitImage(const uint32_t source, const uint32_t destination, const VkFilter filter) const
{
	VulkanDevice& device = VulkanContext::getDevice(m_device);
	const VulkanImage& srcImage = device.getImage(source);
	const VulkanImage& dstImage = device.getImage(destination);
	cmdSimpleBlitImage(srcImage, dstImage, filter);
}

void VulkanCommandBuffer::cmdSimpleBlitImage(const VulkanImage& source, const VulkanImage& destination, const VkFilter filter) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	VkImageBlit region{};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = 1;
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;
	region.srcOffsets[1] = { static_cast<int32_t>(source.getSize().width), static_cast<int32_t>(source.getSize().height), 1 };
	region.dstOffsets[1] = { static_cast<int32_t>(destination.getSize().width), static_cast<int32_t>(destination.getSize().height), 1 };

	//If source image layout is not transfer source, then we need to transition it
	if (source.getLayout() != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = source.getLayout();
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = *source;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(m_vkHandle, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	vkCmdBlitImage(m_vkHandle, *source, source.getLayout(), *destination, destination.getLayout(), 1, &region, filter);
}

void VulkanCommandBuffer::cmdPushConstant(const uint32_t layout, const VkShaderStageFlags stageFlags, const uint32_t offset, const uint32_t size, const void* pValues) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	vkCmdPushConstants(m_vkHandle, VulkanContext::getDevice(m_device).getPipelineLayout(layout).m_vkHandle, stageFlags, offset, size, pValues);
}

void VulkanCommandBuffer::cmdBindDescriptorSet(const VkPipelineBindPoint bindPoint, const uint32_t layout, const uint32_t descriptorSet) const
{
	const VkPipelineLayout vkLayout = *VulkanContext::getDevice(m_device).getPipelineLayout(layout);
	const VkDescriptorSet vkDescriptorSet = *VulkanContext::getDevice(m_device).getDescriptorSet(descriptorSet);
	vkCmdBindDescriptorSets(m_vkHandle, bindPoint, vkLayout, 0, 1, &vkDescriptorSet, 0, nullptr);
}

void VulkanCommandBuffer::submit(const VulkanQueue& queue, const std::vector<std::pair<uint32_t, VkSemaphoreWaitFlags>>& waitSemaphoreData, const std::vector<uint32_t>& signalSemaphores, const uint32_t fence)
{
	if (m_isRecording)
	{
		Logger::print("Tried to submit command buffer (ID:" + std::to_string(m_id) + ") while it is still recording, forcefully ending recording", Logger::LevelBits::WARN);
		endRecording();
	}

	VulkanDevice& device = VulkanContext::getDevice(m_device);

	std::vector<VkSemaphore> waitSemaphores{};
	std::vector<VkPipelineStageFlags> waitStages{};
	waitSemaphores.resize(waitSemaphoreData.size());
	waitStages.resize(waitSemaphoreData.size());
	for (size_t i = 0; i < waitSemaphores.size(); i++)
	{
		waitSemaphores[i] = device.getSemaphore(waitSemaphoreData[i].first).m_vkHandle;
		waitStages[i] = waitSemaphoreData[i].second;
	}

	std::vector<VkSemaphore> signalSemaphoresVk{};
	signalSemaphoresVk.reserve(signalSemaphores.size());
	for (const auto& semaphore : signalSemaphores)
	{
		signalSemaphoresVk.push_back(device.getSemaphore(semaphore).m_vkHandle);
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_vkHandle;
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphoresVk.size());
	submitInfo.pSignalSemaphores = signalSemaphoresVk.data();

	const VkResult ret = vkQueueSubmit(queue.m_vkHandle, 1, &submitInfo, fence != UINT32_MAX ? device.getFence(fence).m_vkHandle : VK_NULL_HANDLE);
	if (ret != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit command buffer (ID:" + std::to_string(m_id) + "), error: " + string_VkResult(ret));
	}
}

void VulkanCommandBuffer::reset() const
{
	if (const VkResult ret = vkResetCommandBuffer(m_vkHandle, 0); ret != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to reset command buffer (ID:" + std::to_string(m_id) + "), error: " + string_VkResult(ret));
	}
}

void VulkanCommandBuffer::cmdBeginRenderPass(const uint32_t renderPass, const uint32_t frameBuffer, const VkExtent2D extent, const std::vector<VkClearValue>& clearValues) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdBeginRenderPass, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	VkRenderPassBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = VulkanContext::getDevice(m_device).getRenderPass(renderPass).m_vkHandle;
	beginInfo.framebuffer = VulkanContext::getDevice(m_device).getFramebuffer(frameBuffer).m_vkHandle;
	beginInfo.renderArea.offset = { 0, 0 };
	beginInfo.renderArea.extent = extent;

	beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	beginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_vkHandle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::cmdEndRenderPass() const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdEndRenderPass, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	vkCmdEndRenderPass(m_vkHandle);
}

void VulkanCommandBuffer::cmdBindPipeline(const VkPipelineBindPoint bindPoint, const uint32_t pipelineID) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdBindPipeline, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	vkCmdBindPipeline(m_vkHandle, bindPoint, VulkanContext::getDevice(m_device).getPipeline(pipelineID).m_vkHandle);
}

void VulkanCommandBuffer::cmdNextSubpass() const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdNextSubpass, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	vkCmdNextSubpass(m_vkHandle, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::cmdPipelineBarrier(const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask, const VkDependencyFlags dependencyFlags,
	const std::vector<VkMemoryBarrier>& memoryBarriers,
	const std::vector<VkBufferMemoryBarrier>& bufferMemoryBarriers,
	const std::vector<VkImageMemoryBarrier>& imageMemoryBarriers) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdPipelineBarrier, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	vkCmdPipelineBarrier(m_vkHandle, srcStageMask, dstStageMask, dependencyFlags, static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data(), static_cast<uint32_t>(bufferMemoryBarriers.size()), bufferMemoryBarriers.data(), static_cast<uint32_t>(imageMemoryBarriers.size()), imageMemoryBarriers.data());
}

void VulkanCommandBuffer::cmdSimpleTransitionImageLayout(const uint32_t image, const VkImageLayout newLayout, const uint32_t srcQueueFamily, const uint32_t dstQueueFamily) const
{
	const VulkanImage& imageObj = VulkanContext::getDevice(m_device).getImage(image);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = imageObj.getLayout();
	barrier.newLayout = newLayout;
	if (srcQueueFamily == dstQueueFamily)
	{
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	}
	else
	{
		barrier.srcQueueFamilyIndex = srcQueueFamily;
		barrier.dstQueueFamilyIndex = dstQueueFamily;
	}
	barrier.image = *imageObj;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Assuming color aspect for simplicity
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (imageObj.getLayout() == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (imageObj.getLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		// Other layout transitions can be added as needed
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		m_vkHandle,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void VulkanCommandBuffer::cmdSimpleAbsoluteBarrier() const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdSimpleAbsoluteBarrier, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	VkMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	vkCmdPipelineBarrier(m_vkHandle, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void VulkanCommandBuffer::cmdBindVertexBuffer(const uint32_t buffer, const VkDeviceSize offset) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdBindVertexBuffers, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	vkCmdBindVertexBuffers(m_vkHandle, 0, 1, &VulkanContext::getDevice(m_device).getBuffer(buffer).m_vkHandle, &offset);
}

void VulkanCommandBuffer::cmdBindVertexBuffers(const std::vector<uint32_t>& bufferIDs, const std::vector<VkDeviceSize>& offsets) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdBindVertexBuffers, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	std::vector<VkBuffer> vkBuffers;
	vkBuffers.reserve(bufferIDs.size());
	for (const auto& buffer : bufferIDs)
	{
		vkBuffers.push_back(VulkanContext::getDevice(m_device).getBuffer(buffer).m_vkHandle);
	}
	vkCmdBindVertexBuffers(m_vkHandle, 0, static_cast<uint32_t>(vkBuffers.size()), vkBuffers.data(), offsets.data());
}

void VulkanCommandBuffer::cmdBindIndexBuffer(const uint32_t bufferID, const VkDeviceSize offset, const VkIndexType indexType) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdBindIndexBuffer, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	vkCmdBindIndexBuffer(m_vkHandle, VulkanContext::getDevice(m_device).getBuffer(bufferID).m_vkHandle, offset, indexType);
}

void VulkanCommandBuffer::cmdSetViewport(const VkViewport& viewport) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdSetViewport, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	vkCmdSetViewport(m_vkHandle, 0, 1, &viewport);
}

void VulkanCommandBuffer::cmdSetScissor(const VkRect2D scissor) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdSetScissor, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	vkCmdSetScissor(m_vkHandle, 0, 1, &scissor);
}

void VulkanCommandBuffer::cmdDraw(const uint32_t vertexCount, const uint32_t firstVertex) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdDraw, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}

	vkCmdDraw(m_vkHandle, vertexCount, 1, firstVertex, 0);
}

void VulkanCommandBuffer::cmdDrawIndexed(const uint32_t indexCount, const uint32_t  firstIndex, const int32_t  vertexOffset) const
{
	if (!m_isRecording)
	{
		throw std::runtime_error("Tried to execute command CmdDrawIndexed, but command buffer (ID:" + std::to_string(m_id) + ") is not recording");
	}
	vkCmdDrawIndexed(m_vkHandle, indexCount, 1, firstIndex, vertexOffset, 0);
}

VkCommandBuffer VulkanCommandBuffer::operator*() const
{
	return m_vkHandle;
}

void VulkanCommandBuffer::free()
{
	if (m_vkHandle != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(VulkanContext::getDevice(m_device).m_vkHandle, VulkanContext::getDevice(m_device).getCommandPool(m_familyIndex, m_threadID, m_flags), 1, &m_vkHandle);
		Logger::print("Freed command buffer (ID:" + std::to_string(m_id) + ")", Logger::DEBUG);
		m_vkHandle = VK_NULL_HANDLE;
	}
}

VulkanCommandBuffer::VulkanCommandBuffer(const uint32_t device, const VkCommandBuffer commandBuffer, const TypeFlags flags, const uint32_t familyIndex, const uint32_t threadID)
	: m_vkHandle(commandBuffer), m_flags(flags), m_familyIndex(familyIndex), m_threadID(threadID), m_device(device)
{
}
