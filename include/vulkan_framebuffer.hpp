#pragma once
#include "utils/identifiable.hpp"
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanFramebuffer : public Identifiable
{
public:
	VkFramebuffer operator*() const;

private:
	void free();

	VulkanFramebuffer(uint32_t device, VkFramebuffer handle);

	VkFramebuffer m_vkHandle = VK_NULL_HANDLE;

	uint32_t m_device;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
};

