#include "ext/vulkan_deferred_host_operation.hpp"

#include "vulkan_context.hpp"


VulkanDeferredHostOperationsExtension* VulkanDeferredHostOperationsExtension::get(const VulkanDevice& p_Device)
{
    return p_Device.getExtensionManager()->getExtension<VulkanDeferredHostOperationsExtension>(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
}

VulkanDeferredHostOperationsExtension* VulkanDeferredHostOperationsExtension::get(const ResourceID p_DeviceID)
{
    return VulkanContext::getDevice(p_DeviceID).getExtensionManager()->getExtension<VulkanDeferredHostOperationsExtension>(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
}
