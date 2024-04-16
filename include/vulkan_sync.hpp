#pragma once
#include <vulkan/vulkan.h>

#include "utils/identifiable.hpp"

class VulkanDevice;

class VulkanFence : public Identifiable
{
public:

	void reset();
	void wait();

	[[nodiscard]] bool isSignaled() const;

	VkFence operator*() const;

private:
	void free();

	VulkanFence(uint32_t device, VkFence fence, bool isSignaled);

	VkFence m_vkHandle = VK_NULL_HANDLE;

	bool m_isSignaled = false;

	uint32_t m_device;

	friend class VulkanDevice;
	friend class SDLWindow;
	friend class VulkanCommandBuffer;
};

class VulkanSemaphore : public Identifiable
{
public:
	VkSemaphore operator*() const;

private:
	void free();

	VulkanSemaphore(uint32_t device, VkSemaphore semaphore);

	VkSemaphore m_vkHandle = VK_NULL_HANDLE;

	uint32_t m_device;

	friend class VulkanDevice;
	friend class SDLWindow;
	friend class VulkanCommandBuffer;
};