#pragma once
#include <vulkan/vulkan.h>

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

	VulkanFence(uint32_t device, VkFence fence, bool isSignaled);

	VkFence m_vkHandle = VK_NULL_HANDLE;

	bool m_isSignaled = false;

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

	VulkanSemaphore(uint32_t device, VkSemaphore semaphore);

	VkSemaphore m_vkHandle = VK_NULL_HANDLE;

	friend class VulkanDevice;
	friend class SDLWindow;
	friend class VulkanCommandBuffer;
};