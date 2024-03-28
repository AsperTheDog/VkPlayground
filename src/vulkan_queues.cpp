#include "vulkan_queues.hpp"

#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_gpu.hpp"
#include "utils/logger.hpp"

uint32_t GPUQueueStructure::getQueueFamilyCount() const
{
	return static_cast<uint32_t>(queueFamilies.size());
}

QueueFamily GPUQueueStructure::getQueueFamily(const uint32_t index) const
{
	return queueFamilies[index];
}

QueueFamily GPUQueueStructure::findQueueFamily(const VkQueueFlags flags, const bool exactMatch) const
{
	for (const auto& queueFamily : queueFamilies)
	{
		if (exactMatch)
		{
			if (queueFamily.properties.queueFlags == flags)
			{
				return queueFamily;
			}
		}
		else if (queueFamily.properties.queueFlags & flags)
		{
			return queueFamily;
		}
	}
	throw std::runtime_error("No queue family found with the flags " + string_VkQueueFlags(flags) + (exactMatch ? " exactly " : " ") + " in " + gpu.getProperties().deviceName);
}

QueueFamily GPUQueueStructure::findPresentQueueFamily(const VkSurfaceKHR surface) const
{
	for (const auto& queueFamily : queueFamilies)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(gpu.m_vkHandle, queueFamily.index, surface, &presentSupport);
		if (presentSupport)
		{
			return queueFamily;
		}
	}
	throw std::runtime_error(std::string("No queue family found with present support in ") + gpu.getProperties().deviceName);
}

std::string GPUQueueStructure::toString() const
{
	std::string result;
	for (const auto& queueFamily : queueFamilies)
	{
		result += "Queue Family " + std::to_string(queueFamily.index) + ":\n";
		result += "  Queue Count: " + std::to_string(queueFamily.properties.queueCount) + "\n";
		result += "  Queue Flags: " + string_VkQueueFlags(queueFamily.properties.queueFlags) + "\n";
		result += "  Timestamp Valid Bits: " + std::to_string(queueFamily.properties.timestampValidBits) + "\n";
		result += "  Min Image Transfer Granularity: " + std::to_string(queueFamily.properties.minImageTransferGranularity.width) + ", " + std::to_string(queueFamily.properties.minImageTransferGranularity.height) + ", " + std::to_string(queueFamily.properties.minImageTransferGranularity.depth) + "\n";
	}
	return result;	
}

GPUQueueStructure::GPUQueueStructure(const VulkanGPU gpu)
	: gpu(gpu)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu.m_vkHandle, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu.m_vkHandle, &queueFamilyCount, queueFamilyProperties.data());
	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		queueFamilies.push_back(QueueFamily(queueFamilyProperties[i], i, gpu));
	}
}

QueueFamily::QueueFamily(const VkQueueFamilyProperties& properties, const uint32_t index, const VulkanGPU gpu)
: properties(properties), index(index), gpu(gpu)
{

}

void VulkanQueue::waitIdle() const
{
	vkQueueWaitIdle(m_vkHandle);
}

VkQueue VulkanQueue::operator*() const
{
	return m_vkHandle;
}

VulkanQueue::VulkanQueue(const VkQueue queue)
	: m_vkHandle(queue)
{
}

QueueFamilySelector::QueueFamilySelector(const GPUQueueStructure& structure)
{
	this->m_structure = structure;
	m_selections.resize(structure.queueFamilies.size());
}

void QueueFamilySelector::selectQueueFamily(const QueueFamily& family, const QueueFamilyTypes typeMask)
{
	m_selections[family.index].familyFlags |= typeMask;
}

QueueSelection QueueFamilySelector::getOrAddQueue(const QueueFamily& family, const float priority)
{
	if (m_selections[family.index].priorities.empty())
	{
        return addQueue(family, priority);
	}
	m_selections[family.index].priorities[0] = std::max(m_selections[family.index].priorities[0], priority);
    Logger::print("Queue family " + std::to_string(family.index) + " already has a queue, setting priority to " + std::to_string(m_selections[family.index].priorities[0]), Logger::LevelBits::DEBUG);
	return {family.index, 0};
}

QueueSelection QueueFamilySelector::addQueue(const QueueFamily& family, const float priority)
{
	m_selections[family.index].priorities.push_back(priority);
    Logger::print("Added queue to family " + std::to_string(family.index) + " with priority " + std::to_string(priority), Logger::LevelBits::DEBUG);
	return {family.index, static_cast<uint32_t>(m_selections[family.index].priorities.size() - 1)};
}

std::optional<QueueFamily> QueueFamilySelector::getQueueFamilyByType(const QueueFamilyTypes type)
{
	for (uint32_t i = 0; i < m_selections.size(); i++)
	{
		if (m_selections[i].familyFlags & type)
		{
			return m_structure.queueFamilies[i];
		}
	}
	return std::nullopt;
}

std::vector<uint32_t> QueueFamilySelector::getUniqueIndices() const
{
	std::vector<uint32_t> indices;
	for (uint32_t i = 0; i < m_selections.size(); i++)
	{
		if (m_selections[i].familyFlags != 0 && !m_selections[i].priorities.empty())
		{
			indices.push_back(i);
		}
	}
	return indices;
}
