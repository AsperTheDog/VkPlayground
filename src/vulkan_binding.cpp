#include "vulkan_binding.hpp"

VulkanBinding::VulkanBinding(const uint32_t p_Binding, const VkVertexInputRate p_Rate, const uint32_t p_Stride)
	: m_Binding(p_Binding), m_Rate(p_Rate), m_Stride(p_Stride)
{
	
}

uint32_t VulkanBinding::getStride() const
{
	return m_Stride;
}

void VulkanBinding::addAttribDescription(const VkFormat p_Format, const uint32_t p_Offset, const uint32_t p_LocationOverride, const uint32_t p_LocationSize)
{
    uint32_t l_Location = p_LocationOverride == UINT32_MAX ? static_cast<uint32_t>(m_Attributes.size()) : p_LocationOverride;
	m_Attributes.emplace_back(l_Location, p_Format, p_Offset, p_LocationSize);
}

VulkanBinding::AttributeData::AttributeData(const uint32_t p_Location, const VkFormat p_Format, const uint32_t p_Offset, const uint32_t p_LocationSize)
	: location(p_Location), format(p_Format), offset(p_Offset), locationSize(p_LocationSize)
{
}

VkVertexInputAttributeDescription VulkanBinding::AttributeData::getAttributeDescription(const uint32_t p_Binding) const
{
	VkVertexInputAttributeDescription l_AttributeDescription;
	l_AttributeDescription.binding = p_Binding;
	l_AttributeDescription.location = locationSize;
	l_AttributeDescription.format = format;
	l_AttributeDescription.offset = offset;
	return l_AttributeDescription;
}

VkVertexInputBindingDescription VulkanBinding::getBindingDescription() const
{
	VkVertexInputBindingDescription l_BindingDescription;
	l_BindingDescription.binding = m_Binding;
	l_BindingDescription.stride = m_Stride;
	l_BindingDescription.inputRate = m_Rate;
	return l_BindingDescription;
}

size_t VulkanBinding::getAttributeDescriptionCount() const
{
    return m_Attributes.size();
}

void VulkanBinding::getAttributeDescriptions(VkVertexInputAttributeDescription p_Container[]) const
{
    for (size_t i = 0; i < m_Attributes.size(); i++)
    {
        p_Container[i] = m_Attributes[i].getAttributeDescription(m_Binding);
    }
}