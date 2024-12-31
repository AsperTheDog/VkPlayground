#pragma once
#include <vulkan/vulkan.h>

#include "utils/identifiable.hpp"


class VulkanDevice;

class VulkanDescriptorPool final : public VulkanDeviceSubresource
{
public:
	[[nodiscard]] VkDescriptorPool operator*() const;

private:
	void free() override;

	VulkanDescriptorPool(uint32_t device, VkDescriptorPool descriptorPool, VkDescriptorPoolCreateFlags flags);

	VkDescriptorPool m_vkHandle = VK_NULL_HANDLE;

	VkDescriptorPoolCreateFlags m_flags = 0;

	friend class VulkanDevice;
	friend class VulkanDescriptorSet;
};

class VulkanDescriptorSetLayout final : public VulkanDeviceSubresource
{
public:
	[[nodiscard]] VkDescriptorSetLayout operator*() const;

private:
	void free() override;

	VulkanDescriptorSetLayout(uint32_t device, VkDescriptorSetLayout descriptorSetLayout);

	VkDescriptorSetLayout m_vkHandle = VK_NULL_HANDLE;

	friend class VulkanDevice;
};

class VulkanDescriptorSet final : public VulkanDeviceSubresource
{
public:
	[[nodiscard]] VkDescriptorSet operator*() const;

	void updateDescriptorSet(const VkWriteDescriptorSet& writeDescriptorSet) const;

private:
	void free() override;

	VulkanDescriptorSet(uint32_t device, uint32_t pool, VkDescriptorSet descriptorSet);

	VkDescriptorSet m_vkHandle = VK_NULL_HANDLE;

	uint32_t m_pool = UINT32_MAX;
	bool m_canBeFreed = false;

	friend class VulkanDevice;
	friend class VulkanDescriptorPool;
	friend class VulkanDescriptorSetLayout;
};
