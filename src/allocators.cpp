#include "utils/allocators.hpp"

#include <stdexcept>
#include <utility>


TransientAllocator::TransientAllocator(const size_t p_Size)
{
    if (p_Size > 0)
    {
        m_StackBegin = new uint8_t[p_Size];
        m_StackPtr = m_StackBegin;
        m_StackEnd = m_StackBegin + p_Size;
    }
    m_ShouldDelete = true;
}

TransientAllocator::TransientAllocator(uint8_t* p_Container, const size_t p_Size, const bool p_ShouldDelete)
{
    if (p_Size == 0)
    {
        return;
    }
    m_StackBegin = p_Container;
    m_StackPtr = m_StackBegin;
    m_StackEnd = m_StackBegin + p_Size;
    m_ShouldDelete = p_ShouldDelete;
}

TransientAllocator::~TransientAllocator()
{
    if (m_ShouldDelete)
    {
        delete[] m_StackBegin;
    }
}

void* TransientAllocator::allocate(const size_t p_Bytes)
{
    if (!m_StackBegin || m_StackPtr + p_Bytes > m_StackEnd)
    {
        return operator new(p_Bytes);
    }

    void* l_Ptr = m_StackPtr;
    m_StackPtr += p_Bytes;
    return l_Ptr;
}

void TransientAllocator::deallocate(void* p_Ptr, const size_t p_SizeInBytes) const
{
    if (p_Ptr < m_StackBegin || p_Ptr >= m_StackEnd)
    {
        operator delete(p_Ptr);
    }
}

void TransientAllocator::initialize(const size_t p_Size)
{
    if (p_Size == 0)
    {
        throw std::runtime_error("Cannot initialize ArenaAllocator with size 0");
    }
    if (m_StackBegin)
    {
        throw std::runtime_error("ArenaAllocator already initialized");
    }

    new(this) TransientAllocator(p_Size);
}

void TransientAllocator::initialize(uint8_t* p_Container, const size_t p_Size, const bool p_ShouldDelete)
{
    if (p_Size == 0)
    {
        throw std::runtime_error("Cannot initialize ArenaAllocator with size 0");
    }
    if (!p_Container)
    {
        throw std::runtime_error("Cannot initialize ArenaAllocator with nullptr");
    }
    if (m_StackBegin)
    {
        throw std::runtime_error("ArenaAllocator already initialized");
    }

    new(this) TransientAllocator(p_Container, p_Size, p_ShouldDelete);
}

std::string TransientAllocator::getVisualization(const size_t p_BarSize) const
{
    std::string l_Visualization = "TransientAllocator visualization:\n|";
    const size_t l_StackSize = m_StackEnd - m_StackBegin;
    const float l_Step = static_cast<float>(l_StackSize) / static_cast<float>(p_BarSize);
    float l_Offset = 0.f;
    while (static_cast<size_t>(l_Offset) < l_StackSize)
    {
        const size_t l_IntOffset = l_Offset;
        if (m_StackBegin + l_IntOffset <= m_StackPtr)
        {
            l_Visualization += "#";
        }
        else
        {
            l_Visualization += "-";
        }
        l_Offset += l_Step;
    }
    l_Visualization += "|\n";
    return l_Visualization;
}

ArenaAllocator::ArenaAllocator(const size_t p_BlockSize)
    : m_BlockSize(p_BlockSize)
{
    if (p_BlockSize == 0)
    {
        return;
    }
    allocateBlock();
}

ArenaAllocator::~ArenaAllocator()
{
    for (Block& l_Block : m_Blocks)
    {
        delete[] l_Block.data;
        l_Block.data = nullptr;
    }
}

// TODO: Optimize this function, this is horrible (low priority since it's only for debugging)
std::string ArenaAllocator::getVisualization(const size_t p_BarSize) const
{
    std::string l_Visualization = "ArenaAllocator visualization:\n";
    const float l_Step = static_cast<float>(m_BlockSize) / static_cast<float>(p_BarSize);
    for (const Block& l_Block : m_Blocks)
    {
        l_Visualization += "|";
        if (l_Block.data == nullptr)
        {
            l_Visualization += "null";
        }
        else
        {
            float l_Offset = 0.f;
            while (static_cast<size_t>(l_Offset) < m_BlockSize)
            {
                const size_t l_IntOffset = l_Offset;
                FreeHeader* l_FreeHeader = l_Block.firstFree;
                bool l_Allocated = true;
                while (l_FreeHeader != nullptr)
                {
                    const uint8_t* l_Ptr = reinterpret_cast<uint8_t*>(l_FreeHeader);
                    const uint8_t* l_EndPtr = l_Ptr + l_FreeHeader->size;
                    if (l_Ptr <= l_Block.data + l_IntOffset && l_EndPtr > l_Block.data + l_IntOffset)
                    {
                        l_Allocated = false;
                        break;
                    }
                    l_FreeHeader = l_FreeHeader->next;
                }
                if (l_Allocated)
                {
                    l_Visualization += "#";
                }
                else
                {
                    l_Visualization += "-";
                }
                l_Offset += l_Step;
            }
        }
        l_Visualization += "|\n";
    }
    return l_Visualization;
}

uint8_t ArenaAllocator::allocateBlock()
{
    if (m_BlockIndex >= m_Blocks.size())
    {
        throw std::runtime_error("Cannot allocate more blocks");
    }

    Block& l_Block = m_Blocks[m_BlockIndex];
    l_Block.data = new uint8_t[m_BlockSize];
    l_Block.firstFree = reinterpret_cast<FreeHeader*>(l_Block.data);
    l_Block.firstFree->size = m_BlockSize;
    l_Block.firstFree->next = nullptr;
    return m_BlockIndex++;
}

void* ArenaAllocator::allocateInFreeChunk(Block& p_Block, FreeHeader* p_PrevHeader, FreeHeader* p_FreeHeader, const size_t p_Bytes) const
{
    const size_t l_NeededSize = std::max(alignUp(p_Bytes, ALIGNMENT) + ALIGNMENT, MIN_FREE_BLOCK_SIZE);
    const int64_t l_NewSize = p_FreeHeader->size - l_NeededSize;
    if (l_NewSize < 0)
    {
        return nullptr;
    }
    if (std::cmp_less(l_NewSize, MIN_FREE_BLOCK_SIZE))
    {
        if (p_PrevHeader)
        {
            p_PrevHeader->next = p_FreeHeader->next;
        }
        else
        {
            p_Block.firstFree = p_FreeHeader->next;
        }
        return p_FreeHeader;
    }

    p_FreeHeader->size = l_NewSize;
    uint8_t* l_Ptr = reinterpret_cast<uint8_t*>(p_FreeHeader) + p_FreeHeader->size;
    auto l_AllocHeader = reinterpret_cast<AllocHeader*>(l_Ptr);
    l_AllocHeader->size = l_NeededSize;
    return l_Ptr + ALIGNMENT;
}

void* ArenaAllocator::allocate(const size_t p_Bytes)
{
    if (p_Bytes == 0)
    {
        return nullptr;
    }

    if (p_Bytes > m_BlockSize)
    {
        return operator new(p_Bytes);
    }

    for (Block& l_Block : m_Blocks)
    {
        if (l_Block.data == nullptr)
        {
            allocateBlock();
            return allocateInFreeChunk(l_Block, nullptr, l_Block.firstFree, p_Bytes);
        }

        FreeHeader* l_PrevHeader = nullptr;
        FreeHeader* l_FreeHeader = l_Block.firstFree;
        while (l_FreeHeader != nullptr)
        {
            void* l_Ptr = allocateInFreeChunk(l_Block, l_PrevHeader, l_FreeHeader, p_Bytes);

            if (l_Ptr)
            {
                return l_Ptr;
            }

            l_PrevHeader = l_FreeHeader;
            l_FreeHeader = l_FreeHeader->next;
        }
    }

    return operator new(p_Bytes);
}

void ArenaAllocator::deallocate(void* p_Ptr, const size_t p_SizeInBytes)
{
    Block* l_Container = nullptr;
    for (Block& l_Block : m_Blocks)
    {
        if (p_Ptr >= l_Block.data && p_Ptr < l_Block.data + m_BlockSize)
        {
            l_Container = &l_Block;
            break;
        }
    }

    if (!l_Container)
    {
        if (p_SizeInBytes == 0)
        {
            operator delete(p_Ptr);
        }
        else
        {
            operator delete(p_Ptr, p_SizeInBytes);
        }
        return;
    }

    auto l_AllocHeader = reinterpret_cast<AllocHeader*>(static_cast<uint8_t*>(p_Ptr) - ALIGNMENT);
    auto l_BeginPtr = reinterpret_cast<uint8_t*>(l_AllocHeader);
    const uint8_t* l_EndPtr = static_cast<uint8_t*>(p_Ptr) + l_AllocHeader->size;
    FreeHeader* l_PrevHeader = nullptr;
    FreeHeader* l_FreeHeader = l_Container->firstFree;
    while (l_FreeHeader != nullptr)
    {
        const uint8_t* l_FreeHeadPtr = reinterpret_cast<uint8_t*>(l_FreeHeader);
        const uint8_t* l_FreeEndPtr = l_FreeHeadPtr + l_FreeHeader->size;
        if (l_FreeEndPtr < l_BeginPtr)
        {
            l_PrevHeader = l_FreeHeader;
            l_FreeHeader = l_FreeHeader->next;
            continue;
        }
        if (l_FreeEndPtr == l_BeginPtr)
        {
            l_FreeHeader->size += l_AllocHeader->size;
            if (l_FreeHeader->next != nullptr && l_FreeHeadPtr + l_FreeHeader->size == reinterpret_cast<uint8_t*>(l_FreeHeader->next))
            {
                l_FreeHeader->size += l_FreeHeader->next->size;
                l_FreeHeader->next = l_FreeHeader->next->next;
            }
            return;
        }
        if (l_FreeHeadPtr > l_EndPtr)
        {
            break;
        }
        if (l_FreeHeadPtr == l_EndPtr)
        {
            auto l_CreatedHeader = reinterpret_cast<FreeHeader*>(l_BeginPtr);
            l_CreatedHeader->next = l_FreeHeader->next;
            l_CreatedHeader->size = l_AllocHeader->size + l_FreeHeader->size;

            if (l_PrevHeader)
            {
                l_PrevHeader->next = l_CreatedHeader;
            }
            else
            {
                l_Container->firstFree = l_CreatedHeader;
            }
            return;
        }

        l_PrevHeader = l_FreeHeader;
        l_FreeHeader = l_FreeHeader->next;
    }

    auto l_CreatedHeader = reinterpret_cast<FreeHeader*>(l_BeginPtr);
    l_CreatedHeader->size = l_AllocHeader->size;
    l_CreatedHeader->next = l_FreeHeader;
    if (l_PrevHeader)
    {
        l_PrevHeader->next = l_CreatedHeader;
    }
    else
    {
        l_Container->firstFree = l_CreatedHeader;
    }
}

void ArenaAllocator::reset()
{
    this->~ArenaAllocator();
    new(this) ArenaAllocator(m_BlockSize);
}

void ArenaAllocator::initialize(const size_t p_Size)
{
    if (p_Size == 0)
    {
        throw std::runtime_error("Cannot initialize ArenaAllocator with size 0");
    }
    if (isInitialized())
    {
        throw std::runtime_error("ArenaAllocator already initialized");
    }

    new(this) ArenaAllocator(p_Size);
}
