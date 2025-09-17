#pragma once
#include <vector>
#if _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <Volk/volk.h>

#include "vulkan_gpu.hpp"
#include "utils/identifiable.hpp"
#include "utils/string_utils.hpp"

class VulkanExtensionChain
{
public:
    ~VulkanExtensionChain();

    template <typename T>
    VulkanExtensionChain& addExtension(const T& p_Extension);

    [[nodiscard]] void* getChain() const;
    [[nodiscard]] bool containsExtensionStruct(VkStructureType p_StructType) const;

private:
    [[nodiscard]] void* allocFromContext(size_t p_Bytes) const;

    // TAKES OWNERSHIP OF POINTER
    VulkanExtensionChain& addExtensionPointer(const VkBaseInStructure* p_Next);

    std::vector<VkBaseInStructure*> m_Next;

    friend class VulkanDeviceExtensionManager;
};

using ExtensionID = uint32_t;

class VulkanDeviceExtension
{
public:
    virtual ~VulkanDeviceExtension() = default;
    [[nodiscard]] ExtensionID getExtensionID() const { return m_ExtensionID; }
    [[nodiscard]] ResourceID getDeviceID() const { return m_DeviceID; }

    [[nodiscard]] virtual VkBaseInStructure* getExtensionStruct() const = 0;
    [[nodiscard]] virtual VkStructureType getExtensionStructType() const = 0;

    virtual void free() = 0;
    void setDevice(const ResourceID p_DeviceID) { m_DeviceID = p_DeviceID; }

    virtual std::string getMainExtensionName() = 0;
    virtual std::vector<std::string> getExtraExtensionNames() { return {}; };

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

    void setGPU(const VulkanGPU p_GPU) { m_GPU = p_GPU; }

    void addExtensionsToChain(VulkanExtensionChain& p_Chain) const;
    void addExtension(VulkanDeviceExtension* p_Extension, bool p_ForceReplace = false);

    [[nodiscard]] VulkanDeviceExtension* getExtension(const std::string& p_ExtensionName) const;
    [[nodiscard]] bool containsExtension(std::string_view p_ExtensionName) const;
    [[nodiscard]] bool isEmpty() const { return m_Extensions.empty(); }
    [[nodiscard]] size_t getExtensionCount() const { return m_Extensions.size(); }
    [[nodiscard]] bool isValid() const { return m_DeviceID != UINT32_MAX; }
    void populateExtensionNames(const char* p_Container[]) const;

    template <typename T>
    T* getExtension(const std::string& p_ExtensionName);

    void freeExtension(const std::string& p_Extension);
    void freeExtensions();

private:
    void addExtension(const std::string& p_Name, VulkanDeviceExtension* p_Extension, bool p_ForceReplace = false);
    void setDevice(ResourceID p_Device);

    StringUMap<VulkanDeviceExtension*> m_Extensions{};

    VulkanGPU m_GPU;
    ResourceID m_DeviceID = UINT32_MAX;

    friend class VulkanDevice;
};


template <typename T>
VulkanExtensionChain& VulkanExtensionChain::addExtension(const T& p_Extension)
{
    static_assert(std::is_trivially_destructible_v<T>, "T must have a trivial or no destructor!");

    T* pNext = new(allocFromContext(sizeof(T))) T(p_Extension);
    m_Next.push_back(reinterpret_cast<VkBaseInStructure*>(pNext));
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
