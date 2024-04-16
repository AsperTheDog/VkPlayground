#include "vulkan_swapchain.hpp"

#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_context.hpp"
#include "utils/logger.hpp"

VkSwapchainKHR VulkanSwapchain::operator*() const
{
	return m_vkHandle;
}

void VulkanSwapchain::free()
{
	if (m_vkHandle != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(VulkanContext::getDevice(m_device).m_vkHandle, m_vkHandle, nullptr);
        Logger::print("Freed Swapchain (ID: " + std::to_string(m_id) + ")", Logger::LevelBits::INFO);
		m_vkHandle = VK_NULL_HANDLE;
	}
}

VulkanSwapchain::VulkanSwapchain(const VkSwapchainKHR handle, const uint32_t device, const VkExtent2D extent, const VkSurfaceFormatKHR format)
	: m_extent(extent), m_format(format), m_vkHandle(handle), m_device(device)
{
	uint32_t imageCount;
	const VulkanDevice& deviceObj = VulkanContext::getDevice(device);
	vkGetSwapchainImagesKHR(deviceObj.m_vkHandle, m_vkHandle, &imageCount, nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(deviceObj.m_vkHandle, m_vkHandle, &imageCount, m_images.data());

	m_imageViews.resize(m_images.size());
    for (uint32_t i = 0; i < m_images.size(); i++) {
		VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (const VkResult ret = vkCreateImageView(deviceObj.m_vkHandle, &viewInfo, nullptr, &m_imageViews[i]); ret != VK_SUCCESS) {
            throw std::runtime_error(std::string("failed to create swapchain image view, error: ") + string_VkResult(ret));
        }
		Logger::print("Created swapchain image view", Logger::LevelBits::INFO);
    }
}
