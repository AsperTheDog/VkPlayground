#include "ext/vulkan_raytracing.hpp"

VulkanRayTracingPipelineExtension::VulkanRayTracingPipelineExtension(const ResourceID p_DeviceID, const bool p_EnablePipeline, const bool p_EnableReplay, const bool p_EnableReplayMixed, const bool p_EnableIndirect, const bool p_EnableCulling)
    : VulkanDeviceExtension(p_DeviceID), m_EnablePipeline(p_EnablePipeline), m_EnableReplay(p_EnableReplay), m_EnableReplayMixed(p_EnableReplayMixed), m_EnableIndirect(p_EnableIndirect), m_EnableCulling(p_EnableCulling)
{
    
}

VulkanExtensionElem* VulkanRayTracingPipelineExtension::getExtensionStruct() const
{
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR* l_Struct = new VkPhysicalDeviceRayTracingPipelineFeaturesKHR{};
    l_Struct->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    l_Struct->pNext = nullptr;
    l_Struct->rayTracingPipeline = m_EnablePipeline;
    l_Struct->rayTracingPipelineShaderGroupHandleCaptureReplay = m_EnableReplay;
    l_Struct->rayTracingPipelineShaderGroupHandleCaptureReplayMixed = m_EnableReplayMixed;
    l_Struct->rayTracingPipelineTraceRaysIndirect = m_EnableIndirect;
    l_Struct->rayTraversalPrimitiveCulling = m_EnableCulling;
    return reinterpret_cast<VulkanExtensionElem*>(l_Struct);
}
