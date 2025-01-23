#pragma once
#include "vulkan_extension_management.hpp"

class VulkanRayTracingPipelineExtension final : public VulkanDeviceExtension
{
public:
    static VulkanRayTracingPipelineExtension* get(const VulkanDevice& p_Device);
    static VulkanRayTracingPipelineExtension* get(ResourceID p_DeviceID);

    explicit VulkanRayTracingPipelineExtension(ResourceID p_DeviceID, bool p_EnablePipeline, bool p_EnableReplay, bool p_EnableReplayMixed, bool p_EnableIndirect, bool p_EnableCulling);

    [[nodiscard]] VkBaseInStructure* getExtensionStruct() const override;
    [[nodiscard]] VkStructureType getExtensionStructType() const override { return VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR; }

    void free() override {}

private:
    bool m_EnablePipeline;
    bool m_EnableReplay;
    bool m_EnableReplayMixed;
    bool m_EnableIndirect;
    bool m_EnableCulling;
};