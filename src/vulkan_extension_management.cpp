#include <ranges>
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

VulkanDeviceExtensionManager::VulkanDeviceExtensionManager(VulkanDeviceExtensionManager&& p_Other) noexcept
{
    m_DeviceID = p_Other.m_DeviceID;
    m_Extensions = std::move(p_Other.m_Extensions);
    p_Other.m_DeviceID = 0;
    p_Other.m_Extensions.clear();
}

VulkanDeviceExtensionManager::VulkanDeviceExtensionManager(const VulkanDeviceExtensionManager& p_Other)
{
    m_DeviceID = p_Other.m_DeviceID;
    m_Extensions = p_Other.m_Extensions;
}

void VulkanDeviceExtensionManager::addExtensionsToChain(VulkanExtensionChain& p_Chain) const
{
    for (const auto& l_Extension : m_Extensions | std::views::values)
    {
        if (l_Extension->getExtensionStructType() == VK_STRUCTURE_TYPE_MAX_ENUM || p_Chain.containsExtensionStruct(l_Extension->getExtensionStructType()))
        {
            continue;
        }
        p_Chain.addExtensionPointer(l_Extension->getExtensionStruct());
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

bool VulkanDeviceExtensionManager::containsExtension(const std::string& p_ExtensionName) const
{
    return m_Extensions.contains(p_ExtensionName);
}

std::vector<const char*> VulkanDeviceExtensionManager::getExtensionNames() const
{
    std::vector<const char*> l_ExtensionNames;
    l_ExtensionNames.reserve(m_Extensions.size());
    for (const auto& l_Extension : m_Extensions | std::views::keys)
    {
        l_ExtensionNames.push_back(l_Extension.c_str());
    }
    return l_ExtensionNames;
}

void VulkanDeviceExtensionManager::freeExtensions()
{
    for (const auto& l_Extension : m_Extensions | std::views::values)
    {
        l_Extension->free();
        delete l_Extension;
    }
    m_Extensions.clear();
}
