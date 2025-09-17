#include "vulkan_descriptors.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include "utils/logger.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

VkDescriptorPool VulkanDescriptorPool::operator*() const
{
    return m_VkHandle;
}

void VulkanDescriptorPool::free()
{
    if (m_VkHandle != VK_NULL_HANDLE)
    {
        const bool l_CanDescrsBeFreed = (m_Flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT) != 0;
        LOG_DEBUG("Freeing descriptor pool (ID: ", m_ID, ")", l_CanDescrsBeFreed ? "" : " alongside all associated descriptor sets");
        VulkanContext::getDevice(getDeviceID()).getTable().vkDestroyDescriptorPool(VulkanContext::getDevice(getDeviceID()).m_VkHandle, m_VkHandle, nullptr);
        m_VkHandle = VK_NULL_HANDLE;
    }
}

VulkanDescriptorPool::VulkanDescriptorPool(const ResourceID p_Device, const VkDescriptorPool p_DescriptorPool, const VkDescriptorPoolCreateFlags p_Flags)
    : VulkanDeviceSubresource(p_Device), m_VkHandle(p_DescriptorPool), m_Flags(p_Flags) {}

VkDescriptorSetLayout VulkanDescriptorSetLayout::operator*() const
{
    return m_VkHandle;
}

void VulkanDescriptorSetLayout::free()
{
    if (m_VkHandle != VK_NULL_HANDLE)
    {
        LOG_DEBUG("Freeing descriptor set layout (ID: ", m_ID, ")");
        VulkanContext::getDevice(getDeviceID()).getTable().vkDestroyDescriptorSetLayout(VulkanContext::getDevice(getDeviceID()).m_VkHandle, m_VkHandle, nullptr);
        m_VkHandle = VK_NULL_HANDLE;
    }
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(const ResourceID p_Device, const VkDescriptorSetLayout p_DescriptorSetLayout)
    : VulkanDeviceSubresource(p_Device), m_VkHandle(p_DescriptorSetLayout) {}

VkDescriptorSet VulkanDescriptorSet::operator*() const
{
    return m_VkHandle;
}

void VulkanDescriptorSet::updateDescriptorSet(const VkWriteDescriptorSet& p_WriteDescriptorSet) const
{
    LOG_DEBUG("Updating descriptor set (ID: ", m_ID, ")");
    LOG_DEBUG("  Update info: descriptor type: ", string_VkDescriptorType(p_WriteDescriptorSet.descriptorType));
    VulkanContext::getDevice(getDeviceID()).getTable().vkUpdateDescriptorSets(VulkanContext::getDevice(getDeviceID()).m_VkHandle, 1, &p_WriteDescriptorSet, 0, nullptr);
}

void VulkanDescriptorSet::free()
{
    if (m_VkHandle != VK_NULL_HANDLE)
    {
        VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

        if (!m_CanBeFreed)
        {
            return;
        }
        LOG_DEBUG("Freeing descriptor set (ID: ", m_ID, ")");
        l_Device.getTable().vkFreeDescriptorSets(l_Device.m_VkHandle, l_Device.getDescriptorPool(m_Pool).m_VkHandle, 1, &m_VkHandle);
        m_VkHandle = VK_NULL_HANDLE;
    }
}

VulkanDescriptorSet::VulkanDescriptorSet(const uint32_t p_Device, const uint32_t p_Pool, const VkDescriptorSet p_DescriptorSet)
    : VulkanDeviceSubresource(p_Device), m_VkHandle(p_DescriptorSet), m_Pool(p_Pool), m_CanBeFreed((VulkanContext::getDevice(getDeviceID()).getDescriptorPool(p_Pool).m_Flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT) != 0) {}
