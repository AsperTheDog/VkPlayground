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
		Logger::print("Freeing descriptor pool (ID: " + std::to_string(m_id) + ")" + (canDescrsBeFreed ? "" : " alongside all associated descriptor sets"), Logger::LevelBits::INFO);
		vkDestroyDescriptorPool(VulkanContext::getDevice(m_device).m_vkHandle, m_vkHandle, nullptr);
		m_vkHandle = VK_NULL_HANDLE;
	}
}

VulkanDescriptorPool::VulkanDescriptorPool(const uint32_t device, const VkDescriptorPool descriptorPool, const VkDescriptorPoolCreateFlags flags)
	: m_vkHandle(descriptorPool), m_device(device), m_flags(flags)
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
		Logger::print("Freeing descriptor set layout (ID: " + std::to_string(m_id) + ")", Logger::LevelBits::INFO);
		vkDestroyDescriptorSetLayout(VulkanContext::getDevice(m_device).m_vkHandle, m_vkHandle, nullptr);
		m_vkHandle = VK_NULL_HANDLE;
	}
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(const uint32_t device, const VkDescriptorSetLayout descriptorSetLayout)
	: m_vkHandle(descriptorSetLayout), m_device(device)
{

}

VkDescriptorSet VulkanDescriptorSet::operator*() const
{
	return m_vkHandle;
}

void VulkanDescriptorSet::updateDescriptorSet(const VkWriteDescriptorSet& writeDescriptorSet) const
{
    Logger::print("Updating descriptor set (ID: " + std::to_string(m_id) + ")", Logger::LevelBits::INFO);
    Logger::print(std::string("  Update info: descriptor type: ") + string_VkDescriptorType(writeDescriptorSet.descriptorType), Logger::LevelBits::DEBUG);
	vkUpdateDescriptorSets(VulkanContext::getDevice(m_device).m_vkHandle, 1, &writeDescriptorSet, 0, nullptr);
}

void VulkanDescriptorSet::free()
{
	if (m_vkHandle != VK_NULL_HANDLE)
	{
		if (!m_canBeFreed) return;
		Logger::print("Freeing descriptor set (ID: " + std::to_string(m_id) + ")", Logger::LevelBits::INFO);
		vkFreeDescriptorSets(VulkanContext::getDevice(m_device).m_vkHandle, VulkanContext::getDevice(m_device).getDescriptorPool(m_pool).m_vkHandle, 1, &m_vkHandle);
		m_vkHandle = VK_NULL_HANDLE;
	}
}

VulkanDescriptorSet::VulkanDescriptorSet(const uint32_t device, const uint32_t pool, const VkDescriptorSet descriptorSet)
	: m_vkHandle(descriptorSet), m_device(device), m_pool(pool), m_canBeFreed((VulkanContext::getDevice(m_device).getDescriptorPool(pool).m_flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT) != 0)
{
}
