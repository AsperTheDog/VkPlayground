#include "vulkan_memory.hpp"

#include <ranges>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>
#include <Volk/volk.h>

#include "utils/logger.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

std::string VulkanMemoryAllocator::compactBytes(const VkDeviceSize p_Bytes)
{
	const char* l_Units[] = { "B", "KB", "MB", "GB", "TB" };
	uint32_t l_Unit = 0;
	float l_Exact = static_cast<float>(p_Bytes);
	while (l_Exact > 1024)
	{
		l_Exact /= 1024;
		l_Unit++;
	}
	return std::to_string(l_Exact) + " " + l_Units[l_Unit];
}

VkPhysicalDeviceMemoryProperties MemoryStructure::getMemoryProperties() const
{
    VkPhysicalDeviceMemoryProperties l_MemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_GPU.getHandle(), &l_MemoryProperties);
    return l_MemoryProperties;
}

std::string MemoryStructure::toString() const
{
    const VkPhysicalDeviceMemoryProperties l_MemoryProperties = getMemoryProperties();

	std::string l_Str;
	for (uint32_t l_MemoryHeapIdx = 0; l_MemoryHeapIdx < l_MemoryProperties.memoryHeapCount; l_MemoryHeapIdx++)
	{
		l_Str += "Memory Heap " + std::to_string(l_MemoryHeapIdx) + ":\n";
		l_Str += " - Size: " + VulkanMemoryAllocator::compactBytes(l_MemoryProperties.memoryHeaps[l_MemoryHeapIdx].size) + "\n";
		l_Str += " - Flags: " + string_VkMemoryHeapFlags(l_MemoryProperties.memoryHeaps[l_MemoryHeapIdx].flags) + "\n";
		l_Str += " - Memory Types:\n";
		for (uint32_t l_MemoryTypeIdx = 0; l_MemoryTypeIdx < l_MemoryProperties.memoryTypeCount; l_MemoryTypeIdx++)
		{
			if (l_MemoryProperties.memoryTypes[l_MemoryTypeIdx].heapIndex == l_MemoryHeapIdx)
			{
				l_Str += "    - Memory Type " + std::to_string(l_MemoryTypeIdx) + ": " + string_VkMemoryPropertyFlags(l_MemoryProperties.memoryTypes[l_MemoryTypeIdx].propertyFlags) + "\n";
			}
		}
	}
	return l_Str;
}

std::optional<uint32_t> MemoryStructure::getStagingMemoryType(const uint32_t p_TypeFilter) const
{
	std::vector<uint32_t> l_Types = getMemoryTypes(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_TypeFilter);

	if (l_Types.empty())
		return std::nullopt;

	return l_Types.front();
}

std::vector<uint32_t> MemoryStructure::getMemoryTypes(const VkMemoryPropertyFlags p_Properties, const uint32_t p_TypeFilter) const
{
	std::vector<uint32_t> l_SuitableTypes{};
	for (uint32_t l_MemoryTypeIdx = 0; l_MemoryTypeIdx < getMemoryProperties().memoryTypeCount; l_MemoryTypeIdx++)
	{
		if ((p_TypeFilter & (1 << l_MemoryTypeIdx)) && doesMemoryContainProperties(l_MemoryTypeIdx, p_Properties)) {
            l_SuitableTypes.push_back(l_MemoryTypeIdx);
        }
	}
	return l_SuitableTypes;
}

bool MemoryStructure::doesMemoryContainProperties(const uint32_t p_Type, const VkMemoryPropertyFlags p_Property) const
{
	return (getMemoryProperties().memoryTypes[p_Type].propertyFlags & p_Property) == p_Property;
}

MemoryStructure::MemoryTypeData MemoryStructure::getTypeData(const uint32_t p_MemoryType) const
{
    const VkPhysicalDeviceMemoryProperties l_MemoryProperties = getMemoryProperties();
    return {
        l_MemoryProperties.memoryTypes[p_MemoryType].propertyFlags,
        l_MemoryProperties.memoryTypes[p_MemoryType].heapIndex,
        l_MemoryProperties.memoryHeaps[l_MemoryProperties.memoryTypes[p_MemoryType].heapIndex].size
    };
}

VkMemoryHeap MemoryStructure::getMemoryTypeHeap(const uint32_t p_MemoryType) const
{
    const VkPhysicalDeviceMemoryProperties l_MemoryProperties = getMemoryProperties();
    return l_MemoryProperties.memoryHeaps[l_MemoryProperties.memoryTypes[p_MemoryType].heapIndex];
}

VkDeviceSize MemoryChunk::getSize() const
{
	return m_Size;
}

uint32_t MemoryChunk::getMemoryType() const
{
	return m_MemoryType;
}

bool MemoryChunk::isEmpty() const
{
	VkDeviceSize l_FreeSize = 0;
	for (const VkDeviceSize& l_Size : m_UnallocatedData | std::views::values)
	{
		l_FreeSize += l_Size;
	}
	return l_FreeSize == m_Size;
}

MemoryChunk::MemoryBlock MemoryChunk::allocate(const VkDeviceSize p_NewSize, const VkDeviceSize p_Alignment)
{
	VkDeviceSize l_Best = m_Size;
	VkDeviceSize l_BestAlignOffset = 0;
	if (p_NewSize > m_UnallocatedData[m_BiggestChunk])
		return {0, 0, m_ID};

	for (const auto& [l_Offset, l_Size] : m_UnallocatedData)
	{
		if (l_Size < p_NewSize) continue;

        const VkDeviceSize l_OffsetDifference = l_Offset % p_Alignment == 0 ? 0 : p_Alignment - l_Offset % p_Alignment;
		if (l_Size + l_OffsetDifference < p_NewSize) continue;

		if (l_Best == m_Size || m_UnallocatedData[l_Best] > l_Size)
		{
			l_Best = l_Offset;
			l_BestAlignOffset = l_OffsetDifference;
		}
	}

	if (l_Best == m_Size)
		return {0, 0, m_ID};

	const VkDeviceSize l_BestSize = m_UnallocatedData[l_Best];
	m_UnallocatedData.erase(l_Best);
	if (l_BestAlignOffset != 0)
	{
		m_UnallocatedData[l_Best] = l_BestAlignOffset;
		l_Best += l_BestAlignOffset;
	}
	if (l_BestSize - l_BestAlignOffset != p_NewSize)
	{
		m_UnallocatedData[l_Best + p_NewSize] = (l_BestSize - l_BestAlignOffset) - p_NewSize;
	}

    LOG_DEBUG("Allocated block of size ", VulkanMemoryAllocator::compactBytes(p_NewSize), " at offset ", l_Best, " of memory type ", m_MemoryType, " from chunk ", m_ID);

	for (const auto& [l_Offset, l_Size] : m_UnallocatedData)
	{
		if (!m_UnallocatedData.contains(m_BiggestChunk) || l_Size > m_UnallocatedData[m_BiggestChunk])
			m_BiggestChunk = l_Offset;
	}

	m_UnallocatedSize -= p_NewSize;

	return {p_NewSize, l_Best, m_ID};
}

void MemoryChunk::deallocate(const MemoryBlock& p_Block)
{
	if (p_Block.chunk != m_ID)
		throw std::runtime_error("Tried to deallocate block from chunk " + std::to_string(p_Block.chunk) + " in chunk " + std::to_string(m_ID));

	m_UnallocatedData[p_Block.offset] = p_Block.size;
    LOG_DEBUG("Deallocated block from chunk ", p_Block.chunk, " of size ", VulkanMemoryAllocator::compactBytes(p_Block.size), " at offset ", p_Block.offset, " of memory type ", m_MemoryType);
	
	m_UnallocatedSize += p_Block.size;

	defragment();
}

VkDeviceMemory MemoryChunk::operator*() const
{
	return m_Memory;
}

VkDeviceSize MemoryChunk::getBiggestChunkSize() const
{
	return m_UnallocatedData.empty() ? 0 : m_UnallocatedData.at(m_BiggestChunk);
}

VkDeviceSize MemoryChunk::getRemainingSize() const
{
	return m_UnallocatedSize;
}

MemoryChunk::MemoryChunk(const VkDeviceSize p_Size, const uint32_t p_MemoryType, const VkDeviceMemory p_VkHandle)
	: m_Size(p_Size), m_MemoryType(p_MemoryType), m_Memory(p_VkHandle), m_UnallocatedSize(p_Size)
{
	m_UnallocatedData[0] = p_Size;
}

void MemoryChunk::defragment()
{
	if (m_UnallocatedSize == m_Size)
	{
        LOG_DEBUG("No need to defragment empty memory chunk (ID: ", m_ID, ")");
		return;
	}
	Logger::pushContext("Memory defragmentation");
    LOG_DEBUG("Defragmenting memory chunk (ID: ", m_ID, ")");
	uint32_t l_MergeCount = 0;
	for (auto l_It = m_UnallocatedData.begin(); l_It != m_UnallocatedData.end();)
	{
		auto l_Next = std::next(l_It);
		if (l_Next == m_UnallocatedData.end())
		{
			break;
		}
		if (l_Next != m_UnallocatedData.end() && l_It->first + l_It->second == l_Next->first)
		{
			
			l_It->second += l_Next->second;
			if (l_Next->first == m_BiggestChunk || l_It->second > m_UnallocatedData[m_BiggestChunk])
				m_BiggestChunk = l_It->first;

            LOG_DEBUG("  Merged blocks at offsets ", l_It->first, " and ", l_Next->first, ", new size: ", VulkanMemoryAllocator::compactBytes(l_It->second));
			l_MergeCount++;

			m_UnallocatedData.erase(l_Next);
		}
		else
		{
			++l_It;
		}
	}
    LOG_DEBUG("  Defragmented ", l_MergeCount, " blocks");
	Logger::popContext();
}

VulkanMemoryAllocator::VulkanMemoryAllocator(const VulkanDevice& p_Device, const VkDeviceSize p_DefaultChunkSize)
	: m_MemoryStructure(p_Device.getGPU()), m_ChunkSize(p_DefaultChunkSize), m_Device(p_Device.getID())
{

}

void VulkanMemoryAllocator::free()
{
    const VulkanDevice& l_Device = VulkanContext::getDevice(m_Device);

	for (const MemoryChunk& l_MemoryBlock : m_MemoryChunks)
	{
		l_Device.getTable().vkFreeMemory(l_Device.m_VkHandle, l_MemoryBlock.m_Memory, nullptr);
        LOG_DEBUG("Freed memory chunk (ID: ", l_MemoryBlock.getID(), ")");
	}
	m_MemoryChunks.clear();
}

MemoryChunk::MemoryBlock VulkanMemoryAllocator::allocate(const VkDeviceSize p_Size, const VkDeviceSize p_Alignment, const uint32_t p_MemoryType)
{
	VkDeviceSize l_ChunkSize = m_ChunkSize;
	if (p_Size < m_ChunkSize)
	{
		for (MemoryChunk& l_MemoryChunk : m_MemoryChunks)
		{
			if (l_MemoryChunk.m_MemoryType == p_MemoryType)
			{
				const MemoryChunk::MemoryBlock l_Block = l_MemoryChunk.allocate(p_Size, p_Alignment);
				if (l_Block.size != 0) return l_Block;
			}
		}
	}
	else
	{
		l_ChunkSize = p_Size;
	}

    const VkDeviceSize l_HeapSize = getMemoryStructure().getMemoryTypeHeap(p_MemoryType).size;
    const VkDeviceSize l_MaxHeapUsage = static_cast<VkDeviceSize>(static_cast<double>(l_HeapSize) * 0.7);
    if (l_ChunkSize > l_MaxHeapUsage) l_ChunkSize = l_MaxHeapUsage;
    if (l_ChunkSize < p_Size) 
        throw std::runtime_error("Allocation of size " + std::to_string(p_Size) + " was requested for memory type " + std::to_string(p_MemoryType) + " but the heap size is only " + std::to_string(l_HeapSize) + " (Cannot allocate more than 80% of heap)");

	VkMemoryAllocateInfo l_AllocInfo{};
	l_AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	l_AllocInfo.allocationSize = l_ChunkSize;
	l_AllocInfo.memoryTypeIndex = p_MemoryType;

    const VulkanDevice& l_Device = VulkanContext::getDevice(m_Device);

	VkDeviceMemory l_Memory;
	if (const VkResult l_Ret = l_Device.getTable().vkAllocateMemory(l_Device.m_VkHandle, &l_AllocInfo, nullptr, &l_Memory); l_Ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to allocate memory, error: ") + string_VkResult(l_Ret));
	}

	m_MemoryChunks.push_back(MemoryChunk(l_ChunkSize, p_MemoryType, l_Memory));
    LOG_DEBUG("Allocated chunk (ID: ", m_MemoryChunks.back().getID(), ") of size ", compactBytes(l_ChunkSize), " of memory type ", p_MemoryType);
	return m_MemoryChunks.back().allocate(p_Size, p_Alignment);
}

MemoryChunk::MemoryBlock VulkanMemoryAllocator::searchAndAllocate(const VkDeviceSize p_Size, const VkDeviceSize p_Alignment, const MemoryPropertyPreferences p_Properties, const uint32_t p_TypeFilter, const bool p_IncludeHidden)
{
	const std::vector<uint32_t> l_MemoryType = m_MemoryStructure.getMemoryTypes(p_Properties.desiredProperties, p_TypeFilter);
	uint32_t l_BestType = 0;
	VkDeviceSize l_BestSize = 0;
	bool l_DoesBestHaveUndesired = false;
	for (const uint32_t& l_Type : l_MemoryType)
	{
		if (!p_IncludeHidden && m_HiddenTypes.contains(l_Type))
			continue;

		const bool l_DoesMemoryHaveUndesired = m_MemoryStructure.doesMemoryContainProperties(l_Type, p_Properties.undesiredProperties);
		if (!p_Properties.allowUndesired && l_DoesMemoryHaveUndesired)
			continue;

		if (l_BestSize != 0 && !l_DoesBestHaveUndesired && l_DoesMemoryHaveUndesired)
			continue;

		if (suitableChunkExists(l_Type, p_Size))
			return allocate(p_Size, p_Alignment, l_Type);

		const VkDeviceSize l_RemainingSize = getRemainingSize(m_MemoryStructure.getMemoryProperties().memoryTypes[l_Type].heapIndex);
		if (l_RemainingSize >= l_BestSize)
		{
			l_BestType = l_Type;
			l_BestSize = l_RemainingSize;
			l_DoesBestHaveUndesired = l_DoesMemoryHaveUndesired;
		}
	}
	return allocate(p_Size, p_Alignment, l_BestType);
}

void VulkanMemoryAllocator::deallocate(const MemoryChunk::MemoryBlock& p_Block)
{
	uint32_t l_ChunkIndex = static_cast<uint32_t>(m_MemoryChunks.size());
	for (uint32_t l_memoryChunkIdx = 0; l_memoryChunkIdx < m_MemoryChunks.size(); l_memoryChunkIdx++)
	{
		if (m_MemoryChunks[l_memoryChunkIdx].getID() == p_Block.chunk)
		{
			l_ChunkIndex = l_memoryChunkIdx;
			break;
		}
	}

	if (l_ChunkIndex == m_MemoryChunks.size())
	{
		throw std::runtime_error("Tried to deallocate block but owner chunk (ID: " + std::to_string(p_Block.chunk) + ") was not found");
	}

	m_MemoryChunks[l_ChunkIndex].deallocate(p_Block);
	if (m_MemoryChunks[l_ChunkIndex].isEmpty())
	{
        const VulkanDevice& l_Device = VulkanContext::getDevice(m_Device);

		l_Device.getTable().vkFreeMemory(l_Device.m_VkHandle, m_MemoryChunks[l_ChunkIndex].m_Memory, nullptr);
		m_MemoryChunks.erase(m_MemoryChunks.begin() + l_ChunkIndex);
        LOG_DEBUG("Freed empty chunk (ID: ", p_Block.chunk, ")");
	}
}

void VulkanMemoryAllocator::hideMemoryType(const uint32_t p_Type)
{
    LOG_DEBUG("Hiding memory type ", p_Type);
	m_HiddenTypes.insert(p_Type);
}

void VulkanMemoryAllocator::unhideMemoryType(const uint32_t p_Type)
{
    LOG_DEBUG("Unhiding memory type ", p_Type);
	m_HiddenTypes.erase(p_Type);
}

const MemoryStructure& VulkanMemoryAllocator::getMemoryStructure() const
{
	return m_MemoryStructure;
}

VkDeviceSize VulkanMemoryAllocator::getRemainingSize(const uint32_t p_Heap) const
{
    const VkPhysicalDeviceMemoryProperties l_MemoryProperties = m_MemoryStructure.getMemoryProperties();

	VkDeviceSize l_RemainingSize = l_MemoryProperties.memoryHeaps[p_Heap].size;
	for (const MemoryChunk& l_Chunk : m_MemoryChunks)
	{
		if (l_MemoryProperties.memoryTypes[l_Chunk.m_MemoryType].heapIndex == p_Heap)
		{
			l_RemainingSize -= l_Chunk.getSize();
		}
	}
	return l_RemainingSize;
}

bool VulkanMemoryAllocator::suitableChunkExists(const uint32_t p_MemoryType, const VkDeviceSize p_Size) const
{
	for (const MemoryChunk& l_Chunk : m_MemoryChunks)
	{
		if (l_Chunk.m_MemoryType == p_MemoryType && l_Chunk.getBiggestChunkSize() >= p_Size)
		{
			return true;
		}
	}
	return false;
}

bool VulkanMemoryAllocator::isMemoryTypeHidden(const unsigned p_Value) const
{
	return m_HiddenTypes.contains(p_Value);
}

uint32_t VulkanMemoryAllocator::getChunkMemoryType(const uint32_t p_Chunk) const
{
	for (const MemoryChunk& l_MemoryChunk : m_MemoryChunks)
	{
		if (l_MemoryChunk.getID() == p_Chunk)
		{
			return l_MemoryChunk.getMemoryType();
		}
	}

    LOG_DEBUG("Chunk search failed out of ", m_MemoryChunks.size(), " chunks");
	throw std::runtime_error("Chunk (ID:" + std::to_string(p_Chunk) + ") not found");
}
