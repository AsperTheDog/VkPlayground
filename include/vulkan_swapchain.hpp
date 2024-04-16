#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "utils/identifiable.hpp"


class VulkanSwapchain : public Identifiable
{
public:
	VkSwapchainKHR operator*() const;

private:
	void free();

	VulkanSwapchain(VkSwapchainKHR handle, uint32_t device, VkExtent2D extent, VkSurfaceFormatKHR format);
	
	VkExtent2D m_extent;
	VkSurfaceFormatKHR m_format;
	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;

	VkSwapchainKHR m_vkHandle = VK_NULL_HANDLE;
	uint32_t m_device = UINT32_MAX;

	friend class VulkanDevice;
};

