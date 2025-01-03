#include "vulkan_memory.hpp"

#include <ranges>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#include "utils/logger.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

std::string VulkanMemoryAllocator::compactBytes(const VkDeviceSize bytes)
{
	const char* units[] = { "B", "KB", "MB", "GB", "TB" };
	uint32_t unit = 0;
	float exact = static_cast<float>(bytes);
	while (exact > 1024)
	{
		exact /= 1024;
		unit++;
	}
	return std::to_string(exact) + " " + units[unit];
}

VkPhysicalDeviceMemoryProperties MemoryStructure::getMemoryProperties() const
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_GPU.getHandle(), &memoryProperties);
    return memoryProperties;
}

std::string MemoryStructure::toString() const
{
    VkPhysicalDeviceMemoryProperties memoryProperties = getMemoryProperties();

	std::string str;
	for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++)
	{
		str += "Memory Heap " + std::to_string(i) + ":\n";
		str += " - Size: " + VulkanMemoryAllocator::compactBytes(memoryProperties.memoryHeaps[i].size) + "\n";
		str += " - Flags: " + string_VkMemoryHeapFlags(memoryProperties.memoryHeaps[i].flags) + "\n";
		str += " - Memory Types:\n";
		for (uint32_t j = 0; j < memoryProperties.memoryTypeCount; j++)
		{
			if (memoryProperties.memoryTypes[j].heapIndex == i)
			{
				str += "    - Memory Type " + std::to_string(j) + ": " + string_VkMemoryPropertyFlags(memoryProperties.memoryTypes[j].propertyFlags) + "\n";
			}
		}
	}
	return str;
}

std::optional<uint32_t> MemoryStructure::getStagingMemoryType(const uint32_t typeFilter) const
{
	std::vector<uint32_t> types = getMemoryTypes(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, typeFilter);

	if (types.empty())
		return std::nullopt;

	return types.front();
}

std::vector<uint32_t> MemoryStructure::getMemoryTypes(const VkMemoryPropertyFlags properties, const uint32_t typeFilter) const
{
	std::vector<uint32_t> suitableTypes{};
	for (uint32_t i = 0; i < getMemoryProperties().memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && doesMemoryContainProperties(i, properties)) {
            suitableTypes.push_back(i);
        }
	}
	return suitableTypes;
}

bool MemoryStructure::doesMemoryContainProperties(const uint32_t type, const VkMemoryPropertyFlags property) const
{
	return (getMemoryProperties().memoryTypes[type].propertyFlags & property) == property;
}

MemoryStructure::MemoryTypeData MemoryStructure::getTypeData(const uint32_t memoryType) const
{
    const VkPhysicalDeviceMemoryProperties memoryProperties = getMemoryProperties();
    return {
        memoryProperties.memoryTypes[memoryType].propertyFlags,
        memoryProperties.memoryTypes[memoryType].heapIndex,
        memoryProperties.memoryHeaps[memoryProperties.memoryTypes[memoryType].heapIndex].size
    };
}

VkMemoryHeap MemoryStructure::getMemoryTypeHeap(const uint32_t memoryType) const
{
    const VkPhysicalDeviceMemoryProperties memoryProperties = getMemoryProperties();
    return memoryProperties.memoryHeaps[memoryProperties.memoryTypes[memoryType].heapIndex];
}

VkDeviceSize MemoryChunk::getSize() const
{
	return m_size;
}

uint32_t MemoryChunk::getMemoryType() const
{
	return m_memoryType;
}

bool MemoryChunk::isEmpty() const
{
	VkDeviceSize freeSize = 0;
	for (const VkDeviceSize& size : m_unallocatedData | std::views::values)
	{
		freeSize += size;
	}
	return freeSize == m_size;
}

MemoryChunk::MemoryBlock MemoryChunk::allocate(const VkDeviceSize newSize, const VkDeviceSize alignment)
{
	VkDeviceSize best = m_size;
	VkDeviceSize bestAlignOffset = 0;
	if (newSize > m_unallocatedData[m_biggestChunk])
		return {0, 0, m_ID};

	for (const auto& [offset, size] : m_unallocatedData)
	{
		if (size < newSize) continue;

        const VkDeviceSize offsetDifference = offset % alignment == 0 ? 0 : alignment - offset % alignment;
		if (size + offsetDifference < newSize) continue;

		if (best == m_size || m_unallocatedData[best] > size)
		{
			best = offset;
			bestAlignOffset = offsetDifference;
		}
	}

	if (best == m_size)
		return {0, 0, m_ID};

	const VkDeviceSize bestSize = m_unallocatedData[best];
	m_unallocatedData.erase(best);
	if (bestAlignOffset != 0)
	{
		m_unallocatedData[best] = bestAlignOffset;
		best += bestAlignOffset;
	}
	if (bestSize - bestAlignOffset != newSize)
	{
		m_unallocatedData[best + newSize] = (bestSize - bestAlignOffset) - newSize;
	}

	Logger::print("Allocated block of size " + VulkanMemoryAllocator::compactBytes(newSize) + " at offset " + std::to_string(best) + " of memory type " + std::to_string(m_memoryType) + " from chunk " + std::to_string(m_ID), Logger::DEBUG);

	for (const auto& [offset, size] : m_unallocatedData)
	{
		if (!m_unallocatedData.contains(m_biggestChunk) || size > m_unallocatedData[m_biggestChunk])
			m_biggestChunk = offset;
	}

	m_unallocatedSize -= newSize;

	return {newSize, best, m_ID};
}

void MemoryChunk::deallocate(const MemoryBlock& block)
{
	if (block.chunk != m_ID)
		throw std::runtime_error("Tried to deallocate block from chunk " + std::to_string(block.chunk) + " in chunk " + std::to_string(m_ID));

	m_unallocatedData[block.offset] = block.size;
	Logger::print("Deallocated block from chunk " + std::to_string(block.chunk) + " of size " + VulkanMemoryAllocator::compactBytes(block.size) + " at offset " + std::to_string(block.offset) + " of memory type " + std::to_string(m_memoryType), Logger::DEBUG);

	m_unallocatedSize += block.size;

	defragment();
}

VkDeviceMemory MemoryChunk::operator*() const
{
	return m_memory;
}

VkDeviceSize MemoryChunk::getBiggestChunkSize() const
{
	return m_unallocatedData.empty() ? 0 : m_unallocatedData.at(m_biggestChunk);
}

VkDeviceSize MemoryChunk::getRemainingSize() const
{
	return m_unallocatedSize;
}

MemoryChunk::MemoryChunk(const VkDeviceSize size, const uint32_t memoryType, const VkDeviceMemory vkHandle)
	: m_size(size), m_memoryType(memoryType), m_memory(vkHandle), m_unallocatedSize(size)
{
	m_unallocatedData[0] = size;
}

void MemoryChunk::defragment()
{
	if (m_unallocatedSize == m_size)
	{
		Logger::print("No need to defragment empty memory chunk (ID: " + std::to_string(m_ID) + ")", Logger::DEBUG);
		return;
	}
	Logger::pushContext("Memory defragmentation");
	Logger::print("Defragmenting memory chunk (ID: " + std::to_string(m_ID) + ")", Logger::DEBUG);
	uint32_t mergeCount = 0;
	for (auto it = m_unallocatedData.begin(); it != m_unallocatedData.end();)
	{
		auto next = std::next(it);
		if (next == m_unallocatedData.end())
		{
			break;
		}
		if (next != m_unallocatedData.end() && it->first + it->second == next->first)
		{
			
			it->second += next->second;
			if (next->first == m_biggestChunk || it->second > m_unallocatedData[m_biggestChunk])
				m_biggestChunk = it->first;

			Logger::print("  Merged blocks at offsets " + std::to_string(it->first) + " and " + std::to_string(next->first) + ", new size: " + VulkanMemoryAllocator::compactBytes(it->second), Logger::DEBUG);
			mergeCount++;

			m_unallocatedData.erase(next);
		}
		else
		{
			++it;
		}
	}
	Logger::print("  Defragmented " + std::to_string(mergeCount) + " blocks", Logger::DEBUG);
	Logger::popContext();
}

VulkanMemoryAllocator::VulkanMemoryAllocator(const VulkanDevice& device, const VkDeviceSize defaultChunkSize)
	: m_memoryStructure(device.getGPU()), m_chunkSize(defaultChunkSize), m_device(device.getID())
{

}

void VulkanMemoryAllocator::free()
{
	for (const MemoryChunk& memoryBlock : m_memoryChunks)
	{
		vkFreeMemory(VulkanContext::getDevice(m_device).m_VkHandle, memoryBlock.m_memory, nullptr);
        Logger::print("Freed memory chunk (ID: " + std::to_string(memoryBlock.getID()) + ")", Logger::DEBUG);
	}
	m_memoryChunks.clear();
}

MemoryChunk::MemoryBlock VulkanMemoryAllocator::allocate(const VkDeviceSize size, const VkDeviceSize alignment, const uint32_t memoryType)
{
	VkDeviceSize chunkSize = m_chunkSize;
	if (size < m_chunkSize)
	{
		for (MemoryChunk& memoryChunk : m_memoryChunks)
		{
			if (memoryChunk.m_memoryType == memoryType)
			{
				const MemoryChunk::MemoryBlock block = memoryChunk.allocate(size, alignment);
				if (block.size != 0) return block;
			}
		}
	}
	else
	{
		chunkSize = size;
	}

    const VkDeviceSize heapSize = getMemoryStructure().getMemoryTypeHeap(memoryType).size;
    const VkDeviceSize maxHeapUsage = static_cast<VkDeviceSize>(static_cast<double>(heapSize) * 0.8);
    if (chunkSize > maxHeapUsage) chunkSize = maxHeapUsage;
    if (chunkSize < size) 
        throw std::runtime_error("Allocation of size " + std::to_string(size) + " was requested for memory type " + std::to_string(memoryType) + " but the heap size is only " + std::to_string(heapSize) + " (Cannot allocate more than 80% of heap)");

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = chunkSize;
	allocInfo.memoryTypeIndex = memoryType;

	VkDeviceMemory memory;
	if (const VkResult ret = vkAllocateMemory(VulkanContext::getDevice(m_device).m_VkHandle, &allocInfo, nullptr, &memory); ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to allocate memory, error: ") + string_VkResult(ret));
	}

	m_memoryChunks.push_back(MemoryChunk(chunkSize, memoryType, memory));
	Logger::print("Allocated chunk (ID: " + std::to_string(m_memoryChunks.back().getID()) + ") of size " + compactBytes(chunkSize) + " of memory type " + std::to_string(memoryType), Logger::DEBUG);
	return m_memoryChunks.back().allocate(size, alignment);
}

MemoryChunk::MemoryBlock VulkanMemoryAllocator::searchAndAllocate(const VkDeviceSize size, const VkDeviceSize alignment, const MemoryPropertyPreferences properties, const uint32_t typeFilter, const bool includeHidden)
{
	const std::vector<uint32_t> memoryType = m_memoryStructure.getMemoryTypes(properties.desiredProperties, typeFilter);
	uint32_t bestType = 0;
	VkDeviceSize bestSize = 0;
	bool doesBestHaveUndesired = false;
	for (const uint32_t& type : memoryType)
	{
		if (!includeHidden && m_hiddenTypes.contains(type))
			continue;

		const bool doesMemoryHaveUndesired = m_memoryStructure.doesMemoryContainProperties(type, properties.undesiredProperties);
		if (!properties.allowUndesired && doesMemoryHaveUndesired)
			continue;

		if (bestSize != 0 && !doesBestHaveUndesired && doesMemoryHaveUndesired)
			continue;

		if (suitableChunkExists(type, size))
			return allocate(size, alignment, type);

		const VkDeviceSize remainingSize = getRemainingSize(m_memoryStructure.getMemoryProperties().memoryTypes[type].heapIndex);
		if (remainingSize >= bestSize)
		{
			bestType = type;
			bestSize = remainingSize;
			doesBestHaveUndesired = doesMemoryHaveUndesired;
		}
	}
	return allocate(size, alignment, bestType);
}

void VulkanMemoryAllocator::deallocate(const MemoryChunk::MemoryBlock& block)
{
	uint32_t chunkIndex = static_cast<uint32_t>(m_memoryChunks.size());
	for (uint32_t i = 0; i < m_memoryChunks.size(); i++)
	{
		if (m_memoryChunks[i].getID() == block.chunk)
		{
			chunkIndex = i;
			break;
		}
	}

	if (chunkIndex == m_memoryChunks.size())
	{
		throw std::runtime_error("Tried to deallocate block but owner chunk (ID: " + std::to_string(block.chunk) + ") was not found");
	}

	m_memoryChunks[chunkIndex].deallocate(block);
	if (m_memoryChunks[chunkIndex].isEmpty())
	{
		vkFreeMemory(VulkanContext::getDevice(m_device).m_VkHandle, m_memoryChunks[chunkIndex].m_memory, nullptr);
		m_memoryChunks.erase(m_memoryChunks.begin() + chunkIndex);
		Logger::print("Freed empty chunk (ID: " + std::to_string(block.chunk) + ")", Logger::DEBUG);
	}
}

void VulkanMemoryAllocator::hideMemoryType(const uint32_t type)
{
	Logger::print("Hiding memory type " + std::to_string(type), Logger::DEBUG);
	m_hiddenTypes.insert(type);
}

void VulkanMemoryAllocator::unhideMemoryType(const uint32_t type)
{
	Logger::print("Unhiding memory type " + std::to_string(type), Logger::DEBUG);
	m_hiddenTypes.erase(type);
}

const MemoryStructure& VulkanMemoryAllocator::getMemoryStructure() const
{
	return m_memoryStructure;
}

VkDeviceSize VulkanMemoryAllocator::getRemainingSize(const uint32_t heap) const
{
    const VkPhysicalDeviceMemoryProperties memoryProperties = m_memoryStructure.getMemoryProperties();

	VkDeviceSize remainingSize = memoryProperties.memoryHeaps[heap].size;
	for (const auto& chunk : m_memoryChunks)
	{
		if (memoryProperties.memoryTypes[chunk.m_memoryType].heapIndex == heap)
		{
			remainingSize -= chunk.getSize();
		}
	}
	return remainingSize;
}

bool VulkanMemoryAllocator::suitableChunkExists(const uint32_t memoryType, const VkDeviceSize size) const
{
	for (const auto& chunk : m_memoryChunks)
	{
		if (chunk.m_memoryType == memoryType && chunk.getBiggestChunkSize() >= size)
		{
			return true;
		}
	}
	return false;
}

bool VulkanMemoryAllocator::isMemoryTypeHidden(const unsigned value) const
{
	return m_hiddenTypes.contains(value);
}

uint32_t VulkanMemoryAllocator::getChunkMemoryType(const uint32_t chunk) const
{
	for (const auto& memoryChunk : m_memoryChunks)
	{
		if (memoryChunk.getID() == chunk)
		{
			return memoryChunk.getMemoryType();
		}
	}

    Logger::print("Chunk search failed out of " + std::to_string(m_memoryChunks.size()) + " chunks", Logger::DEBUG);
	throw std::runtime_error("Chunk (ID:" + std::to_string(chunk) + ") not found");
}
