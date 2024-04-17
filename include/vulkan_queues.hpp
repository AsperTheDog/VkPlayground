#pragma once
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan_gpu.hpp"

class QueueFamily;

enum QueueFamilyTypeBits
{
	GRAPHICS = 1,
	COMPUTE = 2,
	PRESENT = 4,
	TRANSFER = 8,
	SPARSE_BINDING = 16,
	VIDEO_DECODE = 32,
	OPTICAL_FLOW = 64,
	PROTECTED = 128
};
typedef uint8_t QueueFamilyTypes;

class GPUQueueStructure
{
public:
	[[nodiscard]] uint32_t getQueueFamilyCount() const;
	[[nodiscard]] QueueFamily getQueueFamily(uint32_t index) const;
	[[nodiscard]] QueueFamily findQueueFamily(VkQueueFlags flags, bool exactMatch = false) const;
	[[nodiscard]] QueueFamily findPresentQueueFamily(VkSurfaceKHR surface) const;

	[[nodiscard]] std::string toString() const;
	
	[[nodiscard]] bool areQueueFlagsSupported(VkQueueFlags flags, bool singleQueue = false) const;
	[[nodiscard]] bool isQueueFlagSupported(VkQueueFlagBits flag) const;
	[[nodiscard]] bool isPresentSupported(VkSurfaceKHR surface) const;

private:
	GPUQueueStructure() = default;
	explicit GPUQueueStructure(VulkanGPU gpu);

	std::vector<QueueFamily> queueFamilies;
	VulkanGPU gpu;

	friend class VulkanGPU;
	friend class QueueFamilySelector;
};

class QueueFamily
{
public:
	VkQueueFamilyProperties properties;
	uint32_t index;
	VulkanGPU gpu;

	bool isPresentSupported(VkSurfaceKHR surface) const;

private:
	QueueFamily(const VkQueueFamilyProperties& properties, uint32_t index, VulkanGPU gpu);

	friend class GPUQueueStructure;
};

class VulkanQueue
{
public:
	void waitIdle() const;

	VkQueue operator*() const;

private:
	explicit VulkanQueue(VkQueue queue);

	VkQueue m_vkHandle;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
	friend class SDLWindow;
};

struct QueueSelection
{
	uint32_t familyIndex = UINT32_MAX;
	uint32_t queueIndex = UINT32_MAX;
};

class QueueFamilySelector
{
public:
	explicit QueueFamilySelector(const GPUQueueStructure& structure);

	void selectQueueFamily(const QueueFamily& family, QueueFamilyTypes typeMask);
	QueueSelection getOrAddQueue(const QueueFamily& family, float priority);
	QueueSelection addQueue(const QueueFamily& family, float priority);

	[[nodiscard]] std::optional<QueueFamily> getQueueFamilyByType(QueueFamilyTypes type);
	[[nodiscard]] std::vector<uint32_t> getUniqueIndices() const;

	static QueueFamilyTypes getTypesFromFlags(VkQueueFlags flags);

private:
	struct QueueSelections
	{
		QueueFamilyTypes familyFlags;
		std::vector<float> priorities;
	};
	GPUQueueStructure m_structure;

	std::vector<QueueSelections> m_selections;

	friend class GPUQueueStructure;
	friend class VulkanContext;
};