#include "vulkan_image.hpp"

#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "utils/logger.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

VkMemoryRequirements VulkanImage::getMemoryRequirements() const
{
	VkMemoryRequirements requirements;
	vkGetImageMemoryRequirements(VulkanContext::getDevice(m_device).m_vkHandle, m_vkHandle, &requirements);
	return requirements;
}

void VulkanImage::allocateFromIndex(const uint32_t memoryIndex)
{
	Logger::pushContext("Image memory (from index)");
	const VkMemoryRequirements requirements = getMemoryRequirements();
	setBoundMemory(VulkanContext::getDevice(m_device).m_memoryAllocator.allocate(requirements.size, requirements.alignment, memoryIndex));
	Logger::popContext();
}

void VulkanImage::allocateFromFlags(const VulkanMemoryAllocator::MemoryPropertyPreferences memoryProperties)
{
	Logger::pushContext("Image memory (from flags)");
	const VkMemoryRequirements requirements = getMemoryRequirements();
	setBoundMemory(VulkanContext::getDevice(m_device).m_memoryAllocator.searchAndAllocate(requirements.size, requirements.alignment, memoryProperties, requirements.memoryTypeBits));
	Logger::popContext();
}

VkExtent3D VulkanImage::getSize() const
{
	return m_size;
}

VkImageType VulkanImage::getType() const
{
	return m_type;
}

VkImageLayout VulkanImage::getLayout() const
{
	return m_layout;
}

VkImage VulkanImage::operator*() const
{
	return m_vkHandle;
}

VkImageView VulkanImage::createImageView(const VkFormat format, const VkImageAspectFlags aspectFlags)
{
	VkImageViewType type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	switch (m_type)
	{
	case VK_IMAGE_TYPE_1D:
		type = VK_IMAGE_VIEW_TYPE_1D;
		break;
	case VK_IMAGE_TYPE_2D:
		type = VK_IMAGE_VIEW_TYPE_2D;
		break;
	case VK_IMAGE_TYPE_3D:
		type = VK_IMAGE_VIEW_TYPE_3D;
		break;
	default: 
        throw std::invalid_argument("Invalid image type on create view, requested: " + std::to_string(type));
    }

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = m_vkHandle;
	createInfo.viewType = type;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (const VkResult ret = vkCreateImageView(VulkanContext::getDevice(m_device).m_vkHandle, &createInfo, nullptr, &imageView); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create image view, error: ") + string_VkResult(ret));
	}

	m_imageViews.push_back(imageView);
	return imageView;
}

void VulkanImage::freeImageView(const VkImageView imageView)
{
	vkDestroyImageView(VulkanContext::getDevice(m_device).m_vkHandle, imageView, nullptr);
    Logger::print("Freed image view of image " + std::to_string(m_id), Logger::DEBUG);
	std::erase(m_imageViews, imageView);
}

void VulkanImage::transitionLayout(const VkImageLayout layout, const VkImageAspectFlags aspectFlags, const uint32_t threadID)
{
	VulkanDevice& device = VulkanContext::getDevice(m_device);
	VulkanCommandBuffer& commandBuffer = device.getCommandBuffer(device.createOneTimeCommandBuffer(threadID), threadID);
	commandBuffer.beginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	commandBuffer.cmdSimpleTransitionImageLayout(m_id, layout, aspectFlags);
	commandBuffer.endRecording();
	const VulkanQueue queue = device.getQueue(device.m_oneTimeQueue);
	commandBuffer.submit(queue, {}, {});

	m_layout = layout;

	queue.waitIdle();
	device.freeCommandBuffer(commandBuffer, threadID);
    Logger::print(std::string("Transitioned image layout to ") + string_VkImageLayout(layout) + " on image " + std::to_string(m_id), Logger::DEBUG);
}

VulkanImage::VulkanImage(const uint32_t device, const VkImage vkHandle, const VkExtent3D size, const VkImageType type, const VkImageLayout layout)
	: m_size(size), m_type(type), m_layout(layout), m_vkHandle(vkHandle), m_device(device)
{

}

void VulkanImage::setBoundMemory(const MemoryChunk::MemoryBlock& memoryRegion)
{
	if (m_memoryRegion.size > 0)
	{
		throw std::runtime_error("Tried to bind memory to buffer (ID: " + std::to_string(m_id) + ") but it already has memory bound to it");
	}
	m_memoryRegion = memoryRegion;

	if (const VkResult ret = vkBindImageMemory(VulkanContext::getDevice(m_device).m_vkHandle, m_vkHandle, VulkanContext::getDevice(m_device).getMemoryHandle(m_memoryRegion.chunk), m_memoryRegion.offset); ret != VK_SUCCESS)
	{
	    throw std::runtime_error("Failed to bind memory to image (ID: " + std::to_string(m_id) + "), error: " + string_VkResult(ret));
	}
    Logger::print("Bound memory to image " + std::to_string(m_id) + " with size " + std::to_string(m_memoryRegion.size) + " and offset " + std::to_string(m_memoryRegion.offset), Logger::DEBUG);
}

void VulkanImage::free()
{
	VulkanDevice& device = VulkanContext::getDevice(m_device);

	for (const VkImageView imageView : m_imageViews)
	{
		vkDestroyImageView(device.m_vkHandle, imageView, nullptr);
	}
	Logger::print("Freed image (ID: " + std::to_string(m_id) + ") with " + std::to_string(m_imageViews.size()) + " image views", Logger::DEBUG);
	Logger::pushContext("Image memory free");
	m_imageViews.clear();

	vkDestroyImage(device.m_vkHandle, m_vkHandle, nullptr);
	m_vkHandle = VK_NULL_HANDLE;

	if (m_memoryRegion.size > 0)
	{
		device.m_memoryAllocator.deallocate(m_memoryRegion);
		m_memoryRegion = {};
	}
	Logger::popContext();
}
