#include "vulkan_image.hpp"

#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "utils/logger.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

VkMemoryRequirements VulkanImage::getMemoryRequirements() const
{
    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

	VkMemoryRequirements l_Requirements;
	l_Device.getTable().vkGetImageMemoryRequirements(l_Device.m_VkHandle, m_VkHandle, &l_Requirements);
	return l_Requirements;
}

void VulkanImage::allocateFromIndex(const uint32_t p_MemoryIndex)
{
	Logger::pushContext("Image memory (from index)");
	const VkMemoryRequirements l_Requirements = getMemoryRequirements();
	setBoundMemory(VulkanContext::getDevice(getDeviceID()).getMemoryAllocator().allocate(l_Requirements.size, l_Requirements.alignment, p_MemoryIndex));
	Logger::popContext();
}

void VulkanImage::allocateFromFlags(const VulkanMemoryAllocator::MemoryPropertyPreferences p_MemoryProperties)
{
	Logger::pushContext("Image memory (from flags)");
	const VkMemoryRequirements l_Requirements = getMemoryRequirements();
	setBoundMemory(VulkanContext::getDevice(getDeviceID()).getMemoryAllocator().searchAndAllocate(l_Requirements.size, l_Requirements.alignment, p_MemoryProperties, l_Requirements.memoryTypeBits));
	Logger::popContext();
}

VkExtent3D VulkanImage::getSize() const
{
	return m_Size;
}

VkImageType VulkanImage::getType() const
{
	return m_Type;
}

VkImageLayout VulkanImage::getLayout() const
{
	return m_Layout;
}

VkImage VulkanImage::operator*() const
{
	return m_VkHandle;
}

VkImageView VulkanImage::createImageView(const VkFormat p_Format, const VkImageAspectFlags p_AspectFlags)
{
	VkImageViewType l_Type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	switch (m_Type)
	{
	case VK_IMAGE_TYPE_1D:
		l_Type = VK_IMAGE_VIEW_TYPE_1D;
		break;
	case VK_IMAGE_TYPE_2D:
		l_Type = VK_IMAGE_VIEW_TYPE_2D;
		break;
	case VK_IMAGE_TYPE_3D:
		l_Type = VK_IMAGE_VIEW_TYPE_3D;
		break;
	default: 
        throw std::invalid_argument("Invalid image type on create view, requested: " + std::to_string(l_Type));
    }

	VkImageViewCreateInfo l_CreateInfo = {};
	l_CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	l_CreateInfo.image = m_VkHandle;
	l_CreateInfo.viewType = l_Type;
	l_CreateInfo.format = p_Format;
	l_CreateInfo.subresourceRange.aspectMask = p_AspectFlags;
	l_CreateInfo.subresourceRange.baseMipLevel = 0;
	l_CreateInfo.subresourceRange.levelCount = 1;
	l_CreateInfo.subresourceRange.baseArrayLayer = 0;
	l_CreateInfo.subresourceRange.layerCount = 1;

    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

	VkImageView l_ImageView;
	if (const VkResult ret = l_Device.getTable().vkCreateImageView(l_Device.m_VkHandle, &l_CreateInfo, nullptr, &l_ImageView); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create image view, error: ") + string_VkResult(ret));
	}

	m_ImageViews.push_back(l_ImageView);
	return l_ImageView;
}

void VulkanImage::freeImageView(const VkImageView p_ImageView)
{
    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

	l_Device.getTable().vkDestroyImageView(l_Device.m_VkHandle, p_ImageView, nullptr);
    LOG_DEBUG("Freed image view of image ", m_ID);
	std::erase(m_ImageViews, p_ImageView);
}

VkSampler VulkanImage::createSampler(const VkFilter p_Filter, const VkSamplerAddressMode p_SamplerAddressMode)
{
    VkSamplerCreateInfo l_CreateInfo = {};
    l_CreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    l_CreateInfo.magFilter = p_Filter;
    l_CreateInfo.minFilter = p_Filter;
    l_CreateInfo.addressModeU = p_SamplerAddressMode;
    l_CreateInfo.addressModeV = p_SamplerAddressMode;
    l_CreateInfo.addressModeW = p_SamplerAddressMode;
    l_CreateInfo.anisotropyEnable = VK_FALSE;
    l_CreateInfo.maxAnisotropy = 0;
    l_CreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    l_CreateInfo.unnormalizedCoordinates = VK_FALSE;
    l_CreateInfo.compareEnable = VK_FALSE;
    l_CreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    l_CreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    l_CreateInfo.mipLodBias = 0.0f;
    l_CreateInfo.minLod = 0.0f;
    l_CreateInfo.maxLod = 0.0f;

    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

    VkSampler l_Sampler;
    if (const VkResult l_Ret = l_Device.getTable().vkCreateSampler(l_Device.m_VkHandle, &l_CreateInfo, nullptr, &l_Sampler); l_Ret != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("Failed to create sampler, error: ") + string_VkResult(l_Ret));
    }

    m_Samplers.push_back(l_Sampler);
    return l_Sampler;
}

void VulkanImage::freeSampler(const VkSampler p_Sampler)
{
    if (std::ranges::find(m_Samplers, p_Sampler) == m_Samplers.end())
    {
        LOG_WARN("Tried to free sampler that doesn't belong to image ", m_ID);
        return;
    }

    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

    l_Device.getTable().vkDestroySampler(l_Device.m_VkHandle, p_Sampler, nullptr);
    LOG_DEBUG("Freed sampler of image ", m_ID);
    std::erase(m_Samplers, p_Sampler);
}

void VulkanImage::transitionLayout(const VkImageLayout p_Layout, const uint32_t p_ThreadID)
{
	VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());
	VulkanCommandBuffer& l_CommandBuffer = l_Device.getCommandBuffer(l_Device.createOneTimeCommandBuffer(p_ThreadID), p_ThreadID);
	l_CommandBuffer.beginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	l_CommandBuffer.cmdSimpleTransitionImageLayout(m_ID, p_Layout);
	l_CommandBuffer.endRecording();
	const VulkanQueue l_Queue = l_Device.getQueue(l_Device.m_OneTimeQueue);
	l_CommandBuffer.submit(l_Queue, {}, {});

	m_Layout = p_Layout;

	l_Queue.waitIdle();
	l_Device.freeCommandBuffer(l_CommandBuffer, p_ThreadID);
    LOG_DEBUG("Transitioned image layout to ", string_VkImageLayout(p_Layout), " on image ", m_ID);
}

VulkanImage::VulkanImage(const ResourceID p_Device, const VkImage p_VkHandle, const VkExtent3D p_Size, const VkImageType p_Type, const VkImageLayout p_Layout)
	: VulkanDeviceSubresource(p_Device), m_Size(p_Size), m_Type(p_Type), m_Layout(p_Layout), m_VkHandle(p_VkHandle)
{

}

void VulkanImage::setBoundMemory(const MemoryChunk::MemoryBlock& p_MemoryRegion)
{
	if (m_MemoryRegion.size > 0)
	{
		throw std::runtime_error("Tried to bind memory to buffer (ID: " + std::to_string(m_ID) + ") but it already has memory bound to it");
	}
	m_MemoryRegion = p_MemoryRegion;

    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

	if (const VkResult ret = l_Device.getTable().vkBindImageMemory(l_Device.m_VkHandle, m_VkHandle, l_Device.getMemoryHandle(m_MemoryRegion.chunk), m_MemoryRegion.offset); ret != VK_SUCCESS)
	{
	    throw std::runtime_error("Failed to bind memory to image (ID: " + std::to_string(m_ID) + "), error: " + string_VkResult(ret));
	}
    LOG_DEBUG("Bound memory to image ", m_ID, " with size ", m_MemoryRegion.size, " and offset ", m_MemoryRegion.offset);
}

void VulkanImage::free()
{
	VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

    for (const VkSampler l_Sampler : m_Samplers)
    {
        l_Device.getTable().vkDestroySampler(l_Device.m_VkHandle, l_Sampler, nullptr);
    }

	for (const VkImageView l_ImageView : m_ImageViews)
	{
		l_Device.getTable().vkDestroyImageView(l_Device.m_VkHandle, l_ImageView, nullptr);
	}
    LOG_DEBUG("Freed image (ID: ", m_ID, ") with ", m_ImageViews.size(), " image views and ", m_Samplers.size(), " samplers");
	Logger::pushContext("Image memory free");
	m_ImageViews.clear();
    m_Samplers.clear();

	l_Device.getTable().vkDestroyImage(l_Device.m_VkHandle, m_VkHandle, nullptr);
	m_VkHandle = VK_NULL_HANDLE;

	if (m_MemoryRegion.size > 0)
	{
		l_Device.m_MemoryAllocator.deallocate(m_MemoryRegion);
		m_MemoryRegion = {};
	}
	Logger::popContext();
}
