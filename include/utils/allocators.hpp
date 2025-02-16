#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

class TransientAllocator {
public:
    using value_type = uint8_t;

    TransientAllocator() = default;
    explicit TransientAllocator(size_t p_Size);
    explicit TransientAllocator(uint8_t* p_Container, size_t p_Size, bool p_ShouldDelete);

    ~TransientAllocator();

    void* allocate(size_t p_Bytes);
    void deallocate(void* p_Ptr, size_t p_SizeInBytes = 0) const;
    void reset() { m_StackPtr = m_StackBegin; }

    void initialize(size_t p_Size);
    void initialize(uint8_t* p_Container, size_t p_Size, bool p_ShouldDelete);

    [[nodiscard]] bool isInitialized() const { return m_StackBegin != nullptr; }

    std::string getVisualization(size_t p_BarSize) const;

private:
    uint8_t* m_StackBegin = nullptr;
    uint8_t* m_StackPtr = nullptr;
    uint8_t* m_StackEnd = nullptr;

    bool m_ShouldDelete = false;
};

// =====================
// == Arena Allocator ==
// =====================

static constexpr size_t alignUp(const size_t p_Value, const size_t p_Alignment)
{
    return (p_Value + (p_Alignment - 1)) & ~(p_Alignment - 1);
}

class ArenaAllocator {
    struct FreeHeader
    {
        FreeHeader* next = nullptr;
        size_t size = 0;
    };

    struct AllocHeader // Ideally kept at 8 bytes so it matches alignment
    {
        size_t size = 0;
    };

    struct Block
    {
        uint8_t* data = nullptr;
        FreeHeader* firstFree = nullptr;
    };
public:
    static constexpr size_t c_BlockCount = 10;
    static constexpr size_t c_MinFreeBlockSize = 32;
    static constexpr size_t c_Alignment = alignof(std::max_align_t);
    static_assert(c_Alignment % 8 == 0 && c_Alignment >= sizeof(AllocHeader) && "Alignment must be a multiple of 8 bytes and cannot be smaller than the size of AllocHeader");
    static_assert(c_MinFreeBlockSize% c_Alignment == 0 && "Minimum free block size must be a multiple of alignment");

    ArenaAllocator() = default;
    explicit ArenaAllocator(size_t p_BlockSize);

    ~ArenaAllocator();

    void* allocate(size_t p_Bytes);
    void deallocate(void* p_Ptr, size_t p_SizeInBytes = 0);
    void reset();

    void initialize(size_t p_Size);

    [[nodiscard]] bool isInitialized() const { return m_Blocks[0].data != nullptr; }

    std::string getVisualization(size_t p_BarSize) const;

private:
    uint8_t allocateBlock();
    void* allocateInFreeChunk(Block& l_Block, FreeHeader* p_PrevHeader, FreeHeader* p_FreeHeader, size_t p_Bytes) const;

    std::array<Block, c_BlockCount> m_Blocks{};
    size_t m_BlockSize = 0;
    uint8_t m_BlockIndex = 0;
};

template <typename Alloc, typename T>
class AllocHolder {
public:
    using value_type = std::remove_const_t<T>;

    AllocHolder() = default;
    explicit AllocHolder(Alloc* p_Pool) : m_Pool(p_Pool) {}

    template <typename U>
    explicit AllocHolder(const AllocHolder<Alloc, U>& other) : m_Pool(other.m_Pool) {}

    void set(Alloc* p_Pool) { m_Pool = p_Pool; }

    T* allocate(const size_t p_Size)
    {
        return static_cast<T*>(m_Pool->allocate(p_Size * sizeof(T)));
    }

    void deallocate(T* p_Ptr, const size_t p_Size) 
    {
        m_Pool->deallocate(reinterpret_cast<void*>(p_Ptr), p_Size * sizeof(T));
    }

    template <typename U, typename... Args>
    void construct(U* p_Ptr, Args&&... p_Args)
    {
        new (p_Ptr) U(std::forward<Args>(p_Args)...);
    }

    template <typename U>
    void destroy(U* p_Ptr)
    {
        p_Ptr->~U();
    }

    template <typename U>
    bool operator==(const AllocHolder<Alloc, U>& p_Other) const
    {
        return m_Pool == p_Other.m_Pool;
    }

    template <typename U>
    bool operator!=(const AllocHolder<Alloc, U>& p_Other) const
    {
        return !(*this == p_Other);
    }

    template <typename U>
    struct rebind { using other = AllocHolder<Alloc, U>; };

private:
    Alloc* m_Pool = nullptr;

    template <typename U, typename W>
    friend class AllocHolder;
};

template<typename T>
using ArenaAlloc = AllocHolder<ArenaAllocator, T>;

template <typename T>
using TransAlloc = AllocHolder<TransientAllocator, T>;

template<typename T, typename Q>
using arena_umap = std::unordered_map<T, Q, std::hash<uint32_t>, std::equal_to<>, ArenaAlloc<std::pair<const T, Q>>>;

template <typename T>
using arena_vector = std::vector<T, ArenaAlloc<T>>;

template<typename T, typename Q>
using trans_umap = std::unordered_map<T, Q, std::hash<uint32_t>, std::equal_to<>, TransAlloc<std::pair<const T, Q>>>;

template <typename T>
using trans_vector = std::vector<T, TransAlloc<T>>;