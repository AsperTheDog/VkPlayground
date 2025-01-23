#pragma once
#include <Volk/volk.h>

#include "utils/identifiable.hpp"

class VulkanDevice;

class VulkanFence final : public VulkanDeviceSubresource
{
public:

	void reset();
	void wait();

	[[nodiscard]] bool isSignaled() const;

	VkFence operator*() const;

private:
	void free() override;

	VulkanFence(ResourceID p_Device, VkFence p_Fence, bool p_IsSignaled);

	VkFence m_VkHandle = VK_NULL_HANDLE;

	bool m_IsSignaled = false;

	friend class VulkanDevice;
	friend class SDLWindow;
	friend class VulkanCommandBuffer;
};

class VulkanSemaphore final : public VulkanDeviceSubresource
{
public:
	VkSemaphore operator*() const;

private:
	void free() override;

	VulkanSemaphore(ResourceID p_Device, VkSemaphore p_Semaphore);

	VkSemaphore m_VkHandle = VK_NULL_HANDLE;

	friend class VulkanDevice;
	friend class SDLWindow;
	friend class VulkanCommandBuffer;
};