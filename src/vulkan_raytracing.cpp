#include <ext/vulkan_raytracing.hpp>

RaytracingPipelineExtension::RaytracingPipelineExtension(const ResourceID p_DeviceID) : VulkanDeviceExtension(p_DeviceID)
{
    
}

VulkanExtensionElem* RaytracingPipelineExtension::getExtensionStruct() const
{
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR* l_Struct = new VkPhysicalDeviceRayTracingPipelineFeaturesKHR{};
    l_Struct->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    l_Struct->pNext = nullptr;
    l_Struct->rayTracingPipeline = true;
    return reinterpret_cast<VulkanExtensionElem*>(l_Struct);
}

VkStructureType RaytracingPipelineExtension::getExtensionStructType() const
{
    return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
}

void RaytracingPipelineExtension::freeResources()
{

}
