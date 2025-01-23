#pragma once
#include <Volk/volk.h>

#include "utils/identifiable.hpp"


class VulkanDevice;

class VulkanDescriptorPool final : public VulkanDeviceSubresource
{
public:
	[[nodiscard]] VkDescriptorPool operator*() const;

private:
	void free() override;

	VulkanDescriptorPool(ResourceID p_Device, VkDescriptorPool p_DescriptorPool, VkDescriptorPoolCreateFlags p_Flags);

	VkDescriptorPool m_VkHandle = VK_NULL_HANDLE;

	VkDescriptorPoolCreateFlags m_Flags = 0;

	friend class VulkanDevice;
	friend class VulkanDescriptorSet;
};

class VulkanDescriptorSetLayout final : public VulkanDeviceSubresource
{
public:
	[[nodiscard]] VkDescriptorSetLayout operator*() const;

private:
	void free() override;

	VulkanDescriptorSetLayout(ResourceID p_Device, VkDescriptorSetLayout p_DescriptorSetLayout);

	VkDescriptorSetLayout m_VkHandle = VK_NULL_HANDLE;

	friend class VulkanDevice;
};

class VulkanDescriptorSet final : public VulkanDeviceSubresource
{
public:
	[[nodiscard]] VkDescriptorSet operator*() const;

	void updateDescriptorSet(const VkWriteDescriptorSet& p_WriteDescriptorSet) const;

private:
	void free() override;

	VulkanDescriptorSet(ResourceID p_Device, ResourceID p_Pool, VkDescriptorSet p_DescriptorSet);

	VkDescriptorSet m_VkHandle = VK_NULL_HANDLE;

	ResourceID m_Pool = UINT32_MAX;
	bool m_CanBeFreed = false;

	friend class VulkanDevice;
	friend class VulkanDescriptorPool;
	friend class VulkanDescriptorSetLayout;
};
