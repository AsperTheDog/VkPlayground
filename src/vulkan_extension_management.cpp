#include <ext/vulkan_extension_management.hpp>

VulkanExtensionChain::~VulkanExtensionChain()
{
    for (const auto pNext : m_pNext)
    {
        free(pNext);
    }
}

VulkanExtensionChain& VulkanExtensionChain::addExtensionPointer(const VulkanExtensionElem* pNext)
{
    m_pNext.push_back(const_cast<VulkanExtensionElem*>(pNext));
    return *this;
}

void* VulkanExtensionChain::getChain() const
{
    VulkanExtensionElem* pNext = nullptr;
    for (VulkanExtensionElem* const pNextElem : m_pNext)
    {
        if (pNext != nullptr)
        {
            pNext->pNext = pNextElem;
        }
        pNext = pNextElem;
    }
    return pNext;
}

bool VulkanExtensionChain::containsExtensionStruct(const VkStructureType p_StructType) const
{
    for (const VulkanExtensionElem* pNext : m_pNext)
    {
        if (pNext->sType == p_StructType)
        {
            return true;
        }
    }
    return false;
}

void VulkanDeviceExtensionManager::addExtensionsToChain(VulkanExtensionChain& p_Chain) const
{
    for (const std::pair<const std::string, VulkanDeviceExtension*>& l_Extension : m_Extensions)
    {
        if (p_Chain.containsExtensionStruct(l_Extension.second->getExtensionStructType()))
        {
            continue;
        }
        VulkanExtensionElem* l_Struct = l_Extension.second->getExtensionStruct();
        p_Chain.addExtensionPointer(l_Extension.second->getExtensionStruct());
    }
}

void VulkanDeviceExtensionManager::addExtension(const std::string& p_ExtensionName, VulkanDeviceExtension* p_Extension, const bool p_ForceReplace)
{
    if (m_Extensions.contains(p_ExtensionName))
    {
        if (p_ForceReplace)
        {
            m_Extensions.erase(p_ExtensionName);
        }
        else
        {
            return;
        }
    }
    if (p_Extension == nullptr)
    {
        return;
    }

    m_Extensions[p_ExtensionName] = p_Extension;
}

VulkanDeviceExtension* VulkanDeviceExtensionManager::getExtension(const std::string& p_ExtensionName) const
{
    if (m_Extensions.contains(p_ExtensionName))
    {
        return m_Extensions.at(p_ExtensionName);
    }
    return nullptr;
}
