#pragma once
#include <vector>
#include <Volk/volk.h>

#include "utils/identifiable.hpp"

class VulkanDevice;

enum AttachmentType
{
	COLOR,
	DEPTH_STENCIL,
	INPUT,
	RESOLVE,
	PRESERVE
};

class VulkanRenderPassBuilder
{
public:
	struct AttachmentReference
	{
		AttachmentType type;
		uint32_t attachment;
		VkImageLayout layout;
	};

	VulkanRenderPassBuilder& addAttachment(const VkAttachmentDescription& p_Attachment);
	VulkanRenderPassBuilder& addSubpass(VkPipelineBindPoint bindPoint, const std::vector<AttachmentReference>& p_Attachments, VkSubpassDescriptionFlags p_Flags);
	VulkanRenderPassBuilder& addDependency(const VkSubpassDependency& p_Dependency);

	static VkAttachmentDescription createAttachment(VkFormat p_Format, VkAttachmentLoadOp p_LoadOp, VkAttachmentStoreOp p_StoreOp, VkImageLayout p_InitialLayout, VkImageLayout p_FinalLayout);


private:
	struct SubpassInfo
	{
		VkPipelineBindPoint bindPoint;
		VkSubpassDescriptionFlags flags;
		std::vector<VkAttachmentReference> colorAttachments;
		std::vector<VkAttachmentReference> resolveAttachments;
		std::vector<VkAttachmentReference> inputAttachments;
		VkAttachmentReference depthStencilAttachment;
		std::vector<uint32_t> preserveAttachments;
		bool hasDepthStencilAttachment = false;
	};

	std::vector<VkAttachmentDescription> m_Attachments;
	std::vector<SubpassInfo> m_Subpasses;
	std::vector<VkSubpassDependency> m_Dependencies;

	friend class VulkanDevice;
};

class VulkanRenderPass final : public VulkanDeviceSubresource
{
public:
	VkRenderPass operator*() const;

private:
	void free() override;

	VulkanRenderPass(ResourceID p_Device, VkRenderPass p_RenderPass);

private:
    VkRenderPass m_VkHandle = VK_NULL_HANDLE;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
};

