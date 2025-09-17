#include "ext/vulkan_raytracing.hpp"

#include "vulkan_device.hpp"

VulkanRayTracingPipelineExtension* VulkanRayTracingPipelineExtension::get(const VulkanDevice& p_Device)
{
    return p_Device.getExtensionManager()->getExtension<VulkanRayTracingPipelineExtension>(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
}

VulkanRayTracingPipelineExtension* VulkanRayTracingPipelineExtension::get(ResourceID p_DeviceID)
{
    return VulkanContext::getDevice(p_DeviceID).getExtensionManager()->getExtension<VulkanRayTracingPipelineExtension>(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
}

VulkanRayTracingPipelineExtension::VulkanRayTracingPipelineExtension(const ResourceID p_DeviceID, const bool p_EnablePipeline, const bool p_EnableReplay, const bool p_EnableReplayMixed, const bool p_EnableIndirect, const bool p_EnableCulling)
    : VulkanDeviceExtension(p_DeviceID), m_EnablePipeline(p_EnablePipeline), m_EnableReplay(p_EnableReplay), m_EnableReplayMixed(p_EnableReplayMixed), m_EnableIndirect(p_EnableIndirect), m_EnableCulling(p_EnableCulling) {}

VkBaseInStructure* VulkanRayTracingPipelineExtension::getExtensionStruct() const
{
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR* l_Struct = TRANS_ALLOC(VkPhysicalDeviceRayTracingPipelineFeaturesKHR){};
    l_Struct->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    l_Struct->pNext = nullptr;
    l_Struct->rayTracingPipeline = m_EnablePipeline;
    l_Struct->rayTracingPipelineShaderGroupHandleCaptureReplay = m_EnableReplay;
    l_Struct->rayTracingPipelineShaderGroupHandleCaptureReplayMixed = m_EnableReplayMixed;
    l_Struct->rayTracingPipelineTraceRaysIndirect = m_EnableIndirect;
    l_Struct->rayTraversalPrimitiveCulling = m_EnableCulling;
    return reinterpret_cast<VkBaseInStructure*>(l_Struct);
}
