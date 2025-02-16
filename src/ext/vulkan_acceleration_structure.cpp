#include "ext/vulkan_acceleration_structure.hpp"

#include "vulkan_buffer.hpp"
#include "vulkan_device.hpp"
#include "utils/logger.hpp"

VulkanAccelerationStructureExtension* VulkanAccelerationStructureExtension::get(const VulkanDevice& p_Device)
{
    return p_Device.getExtensionManager()->getExtension<VulkanAccelerationStructureExtension>(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
}

VulkanAccelerationStructureExtension* VulkanAccelerationStructureExtension::get(const ResourceID p_DeviceID)
{
    return VulkanContext::getDevice(p_DeviceID).getExtensionManager()->getExtension<VulkanAccelerationStructureExtension>(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
}

VulkanAccelerationStructureExtension::VulkanAccelerationStructureExtension(const ResourceID p_DeviceID, const bool p_EnableStructure, const bool p_EnableIndirectBuild, const bool p_EnableCaptureReplay, const bool p_EnableHostCommands, const bool p_EnableUpdateAfterBuild)
    : VulkanDeviceExtension(p_DeviceID), m_EnableStructure(p_EnableStructure), m_EnableIndirectBuild(p_EnableIndirectBuild), m_EnableCaptureReplay(p_EnableCaptureReplay), m_EnableHostCommands(p_EnableHostCommands), m_EnableUpdateAfterBuild(p_EnableUpdateAfterBuild)
{
}

VkBaseInStructure* VulkanAccelerationStructureExtension::getExtensionStruct() const
{
    VkPhysicalDeviceAccelerationStructureFeaturesKHR* l_Struct = TRANS_ALLOC(VkPhysicalDeviceAccelerationStructureFeaturesKHR){};
    l_Struct->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    l_Struct->pNext = nullptr;
    l_Struct->accelerationStructure = m_EnableStructure;
    l_Struct->accelerationStructureIndirectBuild = m_EnableIndirectBuild;
    l_Struct->accelerationStructureCaptureReplay = m_EnableCaptureReplay;
    l_Struct->accelerationStructureHostCommands = m_EnableHostCommands;
    l_Struct->descriptorBindingAccelerationStructureUpdateAfterBind = m_EnableUpdateAfterBuild;
    return reinterpret_cast<VkBaseInStructure*>(l_Struct);
}

ResourceID VulkanAccelerationStructureExtension::createBLASFromModels(const std::span<const ModelData> p_Models, uint32_t p_BufferQueueFamilyIndex)
{
    VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());
    TRANS_VECTOR(l_Geometries, VkAccelerationStructureGeometryKHR);
    l_Geometries.reserve(p_Models.size());
    TRANS_VECTOR(l_BuildRanges, VkAccelerationStructureBuildRangeInfoKHR);
    l_BuildRanges.reserve(p_Models.size());
    TRANS_VECTOR(l_MaxPrimitiveCounts, uint32_t);
    l_MaxPrimitiveCounts.reserve(p_Models.size());
    for (const ModelData& l_Model : p_Models)
    {
        VulkanBuffer& l_VertexBuffer = l_Device.getBuffer(l_Model.vertexBuffer.buffer);
        VulkanBuffer& l_IndexBuffer = l_Device.getBuffer(l_Model.indexBuffer.buffer);

        VkAccelerationStructureGeometryTrianglesDataKHR l_Triangles;
        l_Triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        l_Triangles.pNext = nullptr;
        l_Triangles.vertexFormat = l_Model.vertexBuffer.format;
        l_Triangles.vertexData.deviceAddress = l_VertexBuffer.getDeviceAddress();
        l_Triangles.vertexStride = l_Model.vertexBuffer.stride;
        l_Triangles.maxVertex = static_cast<uint32_t>(l_VertexBuffer.getSize() / l_Model.vertexBuffer.stride);
        l_Triangles.indexType = l_Model.indexBuffer.format;
        l_Triangles.indexData.deviceAddress = l_IndexBuffer.getDeviceAddress();
        l_Triangles.transformData.deviceAddress = l_Model.transformBuffer != UINT32_MAX ? l_Device.getBuffer(l_Model.transformBuffer).getDeviceAddress() : 0;

        VkAccelerationStructureGeometryKHR l_Geometry;
        l_Geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        l_Geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        l_Geometry.geometry.triangles = l_Triangles;
        l_Geometries.push_back(l_Geometry);

        VkAccelerationStructureBuildRangeInfoKHR l_BuildRange;
        l_BuildRange.primitiveCount = static_cast<uint32_t>(l_Model.indexCount / 3);
        l_BuildRange.primitiveOffset = static_cast<uint32_t>(l_Model.indexByteOffset);
        l_BuildRange.firstVertex = static_cast<uint32_t>(l_Model.vertexOffset);
        l_BuildRange.transformOffset = 0;
        l_BuildRanges.push_back(l_BuildRange);

        l_MaxPrimitiveCounts.push_back(static_cast<uint32_t>(l_Model.indexCount / 3));
    }

    VkAccelerationStructureBuildGeometryInfoKHR l_BuildInfo;
    l_BuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    l_BuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    l_BuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    l_BuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    l_BuildInfo.geometryCount = static_cast<uint32_t>(l_Geometries.size());
    l_BuildInfo.pGeometries = l_Geometries.data();

    VkAccelerationStructureBuildSizesInfoKHR l_SizeInfo{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
    };

    l_Device.getTable().vkGetAccelerationStructureBuildSizesKHR(*l_Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &l_BuildInfo, l_MaxPrimitiveCounts.data(), &l_SizeInfo);

    VkAccelerationStructureCreateInfoKHR l_CreateInfo;
    l_CreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    l_CreateInfo.pNext = nullptr;
    l_CreateInfo.size = l_SizeInfo.accelerationStructureSize;

    VkAccelerationStructureKHR l_Structure;
    l_Device.getTable().vkCreateAccelerationStructureKHR(*l_Device, &l_CreateInfo, nullptr, &l_Structure);

    ResourceID l_Buffer = l_Device.createBuffer(l_SizeInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, p_BufferQueueFamilyIndex);

    VulkanAccelerationStructure* l_NewRes = ARENA_ALLOC(VulkanAccelerationStructure){ l_Device.getID(), l_Structure, l_Buffer };
    m_AccStructures[l_NewRes->getID()] = l_NewRes;
    LOG_DEBUG("Created Acceleration Structure (ID:", l_NewRes->getID(), ")");
    return l_NewRes->getID();
}

void VulkanAccelerationStructure::allocateFromIndex(const uint32_t p_MemoryIndex) const
{
    VulkanContext::getDevice(getDeviceID()).getBuffer(m_Buffer).allocateFromIndex(p_MemoryIndex);
}

void VulkanAccelerationStructure::allocateFromFlags(const VulkanMemoryAllocator::MemoryPropertyPreferences p_MemoryProperties) const
{
    VulkanContext::getDevice(getDeviceID()).getBuffer(m_Buffer).allocateFromFlags(p_MemoryProperties);
}

void VulkanAccelerationStructure::free()
{
    VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

    if (m_VkHandle != VK_NULL_HANDLE)
    {
        l_Device.getTable().vkDestroyAccelerationStructureKHR(*l_Device, m_VkHandle, nullptr);
        LOG_DEBUG("Freed acceleration structure");
    }
    if (m_MemoryRegion.size > 0)
	{
		l_Device.getMemoryAllocator().deallocate(m_MemoryRegion);
		m_MemoryRegion = {};
	}
}

VulkanAccelerationStructure::VulkanAccelerationStructure(const ResourceID p_Device, const VkAccelerationStructureKHR p_VkHandle, ResourceID p_Buffer)
    : VulkanDeviceSubresource(p_Device), m_VkHandle(p_VkHandle), m_Buffer(p_Buffer)
{
}
