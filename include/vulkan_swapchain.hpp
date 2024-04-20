#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan_image.hpp"
#include "vulkan_queues.hpp"
#include "utils/identifiable.hpp"


class VulkanFence;

class VulkanSwapchain : public Identifiable
{
public:
	VkSwapchainKHR operator*() const;

	[[nodiscard]] uint32_t getMinImageCount() const;
	[[nodiscard]] uint32_t getImageCount() const;
	[[nodiscard]] VkExtent2D getExtent() const;
	[[nodiscard]] VkSurfaceFormatKHR getFormat() const;

	uint32_t acquireNextImage(uint32_t fence = UINT32_MAX);
	[[nodiscard]] VulkanImage& getImage(uint32_t index);
	[[nodiscard]] VkImageView getImageView(uint32_t index) const;
	[[nodiscard]] uint32_t getNextImage() const;
	[[nodiscard]] uint32_t getImgSemaphore() const;

	bool present(QueueSelection queue, const std::vector<uint32_t>& semaphores);

private:
	void free();

	VulkanSwapchain(VkSwapchainKHR handle, uint32_t device, VkExtent2D extent, VkSurfaceFormatKHR format, uint32_t minImageCount);
	
	VkExtent2D m_extent;
	VkSurfaceFormatKHR m_format;
	std::vector<VulkanImage> m_images;
	std::vector<VkImageView> m_imageViews;
	uint32_t m_minImageCount = 0;

	uint32_t m_nextImage = 0;
	uint32_t m_imageAvailableSemaphore = UINT32_MAX;
	bool m_wasAcquired = false;

	VkSwapchainKHR m_vkHandle = VK_NULL_HANDLE;
	uint32_t m_device = UINT32_MAX;

	friend class VulkanDevice;
};

