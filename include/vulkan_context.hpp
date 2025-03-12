#pragma once
#include <Volk/volk.h>
#include <vector>

#include "vulkan_queues.hpp"
#include "utils/allocators.hpp"
#include "utils/identifiable.hpp"

#define ARENA_VECTOR(name, T) arena_vector<T> ##name{ArenaAlloc<T>(VulkanContext::getArenaAllocator())}

#define TRANS_VECTOR(name, T) trans_vector<T> ##name{TransAlloc<T>(VulkanContext::getTransAllocator())}

#define ARENA_UMAP(name, K, V) arena_umap<K, V> ##name{ArenaAlloc<std::pair<const K, V>>(VulkanContext::getArenaAllocator())}

#define TRANS_UMAP(name, K, V) trans_umap<K, V> ##name{TransAlloc<std::pair<const K, V>>(VulkanContext::getTransAllocator())}

#define ARENA_USET(name, T) arena_uset<T> ##name{ArenaAlloc<T>(VulkanContext::getArenaAllocator())}

#define TRANS_USET(name, T) trans_uset<T> ##name{TransAlloc<T>(VulkanContext::getTransAllocator())}

#define ARENA_ALLOC(cls) new (VulkanContext::getArenaAllocator()->allocate(sizeof(cls))) cls

#define TRANS_ALLOC(cls) new (VulkanContext::getTransAllocator()->allocate(sizeof(cls))) cls

#define ARENA_FREE(ptr, size) VulkanContext::getArenaAllocator()->deallocate(ptr, size)
#define ARENA_FREE_NOSIZE(ptr) VulkanContext::getArenaAllocator()->deallocate(ptr)

#define TRANS_FREE(ptr, size) VulkanContext::getTransAllocator()->deallocate(ptr, size)
#define TRANS_FREE_NOSIZE(ptr) VulkanContext::getTransAllocator()->deallocate(ptr)

class VulkanGPU;
class VulkanDeviceExtensionManager;
class VulkanDevice;

class VulkanContext : public Identifiable
{
public:

    static void init(uint32_t p_VulkanApiVersion, bool p_EnableValidationLayers, bool p_AssertOnError, std::span<const char*> p_Extensions);

    static void initializeTransientMemory(size_t p_Size);
    static void initializeTransientMemory(uint8_t* p_Container, size_t p_Size, bool p_ShouldDelete);
    static void initializeArenaMemory(size_t p_Size);

	static [[nodiscard]] uint32_t getGPUCount();
	static [[nodiscard]] void getGPUs(VulkanGPU p_Container[]);

	static ResourceID createDevice(VulkanGPU p_GPU, const QueueFamilySelector& p_Queues, const VulkanDeviceExtensionManager* p_Extensions, const VkPhysicalDeviceFeatures& p_Features);
	static VulkanDevice& getDevice(ResourceID p_Index);
	static void freeDevice(ResourceID p_Index);
	static void freeDevice(const VulkanDevice& p_Device);

	static void free();

	static VkInstance getHandle();

    static TransientAllocator* getTransAllocator() { return &m_TransientAllocator; }
    static ArenaAllocator* getArenaAllocator() { return &m_ArenaAllocator; }

    static void resetTransMemory();
    static void resetArenaMemory();

private:
	static bool checkValidationLayerSupport();
	static bool areExtensionsSupported(std::span<const char*> p_Extensions);
    static void setupDebugMessenger();

	inline static VkInstance m_VkHandle = VK_NULL_HANDLE;
	inline static bool m_ValidationLayersEnabled = false;

    inline static TransientAllocator m_TransientAllocator{0};
    inline static ArenaAllocator m_ArenaAllocator{0};

	inline static ARENA_VECTOR(m_Devices, VulkanDevice*);

    inline static VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

	friend class SDLWindow;
};

