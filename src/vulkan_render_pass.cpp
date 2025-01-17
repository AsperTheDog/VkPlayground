#include "vulkan_render_pass.hpp"

#include <iostream>

#include "utils/logger.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

VulkanRenderPassBuilder& VulkanRenderPassBuilder::addAttachment(const VkAttachmentDescription& attachment)
{
	m_attachments.push_back(attachment);
	return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::addSubpass(const VkPipelineBindPoint bindPoint, const std::vector<AttachmentReference>& attachments, const VkSubpassDescriptionFlags flags)
{
	SubpassInfo subpass{};
	subpass.bindPoint = bindPoint;
	subpass.flags = flags;

	uint32_t depthCount = 0;
	for (const AttachmentReference& attachment : attachments)
	{
		switch (attachment.type)
		{
			case COLOR:
				subpass.colorAttachments.push_back({attachment.attachment, attachment.layout});
				break;
			case DEPTH_STENCIL:
				subpass.depthStencilAttachment = {attachment.attachment, attachment.layout};
				depthCount++;
				subpass.hasDepthStencilAttachment = true;
				break;
			case INPUT:
				subpass.inputAttachments.push_back({attachment.attachment, attachment.layout});
				break;
			case RESOLVE:
				subpass.resolveAttachments.push_back({attachment.attachment, attachment.layout});
				break;
			case PRESERVE:
				subpass.preserveAttachments.push_back(attachment.attachment);
				break;
		}
	}

	if (depthCount > 1)
		Logger::print("Only 1 depth stencil attachment allowed in a subpass, received " + std::to_string(depthCount) + " instead", Logger::LevelBits::WARN);

	if ((flags & VK_SUBPASS_DESCRIPTION_SHADER_RESOLVE_BIT_QCOM) == 0 && !subpass.resolveAttachments.empty() && subpass.resolveAttachments.size() != subpass.colorAttachments.size())
		Logger::print("Number of resolve attachments must be equal to the number of color attachments", Logger::LevelBits::WARN);

	m_subpasses.push_back(subpass);

	return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::addDependency(const VkSubpassDependency& dependency)
{
	m_dependencies.push_back(dependency);
	return *this;
}

VkAttachmentDescription VulkanRenderPassBuilder::createAttachment(const VkFormat format, const VkAttachmentLoadOp loadOp, const VkAttachmentStoreOp storeOp, const VkImageLayout initialLayout, const VkImageLayout finalLayout)
{
	VkAttachmentDescription attachment{};
	attachment.format = format;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = loadOp;
	attachment.storeOp = storeOp;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = initialLayout;
	attachment.finalLayout = finalLayout;
	return attachment;
}

VkRenderPass VulkanRenderPass::operator*() const
{
	return m_vkHandle;
}

void VulkanRenderPass::free()
{
	Logger::print("Freeing render pass (ID: " + std::to_string(m_ID) + ")", Logger::DEBUG);
	vkDestroyRenderPass(VulkanContext::getDevice(getDeviceID()).m_VkHandle, m_vkHandle, nullptr);
	m_vkHandle = VK_NULL_HANDLE;
}

VulkanRenderPass::VulkanRenderPass(const uint32_t device, const VkRenderPass renderPass)
	: VulkanDeviceSubresource(device), m_vkHandle(renderPass)
{

}
