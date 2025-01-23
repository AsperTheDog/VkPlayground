#include "vulkan_binding.hpp"

VulkanBinding::VulkanBinding(const uint32_t p_Binding, const VkVertexInputRate p_Rate, const uint32_t p_Stride)
	: m_Binding(p_Binding), m_Rate(p_Rate), m_Stride(p_Stride)
{
	
}

uint32_t VulkanBinding::getStride() const
{
	return m_Stride;
}

void VulkanBinding::addAttribDescription(VkFormat p_Format, uint32_t p_Offset)
{
	m_Attributes.emplace_back(static_cast<uint32_t>(m_Attributes.size()), p_Format, p_Offset);
}

VulkanBinding::AttributeData::AttributeData(const uint32_t p_Location, const VkFormat p_Format, const uint32_t p_Offset)
	: location(p_Location), format(p_Format), offset(p_Offset)
{
}

VkVertexInputAttributeDescription VulkanBinding::AttributeData::getAttributeDescription(const uint32_t p_Binding) const
{
	VkVertexInputAttributeDescription l_AttributeDescription;
	l_AttributeDescription.binding = p_Binding;
	l_AttributeDescription.location = location;
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

std::vector<VkVertexInputAttributeDescription> VulkanBinding::getAttributeDescriptions() const
{
	std::vector<VkVertexInputAttributeDescription> l_AttributeDescriptions;
	l_AttributeDescriptions.reserve(m_Attributes.size());
	for (const AttributeData& l_Attr : m_Attributes)
	{
		l_AttributeDescriptions.push_back(l_Attr.getAttributeDescription(m_Binding));
	}
	return l_AttributeDescriptions;
}