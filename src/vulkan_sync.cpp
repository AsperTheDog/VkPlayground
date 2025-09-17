#include "vulkan_sync.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include "utils/logger.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

void VulkanFence::reset()
{
    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

    l_Device.getTable().vkResetFences(l_Device.m_VkHandle, 1, &m_VkHandle);
    m_IsSignaled = false;
}

void VulkanFence::wait()
{
    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

    const VkResult l_Result = l_Device.getTable().vkWaitForFences(l_Device.m_VkHandle, 1, &m_VkHandle, VK_TRUE, UINT64_MAX);
    if (l_Result != VK_SUCCESS)
    {
        if (l_Result == VK_ERROR_DEVICE_LOST)
        {
            throw std::runtime_error("Device lost while waiting for fence (ID: " + std::to_string(m_ID) + ")");
        }

        if (l_Result == VK_TIMEOUT)
        {
            LOG_WARN("Fence (ID: ", m_ID, ") wait timed out");
            return;
        }
        throw std::runtime_error("Failed to wait for fence (ID: " + std::to_string(m_ID) + "), error: " + string_VkResult(l_Result));
    }

    m_IsSignaled = true;
}

void VulkanFence::free()
{
    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());
    l_Device.getTable().vkDestroyFence(l_Device.m_VkHandle, m_VkHandle, nullptr);
    LOG_DEBUG("Freed fence (ID: ", m_ID, ")");
    m_VkHandle = VK_NULL_HANDLE;
}

bool VulkanFence::isSignaled() const
{
    return m_IsSignaled;
}

VkFence VulkanFence::operator*() const
{
    return m_VkHandle;
}

VulkanFence::VulkanFence(const uint32_t p_Device, const VkFence p_Fence, const bool p_IsSignaled)
    : VulkanDeviceSubresource(p_Device), m_VkHandle(p_Fence), m_IsSignaled(p_IsSignaled) {}

VkSemaphore VulkanSemaphore::operator*() const
{
    return m_VkHandle;
}

void VulkanSemaphore::free()
{
    if (m_VkHandle != VK_NULL_HANDLE)
    {
        const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

        l_Device.getTable().vkDestroySemaphore(l_Device.m_VkHandle, m_VkHandle, nullptr);
        LOG_DEBUG("Freed semaphore (ID: ", m_ID, ")");
        m_VkHandle = VK_NULL_HANDLE;
    }
}

VulkanSemaphore::VulkanSemaphore(const uint32_t p_Device, const VkSemaphore p_Semaphore)
    : VulkanDeviceSubresource(p_Device), m_VkHandle(p_Semaphore) {}
