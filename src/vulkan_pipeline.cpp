#include "vulkan_pipeline.hpp"

#include <array>

#include "vulkan_context.hpp"
#include "vulkan_device.hpp"
#include "vulkan_shader.hpp"
#include "utils/logger.hpp"


VulkanPipelineBuilder::VulkanPipelineBuilder(const ResourceID p_Device)
    : m_Device(p_Device)
{
    m_VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m_VertexInputState.vertexBindingDescriptionCount = 0;
    m_VertexInputState.vertexAttributeDescriptionCount = 0;

    m_InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_InputAssemblyState.primitiveRestartEnable = VK_FALSE;

    m_TessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    m_TessellationState.patchControlPoints = 1;
    m_TesellationStateEnabled = false;

    m_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_ViewportState.viewportCount = 1;
    m_ViewportState.scissorCount = 1;

    m_RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_RasterizationState.depthClampEnable = VK_FALSE;
    m_RasterizationState.rasterizerDiscardEnable = VK_FALSE;
    m_RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    m_RasterizationState.lineWidth = 1.0f;
    m_RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    m_RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    m_RasterizationState.depthBiasEnable = VK_FALSE;

    m_MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_MultisampleState.sampleShadingEnable = VK_FALSE;
    m_MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    m_DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_DepthStencilState.depthTestEnable = VK_TRUE;
    m_DepthStencilState.depthWriteEnable = VK_TRUE;
    m_DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    m_DepthStencilState.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencilState.stencilTestEnable = VK_FALSE;

    m_ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_ColorBlendState.logicOpEnable = VK_FALSE;
    m_ColorBlendState.logicOp = VK_LOGIC_OP_COPY;
    m_ColorBlendState.attachmentCount = 0;
    m_ColorBlendState.blendConstants[0] = 0.0f;
    m_ColorBlendState.blendConstants[1] = 0.0f;
    m_ColorBlendState.blendConstants[2] = 0.0f;
    m_ColorBlendState.blendConstants[3] = 0.0f;

    m_DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_DynamicState.dynamicStateCount = 0;
}

void VulkanPipelineBuilder::addVertexBinding(const VulkanBinding& p_Binding, bool p_RecalculateLocations)
{
    m_VertexInputBindings.push_back(p_Binding.getBindingDescription());
    TRANS_VECTOR(l_Attributes, VkVertexInputAttributeDescription);
    l_Attributes.resize(p_Binding.getAttributeDescriptionCount());
    p_Binding.getAttributeDescriptions(l_Attributes.data());
    for (VkVertexInputAttributeDescription& l_Attr : l_Attributes)
    {
        m_VertexInputAttributes.push_back(l_Attr);
        if (p_RecalculateLocations)
        {
            m_VertexInputAttributes.back().location = m_CurrentVertexAttrLocation;
            m_CurrentVertexAttrLocation += l_Attr.location;
        }
    }
    m_VertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(m_VertexInputBindings.size());
    m_VertexInputState.pVertexBindingDescriptions = m_VertexInputBindings.data();
    m_VertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_VertexInputAttributes.size());
    m_VertexInputState.pVertexAttributeDescriptions = m_VertexInputAttributes.data();
}

void VulkanPipelineBuilder::setInputAssemblyState(const VkPrimitiveTopology p_Topology, const VkBool32 p_PrimitiveRestartEnable)
{
    m_InputAssemblyState.topology = p_Topology;
    m_InputAssemblyState.primitiveRestartEnable = p_PrimitiveRestartEnable;
}

void VulkanPipelineBuilder::setTessellationState(const uint32_t p_PatchControlPoints)
{
    m_TessellationState.patchControlPoints = p_PatchControlPoints;
    m_TesellationStateEnabled = true;
}

void VulkanPipelineBuilder::setViewportState(const uint32_t p_ViewportCount, const uint32_t p_ScissorCount)
{
    m_ViewportState.viewportCount = p_ViewportCount;
    m_ViewportState.scissorCount = p_ScissorCount;
}

void VulkanPipelineBuilder::setViewportState(const std::span<const VkViewport> p_Viewports, const std::span<const VkRect2D> p_Scissors)
{
    m_Viewports = std::vector<VkViewport>(p_Viewports.begin(), p_Viewports.end());
    m_Scissors = std::vector<VkRect2D>(p_Scissors.begin(), p_Scissors.end());
    m_ViewportState.viewportCount = static_cast<uint32_t>(m_Viewports.size());
    m_ViewportState.pViewports = m_Viewports.data();
    m_ViewportState.scissorCount = static_cast<uint32_t>(m_Scissors.size());
    m_ViewportState.pScissors = m_Scissors.data();
}

void VulkanPipelineBuilder::setRasterizationState(const VkPolygonMode p_PolygonMode, const VkCullModeFlags p_CullMode, const VkFrontFace p_FrontFace)
{
    m_RasterizationState.polygonMode = p_PolygonMode;
    m_RasterizationState.cullMode = p_CullMode;
    m_RasterizationState.frontFace = p_FrontFace;
}

void VulkanPipelineBuilder::setMultisampleState(const VkSampleCountFlagBits p_RasterizationSamples, const VkBool32 p_SampleShadingEnable, const float p_MinSampleShading)
{
    m_MultisampleState.rasterizationSamples = p_RasterizationSamples;
    m_MultisampleState.sampleShadingEnable = p_SampleShadingEnable;
    m_MultisampleState.minSampleShading = p_MinSampleShading;
}

void VulkanPipelineBuilder::setDepthStencilState(const VkBool32 p_DepthTestEnable, const VkBool32 p_DepthWriteEnable, const VkCompareOp p_DepthCompareOp)
{
    m_DepthStencilState.depthTestEnable = p_DepthTestEnable;
    m_DepthStencilState.depthWriteEnable = p_DepthWriteEnable;
    m_DepthStencilState.depthCompareOp = p_DepthCompareOp;
}

void VulkanPipelineBuilder::setColorBlendState(const VkBool32 p_LogicOpEnable, const VkLogicOp p_LogicOp, const std::array<float, 4> p_ColorBlendConstants)
{
    m_ColorBlendState.logicOpEnable = p_LogicOpEnable;
    m_ColorBlendState.logicOp = p_LogicOp;
    m_ColorBlendState.blendConstants[0] = p_ColorBlendConstants[0];
    m_ColorBlendState.blendConstants[1] = p_ColorBlendConstants[1];
    m_ColorBlendState.blendConstants[2] = p_ColorBlendConstants[2];
    m_ColorBlendState.blendConstants[3] = p_ColorBlendConstants[3];
}

void VulkanPipelineBuilder::addColorBlendAttachment(const VkPipelineColorBlendAttachmentState& p_Attachment)
{
    m_Attachments.push_back(p_Attachment);
    m_ColorBlendState.attachmentCount = static_cast<uint32_t>(m_Attachments.size());
    m_ColorBlendState.pAttachments = m_Attachments.data();
}

void VulkanPipelineBuilder::setDynamicState(const std::span<VkDynamicState> p_DynamicStates)
{
    m_DynamicStates = std::vector<VkDynamicState>(p_DynamicStates.begin(), p_DynamicStates.end());
    m_DynamicState.dynamicStateCount = static_cast<uint32_t>(m_DynamicStates.size());
    m_DynamicState.pDynamicStates = m_DynamicStates.data();
}

void VulkanPipelineBuilder::addShaderStage(const ResourceID p_Shader, const std::string_view p_Entrypoint)
{
    m_ShaderStages.emplace_back(p_Shader, p_Entrypoint);
}

void VulkanPipelineBuilder::resetShaderStages()
{
    m_ShaderStages.clear();
}

void VulkanPipelineBuilder::setVertexInputState(const VkPipelineVertexInputStateCreateInfo& p_State)
{
    m_VertexInputState = p_State;
    m_VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m_VertexInputBindings.clear();
    m_VertexInputAttributes.clear();
}

void VulkanPipelineBuilder::setInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo& p_State)
{
    m_InputAssemblyState = p_State;
    m_InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
}

void VulkanPipelineBuilder::setTessellationState(const VkPipelineTessellationStateCreateInfo& p_State)
{
    m_TessellationState = p_State;
    m_TessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    m_TesellationStateEnabled = true;
}

void VulkanPipelineBuilder::setViewportState(const VkPipelineViewportStateCreateInfo& p_State)
{
    m_ViewportState = p_State;
    m_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
}

void VulkanPipelineBuilder::setRasterizationState(const VkPipelineRasterizationStateCreateInfo& p_State)
{
    m_RasterizationState = p_State;
    m_RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
}

void VulkanPipelineBuilder::setMultisampleState(const VkPipelineMultisampleStateCreateInfo& p_State)
{
    m_MultisampleState = p_State;
    m_MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
}

void VulkanPipelineBuilder::setDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& p_State)
{
    m_DepthStencilState = p_State;
    m_DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
}

void VulkanPipelineBuilder::setColorBlendState(const VkPipelineColorBlendStateCreateInfo& p_State)
{
    m_ColorBlendState = p_State;
    m_ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_Attachments.clear();
}

void VulkanPipelineBuilder::setDynamicState(const VkPipelineDynamicStateCreateInfo& p_State)
{
    m_DynamicState = p_State;
    m_DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
}

void VulkanPipeline::free()
{
    if (m_VkHandle != VK_NULL_HANDLE)
    {
        const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

        l_Device.getTable().vkDestroyPipeline(l_Device.m_VkHandle, m_VkHandle, nullptr);
        LOG_DEBUG("Freed pipeline (ID: ", m_ID, ")");
        m_VkHandle = VK_NULL_HANDLE;
    }
}

void VulkanPipelineBuilder::createShaderStages(VkPipelineShaderStageCreateInfo p_Container[]) const
{
    for (size_t i = 0; i < m_ShaderStages.size(); i++)
    {
        VkPipelineShaderStageCreateInfo l_StageInfo{};
        const VulkanShaderModule& l_Shader = VulkanContext::getDevice(m_Device).getShaderModule(m_ShaderStages[i].shader);
        l_StageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_StageInfo.stage = l_Shader.m_Stage;
        l_StageInfo.module = l_Shader.m_VkHandle;
        l_StageInfo.pName = m_ShaderStages[i].entrypoint.c_str();
        p_Container[i] = l_StageInfo;
    }
}

ResourceID VulkanPipeline::getLayout() const
{
    return m_Layout;
}

ResourceID VulkanPipeline::getRenderPass() const
{
    return m_RenderPass;
}

ResourceID VulkanPipeline::getSubpass() const
{
    return m_Subpass;
}

VkPipeline VulkanPipeline::operator*() const
{
    return m_VkHandle;
}

VulkanPipeline::VulkanPipeline(const ResourceID p_Device, const VkPipeline p_Handle, const ResourceID p_Layout, const ResourceID p_RenderPass, const ResourceID p_Subpass)
    : VulkanDeviceSubresource(p_Device), m_VkHandle(p_Handle), m_Layout(p_Layout), m_RenderPass(p_RenderPass), m_Subpass(p_Subpass) {}

VkPipelineLayout VulkanPipelineLayout::operator*() const
{
    return m_VkHandle;
}

void VulkanPipelineLayout::free()
{
    if (m_VkHandle != VK_NULL_HANDLE)
    {
        const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

        l_Device.getTable().vkDestroyPipelineLayout(l_Device.m_VkHandle, m_VkHandle, nullptr);
        LOG_DEBUG("Freed pipeline layout (ID: ", m_ID, ")");
        m_VkHandle = VK_NULL_HANDLE;
    }
}

VulkanPipelineLayout::VulkanPipelineLayout(const uint32_t p_Device, const VkPipelineLayout p_Handle)
    : VulkanDeviceSubresource(p_Device), m_VkHandle(p_Handle) {}

void VulkanComputePipeline::free()
{
    if (m_VkHandle != VK_NULL_HANDLE)
    {
        const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

        l_Device.getTable().vkDestroyPipeline(*l_Device, m_VkHandle, nullptr);
        LOG_DEBUG("Freed compute pipeline (ID: ", m_ID, ")");
        m_VkHandle = VK_NULL_HANDLE;
    }
}

VulkanComputePipeline::VulkanComputePipeline(const ResourceID p_Device, const VkPipeline p_Handle)
    : VulkanDeviceSubresource(p_Device), m_VkHandle(p_Handle) {}
