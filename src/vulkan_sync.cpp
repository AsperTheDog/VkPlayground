#include "vulkan_sync.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include "utils/logger.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

void VulkanFence::reset()
{
	vkResetFences(VulkanContext::getDevice(getDeviceID()).m_VkHandle, 1, &m_vkHandle);
	m_isSignaled = false;
}

void VulkanFence::wait()
{
    const VkResult result = vkWaitForFences(VulkanContext::getDevice(getDeviceID()).m_VkHandle, 1, &m_vkHandle, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS)
    {
        if (result == VK_ERROR_DEVICE_LOST)
            throw std::runtime_error("Device lost while waiting for fence (ID: " + std::to_string(m_ID) + ")");

        if (result == VK_TIMEOUT)
        {
            Logger::print("Fence (ID: " + std::to_string(m_ID) + ") wait timed out", Logger::LevelBits::WARN);
            return;
        }
        throw std::runtime_error("Failed to wait for fence (ID: " + std::to_string(m_ID) + "), error: " + string_VkResult(result));
    }

	m_isSignaled = true;
}

void VulkanFence::free()
{
	vkDestroyFence(VulkanContext::getDevice(getDeviceID()).m_VkHandle, m_vkHandle, nullptr);
    Logger::print("Freed fence (ID: " + std::to_string(m_ID) + ")", Logger::DEBUG);
	m_vkHandle = VK_NULL_HANDLE;
}

bool VulkanFence::isSignaled() const
{
	return m_isSignaled;
}

VkFence VulkanFence::operator*() const
{
	return m_vkHandle;
}

VulkanFence::VulkanFence(const uint32_t device, const VkFence fence, const bool isSignaled)
	: VulkanDeviceSubresource(device), m_vkHandle(fence), m_isSignaled(isSignaled)
{
}

VkSemaphore VulkanSemaphore::operator*() const
{
	return m_vkHandle;
}

void VulkanSemaphore::free()
{
	if (m_vkHandle != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(VulkanContext::getDevice(getDeviceID()).m_VkHandle, m_vkHandle, nullptr);
        Logger::print("Freed semaphore (ID: " + std::to_string(m_ID) + ")", Logger::DEBUG);
		m_vkHandle = VK_NULL_HANDLE;
	}
}

VulkanSemaphore::VulkanSemaphore(const uint32_t device, const VkSemaphore semaphore)
	: VulkanDeviceSubresource(device), m_vkHandle(semaphore)
{
}
