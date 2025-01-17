#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan_gpu.hpp"
#include "utils/identifiable.hpp"

class VulkanGPU;
class VulkanDevice;

class MemoryStructure
{
public:
    struct MemoryTypeData
    {
        VkMemoryPropertyFlags properties;
        uint32_t heapIndex;
        VkDeviceSize heapSize;
    };

    [[nodiscard]] VkPhysicalDeviceMemoryProperties getMemoryProperties() const;

	[[nodiscard]] std::string toString() const;

	[[nodiscard]] std::optional<uint32_t> getStagingMemoryType(uint32_t typeFilter) const;
	[[nodiscard]] std::vector<uint32_t> getMemoryTypes(VkMemoryPropertyFlags properties, uint32_t typeFilter) const;
	[[nodiscard]] bool doesMemoryContainProperties(uint32_t type, VkMemoryPropertyFlags property) const;
    [[nodiscard]] MemoryTypeData getTypeData(uint32_t memoryType) const;
    [[nodiscard]] VkMemoryHeap getMemoryTypeHeap(uint32_t memoryType) const;

private:
    explicit MemoryStructure(const VulkanGPU gpu) : m_GPU(gpu) {}

    VulkanGPU m_GPU;

	friend class VulkanMemoryAllocator;
};

class MemoryChunk : public Identifiable
{
public:
	struct MemoryBlock
	{
		VkDeviceSize size = 0;
		VkDeviceSize offset = 0;
		uint32_t chunk = 0;
	};

	[[nodiscard]] VkDeviceSize getSize() const;
	[[nodiscard]] uint32_t getMemoryType() const;
	[[nodiscard]] bool isEmpty() const;
	[[nodiscard]] VkDeviceSize getBiggestChunkSize() const;
	[[nodiscard]] VkDeviceSize getRemainingSize() const;

	MemoryBlock allocate(VkDeviceSize newSize, VkDeviceSize alignment);
	void deallocate(const MemoryBlock& block);

	VkDeviceMemory operator*() const;

private:
	MemoryChunk(VkDeviceSize size, uint32_t memoryType, VkDeviceMemory vkHandle);

	void defragment();

	VkDeviceSize m_size;
	uint32_t m_memoryType;

	VkDeviceMemory m_memory;

	std::map<VkDeviceSize, VkDeviceSize> m_unallocatedData;

	// Metadata
	VkDeviceSize m_unallocatedSize;
	VkDeviceSize m_biggestChunk = 0;

	friend class VulkanResource;
	friend class VulkanMemoryAllocator;
	friend class VulkanDevice;
};

class VulkanMemoryAllocator
{
public:
	struct MemoryPropertyPreferences
	{
		VkMemoryPropertyFlags desiredProperties;
		VkMemoryPropertyFlags undesiredProperties;
		bool allowUndesired;
	};

	MemoryChunk::MemoryBlock allocate(VkDeviceSize size, VkDeviceSize alignment, uint32_t memoryType);
	MemoryChunk::MemoryBlock searchAndAllocate(VkDeviceSize size, VkDeviceSize alignment, MemoryPropertyPreferences properties, uint32_t typeFilter, bool includeHidden = false);
	void deallocate(const MemoryChunk::MemoryBlock& block);

	void hideMemoryType(uint32_t type);
	void unhideMemoryType(uint32_t type);

	[[nodiscard]] const MemoryStructure& getMemoryStructure() const;
	[[nodiscard]] VkDeviceSize getRemainingSize(uint32_t heap) const;
	[[nodiscard]] bool suitableChunkExists(uint32_t memoryType, VkDeviceSize size) const;
	[[nodiscard]] bool isMemoryTypeHidden(unsigned value) const;

	[[nodiscard]] uint32_t getChunkMemoryType(uint32_t chunk) const;
    
    static std::string compactBytes(VkDeviceSize bytes);
private:
	void free();

	explicit VulkanMemoryAllocator(const VulkanDevice& device, VkDeviceSize defaultChunkSize = 256LL * 1024 * 1024);

	MemoryStructure m_memoryStructure;
	VkDeviceSize m_chunkSize;

	std::vector<MemoryChunk> m_memoryChunks;
	std::set<uint32_t> m_hiddenTypes;

	uint32_t m_device;

	friend class VulkanDevice;
};