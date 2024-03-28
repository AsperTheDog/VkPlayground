#include "vulkan_framebuffer.hpp"

#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

VkFramebuffer VulkanFramebuffer::operator*() const
{
	return m_vkHandle;
}

void VulkanFramebuffer::free()
{
	if (m_vkHandle != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(VulkanContext::getDevice(m_device).m_vkHandle, m_vkHandle, nullptr);
		m_vkHandle = VK_NULL_HANDLE;
	}
}

VulkanFramebuffer::VulkanFramebuffer(const uint32_t device, const VkFramebuffer handle)
	: m_vkHandle(handle), m_device(device)
{
	
}