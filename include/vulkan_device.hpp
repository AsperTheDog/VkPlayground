#pragma once
#include <map>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "utils/identifiable.hpp"
#include "vulkan_queues.hpp"
#include "vulkan_gpu.hpp"
#include "vulkan_memory.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_framebuffer.hpp"
#include "vulkan_image.hpp"
#include "vulkan_sync.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_shader.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_swapchain.hpp"


class VulkanDevice : public Identifiable
{
public:

	void configureOneTimeQueue(QueueSelection queue);

	void initializeOneTimeCommandPool(uint32_t threadID);
	void initializeCommandPool(const QueueFamily& family, uint32_t threadID, bool secondary);
	uint32_t createCommandBuffer(const QueueFamily& family, uint32_t threadID, bool isSecondary);
	uint32_t createOneTimeCommandBuffer(uint32_t threadID);
	uint32_t getOrCreateCommandBuffer(const QueueFamily& family, uint32_t threadID, VulkanCommandBuffer::TypeFlags flags);
	VulkanCommandBuffer& getCommandBuffer(uint32_t id, uint32_t threadID);
    [[nodiscard]] const VulkanCommandBuffer& getCommandBuffer(uint32_t id, uint32_t threadID) const;
	void freeCommandBuffer(const VulkanCommandBuffer& commandBuffer, uint32_t threadID);
	void freeCommandBuffer(uint32_t id, uint32_t threadID);

	uint32_t createFramebuffer(VkExtent3D size, const VulkanRenderPass& renderPass, const std::vector<VkImageView>& attachments);
	VulkanFramebuffer& getFramebuffer(uint32_t id);
    [[nodiscard]] const VulkanFramebuffer& getFramebuffer(uint32_t id) const;
	void freeFramebuffer(uint32_t id);
	void freeFramebuffer(const VulkanFramebuffer& framebuffer);

	uint32_t createBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
	VulkanBuffer& getBuffer(uint32_t id);
	[[nodiscard]] const VulkanBuffer& getBuffer(uint32_t id) const;
	void freeBuffer(uint32_t id);
	void freeBuffer(const VulkanBuffer& buffer);

	uint32_t createImage(VkImageType type, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VkImageCreateFlags flags);
	VulkanImage& getImage(uint32_t id);
	[[nodiscard]] const VulkanImage& getImage(uint32_t id) const;
	void freeImage(uint32_t id);
	void freeImage(const VulkanImage& image);

	void disallowMemoryType(uint32_t type);
	void allowMemoryType(uint32_t type);

	uint32_t createRenderPass(const VulkanRenderPassBuilder& builder, VkRenderPassCreateFlags flags);
	VulkanRenderPass& getRenderPass(uint32_t id);
	[[nodiscard]] const VulkanRenderPass& getRenderPass(uint32_t id) const;
	void freeRenderPass(uint32_t id);
	void freeRenderPass(const VulkanRenderPass& renderPass);

	uint32_t createPipelineLayout(const std::vector<uint32_t>& descriptorSetLayouts, const std::vector<VkPushConstantRange>& pushConstantRanges);
	VulkanPipelineLayout& getPipelineLayout(uint32_t id);
    [[nodiscard]] const VulkanPipelineLayout& getPipelineLayout(uint32_t id) const;
	void freePipelineLayout(uint32_t id);
	void freePipelineLayout(const VulkanPipelineLayout& layout);

	uint32_t createShader(const std::string& filename, VkShaderStageFlagBits stage, bool getReflection, const std::vector<VulkanShader::MacroDef>& macros);
	VulkanShader& getShader(uint32_t id);
    [[nodiscard]] const VulkanShader& getShader(uint32_t id) const;
	void freeShader(uint32_t id);
	void freeShader(const VulkanShader& shader);
	void freeAllShaders();

	uint32_t createPipeline(const VulkanPipelineBuilder& builder, uint32_t pipelineLayout, uint32_t renderPass, uint32_t subpass);
	VulkanPipeline& getPipeline(uint32_t id);
    [[nodiscard]] const VulkanPipeline& getPipeline(uint32_t id) const;
	void freePipeline(uint32_t id);
	void freePipeline(const VulkanPipeline& pipeline);

	uint32_t createDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags);
	VulkanDescriptorPool& getDescriptorPool(uint32_t id);
    [[nodiscard]] const VulkanDescriptorPool& getDescriptorPool(uint32_t id) const;
	void freeDescriptorPool(uint32_t id);
	void freeDescriptorPool(const VulkanDescriptorPool& descriptorPool);

	uint32_t createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayoutCreateFlags flags);
	VulkanDescriptorSetLayout& getDescriptorSetLayout(uint32_t id);
    [[nodiscard]] const VulkanDescriptorSetLayout& getDescriptorSetLayout(uint32_t id) const;
	void freeDescriptorSetLayout(uint32_t id);
	void freeDescriptorSetLayout(const VulkanDescriptorSetLayout& layout);

	uint32_t createDescriptorSet(uint32_t pool, uint32_t layout);
	std::vector<uint32_t> createDescriptorSets(uint32_t pool, uint32_t layout, uint32_t count);
	VulkanDescriptorSet& getDescriptorSet(uint32_t id);
    [[nodiscard]] const VulkanDescriptorSet& getDescriptorSet(uint32_t id) const;
	void freeDescriptorSet(uint32_t id);
	void freeDescriptorSet(const VulkanDescriptorSet& descriptorSet);
    void updateDescriptorSets(const std::vector<VkWriteDescriptorSet>& descriptorWrites) const;

	uint32_t createSwapchain(VkSurfaceKHR surface, VkExtent2D extent, VkSurfaceFormatKHR desiredFormat, uint32_t oldSwapchain = UINT32_MAX);
	VulkanSwapchain& getSwapchain(uint32_t id);
	[[nodiscard]] const VulkanSwapchain& getSwapchain(uint32_t id) const;
	void freeSwapchain(uint32_t id);
	void freeSwapchain(const VulkanSwapchain& swapchain);

	uint32_t createSemaphore();
	VulkanSemaphore& getSemaphore(uint32_t id);
    [[nodiscard]] const VulkanSemaphore& getSemaphore(uint32_t id) const;
	void freeSemaphore(uint32_t id);
	void freeSemaphore(const VulkanSemaphore& semaphore);

	uint32_t createFence(bool signaled);
	VulkanFence& getFence(uint32_t id);
    [[nodiscard]] const VulkanFence& getFence(uint32_t id) const;
	void freeFence(uint32_t id);
	void freeFence(const VulkanFence& fence);

	void waitIdle() const;

	[[nodiscard]] bool isStagingBufferConfigured() const;
    [[nodiscard]] VkDeviceSize getStagingBufferSize() const;
	void configureStagingBuffer(VkDeviceSize size, const QueueSelection& queue, bool forceAllowStagingMemory = false);
	void freeStagingBuffer();
	void* mapStagingBuffer(VkDeviceSize size, VkDeviceSize offset);
	void unmapStagingBuffer();
	void dumpStagingBuffer(uint32_t buffer, VkDeviceSize size, VkDeviceSize offset, uint32_t threadID);
	void dumpStagingBuffer(uint32_t buffer, const std::vector<VkBufferCopy>& regions, uint32_t threadID);
    void dumpStagingBufferToImage(uint32_t image, VkExtent3D size, VkOffset3D offset, uint32_t threadID, bool keepLayout = false);

	[[nodiscard]] VulkanQueue getQueue(const QueueSelection& queueSelection) const;
	[[nodiscard]] VulkanGPU getGPU() const;
	[[nodiscard]] const VulkanMemoryAllocator& getMemoryAllocator() const;

	VkDevice operator*() const;

private:
	void free();

	[[nodiscard]] VkDeviceMemory getMemoryHandle(uint32_t chunk) const;
    VkCommandPool getCommandPool(uint32_t uint32, uint32_t m_thread_id, VulkanCommandBuffer::TypeFlags flags);

	VulkanDevice(VulkanGPU pDevice, VkDevice device);

	struct ThreadCommandInfo
	{
		struct CommandPoolInfo
		{
			VkCommandPool pool = VK_NULL_HANDLE;
			VkCommandPool secondaryPool = VK_NULL_HANDLE;
		};

		VkCommandPool oneTimePool = VK_NULL_HANDLE;
		std::map<uint32_t, CommandPoolInfo> commandPools;
	};

	struct StagingBufferInfo
	{
		uint32_t stagingBuffer = UINT32_MAX;
		QueueSelection queue{};
	} m_stagingBufferInfo;

	VkDevice m_vkHandle;

	VulkanGPU m_physicalDevice;

	std::map<uint32_t, ThreadCommandInfo> m_threadCommandInfos;
	std::vector<VulkanFramebuffer> m_framebuffers;
	std::vector<VulkanBuffer> m_buffers;
	std::unordered_map<uint32_t /*threadID*/, std::vector<VulkanCommandBuffer>> m_commandBuffers;
	std::vector<VulkanRenderPass> m_renderPasses;
	std::vector<VulkanPipelineLayout> m_pipelineLayouts;
	std::vector<VulkanShader> m_shaders;
	std::vector<VulkanPipeline> m_pipelines;
	std::vector<VulkanDescriptorPool> m_descriptorPools;
	std::vector<VulkanDescriptorSetLayout> m_descriptorSetLayouts;
	std::vector<VulkanDescriptorSet> m_descriptorSets;
	std::vector<VulkanSwapchain> m_swapchains;
	std::vector<VulkanImage> m_images;
	std::vector<VulkanSemaphore> m_semaphores;
	std::vector<VulkanFence> m_fences;

	VulkanMemoryAllocator m_memoryAllocator;
	QueueSelection m_oneTimeQueue{UINT32_MAX, UINT32_MAX};

	friend class VulkanContext;
	friend class VulkanGPU;

	friend class VulkanResource;
	friend class VulkanMemoryAllocator;
	friend class VulkanBuffer;
    friend class VulkanCommandBuffer;
	friend class VulkanRenderPass;
	friend class VulkanImage;
	friend class VulkanFence;
	friend class VulkanSemaphore;
	friend class VulkanPipeline;
	friend class VulkanPipelineLayout;
	friend class VulkanFramebuffer;
	friend class VulkanShader;
	friend class VulkanDescriptorPool;
	friend class VulkanDescriptorSetLayout;
	friend class VulkanDescriptorSet;
	friend class VulkanSwapchain;
};
