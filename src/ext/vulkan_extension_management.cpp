#include <ranges>
#include <ext/vulkan_extension_management.hpp>

VulkanExtensionChain::~VulkanExtensionChain()
{
    for (VkBaseInStructure* l_Next : m_Next)
    {
        free(l_Next);
    }
}

VulkanExtensionChain& VulkanExtensionChain::addExtensionPointer(const VkBaseInStructure* p_Next)
{
    m_Next.push_back(const_cast<VkBaseInStructure*>(p_Next));
    return *this;
}

void* VulkanExtensionChain::getChain() const
{
    VkBaseInStructure* l_Next = nullptr;
    VkBaseInStructure* l_First = nullptr;
    for (VkBaseInStructure* const l_NextElem : m_Next)
    {
        if (l_Next != nullptr)
        {
            l_Next->pNext = l_NextElem;
        }
        else
        {
            l_First = l_NextElem;
        }
        l_Next = l_NextElem;
    }
    return l_First;
}

bool VulkanExtensionChain::containsExtensionStruct(const VkStructureType p_StructType) const
{
    for (const VkBaseInStructure* l_Next : m_Next)
    {
        if (l_Next->sType == p_StructType)
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
    for (const VulkanDeviceExtension* l_Extension : m_Extensions | std::views::values)
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

void VulkanDeviceExtensionManager::populateExtensionNames(std::vector<const char*>& p_Container) const
{
    p_Container.reserve(m_Extensions.size());
    for (const std::string& l_Extension : m_Extensions | std::views::keys)
    {
        p_Container.push_back(l_Extension.c_str());
    }
}

void VulkanDeviceExtensionManager::freeExtension(const std::string& p_Extension)
{
    if (m_Extensions.contains(p_Extension))
    {
        m_Extensions.at(p_Extension)->free();
        delete m_Extensions.at(p_Extension);
        m_Extensions.erase(p_Extension);
    }
}

void VulkanDeviceExtensionManager::freeExtensions()
{
    for (VulkanDeviceExtension* l_Extension : m_Extensions | std::views::values)
    {
        l_Extension->free();
        delete l_Extension;
    }
    m_Extensions.clear();
}

void VulkanDeviceExtensionManager::setDevice(const ResourceID p_Device)
{
    m_DeviceID = p_Device;
    for (VulkanDeviceExtension* l_Extension : m_Extensions | std::views::values)
    {
        l_Extension->setDevice(p_Device);
    }
}
