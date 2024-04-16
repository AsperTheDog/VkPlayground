#pragma once
#include <vulkan/vulkan.h>

#include "utils/identifiable.hpp"


class VulkanDevice;

class VulkanDescriptorPool : public Identifiable
{
public:
	[[nodiscard]] VkDescriptorPool operator*() const;

private:
	void free();

	VulkanDescriptorPool(uint32_t device, VkDescriptorPool descriptorPool, VkDescriptorPoolCreateFlags flags);

	VkDescriptorPool m_vkHandle = VK_NULL_HANDLE;

	uint32_t m_device = UINT32_MAX;
	VkDescriptorPoolCreateFlags m_flags = 0;

	friend class VulkanDevice;
	friend class VulkanDescriptorSet;
};

class VulkanDescriptorSetLayout : public Identifiable
{
public:
	[[nodiscard]] VkDescriptorSetLayout operator*() const;

private:
	void free();

	VulkanDescriptorSetLayout(uint32_t device, VkDescriptorSetLayout descriptorSetLayout);

	VkDescriptorSetLayout m_vkHandle = VK_NULL_HANDLE;

	uint32_t m_device = UINT32_MAX;

	friend class VulkanDevice;
};

class VulkanDescriptorSet : public Identifiable
{
public:
	[[nodiscard]] VkDescriptorSet operator*() const;

	void updateDescriptorSet(const VkWriteDescriptorSet& writeDescriptorSet) const;

private:
	void free();

	VulkanDescriptorSet(uint32_t device, uint32_t pool, VkDescriptorSet descriptorSet);

	VkDescriptorSet m_vkHandle = VK_NULL_HANDLE;

	uint32_t m_device = UINT32_MAX;
	uint32_t m_pool = UINT32_MAX;
	bool m_canBeFreed = false;

	friend class VulkanDevice;
	friend class VulkanDescriptorPool;
	friend class VulkanDescriptorSetLayout;
};
