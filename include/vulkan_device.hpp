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
    typedef uint32_t ThreadID;

    template<typename T>
    [[nodiscard]] T* getSubresource(ResourceID p_ID) const;

    template<typename T>
    bool freeSubresource(ResourceID p_ID);

    [[nodiscard]] VulkanDeviceSubresource* getSubresource(ResourceID p_ID) const;
    bool freeSubresource(ResourceID p_ID);

	void configureOneTimeQueue(QueueSelection p_Queue);

	void initializeOneTimeCommandPool(ThreadID p_ThreadID);
	void initializeCommandPool(const QueueFamily& p_Family, ThreadID p_ThreadID, bool secondary);
	ResourceID createCommandBuffer(const QueueFamily& p_Family, ThreadID p_ThreadID, bool p_IsSecondary);
	ResourceID createOneTimeCommandBuffer(ThreadID p_ThreadID);
	ResourceID getOrCreateCommandBuffer(const QueueFamily& p_Family, ThreadID p_ThreadID, VulkanCommandBuffer::TypeFlags p_Flags);
	VulkanCommandBuffer& getCommandBuffer(ResourceID p_ID, ThreadID p_ThreadID);
    [[nodiscard]] const VulkanCommandBuffer& getCommandBuffer(ResourceID p_ID, ThreadID p_ThreadID) const;
	void freeCommandBuffer(const VulkanCommandBuffer& p_CommandBuffer, ThreadID p_ThreadID);
	void freeCommandBuffer(ResourceID p_ID, ThreadID p_ThreadID);

    [[nodiscard]] std::vector<VulkanFramebuffer*> getFramebuffers() const;
    [[nodiscard]] uint32_t getFramebufferCount() const;
	ResourceID createFramebuffer(VkExtent3D p_Size, const VulkanRenderPass& p_RenderPass, const std::vector<VkImageView>& p_Attachments);
    VulkanFramebuffer& getFramebuffer(const ResourceID p_ID) { return *getSubresource<VulkanFramebuffer>(p_ID); }
    [[nodiscard]] const VulkanFramebuffer& getFramebuffer(const ResourceID p_ID) const { return *getSubresource<VulkanFramebuffer>(p_ID); }
    [[nodiscard]] bool freeFramebuffer(const ResourceID p_ID) { return freeSubresource<VulkanFramebuffer>(p_ID); }
    [[nodiscard]] bool freeFramebuffer(const VulkanFramebuffer& framebuffer) { return freeSubresource<VulkanFramebuffer>(framebuffer.getID()); }

	ResourceID createBuffer(VkDeviceSize p_Size, VkBufferUsageFlags p_Usage);
    VulkanBuffer& getBuffer(const ResourceID p_ID) { return *getSubresource<VulkanBuffer>(p_ID); }
    [[nodiscard]] const VulkanBuffer& getBuffer(const ResourceID p_ID) const { return *getSubresource<VulkanBuffer>(p_ID); }
    bool freeBuffer(const ResourceID p_ID) { return freeSubresource<VulkanBuffer>(p_ID); }
    bool freeBuffer(const VulkanBuffer& buffer) { return freeSubresource<VulkanBuffer>(buffer.getID()); }

    ResourceID createImage(VkImageType p_Type, VkFormat p_Format, VkExtent3D p_Extent, VkImageUsageFlags p_Usage, VkImageCreateFlags p_Flags);
    VulkanImage& getImage(const ResourceID p_ID) { return *getSubresource<VulkanImage>(p_ID); }
    [[nodiscard]] const VulkanImage& getImage(const ResourceID p_ID) const { return *getSubresource<VulkanImage>(p_ID); }
    bool freeImage(const ResourceID p_ID) { return freeSubresource<VulkanImage>(p_ID); }
    bool freeImage(const VulkanImage& image) { return freeSubresource<VulkanImage>(image.getID()); }

	void disallowMemoryType(uint32_t type);
	void allowMemoryType(uint32_t type);

	ResourceID createRenderPass(const VulkanRenderPassBuilder& builder, VkRenderPassCreateFlags flags);
    VulkanRenderPass& getRenderPass(const ResourceID p_ID) { return *getSubresource<VulkanRenderPass>(p_ID); }
    [[nodiscard]] const VulkanRenderPass& getRenderPass(const ResourceID p_ID) const { return *getSubresource<VulkanRenderPass>(p_ID); }
    bool freeRenderPass(const ResourceID p_ID) { return freeSubresource<VulkanRenderPass>(p_ID); }
    bool freeRenderPass(const VulkanRenderPass& renderPass) { return freeSubresource<VulkanRenderPass>(renderPass.getID()); }

	ResourceID createPipelineLayout(const std::vector<uint32_t>& descriptorSetLayouts, const std::vector<VkPushConstantRange>& pushConstantRanges);
    VulkanPipelineLayout& getPipelineLayout(const ResourceID p_ID) { return *getSubresource<VulkanPipelineLayout>(p_ID); }
    [[nodiscard]] const VulkanPipelineLayout& getPipelineLayout(const ResourceID p_ID) const { return *getSubresource<VulkanPipelineLayout>(p_ID); }
    bool freePipelineLayout(const ResourceID p_ID) { return freeSubresource<VulkanPipelineLayout>(p_ID); }
    bool freePipelineLayout(const VulkanPipelineLayout& layout) { return freeSubresource<VulkanPipelineLayout>(layout.getID()); }

	ResourceID createShader(const std::string& filename, VkShaderStageFlagBits stage, bool getReflection, const std::vector<VulkanShader::MacroDef>& macros);
    VulkanShader& getShader(const ResourceID p_ID) { return *getSubresource<VulkanShader>(p_ID); }
    [[nodiscard]] const VulkanShader& getShader(const ResourceID p_ID) const { return *getSubresource<VulkanShader>(p_ID); }
    bool freeShader(const ResourceID p_ID) { return freeSubresource<VulkanShader>(p_ID); }
    bool freeShader(const VulkanShader& shader) { return freeSubresource<VulkanShader>(shader.getID()); }
	bool freeAllShaders();

	ResourceID createPipeline(const VulkanPipelineBuilder& builder, uint32_t pipelineLayout, uint32_t renderPass, uint32_t subpass);
    VulkanPipeline& getPipeline(const ResourceID p_ID) { return *getSubresource<VulkanPipeline>(p_ID); }
    [[nodiscard]] const VulkanPipeline& getPipeline(const ResourceID p_ID) const { return *getSubresource<VulkanPipeline>(p_ID); }
    bool freePipeline(const ResourceID p_ID) { return freeSubresource<VulkanPipeline>(p_ID); }
    bool freePipeline(const VulkanPipeline& pipeline) { return freeSubresource<VulkanPipeline>(pipeline.getID()); }

	ResourceID createDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags);
    VulkanDescriptorPool& getDescriptorPool(const ResourceID p_ID) { return *getSubresource<VulkanDescriptorPool>(p_ID); }
    [[nodiscard]] const VulkanDescriptorPool& getDescriptorPool(const ResourceID p_ID) const { return *getSubresource<VulkanDescriptorPool>(p_ID); }
    bool freeDescriptorPool(const ResourceID p_ID) { return freeSubresource<VulkanDescriptorPool>(p_ID); }
    bool freeDescriptorPool(const VulkanDescriptorPool& descriptorPool) { return freeSubresource<VulkanDescriptorPool>(descriptorPool.getID()); }

	ResourceID createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayoutCreateFlags flags);
    VulkanDescriptorSetLayout& getDescriptorSetLayout(const ResourceID p_ID) { return *getSubresource<VulkanDescriptorSetLayout>(p_ID); }
    [[nodiscard]] const VulkanDescriptorSetLayout& getDescriptorSetLayout(const ResourceID p_ID) const { return *getSubresource<VulkanDescriptorSetLayout>(p_ID); }
    bool freeDescriptorSetLayout(const ResourceID p_ID) { return freeSubresource<VulkanDescriptorSetLayout>(p_ID); }
    bool freeDescriptorSetLayout(const VulkanDescriptorSetLayout& layout) { return freeSubresource<VulkanDescriptorSetLayout>(layout.getID()); }
    
	ResourceID createDescriptorSet(uint32_t pool, uint32_t layout);
	std::vector<ResourceID> createDescriptorSets(uint32_t pool, uint32_t layout, uint32_t count);
    VulkanDescriptorSet& getDescriptorSet(const ResourceID p_ID) { return *getSubresource<VulkanDescriptorSet>(p_ID); }
    [[nodiscard]] const VulkanDescriptorSet& getDescriptorSet(const ResourceID p_ID) const { return *getSubresource<VulkanDescriptorSet>(p_ID); }
    bool freeDescriptorSet(const ResourceID p_ID) { return freeSubresource<VulkanDescriptorSet>(p_ID); }
    bool freeDescriptorSet(const VulkanDescriptorSet& descriptorSet) { return freeSubresource<VulkanDescriptorSet>(descriptorSet.getID()); }
    void updateDescriptorSets(const std::vector<VkWriteDescriptorSet>& descriptorWrites) const;

	ResourceID createSwapchain(VkSurfaceKHR surface, VkExtent2D extent, VkSurfaceFormatKHR desiredFormat, uint32_t oldSwapchain = UINT32_MAX);
    VulkanSwapchain& getSwapchain(const ResourceID p_ID) { return *getSubresource<VulkanSwapchain>(p_ID); }
    [[nodiscard]] const VulkanSwapchain& getSwapchain(const ResourceID p_ID) const { return *getSubresource<VulkanSwapchain>(p_ID); }
    bool freeSwapchain(const ResourceID p_ID) { return freeSubresource<VulkanSwapchain>(p_ID); }
    bool freeSwapchain(const VulkanSwapchain& swapchain) { return freeSubresource<VulkanSwapchain>(swapchain.getID()); }

	ResourceID createSemaphore();
    VulkanSemaphore& getSemaphore(const ResourceID p_ID) { return *getSubresource<VulkanSemaphore>(p_ID); }
    [[nodiscard]] const VulkanSemaphore& getSemaphore(const ResourceID p_ID) const { return *getSubresource<VulkanSemaphore>(p_ID); }
    bool freeSemaphore(const ResourceID p_ID) { return freeSubresource<VulkanSemaphore>(p_ID); }
    bool freeSemaphore(const VulkanSemaphore& semaphore) { return freeSubresource<VulkanSemaphore>(semaphore.getID()); }

	ResourceID createFence(bool signaled);
    VulkanFence& getFence(const ResourceID p_ID) { return *getSubresource<VulkanFence>(p_ID); }
    [[nodiscard]] const VulkanFence& getFence(const ResourceID p_ID) const { return *getSubresource<VulkanFence>(p_ID); }
    bool freeFence(const ResourceID p_ID) { return freeSubresource<VulkanFence>(p_ID); }
    bool freeFence(const VulkanFence& fence) { return freeSubresource<VulkanFence>(fence.getID()); }

	void waitIdle() const;

	[[nodiscard]] bool isStagingBufferConfigured() const;
    [[nodiscard]] VkDeviceSize getStagingBufferSize() const;
	void configureStagingBuffer(VkDeviceSize size, const QueueSelection& queue, bool forceAllowStagingMemory = false);
    bool freeStagingBuffer();
	void* mapStagingBuffer(VkDeviceSize size, VkDeviceSize offset);
	void unmapStagingBuffer();
	void dumpStagingBuffer(ResourceID buffer, VkDeviceSize size, VkDeviceSize offset, ThreadID threadID);
	void dumpStagingBuffer(ResourceID buffer, const std::vector<VkBufferCopy>& regions, ThreadID threadID);
    void dumpStagingBufferToImage(ResourceID image, VkExtent3D size, VkOffset3D offset, ThreadID threadID, bool keepLayout = false);

	[[nodiscard]] VulkanQueue getQueue(const QueueSelection& p_QueueSelection) const;
    [[nodiscard]] VulkanGPU getGPU() const { return m_PhysicalDevice; }
    [[nodiscard]] const VulkanMemoryAllocator& getMemoryAllocator() const { return m_MemoryAllocator; }

    VkDevice operator*() const { return m_VkHandle; }

private:
	bool free();

    VulkanMemoryAllocator& getMemoryAllocator() { return m_MemoryAllocator; }

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

	VkDevice m_VkHandle;

	VulkanGPU m_PhysicalDevice;

	std::map<ThreadID, ThreadCommandInfo> m_ThreadCommandInfos;
	std::unordered_map<ThreadID, std::vector<VulkanCommandBuffer>> m_CommandBuffers;
    std::unordered_map<ResourceID, VulkanDeviceSubresource*> m_Subresources;

	VulkanMemoryAllocator m_MemoryAllocator;
	QueueSelection m_OneTimeQueue{UINT32_MAX, UINT32_MAX};

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

template <typename T>
T* VulkanDevice::getSubresource(const ResourceID p_ID) const
{

    if (m_Subresources.contains(p_ID))
    {
        return static_cast<T*>(m_Subresources.at(p_ID));
    }
    return nullptr;
}

template <typename T>
bool VulkanDevice::freeSubresource(const ResourceID p_ID)
{
    static_assert(std::is_base_of_v<VulkanDeviceSubresource, T>, "T must be a VulkanDeviceComponent");

    VulkanDeviceSubresource* l_Component = getSubresource(p_ID);
    if (l_Component)
    {
        if (!dynamic_cast<T*>(l_Component))
        {
            return false;
        }
        l_Component->free();
        m_Subresources.erase(p_ID);
        delete l_Component;
        return true;
    }
    return false;
}
