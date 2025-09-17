#include "vulkan_framebuffer.hpp"

#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

VkFramebuffer VulkanFramebuffer::operator*() const
{
    return m_VkHandle;
}

void VulkanFramebuffer::free()
{
    if (m_VkHandle != VK_NULL_HANDLE)
    {
        const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());
        l_Device.getTable().vkDestroyFramebuffer(l_Device.m_VkHandle, m_VkHandle, nullptr);
        m_VkHandle = VK_NULL_HANDLE;
    }
}

VulkanFramebuffer::VulkanFramebuffer(const uint32_t p_Device, const VkFramebuffer p_Handle)
    : VulkanDeviceSubresource(p_Device), m_VkHandle(p_Handle) {}
