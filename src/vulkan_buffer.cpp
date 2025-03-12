#include "vulkan_buffer.hpp"

#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "utils/logger.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

VkMemoryRequirements VulkanBuffer::getMemoryRequirements() const
{
    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

	VkMemoryRequirements l_MemoryRequirements;
	l_Device.getTable().vkGetBufferMemoryRequirements(l_Device.m_VkHandle, m_VkHandle, &l_MemoryRequirements);
	return l_MemoryRequirements;
}

void VulkanBuffer::allocateFromIndex(const uint32_t p_MemoryIndex)
{
	Logger::pushContext("Buffer memory (from index)");
	const VkMemoryRequirements l_Requirements = getMemoryRequirements();
	setBoundMemory(VulkanContext::getDevice(getDeviceID()).getMemoryAllocator().allocate(l_Requirements.size, l_Requirements.alignment, p_MemoryIndex));
	Logger::popContext();
}

void VulkanBuffer::allocateFromFlags(const VulkanMemoryAllocator::MemoryPropertyPreferences p_MemoryProperties)
{
	Logger::pushContext("Buffer memory (from flags)");
	const VkMemoryRequirements l_Requirements = getMemoryRequirements();
    m_Size = l_Requirements.size;
    setBoundMemory(VulkanContext::getDevice(getDeviceID()).getMemoryAllocator().searchAndAllocate(l_Requirements.size, l_Requirements.alignment, p_MemoryProperties, l_Requirements.memoryTypeBits));
	Logger::popContext();
}

bool VulkanBuffer::isMemoryMapped() const
{
	return m_MappedData != nullptr;
}

void* VulkanBuffer::getMappedData() const
{
	return m_MappedData;
}

VkDeviceSize VulkanBuffer::getSize() const
{
	return m_Size;
}

uint32_t VulkanBuffer::getQueue() const
{
    return m_QueueFamilyIndex;
}

void VulkanBuffer::setQueue(const uint32_t p_QueueFamilyIndex)
{
    m_QueueFamilyIndex = p_QueueFamilyIndex;
}

VkBuffer VulkanBuffer::operator*() const
{
	return m_VkHandle;
}

bool VulkanBuffer::isMemoryBound() const
{
	return m_MemoryRegion.size > 0;
}

uint32_t VulkanBuffer::getBoundMemoryType() const
{
	return VulkanContext::getDevice(getDeviceID()).getMemoryAllocator().getChunkMemoryType(m_MemoryRegion.chunk);
}

VkDeviceAddress VulkanBuffer::getDeviceAddress() const
{
    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

    VkBufferDeviceAddressInfo l_Info = {};
    l_Info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    l_Info.buffer = m_VkHandle;
    return l_Device.getTable().vkGetBufferDeviceAddress(*l_Device, &l_Info);
}

void* VulkanBuffer::map(const VkDeviceSize p_Size, const VkDeviceSize p_Offset)
{
    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

	void* l_Data;
	if (const VkResult l_Ret = l_Device.getTable().vkMapMemory(*l_Device, l_Device.getMemoryHandle(m_MemoryRegion.chunk), m_MemoryRegion.offset + p_Offset, p_Size, 0, &l_Data); l_Ret != VK_SUCCESS)
	{
	    throw std::runtime_error("Failed to map buffer (ID:" + std::to_string(m_ID) + ") memory! Error code: " + string_VkResult(l_Ret));
	}
	m_MappedData = l_Data;
    LOG_DEBUG("Mapped buffer (ID:", m_ID, ") memory with size ", VulkanMemoryAllocator::compactBytes(p_Size), " and offset ", p_Offset);
	return l_Data;
}

void VulkanBuffer::unmap()
{
    if (!isMemoryMapped())
    {
        LOG_WARN("Tried to unmap memory for buffer (ID:", m_ID, "), but memory was not mapped");
        return;
    }

    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

	l_Device.getTable().vkUnmapMemory(*l_Device, l_Device.getMemoryHandle(m_MemoryRegion.chunk));
    LOG_DEBUG("Unmapped buffer (ID:", m_ID, ") memory");
	m_MappedData = nullptr;
}

VulkanBuffer::VulkanBuffer(const uint32_t p_Device, const VkBuffer p_VkHandle, const VkDeviceSize p_Size)
	: VulkanDeviceSubresource(p_Device), m_VkHandle(p_VkHandle), m_Size(p_Size)
{
	
}

void VulkanBuffer::setBoundMemory(const MemoryChunk::MemoryBlock& p_MemoryRegion)
{
	if (m_MemoryRegion.size > 0)
	{
		throw std::runtime_error("Buffer (ID:" + std::to_string(m_ID) + ") already has memory bound to it!");
	}
	m_MemoryRegion = p_MemoryRegion;

    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

    LOG_DEBUG("Bound memory region to buffer (ID:", m_ID, ") with size ", m_MemoryRegion.size, " and offset ", m_MemoryRegion.offset, " at chunk ", m_MemoryRegion.chunk);
	if (const VkResult l_Ret = l_Device.getTable().vkBindBufferMemory(*l_Device, m_VkHandle, l_Device.getMemoryHandle(m_MemoryRegion.chunk), m_MemoryRegion.offset); l_Ret != VK_SUCCESS)
	{
	    throw std::runtime_error("Failed to bind buffer (ID:" + std::to_string(m_ID) + ") memory! Error code: " + string_VkResult(l_Ret));
	}
}

void VulkanBuffer::free()
{
    VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

	l_Device.getTable().vkDestroyBuffer(*l_Device, m_VkHandle, nullptr);
    LOG_DEBUG("Freed buffer (ID:", m_ID, ")");
	m_VkHandle = VK_NULL_HANDLE;

	if (m_MemoryRegion.size > 0)
	{
        Logger::pushContext("Buffer memory free");
		l_Device.getMemoryAllocator().deallocate(m_MemoryRegion);
		m_MemoryRegion = {};
        Logger::popContext();
	}
}
