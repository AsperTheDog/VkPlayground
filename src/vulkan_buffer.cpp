#include "vulkan_buffer.hpp"

#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "utils/logger.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

VkMemoryRequirements VulkanBuffer::getMemoryRequirements() const
{
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(VulkanContext::getDevice(m_device).m_vkHandle, m_vkHandle, &memoryRequirements);
	return memoryRequirements;
}

void VulkanBuffer::allocateFromIndex(const uint32_t memoryIndex)
{
	Logger::pushContext("Buffer memory (from index)");
	const VkMemoryRequirements requirements = getMemoryRequirements();
	setBoundMemory(VulkanContext::getDevice(m_device).m_memoryAllocator.allocate(requirements.size, requirements.alignment, memoryIndex));
	Logger::popContext();
}

void VulkanBuffer::allocateFromFlags(const VulkanMemoryAllocator::MemoryPropertyPreferences memoryProperties)
{
	Logger::pushContext("Buffer memory (from flags)");
	const VkMemoryRequirements requirements = getMemoryRequirements();
    m_size = requirements.size;
    setBoundMemory(VulkanContext::getDevice(m_device).m_memoryAllocator.searchAndAllocate(requirements.size, requirements.alignment, memoryProperties, requirements.memoryTypeBits));
	Logger::popContext();
}

bool VulkanBuffer::isMemoryMapped() const
{
	return m_mappedData != nullptr;
}

void* VulkanBuffer::getMappedData() const
{
	return m_mappedData;
}

VkDeviceSize VulkanBuffer::getSize() const
{
	return m_size;
}

VkBuffer VulkanBuffer::operator*() const
{
	return m_vkHandle;
}

bool VulkanBuffer::isMemoryBound() const
{
	return m_memoryRegion.size > 0;
}

uint32_t VulkanBuffer::getBoundMemoryType() const
{
	return VulkanContext::getDevice(m_device).m_memoryAllocator.getChunkMemoryType(m_memoryRegion.chunk);
}

void* VulkanBuffer::map(const VkDeviceSize size, const VkDeviceSize offset)
{
	void* data;
	if (const VkResult ret = vkMapMemory(VulkanContext::getDevice(m_device).m_vkHandle, VulkanContext::getDevice(m_device).getMemoryHandle(m_memoryRegion.chunk), m_memoryRegion.offset + offset, size, 0, &data); ret != VK_SUCCESS)
	{
	    throw std::runtime_error("Failed to map buffer (ID:" + std::to_string(m_id) + ") memory! Error code: " + string_VkResult(ret));
	}
	m_mappedData = data;
    Logger::print("Mapped buffer (ID:" + std::to_string(m_id) + ") memory with size " + VulkanMemoryAllocator::compactBytes(size) + " and offset " + std::to_string(offset), Logger::DEBUG);
	return data;
}

void VulkanBuffer::unmap()
{
    if (!isMemoryMapped())
    {
        Logger::print("Tried to unmap memory for buffer (ID:" + std::to_string(m_id) + "), but memory was not mapped", Logger::LevelBits::WARN);
        return;
    }

	vkUnmapMemory(VulkanContext::getDevice(m_device).m_vkHandle, VulkanContext::getDevice(m_device).getMemoryHandle(m_memoryRegion.chunk));
    Logger::print("Unmapped buffer (ID:" + std::to_string(m_id) + ") memory", Logger::DEBUG);
	m_mappedData = nullptr;
}

VulkanBuffer::VulkanBuffer(const uint32_t device, const VkBuffer vkHandle, const VkDeviceSize size)
	: m_vkHandle(vkHandle), m_size(size), m_device(device)
{
	
}

void VulkanBuffer::setBoundMemory(const MemoryChunk::MemoryBlock& memoryRegion)
{
	if (m_memoryRegion.size > 0)
	{
		throw std::runtime_error("Buffer (ID:" + std::to_string(m_id) + ") already has memory bound to it!");
	}
	m_memoryRegion = memoryRegion;

	Logger::print("Bound memory region to buffer (ID:" + std::to_string(m_id) + ") with size " + std::to_string(m_memoryRegion.size) + " and offset " + std::to_string(m_memoryRegion.offset) + " at chunk " + std::to_string(m_memoryRegion.chunk), Logger::DEBUG);
	if (const VkResult ret = vkBindBufferMemory(VulkanContext::getDevice(m_device).m_vkHandle, m_vkHandle, VulkanContext::getDevice(m_device).getMemoryHandle(m_memoryRegion.chunk), m_memoryRegion.offset); ret != VK_SUCCESS)
	{
	    throw std::runtime_error("Failed to bind buffer (ID:" + std::to_string(m_id) + ") memory! Error code: " + string_VkResult(ret));
	}
}

void VulkanBuffer::free()
{
	vkDestroyBuffer(VulkanContext::getDevice(m_device).m_vkHandle, m_vkHandle, nullptr);
	Logger::print("Freed buffer (ID:" + std::to_string(m_id) + ")", Logger::DEBUG);
	m_vkHandle = VK_NULL_HANDLE;

	if (m_memoryRegion.size > 0)
	{
        Logger::pushContext("Buffer memory free");
		VulkanContext::getDevice(m_device).m_memoryAllocator.deallocate(m_memoryRegion);
		m_memoryRegion = {};
        Logger::popContext();
	}
}
