#include "vulkan_render_pass.hpp"

#include <iostream>

#include "utils/logger.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

VulkanRenderPassBuilder& VulkanRenderPassBuilder::addAttachment(const VkAttachmentDescription& p_Attachment)
{
	m_Attachments.push_back(p_Attachment);
	return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::addSubpass(const VkPipelineBindPoint bindPoint, const std::vector<AttachmentReference>& p_Attachments, const VkSubpassDescriptionFlags p_Flags)
{
	SubpassInfo l_Subpass{};
	l_Subpass.bindPoint = bindPoint;
	l_Subpass.flags = p_Flags;

	uint32_t l_DepthCount = 0;
	for (const AttachmentReference& l_Attachment : p_Attachments)
	{
		switch (l_Attachment.type)
		{
			case COLOR:
				l_Subpass.colorAttachments.push_back({l_Attachment.attachment, l_Attachment.layout});
				break;
			case DEPTH_STENCIL:
				l_Subpass.depthStencilAttachment = {l_Attachment.attachment, l_Attachment.layout};
				l_DepthCount++;
				l_Subpass.hasDepthStencilAttachment = true;
				break;
			case INPUT:
				l_Subpass.inputAttachments.push_back({l_Attachment.attachment, l_Attachment.layout});
				break;
			case RESOLVE:
				l_Subpass.resolveAttachments.push_back({l_Attachment.attachment, l_Attachment.layout});
				break;
			case PRESERVE:
				l_Subpass.preserveAttachments.push_back(l_Attachment.attachment);
				break;
		}
	}

	if (l_DepthCount > 1)
        LOG_WARN("Only 1 depth stencil attachment allowed in a subpass, received ", l_DepthCount, " instead");
		
	if ((p_Flags & VK_SUBPASS_DESCRIPTION_SHADER_RESOLVE_BIT_QCOM) == 0 && !l_Subpass.resolveAttachments.empty() && l_Subpass.resolveAttachments.size() != l_Subpass.colorAttachments.size())
        LOG_WARN("Number of resolve attachments must be equal to the number of color attachments", Logger::LevelBits::WARN);

	m_Subpasses.push_back(l_Subpass);

	return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::addDependency(const VkSubpassDependency& p_Dependency)
{
	m_Dependencies.push_back(p_Dependency);
	return *this;
}

VkAttachmentDescription VulkanRenderPassBuilder::createAttachment(const VkFormat p_Format, const VkAttachmentLoadOp p_LoadOp, const VkAttachmentStoreOp p_StoreOp, const VkImageLayout p_InitialLayout, const VkImageLayout p_FinalLayout)
{
	VkAttachmentDescription l_Attachment{};
	l_Attachment.format = p_Format;
	l_Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	l_Attachment.loadOp = p_LoadOp;
	l_Attachment.storeOp = p_StoreOp;
	l_Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	l_Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	l_Attachment.initialLayout = p_InitialLayout;
	l_Attachment.finalLayout = p_FinalLayout;
	return l_Attachment;
}

VkRenderPass VulkanRenderPass::operator*() const
{
	return m_VkHandle;
}

void VulkanRenderPass::free()
{
    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

	l_Device.getTable().vkDestroyRenderPass(l_Device.m_VkHandle, m_VkHandle, nullptr);
    LOG_DEBUG("Freeing render pass (ID: ", m_ID, ")");
	m_VkHandle = VK_NULL_HANDLE;
}

VulkanRenderPass::VulkanRenderPass(const uint32_t p_Device, const VkRenderPass p_RenderPass)
	: VulkanDeviceSubresource(p_Device), m_VkHandle(p_RenderPass)
{

}
