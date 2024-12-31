#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

struct VulkanExtensionElem
{
    VkStructureType    sType;
    void*              pNext;
};

class VulkanExtensionChain
{
public:
    ~VulkanExtensionChain()
    {
        for (const auto pNext : m_pNext)
        {
            free(pNext);
        }
    }

    template <typename T>
    VulkanExtensionChain& addExtension(const T& extension)
    {
        static_assert(std::is_trivially_destructible_v<T>, "T must have a trivial or no destructor!");
    
        T* pNext = static_cast<T*>(malloc(sizeof(T)));
        *pNext = extension;
        m_pNext.push_back(reinterpret_cast<VulkanExtensionElem*>(pNext));
        return *this;
    }

    [[nodiscard]] void* getChain() const
    {
        VulkanExtensionElem* pNext = nullptr;
        for (VulkanExtensionElem* const pNextElem : m_pNext)
        {
            if (pNext != nullptr)
                pNext->pNext = pNextElem;
            pNext = pNextElem;
        }
        return pNext;
    }

private:
    std::vector<VulkanExtensionElem*> m_pNext;
};

