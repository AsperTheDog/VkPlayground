#include "vulkan_device.hpp"

#include <ranges>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_context.hpp"
#include "utils/logger.hpp"

VulkanQueue VulkanDevice::getQueue(const QueueSelection& p_QueueSelection) const
{
	VkQueue l_Queue;
	vkGetDeviceQueue(m_VkHandle, p_QueueSelection.familyIndex, p_QueueSelection.queueIndex, &l_Queue);
	return VulkanQueue(l_Queue);
}

VulkanDeviceSubresource* VulkanDevice::getSubresource(const ResourceID p_ID) const
{
    if (m_Subresources.contains(p_ID))
    {
        return m_Subresources.at(p_ID);
    }
    return nullptr;
}

bool VulkanDevice::freeSubresource(const ResourceID p_ID)
{
    VulkanDeviceSubresource* l_Component = getSubresource(p_ID);
    if (l_Component)
    {
        l_Component->free();
        m_Subresources.erase(p_ID);
        delete l_Component;
        return true;
    }
    return false;
}

void VulkanDevice::configureOneTimeQueue(const QueueSelection p_Queue)
{
	m_OneTimeQueue = p_Queue;
}

void VulkanDevice::initializeOneTimeCommandPool(const uint32_t p_ThreadID)
{
	if (!m_ThreadCommandInfos.contains(p_ThreadID))
	{
		m_ThreadCommandInfos[p_ThreadID] = {};
	}
	ThreadCommandInfo& l_ThreadInfo = m_ThreadCommandInfos[p_ThreadID];

	if (l_ThreadInfo.oneTimePool != VK_NULL_HANDLE) return;

	VkCommandPoolCreateInfo l_PoolInfo{};
	l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	l_PoolInfo.queueFamilyIndex = m_OneTimeQueue.familyIndex;
	l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (const VkResult l_Ret = vkCreateCommandPool(m_VkHandle, &l_PoolInfo, nullptr, &l_ThreadInfo.oneTimePool); l_Ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create command pool, error: ") + string_VkResult(l_Ret));
	}
}

void VulkanDevice::initializeCommandPool(const QueueFamily& p_Family, const ThreadID p_ThreadID, const bool p_CreateSecondary)
{
	if (!m_ThreadCommandInfos.contains(p_ThreadID))
	{
		m_ThreadCommandInfos[p_ThreadID] = {};
	}

	ThreadCommandInfo& l_ThreadInfo = m_ThreadCommandInfos[p_ThreadID];
	if (!l_ThreadInfo.commandPools.contains(p_Family.index))
	{
		VkCommandPoolCreateInfo l_PoolInfo{};
		l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		l_PoolInfo.queueFamilyIndex = p_Family.index;
		l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (const VkResult l_Ret = vkCreateCommandPool(m_VkHandle, &l_PoolInfo, nullptr, &l_ThreadInfo.commandPools[p_Family.index].pool); l_Ret != VK_SUCCESS)
		{
			throw std::runtime_error(std::string("Failed to create main command pool for thread " + std::to_string(p_ThreadID) + ", error: ") + string_VkResult(l_Ret));
		}
		Logger::print("Created main command pool for thread " + std::to_string(p_ThreadID) + " and family " + std::to_string(p_Family.index), Logger::DEBUG);

		if (p_CreateSecondary)
		{
			if (const VkResult l_Ret = vkCreateCommandPool(m_VkHandle, &l_PoolInfo, nullptr, &l_ThreadInfo.commandPools[p_Family.index].secondaryPool); l_Ret != VK_SUCCESS)
			{
				throw std::runtime_error(std::string("Failed to create secondary command pool for thread " + std::to_string(p_ThreadID) + ", error: ") + string_VkResult(l_Ret));
			}
			Logger::print("Created secondary command pool for thread " + std::to_string(p_ThreadID) + " and family " + std::to_string(p_Family.index), Logger::DEBUG);
		}
	}
}

ResourceID VulkanDevice::createCommandBuffer(const QueueFamily& p_Family, const ThreadID p_ThreadID, const bool p_IsSecondary)
{
	initializeCommandPool(p_Family, p_ThreadID, p_IsSecondary);

    VulkanCommandBuffer::TypeFlags l_Type = 0;
	VkCommandBufferAllocateInfo l_AllocInfo{};
	l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	if (p_IsSecondary)
	{
		l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		l_AllocInfo.commandPool = m_ThreadCommandInfos[p_ThreadID].commandPools[p_Family.index].secondaryPool;
        l_Type = VulkanCommandBuffer::TypeFlagBits::SECONDARY;
	}
	else
	{
		l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		l_AllocInfo.commandPool = m_ThreadCommandInfos[p_ThreadID].commandPools[p_Family.index].pool;
	}
	l_AllocInfo.commandBufferCount = 1;

	VkCommandBuffer l_CommandBuffer;
	if (const VkResult l_Ret = vkAllocateCommandBuffers(m_VkHandle, &l_AllocInfo, &l_CommandBuffer); l_Ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to allocate command buffer in thread + " + std::to_string(p_ThreadID) + ", error:") + string_VkResult(l_Ret));
	}
	Logger::print("Allocated command buffer for thread " + std::to_string(p_ThreadID) + " and family " + std::to_string(p_Family.index), Logger::DEBUG);

	if (!m_CommandBuffers.contains(p_ThreadID))
	{
		m_CommandBuffers[p_ThreadID] = {};
	}

	m_CommandBuffers[p_ThreadID].push_back({ m_ID, l_CommandBuffer, l_Type, p_Family.index, p_ThreadID });
	return m_CommandBuffers[p_ThreadID].back().getID();
}

ResourceID VulkanDevice::createOneTimeCommandBuffer(ThreadID p_ThreadID)
{
	initializeOneTimeCommandPool(p_ThreadID);

	VkCommandBufferAllocateInfo l_AllocInfo{};
	l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	l_AllocInfo.commandPool = m_ThreadCommandInfos[p_ThreadID].oneTimePool;
	l_AllocInfo.commandBufferCount = 1;

	VkCommandBuffer l_CommandBuffer;
	if (const VkResult l_Ret = vkAllocateCommandBuffers(m_VkHandle, &l_AllocInfo, &l_CommandBuffer); l_Ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to allocate one time command buffer in thread + " + std::to_string(p_ThreadID) + ", error:") + string_VkResult(l_Ret));
	}
	Logger::print("Allocated one time command buffer for thread " + std::to_string(p_ThreadID), Logger::DEBUG);

	if (!m_CommandBuffers.contains(p_ThreadID))
	{
		m_CommandBuffers[p_ThreadID] = {};
	}

	m_CommandBuffers[p_ThreadID].push_back({ m_ID, l_CommandBuffer, VulkanCommandBuffer::TypeFlagBits::ONE_TIME, m_OneTimeQueue.familyIndex, p_ThreadID });
	return m_CommandBuffers[p_ThreadID].back().getID();
}

ResourceID VulkanDevice::getOrCreateCommandBuffer(const QueueFamily& p_Family, const ThreadID p_ThreadID, const VulkanCommandBuffer::TypeFlags p_Flags)
{
	for (const VulkanCommandBuffer& l_Buffer : m_CommandBuffers[p_ThreadID])
		if (l_Buffer.m_familyIndex == p_Family.index && l_Buffer.m_threadID == p_ThreadID && l_Buffer.m_flags == p_Flags)
		{
			Logger::print("Reusing command buffer for thread " + std::to_string(p_ThreadID) + " and family " + std::to_string(p_Family.index), Logger::DEBUG);
			return l_Buffer.getID();
		}

	return createCommandBuffer(p_Family, p_ThreadID, (p_Flags & VulkanCommandBuffer::TypeFlagBits::SECONDARY) != 0);
}

VulkanCommandBuffer& VulkanDevice::getCommandBuffer(const ResourceID p_ID, const ThreadID p_ThreadID)
{
	for (VulkanCommandBuffer& l_Buffer : m_CommandBuffers[p_ThreadID])
		if (l_Buffer.m_ID == p_ID)
			return l_Buffer;

	Logger::print("Command buffer search failed out of " + std::to_string(m_CommandBuffers.size()) + " command buffers", Logger::DEBUG);
	throw std::runtime_error("Command buffer (ID:" + std::to_string(p_ID) + ") not found");
}

const VulkanCommandBuffer& VulkanDevice::getCommandBuffer(const ResourceID p_ID, const ThreadID p_ThreadID) const
{
	return const_cast<VulkanDevice*>(this)->getCommandBuffer(p_ID, p_ThreadID);
}

void VulkanDevice::freeCommandBuffer(const VulkanCommandBuffer& p_CommandBuffer, const ThreadID p_ThreadID)
{
	freeCommandBuffer(p_CommandBuffer.m_ID, p_ThreadID);
}

void VulkanDevice::freeCommandBuffer(const ResourceID p_ID, const ThreadID p_ThreadID)
{
	for (auto l_It = m_CommandBuffers[p_ThreadID].begin(); l_It != m_CommandBuffers[p_ThreadID].end(); ++l_It)
	{
		if (l_It->m_ID == p_ID)
		{
			l_It->free();
			m_CommandBuffers[p_ThreadID].erase(l_It);
			break;
		}
	}
}

std::vector<VulkanFramebuffer*> VulkanDevice::getFramebuffers() const
{
    std::vector<VulkanFramebuffer*> l_Framebuffers;
    for (VulkanDeviceSubresource* const l_Component : m_Subresources | std::views::values)
    {
        if (VulkanFramebuffer* l_Framebuffer = dynamic_cast<VulkanFramebuffer*>(l_Component))
        {
            l_Framebuffers.push_back(l_Framebuffer);
        }
    }
    return l_Framebuffers;
}

uint32_t VulkanDevice::getFramebufferCount() const
{
    uint32_t l_Count = 0;
    for (VulkanDeviceSubresource* const l_Component : m_Subresources | std::views::values)
    {
        if (dynamic_cast<VulkanFramebuffer*>(l_Component))
        {
            l_Count++;
        }
    }
    return l_Count;
}

ResourceID VulkanDevice::createFramebuffer(const VkExtent3D p_Size, const VulkanRenderPass& p_RenderPass, const std::vector<VkImageView>& p_Attachments)
{
	VkFramebufferCreateInfo l_FramebufferInfo{};
	l_FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	l_FramebufferInfo.renderPass = p_RenderPass.m_vkHandle;
	l_FramebufferInfo.attachmentCount = static_cast<uint32_t>(p_Attachments.size());
	l_FramebufferInfo.pAttachments = p_Attachments.data();
	l_FramebufferInfo.width = p_Size.width;
	l_FramebufferInfo.height = p_Size.height;
	l_FramebufferInfo.layers = p_Size.depth;

	VkFramebuffer l_Framebuffer;
	if (const VkResult l_Ret = vkCreateFramebuffer(m_VkHandle, &l_FramebufferInfo, nullptr, &l_Framebuffer); l_Ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create framebuffer, error: ") + string_VkResult(l_Ret));
	}

    VulkanFramebuffer* l_NewComp = new VulkanFramebuffer{m_ID, l_Framebuffer};
    m_Subresources[l_NewComp->getID()] = l_NewComp;
	Logger::print("Created framebuffer (ID:" + std::to_string(l_NewComp->getID()) + ")", Logger::DEBUG);
	return l_NewComp->getID();
}

ResourceID VulkanDevice::createBuffer(const VkDeviceSize p_Size, const VkBufferUsageFlags p_Usage)
{

	VkBufferCreateInfo l_BufferInfo{};
	l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	l_BufferInfo.size = p_Size;
	l_BufferInfo.usage = p_Usage;
	l_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	l_BufferInfo.flags = 0;

	VkBuffer l_Buffer;
	if (const VkResult l_Ret = vkCreateBuffer(m_VkHandle, &l_BufferInfo, nullptr, &l_Buffer); l_Ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create buffer, error: ") + string_VkResult(l_Ret));
	}

    VulkanBuffer* l_NewComp = new VulkanBuffer{m_ID, l_Buffer, p_Size};
    m_Subresources[l_NewComp->getID()] = l_NewComp;
	Logger::print("Created buffer (ID:" + std::to_string(l_NewComp->getID()) + ") with size " + VulkanMemoryAllocator::compactBytes(l_NewComp->getSize()), Logger::DEBUG);
	return l_NewComp->getID();
}

ResourceID VulkanDevice::createImage(const VkImageType p_Type, const VkFormat p_Format, const VkExtent3D p_Extent, const VkImageUsageFlags p_Usage, const VkImageCreateFlags p_Flags)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = p_Type;
	imageInfo.format = p_Format;
	imageInfo.extent = p_Extent;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = p_Usage;
	imageInfo.flags = p_Flags;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices = nullptr;

	VkImage image;
	if (const VkResult ret = vkCreateImage(m_VkHandle, &imageInfo, nullptr, &image); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create image, error: ") + string_VkResult(ret));
	}

    VulkanImage* l_NewComp = new VulkanImage{m_ID, image, p_Extent, p_Type, VK_IMAGE_LAYOUT_UNDEFINED};
    m_Subresources[l_NewComp->getID()] = l_NewComp;
	Logger::print("Created image (ID:" + std::to_string(l_NewComp->getID()) + ")", Logger::DEBUG);
	return l_NewComp->getID();
}

void VulkanDevice::configureStagingBuffer(const VkDeviceSize size, const QueueSelection& queue, const bool forceAllowStagingMemory)
{
	if (m_stagingBufferInfo.stagingBuffer != UINT32_MAX)
	{
		freeStagingBuffer();
	}
	m_stagingBufferInfo.stagingBuffer = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	m_stagingBufferInfo.queue = queue;

	VulkanBuffer& stagingBuffer = getBuffer(m_stagingBufferInfo.stagingBuffer);
	const VkMemoryRequirements memRequirements = stagingBuffer.getMemoryRequirements();
	const std::optional<uint32_t> memoryType = m_MemoryAllocator.getMemoryStructure().getStagingMemoryType(memRequirements.memoryTypeBits);
	if (memoryType.has_value() && !m_MemoryAllocator.isMemoryTypeHidden(memoryType.value()))
	{
		const uint32_t heapIndex = m_PhysicalDevice.getMemoryProperties().memoryTypes[memoryType.value()].heapIndex;
		const VkDeviceSize heapSize = m_PhysicalDevice.getMemoryProperties().memoryHeaps[heapIndex].size;
		if (static_cast<double>(heapSize) < static_cast<double>(size) * 0.8)
		{
			Logger::print("Staging buffer size is " + VulkanMemoryAllocator::compactBytes(size) + ", but special staging memory heap size is " + VulkanMemoryAllocator::compactBytes(heapSize) + " for memory type " + std::to_string(memoryType.value()) + ", allocating in host memory", Logger::WARN);
			stagingBuffer.allocateFromFlags({ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT, true });
			return;
		}
		Logger::print("Staging buffer size is " + VulkanMemoryAllocator::compactBytes(size) + ", allocating in special staging memory type " + std::to_string(memoryType.value()), Logger::DEBUG);
        try
        {
	        stagingBuffer.allocateFromIndex(memoryType.value());
		    if (!forceAllowStagingMemory)
			    m_MemoryAllocator.hideMemoryType(memoryType.value());
        }
        catch (const std::runtime_error&)
        {
            Logger::print("Failed to allocate staging buffer in special memory type " + std::to_string(memoryType.value()) + ", allocating in host memory", Logger::WARN);
            stagingBuffer.allocateFromFlags({ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT, true });
        }
	}
	else
	{
		Logger::print("Staging buffer size is " + VulkanMemoryAllocator::compactBytes(size) + ", but no suitable special staging memory type found, allocating in host memory", Logger::WARN);
		stagingBuffer.allocateFromFlags({ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT, true });
	}
}

VkDeviceSize VulkanDevice::getStagingBufferSize() const
{
	if (m_stagingBufferInfo.stagingBuffer == UINT32_MAX) return 0;
	return getBuffer(m_stagingBufferInfo.stagingBuffer).getSize();
}

bool VulkanDevice::freeStagingBuffer()
{
	if (m_stagingBufferInfo.stagingBuffer != UINT32_MAX)
	{
		{
			VulkanBuffer& stagingBuffer = getBuffer(m_stagingBufferInfo.stagingBuffer);
			if (stagingBuffer.isMemoryBound())
				m_MemoryAllocator.unhideMemoryType(stagingBuffer.getBoundMemoryType());
			stagingBuffer.free();
		}
		m_stagingBufferInfo.stagingBuffer = UINT32_MAX;
        return true;
	}
    return false;
}

void* VulkanDevice::mapStagingBuffer(const VkDeviceSize size, const VkDeviceSize offset)
{
	return getBuffer(m_stagingBufferInfo.stagingBuffer).map(size, offset);
}

void VulkanDevice::unmapStagingBuffer()
{
	getBuffer(m_stagingBufferInfo.stagingBuffer).unmap();
}

void VulkanDevice::dumpStagingBuffer(const ResourceID buffer, const VkDeviceSize size, const VkDeviceSize offset, const ThreadID threadID)
{
	dumpStagingBuffer(buffer, { {0, offset, size} }, threadID);
}

void VulkanDevice::dumpStagingBuffer(const ResourceID buffer, const std::vector<VkBufferCopy>& regions, const ThreadID threadID)
{
	VulkanBuffer& stagingBuffer = getBuffer(m_stagingBufferInfo.stagingBuffer);
	if (stagingBuffer.m_vkHandle == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Tried to dump staging buffer (ID: " + std::to_string(buffer) + ") data, but staging buffer is not configured");
	}

	if (stagingBuffer.isMemoryMapped())
	{
		Logger::print("Automatically unmapping staging buffer before dumping into buffer (ID: " + std::to_string(buffer) + ")", Logger::DEBUG);
		stagingBuffer.unmap();
	}

	VulkanCommandBuffer& commandBuffer = getCommandBuffer(createOneTimeCommandBuffer(threadID), threadID);

	commandBuffer.beginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	commandBuffer.cmdCopyBuffer(m_stagingBufferInfo.stagingBuffer, buffer, regions);
	commandBuffer.endRecording();
	const VulkanQueue queue = getQueue(m_stagingBufferInfo.queue);
	commandBuffer.submit(queue, {}, {});
	Logger::print("Submitted staging buffer data to buffer (ID: " + std::to_string(buffer) + "), total size: " + VulkanMemoryAllocator::compactBytes(stagingBuffer.getSize()), Logger::DEBUG);

	queue.waitIdle();
	freeCommandBuffer(commandBuffer, threadID);
}

void VulkanDevice::dumpStagingBufferToImage(const uint32_t image, const VkExtent3D size, const VkOffset3D offset, const uint32_t threadID, const bool keepLayout)
{
    const VulkanBuffer& stagingBuffer = getBuffer(m_stagingBufferInfo.stagingBuffer);

    VkBufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = offset;
    region.imageExtent = size;

	VulkanCommandBuffer& commandBuffer = getCommandBuffer(createOneTimeCommandBuffer(threadID), threadID);
    const VkImageLayout layout = getImage(image).getLayout();

	commandBuffer.beginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    if (layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
        commandBuffer.cmdSimpleTransitionImageLayout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	commandBuffer.cmdCopyBufferToImage(m_stagingBufferInfo.stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {region});
    if (layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && keepLayout) 
        commandBuffer.cmdSimpleTransitionImageLayout(image, layout);
	commandBuffer.endRecording();
	const VulkanQueue queue = getQueue(m_stagingBufferInfo.queue);
	commandBuffer.submit(queue, {}, {});
	Logger::print("Submitted staging buffer data to image (ID: " + std::to_string(image) + "), total size: " + VulkanMemoryAllocator::compactBytes(stagingBuffer.getSize()), Logger::DEBUG);

	queue.waitIdle();
	freeCommandBuffer(commandBuffer, threadID);
}

void VulkanDevice::disallowMemoryType(const uint32_t type)
{
	m_MemoryAllocator.hideMemoryType(type);
}

void VulkanDevice::allowMemoryType(const uint32_t type)
{
	m_MemoryAllocator.unhideMemoryType(type);
}

ResourceID VulkanDevice::createRenderPass(const VulkanRenderPassBuilder& builder, const VkRenderPassCreateFlags flags)
{
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(builder.m_attachments.size());
	renderPassInfo.pAttachments = builder.m_attachments.data();
	renderPassInfo.subpassCount = static_cast<uint32_t>(builder.m_subpasses.size());
	std::vector<VkSubpassDescription> subpasses;
	for (const auto& subpass : builder.m_subpasses)
	{
		VkSubpassDescription subpassInfo{};
		subpassInfo.pipelineBindPoint = subpass.bindPoint;
		subpassInfo.flags = subpass.flags;
		subpassInfo.colorAttachmentCount = static_cast<uint32_t>(subpass.colorAttachments.size());
		subpassInfo.pColorAttachments = subpass.colorAttachments.data();
		subpassInfo.pResolveAttachments = subpass.resolveAttachments.data();
		subpassInfo.pDepthStencilAttachment = subpass.hasDepthStencilAttachment ? &subpass.depthStencilAttachment : VK_NULL_HANDLE;
		subpassInfo.inputAttachmentCount = static_cast<uint32_t>(subpass.inputAttachments.size());
		subpassInfo.pInputAttachments = subpass.inputAttachments.data();
		subpassInfo.preserveAttachmentCount = static_cast<uint32_t>(subpass.preserveAttachments.size());
		subpassInfo.pPreserveAttachments = subpass.preserveAttachments.data();

		subpasses.push_back(subpassInfo);
	}
	renderPassInfo.pSubpasses = subpasses.data();
	renderPassInfo.dependencyCount = static_cast<uint32_t>(builder.m_dependencies.size());
	renderPassInfo.pDependencies = builder.m_dependencies.data();
	renderPassInfo.flags = flags;

	VkRenderPass renderPass;
	if (const VkResult ret = vkCreateRenderPass(m_VkHandle, &renderPassInfo, nullptr, &renderPass); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create render pass, error: ") + string_VkResult(ret));
	}

    VulkanRenderPass* l_NewComp = new VulkanRenderPass{ m_ID, renderPass };
    m_Subresources[l_NewComp->getID()] = l_NewComp;
	Logger::print("Created renderpass (ID: " + std::to_string(l_NewComp->getID()) + ") with " + std::to_string(builder.m_attachments.size()) + " attachment(s) and " + std::to_string(builder.m_subpasses.size()) + " subpass(es)", Logger::DEBUG);

	return l_NewComp->getID();
}

ResourceID VulkanDevice::createPipelineLayout(const std::vector<uint32_t>& descriptorSetLayouts, const std::vector<VkPushConstantRange>& pushConstantRanges)
{
	std::vector<VkDescriptorSetLayout> layouts;
	for (const uint32_t id : descriptorSetLayouts)
	{
		layouts.push_back(getDescriptorSetLayout(id).m_vkHandle);
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

	VkPipelineLayout layout;
	if (const VkResult ret = vkCreatePipelineLayout(m_VkHandle, &pipelineLayoutInfo, nullptr, &layout); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create pipeline layout, error: ") + string_VkResult(ret));
	}

    VulkanPipelineLayout* l_NewComp = new VulkanPipelineLayout{ m_ID, layout };
    m_Subresources[l_NewComp->getID()] = l_NewComp;
	Logger::print("Created pipeline layout (ID: " + std::to_string(l_NewComp->getID()) + ")", Logger::DEBUG);
	return l_NewComp->getID();
}

ResourceID VulkanDevice::createDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, const uint32_t maxSets, const VkDescriptorPoolCreateFlags flags)
{
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = flags;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = maxSets;

	VkDescriptorPool descriptorPool;
	if (const VkResult ret = vkCreateDescriptorPool(m_VkHandle, &poolInfo, nullptr, &descriptorPool); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create descriptor pool, error: ") + string_VkResult(ret));
	}

    VulkanDescriptorPool* l_NewComp = new VulkanDescriptorPool{ m_ID, descriptorPool, flags };
    m_Subresources[l_NewComp->getID()] = l_NewComp;
	Logger::print("Created descriptor pool (ID: " + std::to_string(l_NewComp->getID()) + ")", Logger::DEBUG);
	return l_NewComp->getID();
}

ResourceID VulkanDevice::createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, const VkDescriptorSetLayoutCreateFlags flags)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.flags = flags;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VkDescriptorSetLayout descriptorSetLayout;
	if (const VkResult ret = vkCreateDescriptorSetLayout(m_VkHandle, &layoutInfo, nullptr, &descriptorSetLayout); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create descriptor set layout, error: ") + string_VkResult(ret));
	}

    VulkanDescriptorSetLayout* l_NewComp = new VulkanDescriptorSetLayout{ m_ID, descriptorSetLayout };
    m_Subresources[l_NewComp->getID()] = l_NewComp;
	Logger::print("Created descriptor set layout (ID: " + std::to_string(l_NewComp->getID()) + ")", Logger::DEBUG);
	return l_NewComp->getID();

}

ResourceID VulkanDevice::createDescriptorSet(const uint32_t pool, const uint32_t layout)
{
	const VkDescriptorSetLayout descriptorSetLayout = getDescriptorSetLayout(layout).m_vkHandle;
	const VkDescriptorPool descriptorPool = getDescriptorPool(pool).m_vkHandle;

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	VkDescriptorSet descriptorSet;
	if (const VkResult ret = vkAllocateDescriptorSets(m_VkHandle, &allocInfo, &descriptorSet); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to allocate descriptor set, error: ") + string_VkResult(ret));
	}

    VulkanDescriptorSet* l_NewComp = new VulkanDescriptorSet{ m_ID, pool, descriptorSet };
    m_Subresources[l_NewComp->getID()] = l_NewComp;
	Logger::print("Created descriptor set (ID: " + std::to_string(l_NewComp->getID()) + ")", Logger::DEBUG);
	return l_NewComp->getID();
}

std::vector<ResourceID> VulkanDevice::createDescriptorSets(const uint32_t pool, const uint32_t layout, const uint32_t count)
{
	const VkDescriptorSetLayout descriptorSetLayout = getDescriptorSetLayout(layout).m_vkHandle;
	const VkDescriptorPool descriptorPool = getDescriptorPool(pool).m_vkHandle;

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	std::vector<VkDescriptorSet> descriptorSets(count);
	if (const VkResult ret = vkAllocateDescriptorSets(m_VkHandle, &allocInfo, descriptorSets.data()); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to allocate descriptor sets, error: ") + string_VkResult(ret));
	}

	std::vector<uint32_t> ids;
	for (const VkDescriptorSet descriptorSet : descriptorSets)
	{
        VulkanDescriptorSet* l_NewComp = new VulkanDescriptorSet{ m_ID, pool, descriptorSet };
        m_Subresources[l_NewComp->getID()] = l_NewComp;
		ids.push_back(l_NewComp->getID());
		Logger::print("Created descriptor set (ID: " + std::to_string(l_NewComp->getID()) + ") in batch", Logger::DEBUG);
	}
	return ids;
}

void VulkanDevice::updateDescriptorSets(const std::vector<VkWriteDescriptorSet>& descriptorWrites) const
{
    Logger::print("Updating " + std::to_string(descriptorWrites.size()) + " descriptor sets directly from device (ID: " + std::to_string(m_ID) + ")", Logger::DEBUG);
    vkUpdateDescriptorSets(m_VkHandle, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

ResourceID VulkanDevice::createSwapchain(const VkSurfaceKHR surface, const VkExtent2D extent, const VkSurfaceFormatKHR desiredFormat, const uint32_t oldSwapchain)
{
	const VkSurfaceFormatKHR selectedFormat = m_PhysicalDevice.getClosestFormat(surface, desiredFormat);

	const VkSurfaceCapabilitiesKHR capabilities = m_PhysicalDevice.getCapabilities(surface);

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && createInfo.minImageCount > capabilities.maxImageCount)
		createInfo.minImageCount = capabilities.maxImageCount;
	createInfo.imageFormat = selectedFormat.format;
	createInfo.imageColorSpace = selectedFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	createInfo.clipped = VK_TRUE;
	if (oldSwapchain != UINT32_MAX)
		createInfo.oldSwapchain = *getSwapchain(oldSwapchain);

	VkSwapchainKHR swapchainHandle;
	if (const VkResult ret = vkCreateSwapchainKHR(m_VkHandle, &createInfo, nullptr, &swapchainHandle); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("failed to create swap chain, error: ") + string_VkResult(ret));
	}

	if (oldSwapchain != UINT32_MAX)
		freeSwapchain(oldSwapchain);

    VulkanSwapchain* l_NewComp = new VulkanSwapchain{ m_ID, swapchainHandle, extent, selectedFormat, createInfo.minImageCount };
    m_Subresources[l_NewComp->getID()] = l_NewComp;
	Logger::print("Created swapchain (ID: " + std::to_string(l_NewComp->getID()) + ")", Logger::DEBUG);
	return l_NewComp->getID();
}

ResourceID VulkanDevice::createShader(const std::string& filename, const VkShaderStageFlagBits stage, const bool getReflection, const std::vector<VulkanShader::MacroDef>& macros)
{
	const VulkanShader::Result result = VulkanShader::compileFile(filename, VulkanShader::getKindFromStage(stage), VulkanShader::readFile(filename),
#ifdef _DEBUG
		false, macros);
#else
		true, macros);
#endif
	if (result.code.empty())
	{
		Logger::print("Failed to load shader: " + result.error, Logger::LevelBits::ERR);
		throw std::runtime_error("Failed to create shader module");
	}

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = 4 * result.code.size();
	createInfo.pCode = result.code.data();

	VkShaderModule shader;
	if (vkCreateShaderModule(m_VkHandle, &createInfo, nullptr, &shader) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}

    VulkanShader* l_NewComp = new VulkanShader{ m_ID, shader, stage };
    m_Subresources[l_NewComp->getID()] = l_NewComp;
    if (getReflection)
	    l_NewComp->reflect(result.code);
	Logger::print("Created shader (ID: " + std::to_string(l_NewComp->getID()) + ") and stage " + string_VkShaderStageFlagBits(stage), Logger::DEBUG);
	return l_NewComp->getID();
}

bool VulkanDevice::freeAllShaders()
{
    uint32_t l_Count = 0;
    for (VulkanDeviceSubresource* const l_Component : m_Subresources | std::views::values)
    {
        if (const VulkanShader* l_Shader = dynamic_cast<VulkanShader*>(l_Component))
        {
            freeSubresource(l_Shader->getID());
            l_Count++;
        }
    }
    Logger::print("Freed " + std::to_string(l_Count) + " shaders", Logger::DEBUG);
    return l_Count > 0;
}

ResourceID VulkanDevice::createSemaphore()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore semaphore;
	if (const VkResult ret = vkCreateSemaphore(m_VkHandle, &semaphoreInfo, nullptr, &semaphore); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create semaphore, error: ") + string_VkResult(ret));
	}

    VulkanSemaphore* l_NewComp = new VulkanSemaphore{ m_ID, semaphore };
    m_Subresources[l_NewComp->getID()] = l_NewComp;
	Logger::print("Created semaphore (ID: " + std::to_string(l_NewComp->getID()) + ")", Logger::DEBUG);
	return l_NewComp->getID();
}

ResourceID VulkanDevice::createFence(const bool signaled)
{
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	VkFence fence;
	if (const VkResult ret = vkCreateFence(m_VkHandle, &fenceInfo, nullptr, &fence); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create fence, error: ") + string_VkResult(ret));
	}

    VulkanFence* l_NewComp = new VulkanFence{ m_ID, fence, signaled };
    m_Subresources[l_NewComp->getID()] = l_NewComp;
	Logger::print("Created fence (ID: " + std::to_string(l_NewComp->getID()) + ")", Logger::DEBUG);
	return l_NewComp->getID();
}

void VulkanDevice::waitIdle() const
{
	vkDeviceWaitIdle(m_VkHandle);
}

bool VulkanDevice::isStagingBufferConfigured() const
{
	return m_stagingBufferInfo.stagingBuffer != UINT32_MAX;
}

ResourceID VulkanDevice::createPipeline(const VulkanPipelineBuilder& builder, const uint32_t pipelineLayout, const uint32_t renderPass, const uint32_t subpass)
{
	const std::vector<VkPipelineShaderStageCreateInfo> shaderModules = builder.createShaderStages();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderModules.size());
	pipelineInfo.pStages = shaderModules.data();
	pipelineInfo.pVertexInputState = &builder.m_vertexInputState;
	pipelineInfo.pInputAssemblyState = &builder.m_inputAssemblyState;
	pipelineInfo.pViewportState = &builder.m_viewportState;
	pipelineInfo.pRasterizationState = &builder.m_rasterizationState;
	pipelineInfo.pMultisampleState = &builder.m_multisampleState;
	pipelineInfo.pDepthStencilState = &builder.m_depthStencilState;
	pipelineInfo.pColorBlendState = &builder.m_colorBlendState;
	pipelineInfo.pDynamicState = &builder.m_dynamicState;
	pipelineInfo.layout = getPipelineLayout(pipelineLayout).m_vkHandle;
	pipelineInfo.renderPass = getRenderPass(renderPass).m_vkHandle;
	pipelineInfo.subpass = subpass;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline pipeline;
	if (const VkResult ret = vkCreateGraphicsPipelines(m_VkHandle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create graphics pipeline, error: ") + string_VkResult(ret));
	}

    VulkanPipeline* l_NewComp = new VulkanPipeline{ m_ID, pipeline, pipelineLayout, renderPass, subpass };
    m_Subresources[l_NewComp->getID()] = l_NewComp;
	Logger::print("Created pipeline (ID: " + std::to_string(l_NewComp->getID()) + ")", Logger::DEBUG);
	return l_NewComp->getID();
}

bool VulkanDevice::free()
{
	for (const auto& commandBuffers : m_CommandBuffers | std::views::values)
		for (const VulkanCommandBuffer& buffer : commandBuffers)
			vkFreeCommandBuffers(m_VkHandle, m_ThreadCommandInfos[buffer.m_threadID].commandPools[buffer.m_familyIndex].pool, 1, &buffer.m_vkHandle);

	for (const ThreadCommandInfo& threadInfo : m_ThreadCommandInfos | std::views::values)
	{
		for (const ThreadCommandInfo::CommandPoolInfo& commandPoolInfo : threadInfo.commandPools | std::views::values)
		{
			if (commandPoolInfo.pool != VK_NULL_HANDLE)
				vkDestroyCommandPool(m_VkHandle, commandPoolInfo.pool, nullptr);

			if (commandPoolInfo.secondaryPool != VK_NULL_HANDLE)
				vkDestroyCommandPool(m_VkHandle, commandPoolInfo.secondaryPool, nullptr);
		}
        if (threadInfo.oneTimePool != VK_NULL_HANDLE)
            vkDestroyCommandPool(m_VkHandle, threadInfo.oneTimePool, nullptr);
	}

	m_ThreadCommandInfos.clear();

    std::vector<ResourceID> l_Subresources;
    for (const auto& l_Component : m_Subresources | std::views::values)
    {
        l_Subresources.push_back(l_Component->getID());
    }
    for (const ResourceID l_ID : l_Subresources)
    {
        freeSubresource(l_ID);
    }

    if (!m_VkHandle)
        return false;
	vkDestroyDevice(m_VkHandle, nullptr);
	Logger::print("Freed device (ID: " + std::to_string(m_ID) + ")", Logger::DEBUG);
	m_VkHandle = VK_NULL_HANDLE;
    return true;
}

VkDeviceMemory VulkanDevice::getMemoryHandle(const uint32_t chunkID) const
{
	for (const auto& chunk : m_MemoryAllocator.m_memoryChunks)
	{
		if (chunk.getID() == chunkID)
		{
			return chunk.m_memory;
		}
	}
	Logger::print("Memory chunk search failed out of " + std::to_string(m_MemoryAllocator.m_memoryChunks.size()) + " memory chunks", Logger::DEBUG);
	throw std::runtime_error("Memory chunk (ID: " + std::to_string(chunkID) + ") not found");
}

VkCommandPool VulkanDevice::getCommandPool(const uint32_t uint32, const uint32_t m_thread_id, const VulkanCommandBuffer::TypeFlags flags)
{
	if ((flags & VulkanCommandBuffer::TypeFlagBits::ONE_TIME) != 0)
		return m_ThreadCommandInfos[m_thread_id].oneTimePool;
	if ((flags & VulkanCommandBuffer::TypeFlagBits::SECONDARY) != 0)
		return m_ThreadCommandInfos[m_thread_id].commandPools[uint32].secondaryPool;
    return m_ThreadCommandInfos[m_thread_id].commandPools[uint32].pool;
}

VulkanDevice::VulkanDevice(const VulkanGPU pDevice, const VkDevice device)
	: m_VkHandle(device), m_PhysicalDevice(pDevice), m_MemoryAllocator(*this)
{

}
