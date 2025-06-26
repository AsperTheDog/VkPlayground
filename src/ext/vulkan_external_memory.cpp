#include "ext/vulkan_external_memory.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_device.hpp"
#include "utils/logger.hpp"

VulkanExternalMemoryExtension* VulkanExternalMemoryExtension::get(const VulkanDevice& p_Device)
{
    return p_Device.getExtensionManager()->getExtension<VulkanExternalMemoryExtension>(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
}

VulkanExternalMemoryExtension* VulkanExternalMemoryExtension::get(const ResourceID p_DeviceID)
{
    return VulkanContext::getDevice(p_DeviceID).getExtensionManager()->getExtension<VulkanExternalMemoryExtension>(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
}

VulkanExternalMemoryExtension::VulkanExternalMemoryExtension(const ResourceID p_DeviceID)
    : VulkanDeviceExtension(p_DeviceID)
{
}

ResourceID VulkanExternalMemoryExtension::createExternalImage(const VkImageType p_Type, const VkFormat p_Format, const VkExtent3D p_Extent, const VkImageUsageFlags p_Usage, const VkImageCreateFlags p_Flags, VkImageTiling p_Tiling) const
{
    VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

    VkImageCreateInfo l_ImageInfo{};
	l_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	l_ImageInfo.imageType = p_Type;
	l_ImageInfo.format = p_Format;
	l_ImageInfo.extent = p_Extent;
	l_ImageInfo.mipLevels = 1;
	l_ImageInfo.arrayLayers = 1;
	l_ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	l_ImageInfo.tiling = p_Tiling;
	l_ImageInfo.usage = p_Usage;
	l_ImageInfo.flags = p_Flags;
	l_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	l_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	l_ImageInfo.queueFamilyIndexCount = 0;
	l_ImageInfo.pQueueFamilyIndices = nullptr;

    VkExternalMemoryImageCreateInfo l_ExternInfo{};
    l_ExternInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
#ifdef _WIN32
    l_ExternInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    l_ExternInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
    l_ImageInfo.pNext = &l_ExternInfo;

    VkImage l_Image;
	if (const VkResult l_Ret = l_Device.getTable().vkCreateImage(*l_Device, &l_ImageInfo, nullptr, &l_Image); l_Ret != VK_SUCCESS)
	{
		throw std::runtime_error(std::string("Failed to create image, error: ") + string_VkResult(l_Ret));
	}

    VulkanImage* l_NewRes = ARENA_ALLOC(VulkanImage){getDeviceID(), l_Image, p_Extent, p_Type, VK_IMAGE_LAYOUT_UNDEFINED};
    LOG_DEBUG("Created image (ID:", l_NewRes->getID(), ")");
    l_Device.insertImage(l_NewRes);

	return l_NewRes->getID();
}

void VulkanExternalMemoryExtension::allocateExportFromIndex(const ResourceID p_Resource, const uint32_t p_MemoryIndex) const
{
    VkExportMemoryAllocateInfo exportAlloc = {};
    exportAlloc.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
#ifdef _WIN32
    exportAlloc.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    exportAlloc.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

    VulkanDeviceSubresource* l_Resource = VulkanContext::getDevice(getDeviceID()).getSubresource(p_Resource);
    if (VulkanMemArray* l_MemArray = dynamic_cast<VulkanMemArray*>(l_Resource))
    {
        const VkMemoryRequirements l_Requirements = l_MemArray->getMemoryRequirements();
	    l_MemArray->setBoundMemory(VulkanContext::getDevice(getDeviceID()).getMemoryAllocator().allocateIsolated(l_Requirements.size, p_MemoryIndex, &exportAlloc));
    }
    else
    {
        LOG_ERR("Cannot allocate export from index for resource ID ", p_Resource, ", unsupported type");
    }
}

void VulkanExternalMemoryExtension::allocateExportFromFlags(ResourceID p_Resource, const VulkanMemoryAllocator::MemoryPropertyPreferences p_MemoryProperties) const
{
    VkExportMemoryAllocateInfo exportAlloc = {};
    exportAlloc.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
#ifdef _WIN32
    exportAlloc.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    exportAlloc.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

    VulkanDeviceSubresource* l_Resource = VulkanContext::getDevice(getDeviceID()).getSubresource(p_Resource);
    if (VulkanMemArray* l_MemArray = dynamic_cast<VulkanMemArray*>(l_Resource))
    {
        const VkMemoryRequirements l_Requirements = l_MemArray->getMemoryRequirements();
        const uint32_t l_MemoryIndex = VulkanContext::getDevice(getDeviceID()).getMemoryAllocator().search(l_Requirements.size, 1, p_MemoryProperties, l_Requirements.memoryTypeBits);
	    l_MemArray->setBoundMemory(VulkanContext::getDevice(getDeviceID()).getMemoryAllocator().allocateIsolated(l_Requirements.size, l_MemoryIndex, &exportAlloc));
    }
    else
    {
        LOG_ERR("Cannot allocate export from flags for resource ID ", p_Resource, ", unsupported type");
    }
}

HANDLE VulkanExternalMemoryExtension::getResourceOpaqueHandle(const ResourceID p_Resource) const
{
    const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());
    const VulkanMemArray* l_Resource = dynamic_cast<VulkanMemArray*>(l_Device.getSubresource(p_Resource));
    if (!l_Resource)
    {
        LOG_ERR("Cannot get external memory handle for resource ID ", p_Resource, ", unsupported type");
        return nullptr;
    }

#ifdef _WIN32
    VkMemoryGetWin32HandleInfoKHR getWin32 = {};
    getWin32.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    getWin32.memory = l_Resource->getChunkMemoryHandle();
    getWin32.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    HANDLE externalHandle;
    if (VkResult l_Ret = l_Device.getTable().vkGetMemoryWin32HandleKHR(*l_Device, &getWin32, &externalHandle); l_Ret != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("Failed to obtain win32 memory handle, error: ") + string_VkResult(l_Ret));
    }
    return externalHandle;
#else
    VkMemoryGetFdInfoKHR getFd = {};
    getFd.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    getFd.memory = l_Resource->getChunkMemoryHandle();
    getFd.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    HANDLE externalFd = -1;
    if (VkResult l_Ret = l_Device.getTable().vkGetMemoryFdKHR(*l_Device, &getFd, &externalFd); l_Ret != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("Failed to obtain fd memory handle, error: ") + string_VkResult(l_Ret));
    }
    return externalFd;
#endif
}

std::vector<std::string> VulkanExternalMemoryExtension::getExtraExtensionNames()
{
#ifdef _WIN32
    return { "VK_KHR_external_memory_win32" };
#else
    return { VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME };
#endif
}
