#pragma once
#include "vulkan_extension_management.hpp"

class RaytracingPipelineExtension final : public VulkanDeviceExtension
{
public:
    explicit RaytracingPipelineExtension(ResourceID p_DeviceID);

    [[nodiscard]] VulkanExtensionElem* getExtensionStruct() const override;
    [[nodiscard]] VkStructureType getExtensionStructType() const override;

protected:
    void freeResources() override;

private:


};