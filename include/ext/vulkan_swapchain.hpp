#pragma once
#include <span>
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

    [[nodiscard]] VkBaseInStructure* getExtensionStruct() const override { return nullptr; }
    [[nodiscard]] VkStructureType getExtensionStructType() const override { return VK_STRUCTURE_TYPE_MAX_ENUM; }

    ResourceID createSwapchain(VkSurfaceKHR p_Surface, VkExtent2D p_Extent, VkSurfaceFormatKHR p_DesiredFormat, uint32_t p_OldSwapchain = UINT32_MAX);
    VulkanSwapchain& getSwapchain(ResourceID p_ID);
    [[nodiscard]] const VulkanSwapchain& getSwapchain(ResourceID p_ID) const;
    bool freeSwapchain(ResourceID p_ID);
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

	uint32_t acquireNextImage(ResourceID p_Fence = UINT32_MAX);
	[[nodiscard]] VulkanImage& getImage(uint32_t p_Index);
	[[nodiscard]] ResourceID getImageView(uint32_t p_Index) const;
	[[nodiscard]] uint32_t getNextImage() const;
	[[nodiscard]] uint32_t getImgSemaphore() const;

	bool present(QueueSelection p_Queue, std::span<const ResourceID> p_Semaphores);

private:
	void free() override;

	VulkanSwapchain(ResourceID p_Device, VkSwapchainKHR p_Handle, VkExtent2D p_Extent, VkSurfaceFormatKHR p_Format, uint32_t p_MinImageCount);
	
	VkExtent2D m_Extent;
	VkSurfaceFormatKHR m_Format;
	std::vector<VulkanImage> m_Images;
	std::vector<ResourceID> m_ImageViews;
	uint32_t m_MinImageCount = 0;

	uint32_t m_NextImage = 0;
	uint32_t m_ImageAvailableSemaphore = UINT32_MAX;
	bool m_WasAcquired = false;

	VkSwapchainKHR m_VkHandle = VK_NULL_HANDLE;

    friend class VulkanSwapchainExtension;
};

