#pragma once
#include <span>
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

        AttachmentReference(const AttachmentType p_Type, const uint32_t p_Attachment, const VkImageLayout p_Layout)
            : type(p_Type), attachment(p_Attachment), layout(p_Layout)
        {
        }

        AttachmentReference(const AttachmentType p_Type, const uint32_t p_Attachment)
            : type(p_Type), attachment(p_Attachment)
        {
            switch (p_Type)
            {
            case COLOR:
                layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                break;

            case DEPTH_STENCIL:
                layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                break;

            case INPUT:
                layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                break;

            case RESOLVE:
                layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                break;

            case PRESERVE:
                layout = VK_IMAGE_LAYOUT_UNDEFINED;
                break;
            }
        }
	};

	VulkanRenderPassBuilder& addAttachment(const VkAttachmentDescription& p_Attachment);
	VulkanRenderPassBuilder& addSubpass(std::span<const AttachmentReference> p_Attachments, VkSubpassDescriptionFlags p_Flags);
	VulkanRenderPassBuilder& addDependency(const VkSubpassDependency& p_Dependency);

	static VkAttachmentDescription createAttachment(VkFormat p_Format, VkAttachmentLoadOp p_LoadOp, VkAttachmentStoreOp p_StoreOp, VkImageLayout p_InitialLayout, VkImageLayout p_FinalLayout);


private:
	struct SubpassInfo
	{
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

