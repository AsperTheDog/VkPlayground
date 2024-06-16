#include "vulkan_device.hpp"

#include <ranges>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_context.hpp"
#include "utils/logger.hpp"

VulkanQueue VulkanDevice::getQueue(const QueueSelection& queueSelection) const
{
	VkQueue queue;
	vkGetDeviceQueue(m_vkHandle, queueSelection.familyIndex, queueSelection.queueIndex, &queue);
	return VulkanQueue(queue);
}

VulkanGPU VulkanDevice::getGPU() const
{
	return m_physicalDevice;
}

const VulkanMemoryAllocator& VulkanDevice::getMemoryAllocator() const
{
	return m_memoryAllocator;
}

VkDevice VulkanDevice::operator*() const
{
	return m_vkHandle;
}

void VulkanDevice::configureOneTimeQueue(const QueueSelection queue)
{
	m_oneTimeQueue = queue;
}

void VulkanDevice::initializeOneTimeCommandPool(const uint32_t threadID)
{
	if (!m_threadCommandInfos.contains(threadID))
	{
		m_threadCommandInfos[threadID] = {};
	}
	ThreadCommandInfo& threadInfo = m_threadCommandInfos[threadID];

	if (threadInfo.oneTimePool != VK_NULL_HANDLE) return;

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_oneTimeQueue.familyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (const VkResult ret = vkCreateCommandPool(m_vkHandle, &poolInfo, nullptr, &threadInfo.oneTimePool); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create command pool, error: ") + string_VkResult(ret));
	}
}

void VulkanDevice::initializeCommandPool(const QueueFamily& family, const uint32_t threadID, const bool createSecondary)
{
	if (!m_threadCommandInfos.contains(threadID))
	{
		m_threadCommandInfos[threadID] = {};
	}

	ThreadCommandInfo& threadInfo = m_threadCommandInfos[threadID];
	if (!threadInfo.commandPools.contains(family.index))
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = family.index;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (const VkResult ret = vkCreateCommandPool(m_vkHandle, &poolInfo, nullptr, &threadInfo.commandPools[family.index].pool); ret != VK_SUCCESS)
		{
			throw std::runtime_error(std::string("Failed to create main command pool for thread " + std::to_string(threadID) + ", error: ") + string_VkResult(ret));
		}
		Logger::print("Created main command pool for thread " + std::to_string(threadID) + " and family " + std::to_string(family.index), Logger::DEBUG);

		if (createSecondary)
		{
			if (const VkResult ret = vkCreateCommandPool(m_vkHandle, &poolInfo, nullptr, &threadInfo.commandPools[family.index].secondaryPool); ret != VK_SUCCESS)
			{
				throw std::runtime_error(std::string("Failed to create secondary command pool for thread " + std::to_string(threadID) + ", error: ") + string_VkResult(ret));
			}
			Logger::print("Created secondary command pool for thread " + std::to_string(threadID) + " and family " + std::to_string(family.index), Logger::DEBUG);
		}
	}
}

uint32_t VulkanDevice::createCommandBuffer(const QueueFamily& family, const uint32_t threadID, const bool isSecondary)
{
	initializeCommandPool(family, threadID, isSecondary);

    VulkanCommandBuffer::TypeFlags type = 0;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	if (isSecondary)
	{
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandPool = m_threadCommandInfos[threadID].commandPools[family.index].secondaryPool;
        type = VulkanCommandBuffer::TypeFlagBits::SECONDARY;
	}
	else
	{
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_threadCommandInfos[threadID].commandPools[family.index].pool;
	}
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	if (const VkResult ret = vkAllocateCommandBuffers(m_vkHandle, &allocInfo, &commandBuffer); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to allocate command buffer in thread + " + std::to_string(threadID) + ", error:") + string_VkResult(ret));
	}
	Logger::print("Allocated command buffer for thread " + std::to_string(threadID) + " and family " + std::to_string(family.index), Logger::DEBUG);

	if (!m_commandBuffers.contains(threadID))
	{
		m_commandBuffers[threadID] = {};
	}

	m_commandBuffers[threadID].push_back({ m_id, commandBuffer, type, family.index, threadID });
	return m_commandBuffers[threadID].back().getID();
}

uint32_t VulkanDevice::createOneTimeCommandBuffer(uint32_t threadID)
{
	initializeOneTimeCommandPool(threadID);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_threadCommandInfos[threadID].oneTimePool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	if (const VkResult ret = vkAllocateCommandBuffers(m_vkHandle, &allocInfo, &commandBuffer); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to allocate one time command buffer in thread + " + std::to_string(threadID) + ", error:") + string_VkResult(ret));
	}
	Logger::print("Allocated one time command buffer for thread " + std::to_string(threadID), Logger::DEBUG);

	if (!m_commandBuffers.contains(threadID))
	{
		m_commandBuffers[threadID] = {};
	}

	m_commandBuffers[threadID].push_back({ m_id, commandBuffer, VulkanCommandBuffer::TypeFlagBits::ONE_TIME, m_oneTimeQueue.familyIndex, threadID });
	return m_commandBuffers[threadID].back().getID();
}

uint32_t VulkanDevice::getOrCreateCommandBuffer(const QueueFamily& family, const uint32_t threadID, const VulkanCommandBuffer::TypeFlags flags)
{
	for (const VulkanCommandBuffer& buffer : m_commandBuffers[threadID])
		if (buffer.m_familyIndex == family.index && buffer.m_threadID == threadID && buffer.m_flags == flags)
		{
			Logger::print("Reusing command buffer for thread " + std::to_string(threadID) + " and family " + std::to_string(family.index), Logger::DEBUG);
			return buffer.getID();
		}

	return createCommandBuffer(family, threadID, (flags & VulkanCommandBuffer::TypeFlagBits::SECONDARY) != 0);
}

VulkanCommandBuffer& VulkanDevice::getCommandBuffer(const uint32_t id, const uint32_t threadID)
{
	for (VulkanCommandBuffer& buffer : m_commandBuffers[threadID])
		if (buffer.m_id == id)
			return buffer;

	Logger::print("Command buffer search failed out of " + std::to_string(m_commandBuffers.size()) + " command buffers", Logger::DEBUG);
	throw std::runtime_error("Command buffer (ID:" + std::to_string(id) + ") not found");
}

const VulkanCommandBuffer& VulkanDevice::getCommandBuffer(const uint32_t id, const uint32_t threadID) const
{
	return const_cast<VulkanDevice*>(this)->getCommandBuffer(id, threadID);
}

void VulkanDevice::freeCommandBuffer(const VulkanCommandBuffer& commandBuffer, const uint32_t threadID)
{
	freeCommandBuffer(commandBuffer.m_id, threadID);
}

void VulkanDevice::freeCommandBuffer(const uint32_t id, const uint32_t threadID)
{
	for (auto it = m_commandBuffers[threadID].begin(); it != m_commandBuffers[threadID].end(); ++it)
	{
		if (it->m_id == id)
		{
			it->free();
			m_commandBuffers[threadID].erase(it);
			break;
		}
	}
}

uint32_t VulkanDevice::createFramebuffer(const VkExtent3D size, const VulkanRenderPass& renderPass, const std::vector<VkImageView>& attachments)
{
	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass.m_vkHandle;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = size.width;
	framebufferInfo.height = size.height;
	framebufferInfo.layers = size.depth;

	VkFramebuffer framebuffer;
	if (const VkResult ret = vkCreateFramebuffer(m_vkHandle, &framebufferInfo, nullptr, &framebuffer); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create framebuffer, error: ") + string_VkResult(ret));
	}

	m_framebuffers.push_back({ m_id, framebuffer });
	Logger::print("Created framebuffer (ID:" + std::to_string(m_framebuffers.back().getID()) + ")", Logger::DEBUG);
	return m_framebuffers.back().getID();
}

VulkanFramebuffer& VulkanDevice::getFramebuffer(const uint32_t id)
{
	for (auto& framebuffer : m_framebuffers)
	{
		if (framebuffer.getID() == id)
		{
			return framebuffer;
		}
	}

	Logger::print("Framebuffer search failed out of " + std::to_string(m_framebuffers.size()) + " framebuffers", Logger::DEBUG);
	throw std::runtime_error("Framebuffer (ID:" + std::to_string(id) + ") not found");
}

const VulkanFramebuffer& VulkanDevice::getFramebuffer(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getFramebuffer(id);
}

void VulkanDevice::freeFramebuffer(const uint32_t id)
{
	for (auto it = m_framebuffers.begin(); it != m_framebuffers.end(); ++it)
	{
		if (it->getID() == id)
		{
			it->free();
			m_framebuffers.erase(it);
			break;
		}
	}
}

void VulkanDevice::freeFramebuffer(const VulkanFramebuffer& framebuffer)
{
	freeFramebuffer(framebuffer.getID());
}

uint32_t VulkanDevice::createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage)
{

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.flags = 0;

	VkBuffer buffer;
	if (const VkResult ret = vkCreateBuffer(m_vkHandle, &bufferInfo, nullptr, &buffer); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create buffer, error: ") + string_VkResult(ret));
	}

	m_buffers.push_back({ m_id, buffer, size });
	Logger::print("Created buffer (ID:" + std::to_string(m_buffers.back().getID()) + ") with size " + VulkanMemoryAllocator::compactBytes(m_buffers.back().getSize()), Logger::DEBUG);
	return m_buffers.back().getID();
}

VulkanBuffer& VulkanDevice::getBuffer(const uint32_t id)
{
	for (auto& buffer : m_buffers)
	{
		if (buffer.m_id == id)
		{
			return buffer;
		}
	}

	Logger::print("Buffer search failed out of " + std::to_string(m_buffers.size()) + " buffers", Logger::DEBUG);
	throw std::runtime_error("Buffer (ID:" + std::to_string(id) + ") not found");
}

const VulkanBuffer& VulkanDevice::getBuffer(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getBuffer(id);
}

void VulkanDevice::freeBuffer(const uint32_t id)
{
	for (auto it = m_buffers.begin(); it != m_buffers.end(); ++it)
	{
		if (it->m_id == id)
		{
			it->free();
			m_buffers.erase(it);
			break;
		}
	}
}

void VulkanDevice::freeBuffer(const VulkanBuffer& buffer)
{
	freeBuffer(buffer.m_id);
}

uint32_t VulkanDevice::createImage(const VkImageType type, const VkFormat format, const VkExtent3D extent, const VkImageUsageFlags usage, const VkImageCreateFlags flags)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = type;
	imageInfo.format = format;
	imageInfo.extent = extent;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = usage;
	imageInfo.flags = flags;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices = nullptr;

	VkImage image;
	if (const VkResult ret = vkCreateImage(m_vkHandle, &imageInfo, nullptr, &image); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create image, error: ") + string_VkResult(ret));
	}

	m_images.push_back({ m_id, image, extent, type, VK_IMAGE_LAYOUT_UNDEFINED });
	Logger::print("Created image (ID:" + std::to_string(m_images.back().getID()) + ")", Logger::DEBUG);
	return m_images.back().getID();
}

VulkanImage& VulkanDevice::getImage(const uint32_t id)
{
	for (VulkanImage& image : m_images)
	{
		if (image.m_id == id)
		{
			return image;
		}
	}

	Logger::print("Image search failed out of " + std::to_string(m_images.size()) + " images", Logger::DEBUG);
	throw std::runtime_error("Image (ID:" + std::to_string(id) + ") not found");
}

const VulkanImage& VulkanDevice::getImage(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getImage(id);
}

void VulkanDevice::freeImage(const uint32_t id)
{
	for (auto it = m_images.begin(); it != m_images.end(); ++it)
	{
		if (it->m_id == id)
		{
			it->free();
			m_images.erase(it);
			break;
		}
	}
}

void VulkanDevice::freeImage(const VulkanImage& image)
{
	freeImage(image.m_id);
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
	const std::optional<uint32_t> memoryType = m_memoryAllocator.getMemoryStructure().getStagingMemoryType(memRequirements.memoryTypeBits);
	if (memoryType.has_value() && !m_memoryAllocator.isMemoryTypeHidden(memoryType.value()))
	{
		const uint32_t heapIndex = m_physicalDevice.getMemoryProperties().memoryTypes[memoryType.value()].heapIndex;
		const VkDeviceSize heapSize = m_physicalDevice.getMemoryProperties().memoryHeaps[heapIndex].size;
		if (heapSize < size * 0.8)
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
			    m_memoryAllocator.hideMemoryType(memoryType.value());
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

void VulkanDevice::freeStagingBuffer()
{
	if (m_stagingBufferInfo.stagingBuffer != UINT32_MAX)
	{
		{
			VulkanBuffer& stagingBuffer = getBuffer(m_stagingBufferInfo.stagingBuffer);
			if (stagingBuffer.isMemoryBound())
				m_memoryAllocator.unhideMemoryType(stagingBuffer.getBoundMemoryType());
			stagingBuffer.free();
		}
		m_stagingBufferInfo.stagingBuffer = UINT32_MAX;
	}
}

void* VulkanDevice::mapStagingBuffer(const VkDeviceSize size, const VkDeviceSize offset)
{
	return getBuffer(m_stagingBufferInfo.stagingBuffer).map(size, offset);
}

void VulkanDevice::unmapStagingBuffer()
{
	getBuffer(m_stagingBufferInfo.stagingBuffer).unmap();
}

void VulkanDevice::dumpStagingBuffer(const uint32_t buffer, const VkDeviceSize size, const VkDeviceSize offset, const uint32_t threadID)
{
	dumpStagingBuffer(buffer, { {0, offset, size} }, threadID);
}

void VulkanDevice::dumpStagingBuffer(const uint32_t buffer, const std::vector<VkBufferCopy>& regions, const uint32_t threadID)
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
	m_memoryAllocator.hideMemoryType(type);
}

void VulkanDevice::allowMemoryType(const uint32_t type)
{
	m_memoryAllocator.unhideMemoryType(type);
}

uint32_t VulkanDevice::createRenderPass(const VulkanRenderPassBuilder& builder, const VkRenderPassCreateFlags flags)
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
	if (const VkResult ret = vkCreateRenderPass(m_vkHandle, &renderPassInfo, nullptr, &renderPass); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create render pass, error: ") + string_VkResult(ret));
	}

	m_renderPasses.push_back({ m_id, renderPass });
	Logger::print("Created renderpass (ID: " + std::to_string(m_renderPasses.back().getID()) + ") with " + std::to_string(builder.m_attachments.size()) + " attachment(s) and " + std::to_string(builder.m_subpasses.size()) + " subpass(es)", Logger::DEBUG);

	return m_renderPasses.back().getID();
}

VulkanRenderPass& VulkanDevice::getRenderPass(const uint32_t id)
{
	for (auto& renderPass : m_renderPasses)
	{
		if (renderPass.m_id == id)
		{
			return renderPass;
		}
	}
	Logger::print("Render pass search failed out of " + std::to_string(m_renderPasses.size()) + " render passes", Logger::DEBUG);
	throw std::runtime_error("Render pass (ID: " + std::to_string(id) + ") not found");
}

const VulkanRenderPass& VulkanDevice::getRenderPass(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getRenderPass(id);
}

void VulkanDevice::freeRenderPass(const uint32_t id)
{
	for (auto it = m_renderPasses.begin(); it != m_renderPasses.end(); ++it)
	{
		if (it->m_id == id)
		{
			it->free();
			m_renderPasses.erase(it);
			break;
		}
	}
}

void VulkanDevice::freeRenderPass(const VulkanRenderPass& renderPass)
{
	freeRenderPass(renderPass.m_id);
}

uint32_t VulkanDevice::createPipelineLayout(const std::vector<uint32_t>& descriptorSetLayouts, const std::vector<VkPushConstantRange>& pushConstantRanges)
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
	if (const VkResult ret = vkCreatePipelineLayout(m_vkHandle, &pipelineLayoutInfo, nullptr, &layout); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create pipeline layout, error: ") + string_VkResult(ret));
	}

	m_pipelineLayouts.push_back({ m_id, layout });
	Logger::print("Created pipeline layout (ID: " + std::to_string(m_pipelineLayouts.back().getID()) + ")", Logger::DEBUG);
	return m_pipelineLayouts.back().getID();
}

VulkanPipelineLayout& VulkanDevice::getPipelineLayout(const uint32_t id)
{
	for (VulkanPipelineLayout& layout : m_pipelineLayouts)
	{
		if (layout.m_id == id)
		{
			return layout;
		}
	}
	Logger::print("Pipeline layout search failed out of " + std::to_string(m_pipelineLayouts.size()) + " pipeline layouts", Logger::DEBUG);
	throw std::runtime_error("Pipeline layout (ID: " + std::to_string(id) + ") not found");
}

const VulkanPipelineLayout& VulkanDevice::getPipelineLayout(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getPipelineLayout(id);
}

void VulkanDevice::freePipelineLayout(const uint32_t id)
{
	for (auto it = m_pipelineLayouts.begin(); it != m_pipelineLayouts.end(); ++it)
	{
		if (it->m_id == id)
		{
			it->free();
			m_pipelineLayouts.erase(it);
			break;
		}
	}
}

void VulkanDevice::freePipelineLayout(const VulkanPipelineLayout& layout)
{
	freePipelineLayout(layout.m_id);
}

VulkanPipeline& VulkanDevice::getPipeline(const uint32_t id)
{
	for (VulkanPipeline& pipeline : m_pipelines)
	{
		if (pipeline.m_id == id)
		{
			return pipeline;
		}
	}
	Logger::print("Pipeline search failed out of " + std::to_string(m_pipelines.size()) + " pipelines", Logger::DEBUG);
	throw std::runtime_error("Pipeline (ID: " + std::to_string(id) + ") not found");
}

void VulkanDevice::freePipeline(const uint32_t id)
{
	for (auto it = m_pipelines.begin(); it != m_pipelines.end(); ++it)
	{
		if (it->m_id == id)
		{
			it->free();
			m_pipelines.erase(it);
			break;
		}
	}
}

void VulkanDevice::freePipeline(const VulkanPipeline& pipeline)
{
	freePipeline(pipeline.m_id);
}

uint32_t VulkanDevice::createDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, const uint32_t maxSets, const VkDescriptorPoolCreateFlags flags)
{
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = flags;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = maxSets;

	VkDescriptorPool descriptorPool;
	if (const VkResult ret = vkCreateDescriptorPool(m_vkHandle, &poolInfo, nullptr, &descriptorPool); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create descriptor pool, error: ") + string_VkResult(ret));
	}

	m_descriptorPools.push_back({ m_id, descriptorPool, flags });
	Logger::print("Created descriptor pool (ID: " + std::to_string(m_descriptorPools.back().getID()) + ")", Logger::DEBUG);
	return m_descriptorPools.back().getID();
}

VulkanDescriptorPool& VulkanDevice::getDescriptorPool(const uint32_t id)
{
	for (VulkanDescriptorPool& descriptorPool : m_descriptorPools)
	{
		if (descriptorPool.getID() == id)
		{
			return descriptorPool;
		}
	}
	Logger::print("Descriptor pool search failed out of " + std::to_string(m_descriptorPools.size()) + " descriptor pools", Logger::DEBUG);
	throw std::runtime_error("Descriptor pool (ID: " + std::to_string(id) + ") not found");
}

void VulkanDevice::freeDescriptorPool(const uint32_t id)
{
	for (auto it = m_descriptorPools.begin(); it != m_descriptorPools.end(); ++it)
	{
		if (it->getID() == id)
		{
			it->free();
			m_descriptorPools.erase(it);
			break;
		}
	}
}

void VulkanDevice::freeDescriptorPool(const VulkanDescriptorPool& descriptorPool)
{
	freeDescriptorPool(descriptorPool.getID());
}

uint32_t VulkanDevice::createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, const VkDescriptorSetLayoutCreateFlags flags)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.flags = flags;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VkDescriptorSetLayout descriptorSetLayout;
	if (const VkResult ret = vkCreateDescriptorSetLayout(m_vkHandle, &layoutInfo, nullptr, &descriptorSetLayout); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create descriptor set layout, error: ") + string_VkResult(ret));
	}

	m_descriptorSetLayouts.push_back({ m_id, descriptorSetLayout });
	Logger::print("Created descriptor set layout (ID: " + std::to_string(m_descriptorSetLayouts.back().getID()) + ")", Logger::DEBUG);
	return m_descriptorSetLayouts.back().getID();

}

VulkanDescriptorSetLayout& VulkanDevice::getDescriptorSetLayout(const uint32_t id)
{
	for (VulkanDescriptorSetLayout& layout : m_descriptorSetLayouts)
	{
		if (layout.m_id == id)
		{
			return layout;
		}
	}
	Logger::print("Descriptor set layout search failed out of " + std::to_string(m_descriptorSetLayouts.size()) + " descriptor set layouts", Logger::DEBUG);
	throw std::runtime_error("Descriptor set layout (ID: " + std::to_string(id) + ")  not found");
}

void VulkanDevice::freeDescriptorSetLayout(const uint32_t id)
{
	for (auto it = m_descriptorSetLayouts.begin(); it != m_descriptorSetLayouts.end(); ++it)
	{
		if (it->m_id == id)
		{
			it->free();
			m_descriptorSetLayouts.erase(it);
			break;
		}
	}
}

void VulkanDevice::freeDescriptorSetLayout(const VulkanDescriptorSetLayout& layout)
{
	freeDescriptorSetLayout(layout.m_id);
}

uint32_t VulkanDevice::createDescriptorSet(const uint32_t pool, const uint32_t layout)
{
	const VkDescriptorSetLayout descriptorSetLayout = getDescriptorSetLayout(layout).m_vkHandle;
	const VkDescriptorPool descriptorPool = getDescriptorPool(pool).m_vkHandle;

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	VkDescriptorSet descriptorSet;
	if (const VkResult ret = vkAllocateDescriptorSets(m_vkHandle, &allocInfo, &descriptorSet); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to allocate descriptor set, error: ") + string_VkResult(ret));
	}

	m_descriptorSets.push_back({ m_id, pool, descriptorSet });
	Logger::print("Created descriptor set (ID: " + std::to_string(m_descriptorSets.back().getID()) + ")", Logger::DEBUG);
	return m_descriptorSets.back().getID();
}

std::vector<uint32_t> VulkanDevice::createDescriptorSets(const uint32_t pool, const uint32_t layout, const uint32_t count)
{
	const VkDescriptorSetLayout descriptorSetLayout = getDescriptorSetLayout(layout).m_vkHandle;
	const VkDescriptorPool descriptorPool = getDescriptorPool(pool).m_vkHandle;

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	std::vector<VkDescriptorSet> descriptorSets(count);
	if (const VkResult ret = vkAllocateDescriptorSets(m_vkHandle, &allocInfo, descriptorSets.data()); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to allocate descriptor sets, error: ") + string_VkResult(ret));
	}

	std::vector<uint32_t> ids;
	for (VkDescriptorSet descriptorSet : descriptorSets)
	{
		m_descriptorSets.push_back({ m_id, pool, descriptorSet });
		ids.push_back(m_descriptorSets.back().getID());
		Logger::print("Created descriptor set (ID: " + std::to_string(m_descriptorSets.back().getID()) + ") in batch", Logger::DEBUG);
	}
	return ids;
}

VulkanDescriptorSet& VulkanDevice::getDescriptorSet(const uint32_t id)
{
	for (VulkanDescriptorSet& descriptorSet : m_descriptorSets)
	{
		if (descriptorSet.m_id == id)
		{
			return descriptorSet;
		}
	}
	Logger::print("Descriptor set search failed out of " + std::to_string(m_descriptorSets.size()) + " descriptor sets", Logger::DEBUG);
	throw std::runtime_error("Descriptor set (ID: " + std::to_string(id) + ") not found");
}

void VulkanDevice::freeDescriptorSet(const uint32_t id)
{
	for (auto it = m_descriptorSets.begin(); it != m_descriptorSets.end(); ++it)
	{
		if (it->m_id == id)
		{
			vkFreeDescriptorSets(m_vkHandle, getDescriptorPool(it->getID()).m_vkHandle, 1, &it->m_vkHandle);
			m_descriptorSets.erase(it);
			break;
		}
	}
}

void VulkanDevice::freeDescriptorSet(const VulkanDescriptorSet& descriptorSet)
{
	freeDescriptorSet(descriptorSet.m_id);
}

void VulkanDevice::updateDescriptorSets(const std::vector<VkWriteDescriptorSet>& descriptorWrites) const
{
    Logger::print("Updating " + std::to_string(descriptorWrites.size()) + " descriptor sets directly from device (ID: " + std::to_string(m_id) + ")", Logger::DEBUG);
    vkUpdateDescriptorSets(m_vkHandle, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

uint32_t VulkanDevice::createSwapchain(const VkSurfaceKHR surface, const VkExtent2D extent, const VkSurfaceFormatKHR desiredFormat, const uint32_t oldSwapchain)
{
	const VkSurfaceFormatKHR selectedFormat = m_physicalDevice.getClosestFormat(surface, desiredFormat);

	const VkSurfaceCapabilitiesKHR capabilities = m_physicalDevice.getCapabilities(surface);

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
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	createInfo.clipped = VK_TRUE;
	if (oldSwapchain != UINT32_MAX)
		createInfo.oldSwapchain = *getSwapchain(oldSwapchain);

	VkSwapchainKHR swapchainHandle;
	if (const VkResult ret = vkCreateSwapchainKHR(m_vkHandle, &createInfo, nullptr, &swapchainHandle); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("failed to create swap chain, error: ") + string_VkResult(ret));
	}

	if (oldSwapchain != UINT32_MAX)
		freeSwapchain(oldSwapchain);

	m_swapchains.push_back({ swapchainHandle, m_id, extent, selectedFormat, createInfo.minImageCount });
	Logger::print("Created swapchain (ID: " + std::to_string(m_swapchains.back().getID()) + ")", Logger::DEBUG);
	return m_swapchains.back().getID();
}

VulkanSwapchain& VulkanDevice::getSwapchain(const uint32_t id)
{
	for (VulkanSwapchain& swapchain : m_swapchains)
	{
		if (swapchain.getID() == id)
		{
			return swapchain;
		}
	}
	Logger::print("Swapchain search failed out of " + std::to_string(m_swapchains.size()) + " swapchains", Logger::DEBUG);
	throw std::runtime_error("Swapchain (ID:" + std::to_string(id) + ") not found");
}

const VulkanSwapchain& VulkanDevice::getSwapchain(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getSwapchain(id);
}

void VulkanDevice::freeSwapchain(uint32_t id)
{
	for (auto it = m_swapchains.begin(); it != m_swapchains.end(); ++it)
	{
		if (it->getID() == id)
		{
			it->free();
			m_swapchains.erase(it);
			break;
		}
	}
}

void VulkanDevice::freeSwapchain(const VulkanSwapchain& swapchain)
{
	freeSwapchain(swapchain.getID());
}

VulkanSemaphore& VulkanDevice::getSemaphore(const uint32_t id)
{
	for (VulkanSemaphore& semaphore : m_semaphores)
	{
		if (semaphore.m_id == id)
		{
			return semaphore;
		}
	}
	Logger::print("Semaphore search failed out of " + std::to_string(m_semaphores.size()) + " semaphores", Logger::DEBUG);
	throw std::runtime_error("Semaphore (ID:" + std::to_string(id) + ") not found");
}

void VulkanDevice::freeSemaphore(const uint32_t id)
{
	for (auto it = m_semaphores.begin(); it != m_semaphores.end(); ++it)
	{
		if (it->m_id == id)
		{
			it->free();
			m_semaphores.erase(it);
			break;
		}
	}
}

void VulkanDevice::freeSemaphore(const VulkanSemaphore& semaphore)
{
	freeSemaphore(semaphore.m_id);
}

uint32_t VulkanDevice::createShader(const std::string& filename, const VkShaderStageFlagBits stage, const bool getReflection, const std::vector<VulkanShader::MacroDef>& macros)
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
	if (vkCreateShaderModule(m_vkHandle, &createInfo, nullptr, &shader) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}

    if (getReflection)
	    m_shaders.push_back({ m_id, shader, stage, result.code });
    else
	    m_shaders.push_back({ m_id, shader, stage });
	Logger::print("Created shader (ID: " + std::to_string(m_shaders.back().getID()) + ") and stage " + string_VkShaderStageFlagBits(stage), Logger::DEBUG);
	return m_shaders.back().getID();
}

VulkanShader& VulkanDevice::getShader(const uint32_t id)
{
	for (auto& shader : m_shaders)
	{
		if (shader.m_id == id)
		{
			return shader;
		}
	}
	Logger::print("Shader search failed out of " + std::to_string(m_shaders.size()) + " shaders", Logger::DEBUG);
	throw std::runtime_error("Shader (ID:" + std::to_string(id) + ") not found");
}

const VulkanShader& VulkanDevice::getShader(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getShader(id);
}

const VulkanPipeline& VulkanDevice::getPipeline(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getPipeline(id);
}

const VulkanDescriptorPool& VulkanDevice::getDescriptorPool(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getDescriptorPool(id);
}

const VulkanDescriptorSetLayout& VulkanDevice::getDescriptorSetLayout(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getDescriptorSetLayout(id);
}

const VulkanDescriptorSet& VulkanDevice::getDescriptorSet(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getDescriptorSet(id);
}

const VulkanSemaphore& VulkanDevice::getSemaphore(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getSemaphore(id);
}

const VulkanFence& VulkanDevice::getFence(const uint32_t id) const
{
	return const_cast<VulkanDevice*>(this)->getFence(id);
}

void VulkanDevice::freeShader(const uint32_t id)
{
	for (auto it = m_shaders.begin(); it != m_shaders.end(); ++it)
	{
		if (it->m_id == id)
		{
			it->free();
			m_shaders.erase(it);
			break;
		}
	}
}

void VulkanDevice::freeShader(const VulkanShader& shader)
{
	freeShader(shader.m_id);
}

void VulkanDevice::freeAllShaders()
{
	for (VulkanShader& shader : m_shaders)
		shader.free();
	m_shaders.clear();
}

uint32_t VulkanDevice::createSemaphore()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore semaphore;
	if (const VkResult ret = vkCreateSemaphore(m_vkHandle, &semaphoreInfo, nullptr, &semaphore); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create semaphore, error: ") + string_VkResult(ret));
	}

	m_semaphores.push_back({ m_id, semaphore });
	Logger::print("Created semaphore (ID: " + std::to_string(m_semaphores.back().getID()) + ")", Logger::DEBUG);
	return m_semaphores.back().getID();
}

uint32_t VulkanDevice::createFence(const bool signaled)
{
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	VkFence fence;
	if (const VkResult ret = vkCreateFence(m_vkHandle, &fenceInfo, nullptr, &fence); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create fence, error: ") + string_VkResult(ret));
	}

	m_fences.push_back({ m_id, fence, signaled });
	Logger::print("Created fence (ID: " + std::to_string(m_fences.back().getID()) + ")", Logger::DEBUG);
	return m_fences.back().getID();
}

VulkanFence& VulkanDevice::getFence(const uint32_t id)
{
	for (VulkanFence& fence : m_fences)
	{
		if (fence.m_id == id)
		{
			return fence;
		}
	}
	Logger::print("Fence search failed out of " + std::to_string(m_fences.size()) + " fences", Logger::DEBUG);
	throw std::runtime_error("Fence (ID:" + std::to_string(id) + ") not found");
}

void VulkanDevice::freeFence(const uint32_t id)
{
	for (auto it = m_fences.begin(); it != m_fences.end(); ++it)
	{
		if (it->m_id == id)
		{
			it->free();
			m_fences.erase(it);
			break;
		}
	}
}

void VulkanDevice::freeFence(const VulkanFence& fence)
{
	freeFence(fence.m_id);
}

void VulkanDevice::waitIdle() const
{
	vkDeviceWaitIdle(m_vkHandle);
}

bool VulkanDevice::isStagingBufferConfigured() const
{
	return m_stagingBufferInfo.stagingBuffer != UINT32_MAX;
}

uint32_t VulkanDevice::createPipeline(const VulkanPipelineBuilder& builder, const uint32_t pipelineLayout, const uint32_t renderPass, const uint32_t subpass)
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
	if (const VkResult ret = vkCreateGraphicsPipelines(m_vkHandle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create graphics pipeline, error: ") + string_VkResult(ret));
	}

	m_pipelines.push_back({ *this, pipeline, pipelineLayout, renderPass, subpass });
	Logger::print("Created pipeline (ID: " + std::to_string(m_pipelines.back().getID()) + ")", Logger::DEBUG);
	return m_pipelines.back().getID();
}

void VulkanDevice::free()
{
	for (const auto& commandBuffers : m_commandBuffers | std::views::values)
		for (const VulkanCommandBuffer& buffer : commandBuffers)
			vkFreeCommandBuffers(m_vkHandle, m_threadCommandInfos[buffer.m_threadID].commandPools[buffer.m_familyIndex].pool, 1, &buffer.m_vkHandle);

	for (const ThreadCommandInfo& threadInfo : m_threadCommandInfos | std::views::values)
	{
		for (const ThreadCommandInfo::CommandPoolInfo& commandPoolInfo : threadInfo.commandPools | std::views::values)
		{
			if (commandPoolInfo.pool != VK_NULL_HANDLE)
				vkDestroyCommandPool(m_vkHandle, commandPoolInfo.pool, nullptr);

			if (commandPoolInfo.secondaryPool != VK_NULL_HANDLE)
				vkDestroyCommandPool(m_vkHandle, commandPoolInfo.secondaryPool, nullptr);
		}
        if (threadInfo.oneTimePool != VK_NULL_HANDLE)
            vkDestroyCommandPool(m_vkHandle, threadInfo.oneTimePool, nullptr);
	}

	m_threadCommandInfos.clear();

	for (VulkanBuffer& buffer : m_buffers)
		buffer.free();
	m_buffers.clear();

	m_stagingBufferInfo = {};

	for (VulkanImage& image : m_images)
		image.free();
	m_images.clear();

	m_memoryAllocator.free();

	for (VulkanRenderPass& renderPass : m_renderPasses)
		renderPass.free();
	m_renderPasses.clear();

	for (VulkanPipelineLayout& pipelineLayout : m_pipelineLayouts)
		pipelineLayout.free();
	m_pipelineLayouts.clear();

	for (VulkanShader& shader : m_shaders)
		shader.free();
	m_shaders.clear();

	for (VulkanPipeline& pipeline : m_pipelines)
		pipeline.free();
	m_pipelines.clear();

	for (VulkanDescriptorSet& descriptorSet : m_descriptorSets)
		descriptorSet.free();
	m_descriptorSets.clear();

	for (VulkanDescriptorSetLayout& descriptorSetLayout : m_descriptorSetLayouts)
		descriptorSetLayout.free();
	m_descriptorSetLayouts.clear();

	for (VulkanDescriptorPool& descriptorPool : m_descriptorPools)
		descriptorPool.free();
	m_descriptorPools.clear();

	for (VulkanSwapchain& swapchain : m_swapchains)
		swapchain.free();
	m_swapchains.clear();

	for (VulkanSemaphore& semaphore : m_semaphores)
		semaphore.free();
	m_semaphores.clear();

	for (VulkanFramebuffer& framebuffer : m_framebuffers)
		framebuffer.free();
	m_framebuffers.clear();

	for (VulkanFence& fence : m_fences)
		fence.free();
	m_fences.clear();

	vkDestroyDevice(m_vkHandle, nullptr);
	Logger::print("Freed device (ID: " + std::to_string(m_id) + ")", Logger::DEBUG);
	m_vkHandle = VK_NULL_HANDLE;
}

VkDeviceMemory VulkanDevice::getMemoryHandle(const uint32_t chunkID) const
{
	for (const auto& chunk : m_memoryAllocator.m_memoryChunks)
	{
		if (chunk.getID() == chunkID)
		{
			return chunk.m_memory;
		}
	}
	Logger::print("Memory chunk search failed out of " + std::to_string(m_memoryAllocator.m_memoryChunks.size()) + " memory chunks", Logger::DEBUG);
	throw std::runtime_error("Memory chunk (ID: " + std::to_string(chunkID) + ") not found");
}

VkCommandPool VulkanDevice::getCommandPool(const uint32_t uint32, const uint32_t m_thread_id, const VulkanCommandBuffer::TypeFlags flags)
{
	if ((flags & VulkanCommandBuffer::TypeFlagBits::ONE_TIME) != 0)
		return m_threadCommandInfos[m_thread_id].oneTimePool;
	if ((flags & VulkanCommandBuffer::TypeFlagBits::SECONDARY) != 0)
		return m_threadCommandInfos[m_thread_id].commandPools[uint32].secondaryPool;
    return m_threadCommandInfos[m_thread_id].commandPools[uint32].pool;
}

VulkanDevice::VulkanDevice(const VulkanGPU pDevice, const VkDevice device)
	: m_vkHandle(device), m_physicalDevice(pDevice), m_memoryAllocator(*this)
{

}
