#pragma once
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

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

    [[nodiscard]] std::optional<uint32_t> getStagingMemoryType(uint32_t p_TypeFilter) const;
    [[nodiscard]] std::vector<uint32_t> getMemoryTypes(VkMemoryPropertyFlags p_Properties, uint32_t p_TypeFilter) const;
    [[nodiscard]] bool doesMemoryContainProperties(uint32_t p_Type, VkMemoryPropertyFlags p_Property) const;
    [[nodiscard]] MemoryTypeData getTypeData(uint32_t p_MemoryType) const;
    [[nodiscard]] VkMemoryHeap getMemoryTypeHeap(uint32_t p_MemoryType) const;

private:
    explicit MemoryStructure(const VulkanGPU p_GPU) : m_GPU(p_GPU) {}

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

    MemoryBlock allocate(VkDeviceSize p_NewSize, VkDeviceSize p_Alignment);
    void deallocate(const MemoryBlock& p_Block);

    VkDeviceMemory operator*() const;

private:
    MemoryChunk(VkDeviceSize p_Size, uint32_t p_MemoryType, VkDeviceMemory p_VkHandle);

    void defragment();

    VkDeviceSize m_Size;
    uint32_t m_MemoryType;

    VkDeviceMemory m_Memory;

    std::map<VkDeviceSize, VkDeviceSize> m_UnallocatedData;

    // Metadata
    VkDeviceSize m_UnallocatedSize;
    VkDeviceSize m_BiggestChunk = 0;

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

    uint32_t search(VkDeviceSize p_Size, VkDeviceSize p_Alignment, MemoryPropertyPreferences p_Properties, uint32_t p_TypeFilter, bool p_IncludeHidden = false);

    MemoryChunk::MemoryBlock allocate(VkDeviceSize p_Size, VkDeviceSize p_Alignment, uint32_t p_MemoryType);
    MemoryChunk::MemoryBlock allocateIsolated(VkDeviceSize p_Size, uint32_t p_MemoryType, const void* p_Next);
    MemoryChunk::MemoryBlock searchAndAllocate(VkDeviceSize p_Size, VkDeviceSize p_Alignment, MemoryPropertyPreferences p_Properties, uint32_t p_TypeFilter, bool p_IncludeHidden = false);

    void deallocate(const MemoryChunk::MemoryBlock& p_Block);

    void hideMemoryType(uint32_t p_Type);
    void unhideMemoryType(uint32_t p_Type);

    [[nodiscard]] const MemoryStructure& getMemoryStructure() const;
    [[nodiscard]] VkDeviceSize getRemainingSize(uint32_t p_Heap) const;
    [[nodiscard]] bool suitableChunkExists(uint32_t p_MemoryType, VkDeviceSize p_Size) const;
    [[nodiscard]] bool isMemoryTypeHidden(uint32_t p_Value) const;

    [[nodiscard]] uint32_t getChunkMemoryType(uint32_t p_Chunk) const;
    [[nodiscard]] VkDeviceMemory getChunkMemoryHandle(uint32_t p_Chunk) const;

    static std::string compactBytes(VkDeviceSize p_Bytes);

private:
    void free();

    explicit VulkanMemoryAllocator(const VulkanDevice& p_Device, VkDeviceSize p_DefaultChunkSize = 256LL * 1024 * 1024);

    MemoryStructure m_MemoryStructure;
    VkDeviceSize m_ChunkSize;

    std::vector<MemoryChunk> m_MemoryChunks;
    std::set<uint32_t> m_HiddenTypes;

    uint32_t m_Device;

    friend class VulkanDevice;
};
