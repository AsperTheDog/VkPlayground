#include "vulkan_descriptors.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include "utils/logger.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

VkDescriptorPool VulkanDescriptorPool::operator*() const
{
	return m_vkHandle;
}

void VulkanDescriptorPool::free()
{
	if (m_vkHandle != VK_NULL_HANDLE)
	{
		const bool canDescrsBeFreed = (m_flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT) != 0;
		Logger::print("Freeing descriptor pool (ID: " + std::to_string(m_ID) + ")" + (canDescrsBeFreed ? "" : " alongside all associated descriptor sets"), Logger::DEBUG);
		vkDestroyDescriptorPool(VulkanContext::getDevice(getDeviceID()).m_VkHandle, m_vkHandle, nullptr);
		m_vkHandle = VK_NULL_HANDLE;
	}
}

VulkanDescriptorPool::VulkanDescriptorPool(const uint32_t device, const VkDescriptorPool descriptorPool, const VkDescriptorPoolCreateFlags flags)
	: VulkanDeviceSubresource(device), m_vkHandle(descriptorPool), m_flags(flags)
{

}

VkDescriptorSetLayout VulkanDescriptorSetLayout::operator*() const
{
	return m_vkHandle;
}

void VulkanDescriptorSetLayout::free()
{
	if (m_vkHandle != VK_NULL_HANDLE)
	{
		Logger::print("Freeing descriptor set layout (ID: " + std::to_string(m_ID) + ")", Logger::DEBUG);
		vkDestroyDescriptorSetLayout(VulkanContext::getDevice(getDeviceID()).m_VkHandle, m_vkHandle, nullptr);
		m_vkHandle = VK_NULL_HANDLE;
	}
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(const uint32_t device, const VkDescriptorSetLayout descriptorSetLayout)
	: VulkanDeviceSubresource(device), m_vkHandle(descriptorSetLayout)
{

}

VkDescriptorSet VulkanDescriptorSet::operator*() const
{
	return m_vkHandle;
}

void VulkanDescriptorSet::updateDescriptorSet(const VkWriteDescriptorSet& writeDescriptorSet) const
{
    Logger::print("Updating descriptor set (ID: " + std::to_string(m_ID) + ")", Logger::DEBUG);
    Logger::print(std::string("  Update info: descriptor type: ") + string_VkDescriptorType(writeDescriptorSet.descriptorType), Logger::DEBUG);
	vkUpdateDescriptorSets(VulkanContext::getDevice(getDeviceID()).m_VkHandle, 1, &writeDescriptorSet, 0, nullptr);
}

void VulkanDescriptorSet::free()
{
    VulkanDevice& device = VulkanContext::getDevice(getDeviceID());

	if (m_vkHandle != VK_NULL_HANDLE)
	{
		if (!m_canBeFreed) return;
		Logger::print("Freeing descriptor set (ID: " + std::to_string(m_ID) + ")", Logger::DEBUG);
		vkFreeDescriptorSets(device.m_VkHandle, device.getDescriptorPool(m_pool).m_vkHandle, 1, &m_vkHandle);
		m_vkHandle = VK_NULL_HANDLE;
	}
}

VulkanDescriptorSet::VulkanDescriptorSet(const uint32_t device, const uint32_t pool, const VkDescriptorSet descriptorSet)
	: VulkanDeviceSubresource(device), m_vkHandle(descriptorSet), m_pool(pool), m_canBeFreed((VulkanContext::getDevice(getDeviceID()).getDescriptorPool(pool).m_flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT) != 0)
{
}
