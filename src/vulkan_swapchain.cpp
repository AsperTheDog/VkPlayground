#include "vulkan_swapchain.hpp"

#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_context.hpp"
#include "utils/logger.hpp"

VkSwapchainKHR VulkanSwapchain::operator*() const
{
	return m_vkHandle;
}

uint32_t VulkanSwapchain::getMinImageCount() const
{
    return m_minImageCount;
}

uint32_t VulkanSwapchain::getImageCount() const
{
    return static_cast<uint32_t>(m_images.size());
}

VkExtent2D VulkanSwapchain::getExtent() const
{
    return m_extent;
}

VkSurfaceFormatKHR VulkanSwapchain::getFormat() const
{
    return m_format;
}

uint32_t VulkanSwapchain::acquireNextImage(const uint32_t fence)
{
    if (m_vkHandle == nullptr)
		throw std::runtime_error("Swapchain not created");

	uint32_t imageIndex;
	VulkanDevice& device = VulkanContext::getDevice(getDeviceID());
	const VulkanSemaphore& semaphore = device.getSemaphore(m_imageAvailableSemaphore);
	const VkResult result = vkAcquireNextImageKHR(*device, m_vkHandle, UINT64_MAX, *semaphore, fence != UINT32_MAX ? *device.getFence(fence) : nullptr, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return UINT32_MAX;
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	m_nextImage = imageIndex;
	m_wasAcquired = true;
	return imageIndex;
}

VulkanImage& VulkanSwapchain::getImage(const uint32_t index)
{
	return m_images[index];
}

VkImageView VulkanSwapchain::getImageView(const uint32_t index) const
{
	return m_imageViews[index];
}

uint32_t VulkanSwapchain::getNextImage() const
{
	return m_nextImage;
}

uint32_t VulkanSwapchain::getImgSemaphore() const
{
	return m_imageAvailableSemaphore;
}

bool VulkanSwapchain::present(const QueueSelection queue, const std::vector<uint32_t>& semaphores)
{
	if (!m_wasAcquired)
		throw std::runtime_error("Tried to present swpachain, but image was not acquired");
	m_wasAcquired = false;

	std::vector<VkSemaphore> vkSemaphores{semaphores.size()};
	for (uint32_t i = 0; i < semaphores.size(); i++)
		vkSemaphores[i] = *VulkanContext::getDevice(getDeviceID()).getSemaphore(semaphores[i]);

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = static_cast<uint32_t>(vkSemaphores.size());
	presentInfo.pWaitSemaphores = vkSemaphores.data();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_vkHandle;
	presentInfo.pImageIndices = &m_nextImage;

	const VulkanQueue queueObj = VulkanContext::getDevice(getDeviceID()).getQueue(queue);
	const VkResult result = vkQueuePresentKHR(*queueObj, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) 
		return false;
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}
	return true;
}

void VulkanSwapchain::free()
{
	if (m_vkHandle != VK_NULL_HANDLE)
	{
        for (uint32_t i = 0; i < m_imageViews.size(); i++)
        {
	        m_images[i].freeImageView(m_imageViews[i]);
		}
        m_imageViews.clear();
		m_images.clear();

		VulkanContext::getDevice(getDeviceID()).freeSemaphore(m_imageAvailableSemaphore);

		vkDestroySwapchainKHR(VulkanContext::getDevice(getDeviceID()).m_VkHandle, m_vkHandle, nullptr);
        Logger::print("Freed Swapchain (ID: " + std::to_string(m_ID) + ")", Logger::DEBUG);
		m_vkHandle = VK_NULL_HANDLE;
	}
}

VulkanSwapchain::VulkanSwapchain(const uint32_t device, const VkSwapchainKHR handle, const VkExtent2D extent, const VkSurfaceFormatKHR format, const uint32_t minImageCount)
	: VulkanDeviceSubresource(device), m_extent(extent), m_format(format), m_minImageCount(minImageCount), m_vkHandle(handle)
{
	uint32_t imageCount;
	VulkanDevice& deviceObj = VulkanContext::getDevice(device);
	std::vector<VkImage> images;
	vkGetSwapchainImagesKHR(deviceObj.m_VkHandle, m_vkHandle, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(deviceObj.m_VkHandle, m_vkHandle, &imageCount, images.data());

	for (const VkImage image : images)
		m_images.push_back({device, image, VkExtent3D{ extent.width, extent.height, 1 }, VK_IMAGE_TYPE_2D, VK_IMAGE_LAYOUT_UNDEFINED});

	m_imageViews.reserve(m_images.size());
    for (auto& m_image : m_images)
    {
		m_imageViews.push_back(m_image.createImageView(format.format, VK_IMAGE_ASPECT_COLOR_BIT));
		Logger::print("Created swapchain image view", Logger::DEBUG);
    }

	m_imageAvailableSemaphore = deviceObj.createSemaphore();
}
