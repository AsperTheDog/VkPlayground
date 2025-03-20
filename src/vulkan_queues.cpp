#include "vulkan_queues.hpp"

#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_gpu.hpp"
#include "utils/logger.hpp"

uint32_t GPUQueueStructure::getQueueFamilyCount() const
{
	return static_cast<uint32_t>(m_QueueFamilies.size());
}

QueueFamily GPUQueueStructure::getQueueFamily(const uint32_t p_Index) const
{
	return m_QueueFamilies[p_Index];
}

bool GPUQueueStructure::isQueueFlagSupported(const VkQueueFlagBits p_Flag) const
{
	for (const QueueFamily& l_QueueFamily : m_QueueFamilies)
	{
		if (l_QueueFamily.properties.queueFlags & p_Flag && l_QueueFamily.properties.queueCount > 0)
		{
			return true;
		}
	}
	return false;
}

bool GPUQueueStructure::areQueueFlagsSupported(const VkQueueFlags p_Flags, const bool SingleQueue) const
{
	if (SingleQueue)
	{
		for (const QueueFamily& l_QueueFamily : m_QueueFamilies)
		{
			if ((l_QueueFamily.properties.queueFlags & p_Flags) == p_Flags)
			{
				return true;
			}
		}
		return false;
	}
	
	VkQueueFlags l_SupportedFlags = 0;
	for (const QueueFamily& l_QueueFamily : m_QueueFamilies)
	{
		l_SupportedFlags |= l_QueueFamily.properties.queueFlags;
	}
	return (l_SupportedFlags & p_Flags) == p_Flags;
}

bool GPUQueueStructure::isPresentSupported(const VkSurfaceKHR p_Surface) const
{
	for (const QueueFamily& l_QueueFamily : m_QueueFamilies)
	{
		VkBool32 l_PresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_GPU.m_VkHandle, l_QueueFamily.index, p_Surface, &l_PresentSupport);
		if (l_PresentSupport)
		{
			return true;
		}
	}
	return false;
}

QueueFamily GPUQueueStructure::findQueueFamily(const VkQueueFlags p_Flags, const bool p_ExactMatch) const
{
	for (const QueueFamily& l_QueueFamily : m_QueueFamilies)
	{
		if (p_ExactMatch)
		{
			if (l_QueueFamily.properties.queueFlags == p_Flags)
			{
				return l_QueueFamily;
			}
		}
		else if (l_QueueFamily.properties.queueFlags & p_Flags)
		{
			return l_QueueFamily;
		}
	}

	throw std::runtime_error("No queue family found with the flags " + string_VkQueueFlags(p_Flags) + (p_ExactMatch ? " exactly " : " ") + " in " + m_GPU.getProperties().deviceName);
}

QueueFamily GPUQueueStructure::findQueueFamily(const VkQueueFlags p_Flags, std::span<uint32_t> p_Exclude, bool p_ExactMatch) const
{
    for (const QueueFamily& l_QueueFamily : m_QueueFamilies)
    {
        if (std::ranges::find(p_Exclude, l_QueueFamily.index) != p_Exclude.end())
            continue;
        if (p_ExactMatch)
        {
            if (l_QueueFamily.properties.queueFlags == p_Flags)
                return l_QueueFamily;
        }
        else if (l_QueueFamily.properties.queueFlags & p_Flags)
            return l_QueueFamily;
    }
    throw std::runtime_error("No queue family found with the flags " + string_VkQueueFlags(p_Flags) + (p_ExactMatch ? " exactly " : " ") + " in " + m_GPU.getProperties().deviceName);
}

QueueFamily GPUQueueStructure::findPresentQueueFamily(const VkSurfaceKHR p_Surface) const
{
	for (const QueueFamily& l_QueueFamily : m_QueueFamilies)
	{
		VkBool32 l_PresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_GPU.m_VkHandle, l_QueueFamily.index, p_Surface, &l_PresentSupport);
		if (l_PresentSupport)
		{
			return l_QueueFamily;
		}
	}
	throw std::runtime_error(std::string("No queue family found with present support in ") + m_GPU.getProperties().deviceName);
}

std::string GPUQueueStructure::toString() const
{
	std::string l_Result;
	for (const QueueFamily& l_QueueFamily : m_QueueFamilies)
	{
		l_Result += "Queue Family " + std::to_string(l_QueueFamily.index) + ":\n";
		l_Result += "  Queue Count: " + std::to_string(l_QueueFamily.properties.queueCount) + "\n";
		l_Result += "  Queue Flags: " + string_VkQueueFlags(l_QueueFamily.properties.queueFlags) + "\n";
		l_Result += "  Timestamp Valid Bits: " + std::to_string(l_QueueFamily.properties.timestampValidBits) + "\n";
		l_Result += "  Min Image Transfer Granularity: " + std::to_string(l_QueueFamily.properties.minImageTransferGranularity.width) + ", " + std::to_string(l_QueueFamily.properties.minImageTransferGranularity.height) + ", " + std::to_string(l_QueueFamily.properties.minImageTransferGranularity.depth) + "\n";
	}
	return l_Result;
}

GPUQueueStructure::GPUQueueStructure(const VulkanGPU p_GPU)
	: m_GPU(p_GPU)
{
	uint32_t l_QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(p_GPU.m_VkHandle, &l_QueueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> l_QueueFamilyProperties(l_QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(p_GPU.m_VkHandle, &l_QueueFamilyCount, l_QueueFamilyProperties.data());
	for (uint32_t i = 0; i < l_QueueFamilyCount; i++)
	{
		m_QueueFamilies.push_back(QueueFamily(l_QueueFamilyProperties[i], i, p_GPU));
	}
}

bool QueueFamily::isPresentSupported(const VkSurfaceKHR p_Surface) const
{
	VkBool32 l_PresentSupport = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(*gpu, index, p_Surface, &l_PresentSupport);
	return l_PresentSupport;
}

QueueFamily::QueueFamily(const VkQueueFamilyProperties& p_Properties, const uint32_t p_Index, const VulkanGPU p_GPU)
	: properties(p_Properties), index(p_Index), gpu(p_GPU)
{

}

void VulkanQueue::waitIdle() const
{
	vkQueueWaitIdle(m_VkHandle);
}

VkQueue VulkanQueue::operator*() const
{
	return m_VkHandle;
}

VulkanQueue::VulkanQueue(const VkQueue p_Queue)
	: m_VkHandle(p_Queue)
{
}

QueueFamilySelector::QueueFamilySelector(const GPUQueueStructure& p_Structure)
{
	this->m_Structure = p_Structure;
	m_Selections.resize(p_Structure.m_QueueFamilies.size());
}

void QueueFamilySelector::selectQueueFamily(const QueueFamily& p_Family, const QueueFamilyTypes p_TypeMask)
{
	m_Selections[p_Family.index].familyFlags |= p_TypeMask;
}

QueueSelection QueueFamilySelector::getOrAddQueue(const QueueFamily& p_Family, const float p_Priority)
{
	if (m_Selections[p_Family.index].priorities.empty())
	{
		return addQueue(p_Family, p_Priority);
	}
	m_Selections[p_Family.index].priorities[0] = std::max(m_Selections[p_Family.index].priorities[0], p_Priority);
    LOG_DEBUG("Queue family ", p_Family.index, " already has a queue, setting priority to ", m_Selections[p_Family.index].priorities[0]);
	return { p_Family.index, 0 };
}

QueueSelection QueueFamilySelector::addQueue(const QueueFamily& p_Family, const float p_Priority)
{
	m_Selections[p_Family.index].priorities.push_back(p_Priority);
    LOG_DEBUG("Added queue to family ", p_Family.index, " with priority ", p_Priority);
	return { p_Family.index, static_cast<uint32_t>(m_Selections[p_Family.index].priorities.size() - 1) };
}

std::optional<QueueFamily> QueueFamilySelector::getQueueFamilyByType(const QueueFamilyTypes p_Type)
{
	for (uint32_t i = 0; i < m_Selections.size(); i++)
	{
		if (m_Selections[i].familyFlags & p_Type)
		{
			return m_Structure.m_QueueFamilies[i];
		}
	}
	return std::nullopt;
}

std::vector<uint32_t> QueueFamilySelector::getUniqueIndices() const
{
	std::vector<uint32_t> l_Indices;
	for (uint32_t i = 0; i < m_Selections.size(); i++)
	{
		if (m_Selections[i].familyFlags != 0 && !m_Selections[i].priorities.empty())
		{
			l_Indices.push_back(i);
		}
	}
	return l_Indices;
}

QueueFamilyTypes QueueFamilySelector::getTypesFromFlags(const VkQueueFlags P_Flags)
{
	QueueFamilyTypes l_Types = 0;
	if (P_Flags & VK_QUEUE_GRAPHICS_BIT) l_Types |= QueueFamilyTypeBits::GRAPHICS;
	if (P_Flags & VK_QUEUE_COMPUTE_BIT) l_Types |= QueueFamilyTypeBits::COMPUTE;
	if (P_Flags & VK_QUEUE_TRANSFER_BIT) l_Types |= QueueFamilyTypeBits::TRANSFER;
	if (P_Flags & VK_QUEUE_SPARSE_BINDING_BIT) l_Types |= QueueFamilyTypeBits::SPARSE_BINDING;
	if (P_Flags & VK_QUEUE_PROTECTED_BIT) l_Types |= QueueFamilyTypeBits::PROTECTED;
	if (P_Flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) l_Types |= QueueFamilyTypeBits::VIDEO_DECODE;
	return l_Types;
}
