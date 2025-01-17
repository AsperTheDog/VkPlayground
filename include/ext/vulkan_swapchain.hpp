#pragma once
#include <vector>

#include "vulkan_extension_management.hpp"
#include "vulkan_image.hpp"
#include "vulkan_queues.hpp"
#include "utils/identifiable.hpp"


class VulkanFence;

class VulkanSwapchainExtension final : public VulkanDeviceExtension
{
public:
    static VulkanSwapchainExtension* get(const VulkanDevice& p_Device);
    static VulkanSwapchainExtension* get(ResourceID p_DeviceID);

    explicit VulkanSwapchainExtension(const ResourceID p_DeviceID) : VulkanDeviceExtension(p_DeviceID) {}

    [[nodiscard]] VulkanExtensionElem* getExtensionStruct() const override { return nullptr; }
    [[nodiscard]] VkStructureType getExtensionStructType() const override { return VK_STRUCTURE_TYPE_MAX_ENUM; }

    ResourceID createSwapchain(VkSurfaceKHR p_Surface, VkExtent2D p_Extent, VkSurfaceFormatKHR p_DesiredFormat, uint32_t p_OldSwapchain = UINT32_MAX);
    VulkanSwapchain& getSwapchain(ResourceID p_ID);
    [[nodiscard]] const VulkanSwapchain& getSwapchain(ResourceID p_ID) const;
    bool freeSwapchain(const ResourceID p_ID);
    bool freeSwapchain(const VulkanSwapchain& p_Swapchain);

    void free() override;

private:
    std::map<ResourceID, VulkanSwapchain*> m_Swapchains;
};

class VulkanSwapchain final : public VulkanDeviceSubresource
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
	void free() override;

	VulkanSwapchain(uint32_t device, VkSwapchainKHR handle, VkExtent2D extent, VkSurfaceFormatKHR format, uint32_t minImageCount);
	
	VkExtent2D m_extent;
	VkSurfaceFormatKHR m_format;
	std::vector<VulkanImage> m_images;
	std::vector<VkImageView> m_imageViews;
	uint32_t m_minImageCount = 0;

	uint32_t m_nextImage = 0;
	uint32_t m_imageAvailableSemaphore = UINT32_MAX;
	bool m_wasAcquired = false;

	VkSwapchainKHR m_vkHandle = VK_NULL_HANDLE;

    friend class VulkanSwapchainExtension;
};

