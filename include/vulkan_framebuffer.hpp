#pragma once
#include "utils/identifiable.hpp"
#include <Volk/volk.h>

class VulkanDevice;

class VulkanFramebuffer final : public VulkanDeviceSubresource
{
public:
	VkFramebuffer operator*() const;

private:
	void free() override;

	VulkanFramebuffer(ResourceID p_Device, VkFramebuffer p_Handle);

private:
    VkFramebuffer m_VkHandle = VK_NULL_HANDLE;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
};

