#pragma once

#include <vector>
#include <Volk/volk.h>


class VulkanBinding
{
public:
    VulkanBinding(uint32_t p_Binding, VkVertexInputRate p_Rate, uint32_t p_Stride);

    void addAttribDescription(VkFormat p_Format, uint32_t p_Offset, uint32_t p_LocationOverride = UINT32_MAX, uint32_t p_LocationSize = 1);

    [[nodiscard]] uint32_t getStride() const;

private:
    struct AttributeData
    {
        uint32_t location;
        VkFormat format;
        uint32_t offset;
        uint32_t locationSize;

        AttributeData(uint32_t p_Location, VkFormat p_Format, uint32_t p_Offset, uint32_t p_LocationSize);

        [[nodiscard]] VkVertexInputAttributeDescription getAttributeDescription(uint32_t p_Binding) const;
    };

    uint32_t m_Binding;
    VkVertexInputRate m_Rate;
    uint32_t m_Stride;
    std::vector<AttributeData> m_Attributes;

    [[nodiscard]] VkVertexInputBindingDescription getBindingDescription() const;
    [[nodiscard]] size_t getAttributeDescriptionCount() const;
    void getAttributeDescriptions(VkVertexInputAttributeDescription p_Container[]) const;

    friend class VulkanPipeline;
    friend struct VulkanPipelineBuilder;
};
