#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "utils/identifiable.hpp"

struct VulkanExtensionElem
{
    VkStructureType    sType;
    void*              pNext;
};

class VulkanExtensionChain
{
public:
    ~VulkanExtensionChain();

    template <typename T>
    VulkanExtensionChain& addExtension(const T& extension);

    [[nodiscard]] void* getChain() const;
    [[nodiscard]] bool containsExtensionStruct(VkStructureType p_StructType) const;

private:
    // TRANSFERS OWNERSHIP OF POINTER
    VulkanExtensionChain& addExtensionPointer(const VulkanExtensionElem* pNext);

    std::vector<VulkanExtensionElem*> m_pNext;

    friend class VulkanDeviceExtensionManager;
};

typedef uint32_t ExtensionID;

class VulkanDeviceExtension
{
public:
    [[nodiscard]] ExtensionID getExtensionID() const { return m_ExtensionID; }
    [[nodiscard]] ResourceID getDeviceID() const { return m_DeviceID; }

    [[nodiscard]] virtual VulkanExtensionElem* getExtensionStruct() const = 0;
    [[nodiscard]] virtual VkStructureType getExtensionStructType() const = 0;

    virtual void free() = 0;
    void setDevice(const uint32_t p_DeviceID) { m_DeviceID = p_DeviceID; }

protected:
    explicit VulkanDeviceExtension(const ResourceID p_DeviceID) : m_DeviceID(p_DeviceID) {}

private:

    ResourceID m_DeviceID;
    ExtensionID m_ExtensionID = s_ExtensionCounter++;

    inline static ExtensionID s_ExtensionCounter = 0;

    friend class VulkanDeviceExtensionManager;
};

class VulkanDeviceExtensionManager
{
public:
    explicit VulkanDeviceExtensionManager() = default;
    VulkanDeviceExtensionManager(VulkanDeviceExtensionManager&& p_Other) noexcept;
    VulkanDeviceExtensionManager(const VulkanDeviceExtensionManager& p_Other);

    void addExtensionsToChain(VulkanExtensionChain& p_Chain) const;
    void addExtension(const std::string& p_ExtensionName, VulkanDeviceExtension* p_Extension, bool p_ForceReplace = false);

    [[nodiscard]] VulkanDeviceExtension* getExtension(const std::string& p_ExtensionName) const;
    [[nodiscard]] bool containsExtension(const std::string& p_ExtensionName) const;
    [[nodiscard]] bool isEmpty() const { return m_Extensions.empty(); }
    [[nodiscard]] size_t getExtensionCount() const { return m_Extensions.size(); }
    [[nodiscard]] bool isValid() const { return m_DeviceID != -1; }
    void populateExtensionNames(std::vector<const char*>& p_Container) const;

    template <typename T>
    T* getExtension(const std::string& p_ExtensionName);

    void freeExtension(const std::string& p_Extension);
    void freeExtensions();

private:
    void setDevice(const ResourceID p_Device);

    std::unordered_map<std::string, VulkanDeviceExtension*> m_Extensions{};

    ResourceID m_DeviceID = -1;

    friend class VulkanDevice;
};




template <typename T>
VulkanExtensionChain& VulkanExtensionChain::addExtension(const T& extension)
{
    static_assert(std::is_trivially_destructible_v<T>, "T must have a trivial or no destructor!");
    
    T* pNext = new T(extension);
    m_pNext.push_back(reinterpret_cast<VulkanExtensionElem*>(pNext));
    return *this;
}

template <typename T>
T* VulkanDeviceExtensionManager::getExtension(const std::string& p_ExtensionName)
{
    if (m_Extensions.contains(p_ExtensionName) && dynamic_cast<T*>(m_Extensions[p_ExtensionName]))
    {
        return dynamic_cast<T*>(m_Extensions[p_ExtensionName]);
    }
    return nullptr;
}