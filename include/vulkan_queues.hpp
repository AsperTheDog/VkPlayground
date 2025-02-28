#pragma once
#include <optional>
#include <string>
#include <vector>
#include <Volk/volk.h>

#include "vulkan_gpu.hpp"

struct QueueFamily;

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
	[[nodiscard]] QueueFamily getQueueFamily(uint32_t p_Index) const;
	[[nodiscard]] QueueFamily findQueueFamily(VkQueueFlags p_Flags, bool p_ExactMatch = false) const;
	[[nodiscard]] QueueFamily findPresentQueueFamily(VkSurfaceKHR p_Surface) const;

	[[nodiscard]] std::string toString() const;
	
	[[nodiscard]] bool areQueueFlagsSupported(VkQueueFlags p_Flags, bool SingleQueue = false) const;
	[[nodiscard]] bool isQueueFlagSupported(VkQueueFlagBits p_Flag) const;
	[[nodiscard]] bool isPresentSupported(VkSurfaceKHR p_Surface) const;

private:
	GPUQueueStructure() = default;
	explicit GPUQueueStructure(VulkanGPU p_GPU);

	std::vector<QueueFamily> m_QueueFamilies;
	VulkanGPU m_GPU;

	friend class VulkanGPU;
	friend class QueueFamilySelector;
};

struct QueueFamily
{
	VkQueueFamilyProperties properties;
	uint32_t index;
	VulkanGPU gpu;

	bool isPresentSupported(VkSurfaceKHR p_Surface) const;

    bool operator==(const QueueFamily& p_Other) const { return index == p_Other.index && gpu == p_Other.gpu; }

private:
	QueueFamily(const VkQueueFamilyProperties& p_Properties, uint32_t p_Index, VulkanGPU p_GPU);

	friend class GPUQueueStructure;
};

class VulkanQueue
{
public:
	void waitIdle() const;

	VkQueue operator*() const;

private:
	explicit VulkanQueue(VkQueue p_Queue);

	VkQueue m_VkHandle;

	friend class VulkanDevice;
	friend class VulkanCommandBuffer;
	friend class SDLWindow;
};

struct QueueSelection
{
	uint32_t familyIndex = UINT32_MAX;
	uint32_t queueIndex = UINT32_MAX;

    bool operator==(const QueueSelection& p_Other) const { return familyIndex == p_Other.familyIndex && queueIndex == p_Other.queueIndex; }
};

class QueueFamilySelector
{
public:
	explicit QueueFamilySelector(const GPUQueueStructure& p_Structure);

	void selectQueueFamily(const QueueFamily& p_Family, QueueFamilyTypes p_TypeMask);
	QueueSelection getOrAddQueue(const QueueFamily& p_Family, float p_Priority);
	QueueSelection addQueue(const QueueFamily& p_Family, float p_Priority);

	[[nodiscard]] std::optional<QueueFamily> getQueueFamilyByType(QueueFamilyTypes p_Type);
	[[nodiscard]] std::vector<uint32_t> getUniqueIndices() const;

	static QueueFamilyTypes getTypesFromFlags(VkQueueFlags P_Flags);

private:
	struct QueueSelections
	{
		QueueFamilyTypes familyFlags;
		std::vector<float> priorities;
	};
	GPUQueueStructure m_Structure;

	std::vector<QueueSelections> m_Selections;

	friend class GPUQueueStructure;
	friend class VulkanContext;
};