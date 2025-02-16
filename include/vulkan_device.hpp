#pragma once
#include <unordered_map>

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

#include "vulkan_context.hpp"
#include "utils/allocators.hpp"

class VulkanDeviceExtensionManager;
typedef uint32_t ThreadID;

class VulkanDevice : public Identifiable
{
public:

    [[nodiscard]] VulkanDeviceExtensionManager* getExtensionManager() const { return m_ExtensionManager; }

    template<typename T>
    [[nodiscard]] T* getSubresource(ResourceID p_ID) const;

    template<typename T>
    bool freeSubresource(ResourceID p_ID);

    [[nodiscard]] VulkanDeviceSubresource* getSubresource(ResourceID p_ID) const;
    bool freeSubresource(ResourceID p_ID);

	void configureOneTimeQueue(QueueSelection p_Queue);

	void initializeOneTimeCommandPool(ThreadID p_ThreadID);
	void initializeCommandPool(const QueueFamily& p_Family, ThreadID p_ThreadID, bool p_AllowBufferReset = false);
	ResourceID createCommandBuffer(const QueueFamily& p_Family, ThreadID p_ThreadID, bool p_IsSecondary);
	ResourceID createOneTimeCommandBuffer(ThreadID p_ThreadID);
	ResourceID getOrCreateCommandBuffer(const QueueFamily& p_Family, ThreadID p_ThreadID, VulkanCommandBuffer::TypeFlags p_Flags);
	VulkanCommandBuffer& getCommandBuffer(ResourceID p_ID, ThreadID p_ThreadID);
    [[nodiscard]] const VulkanCommandBuffer& getCommandBuffer(ResourceID p_ID, ThreadID p_ThreadID) const;
	void freeCommandBuffer(const VulkanCommandBuffer& p_CommandBuffer, ThreadID p_ThreadID);
	void freeCommandBuffer(ResourceID p_ID, ThreadID p_ThreadID);

    [[nodiscard]] std::vector<VulkanFramebuffer*> getFramebuffers() const;
    [[nodiscard]] uint32_t getFramebufferCount() const;
	ResourceID createFramebuffer(VkExtent3D p_Size, ResourceID p_RenderPass, std::span<const VkImageView> p_Attachments);
    VulkanFramebuffer& getFramebuffer(const ResourceID p_ID) { return *getSubresource<VulkanFramebuffer>(p_ID); }
    [[nodiscard]] const VulkanFramebuffer& getFramebuffer(const ResourceID p_ID) const { return *getSubresource<VulkanFramebuffer>(p_ID); }
    bool freeFramebuffer(const ResourceID p_ID) { return freeSubresource<VulkanFramebuffer>(p_ID); }
    bool freeFramebuffer(const VulkanFramebuffer& p_Framebuffer) { return freeSubresource<VulkanFramebuffer>(p_Framebuffer.getID()); }

	ResourceID createBuffer(VkDeviceSize p_Size, VkBufferUsageFlags p_Usage, uint32_t p_OwnerQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);
    VulkanBuffer& getBuffer(const ResourceID p_ID) { return *getSubresource<VulkanBuffer>(p_ID); }
    [[nodiscard]] const VulkanBuffer& getBuffer(const ResourceID p_ID) const { return *getSubresource<VulkanBuffer>(p_ID); }
    bool freeBuffer(const ResourceID p_ID) { return freeSubresource<VulkanBuffer>(p_ID); }
    bool freeBuffer(const VulkanBuffer& p_Buffer) { return freeSubresource<VulkanBuffer>(p_Buffer.getID()); }

    ResourceID createImage(VkImageType p_Type, VkFormat p_Format, VkExtent3D p_Extent, VkImageUsageFlags p_Usage, VkImageCreateFlags p_Flags);
    VulkanImage& getImage(const ResourceID p_ID) { return *getSubresource<VulkanImage>(p_ID); }
    [[nodiscard]] const VulkanImage& getImage(const ResourceID p_ID) const { return *getSubresource<VulkanImage>(p_ID); }
    bool freeImage(const ResourceID p_ID) { return freeSubresource<VulkanImage>(p_ID); }
    bool freeImage(const VulkanImage& p_Image) { return freeSubresource<VulkanImage>(p_Image.getID()); }

	void disallowMemoryType(uint32_t p_Type);
	void allowMemoryType(uint32_t p_Type);

	ResourceID createRenderPass(const VulkanRenderPassBuilder& p_Builder, VkRenderPassCreateFlags p_Flags);
    VulkanRenderPass& getRenderPass(const ResourceID p_ID) { return *getSubresource<VulkanRenderPass>(p_ID); }
    [[nodiscard]] const VulkanRenderPass& getRenderPass(const ResourceID p_ID) const { return *getSubresource<VulkanRenderPass>(p_ID); }
    bool freeRenderPass(const ResourceID p_ID) { return freeSubresource<VulkanRenderPass>(p_ID); }
    bool freeRenderPass(const VulkanRenderPass& p_RenderPass) { return freeSubresource<VulkanRenderPass>(p_RenderPass.getID()); }

	ResourceID createPipelineLayout(std::span<const ResourceID> p_DescriptorSetLayouts, std::span<const VkPushConstantRange> p_PushConstantRanges);
    VulkanPipelineLayout& getPipelineLayout(const ResourceID p_ID) { return *getSubresource<VulkanPipelineLayout>(p_ID); }
    [[nodiscard]] const VulkanPipelineLayout& getPipelineLayout(const ResourceID p_ID) const { return *getSubresource<VulkanPipelineLayout>(p_ID); }
    bool freePipelineLayout(const ResourceID p_ID) { return freeSubresource<VulkanPipelineLayout>(p_ID); }
    bool freePipelineLayout(const VulkanPipelineLayout& p_Layout) { return freeSubresource<VulkanPipelineLayout>(p_Layout.getID()); }

	ResourceID createShader(std::string_view p_Filename, VkShaderStageFlagBits p_Stage, bool p_GetReflection, std::span<const VulkanShader::MacroDef> p_Macros);
    VulkanShader& getShader(const ResourceID p_ID) { return *getSubresource<VulkanShader>(p_ID); }
    [[nodiscard]] const VulkanShader& getShader(const ResourceID p_ID) const { return *getSubresource<VulkanShader>(p_ID); }
    bool freeShader(const ResourceID p_ID) { return freeSubresource<VulkanShader>(p_ID); }
    bool freeShader(const VulkanShader& p_Shader) { return freeSubresource<VulkanShader>(p_Shader.getID()); }
	bool freeAllShaders();

	ResourceID createPipeline(const VulkanPipelineBuilder& p_Builder, uint32_t p_PipelineLayout, uint32_t p_RenderPass, uint32_t p_Subpass);
    VulkanPipeline& getPipeline(const ResourceID p_ID) { return *getSubresource<VulkanPipeline>(p_ID); }
    [[nodiscard]] const VulkanPipeline& getPipeline(const ResourceID p_ID) const { return *getSubresource<VulkanPipeline>(p_ID); }
    bool freePipeline(const ResourceID p_ID) { return freeSubresource<VulkanPipeline>(p_ID); }
    bool freePipeline(const VulkanPipeline& p_Pipeline) { return freeSubresource<VulkanPipeline>(p_Pipeline.getID()); }

	ResourceID createDescriptorPool(std::span<const VkDescriptorPoolSize> p_PoolSizes, uint32_t p_MaxSets, VkDescriptorPoolCreateFlags p_Flags);
    VulkanDescriptorPool& getDescriptorPool(const ResourceID p_ID) { return *getSubresource<VulkanDescriptorPool>(p_ID); }
    [[nodiscard]] const VulkanDescriptorPool& getDescriptorPool(const ResourceID p_ID) const { return *getSubresource<VulkanDescriptorPool>(p_ID); }
    bool freeDescriptorPool(const ResourceID p_ID) { return freeSubresource<VulkanDescriptorPool>(p_ID); }
    bool freeDescriptorPool(const VulkanDescriptorPool& p_DescriptorPool) { return freeSubresource<VulkanDescriptorPool>(p_DescriptorPool.getID()); }

	ResourceID createDescriptorSetLayout(std::span<const VkDescriptorSetLayoutBinding> p_Bindings, VkDescriptorSetLayoutCreateFlags p_Flags);
    VulkanDescriptorSetLayout& getDescriptorSetLayout(const ResourceID p_ID) { return *getSubresource<VulkanDescriptorSetLayout>(p_ID); }
    [[nodiscard]] const VulkanDescriptorSetLayout& getDescriptorSetLayout(const ResourceID p_ID) const { return *getSubresource<VulkanDescriptorSetLayout>(p_ID); }
    bool freeDescriptorSetLayout(const ResourceID p_ID) { return freeSubresource<VulkanDescriptorSetLayout>(p_ID); }
    bool freeDescriptorSetLayout(const VulkanDescriptorSetLayout& p_Layout) { return freeSubresource<VulkanDescriptorSetLayout>(p_Layout.getID()); }
    
	ResourceID createDescriptorSet(uint32_t p_Pool, uint32_t p_Layout);
    void createDescriptorSets(uint32_t p_Pool, uint32_t p_Layout, uint32_t p_Count, ResourceID p_Container[]);
    VulkanDescriptorSet& getDescriptorSet(const ResourceID p_ID) { return *getSubresource<VulkanDescriptorSet>(p_ID); }
    [[nodiscard]] const VulkanDescriptorSet& getDescriptorSet(const ResourceID p_ID) const { return *getSubresource<VulkanDescriptorSet>(p_ID); }
    bool freeDescriptorSet(const ResourceID p_ID) { return freeSubresource<VulkanDescriptorSet>(p_ID); }
    bool freeDescriptorSet(const VulkanDescriptorSet& p_DescriptorSet) { return freeSubresource<VulkanDescriptorSet>(p_DescriptorSet.getID()); }
    void updateDescriptorSets(std::span<const VkWriteDescriptorSet> p_DescriptorWrites) const;

	ResourceID createSemaphore();
    VulkanSemaphore& getSemaphore(const ResourceID p_ID) { return *getSubresource<VulkanSemaphore>(p_ID); }
    [[nodiscard]] const VulkanSemaphore& getSemaphore(const ResourceID p_ID) const { return *getSubresource<VulkanSemaphore>(p_ID); }
    bool freeSemaphore(const ResourceID p_ID) { return freeSubresource<VulkanSemaphore>(p_ID); }
    bool freeSemaphore(const VulkanSemaphore& p_Semaphore) { return freeSubresource<VulkanSemaphore>(p_Semaphore.getID()); }

	ResourceID createFence(bool p_Signaled);
    VulkanFence& getFence(const ResourceID p_ID) { return *getSubresource<VulkanFence>(p_ID); }
    [[nodiscard]] const VulkanFence& getFence(const ResourceID p_ID) const { return *getSubresource<VulkanFence>(p_ID); }
    bool freeFence(const ResourceID p_ID) { return freeSubresource<VulkanFence>(p_ID); }
    bool freeFence(const VulkanFence& p_Fence) { return freeSubresource<VulkanFence>(p_Fence.getID()); }

	void waitIdle() const;

	[[nodiscard]] bool isStagingBufferConfigured() const;
    [[nodiscard]] VkDeviceSize getStagingBufferSize() const;
	void configureStagingBuffer(VkDeviceSize p_Size, const QueueSelection& p_Queue, bool p_ForceAllowStagingMemory = false);
    bool freeStagingBuffer();
	void* mapStagingBuffer(VkDeviceSize p_Size, VkDeviceSize p_Offset);
	void unmapStagingBuffer();
	void dumpStagingBuffer(ResourceID p_Buffer, VkDeviceSize p_Size, VkDeviceSize p_Offset, ThreadID p_ThreadID);
	void dumpStagingBuffer(ResourceID p_Buffer, std::span<const VkBufferCopy> p_Regions, ThreadID p_ThreadID);
    void dumpStagingBufferToImage(ResourceID p_Image, VkExtent3D p_Size, VkOffset3D p_Offset, ThreadID p_ThreadID, bool p_KeepLayout = false);
    void dumpDataIntoBuffer(ResourceID p_DestBuffer, const uint8_t* p_Data, VkDeviceSize p_Size, ThreadID p_ThreadID);
    void dumpDataIntoImage(ResourceID p_DestImage, const uint8_t* p_Data, VkExtent3D p_Extent, uint32_t p_BytesPerPixel, ThreadID p_ThreadID, bool p_KeepLayout);

	[[nodiscard]] VulkanQueue getQueue(const QueueSelection& p_QueueSelection) const;
    [[nodiscard]] VulkanGPU getGPU() const { return m_PhysicalDevice; }
    [[nodiscard]] const VulkanMemoryAllocator& getMemoryAllocator() const { return m_MemoryAllocator; }

    VkDevice operator*() const { return m_VkHandle; }

    VulkanMemoryAllocator& getMemoryAllocator() { return m_MemoryAllocator; }
	[[nodiscard]] VkDeviceMemory getMemoryHandle(uint32_t p_ChunkID) const;
    [[nodiscard]] const VolkDeviceTable& getTable() const { return m_VolkDeviceTable; }

private:
	bool free();

    VkCommandPool getCommandPool(uint32_t p_QueueFamilyIndex, uint32_t p_ThreadID, VulkanCommandBuffer::TypeFlags p_Flags);

	VulkanDevice(VulkanGPU p_PhysicalDevice, VkDevice p_Device, VulkanDeviceExtensionManager* p_ExtensionManager);

	struct ThreadCommandInfo
	{
        using QueueFamilyIndex = uint32_t;

		VkCommandPool oneTimePool = VK_NULL_HANDLE;
        ARENA_UMAP(commandPools, QueueFamilyIndex, VkCommandPool);
	};

	struct StagingBufferInfo
	{
		uint32_t stagingBuffer = UINT32_MAX;
		QueueSelection queue{};
	} m_StagingBufferInfo;

    struct ThreadCmdBuffers
    {
        ARENA_VECTOR(buffers, VulkanCommandBuffer);
    };

	VkDevice m_VkHandle;

	VulkanGPU m_PhysicalDevice;

    ARENA_UMAP(m_ThreadCommandInfos, ThreadID, ThreadCommandInfo);
    ARENA_UMAP(m_CommandBuffers, ThreadID, ThreadCmdBuffers);
    ARENA_UMAP(m_Subresources, ResourceID, VulkanDeviceSubresource*);

	VulkanMemoryAllocator m_MemoryAllocator;
	QueueSelection m_OneTimeQueue{UINT32_MAX, UINT32_MAX};

    VulkanDeviceExtensionManager* m_ExtensionManager = nullptr;

    VolkDeviceTable m_VolkDeviceTable;

private:
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

    friend class VulkanDeviceExtensionManager;
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
        ARENA_FREE_NOSIZE(l_Component);
        return true;
    }
    return false;
}
