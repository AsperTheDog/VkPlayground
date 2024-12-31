#pragma once
#include "utils/identifiable.hpp"
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanFramebuffer final : public VulkanDeviceSubresource
{
public:
	VkFramebuffer operator*() const;

private:
	void free() override;

	VulkanFramebuffer(uint32_t device, VkFramebuffer handle);

private:
    VkFramebuffer m_vkHandle = VK_NULL_HANDLE;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
};

