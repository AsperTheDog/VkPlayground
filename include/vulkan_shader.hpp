#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

#include "utils/identifiable.hpp"

class VulkanDevice;

class VulkanShader : public Identifiable
{
public:
	static [[nodiscard]] shaderc_shader_kind getKindFromStage(VkShaderStageFlagBits stage);

	VkShaderModule operator*() const;

private:
	void free();

	struct Result
	{
		bool success = false;
		std::vector<uint32_t> code;
		std::string error;
	};

	VulkanShader(uint32_t device, VkShaderModule handle, VkShaderStageFlagBits stage);

	static std::string readFile(const std::string_view p_filename, const std::vector<std::pair<std::string_view, std::string_view>>& replaceTags);
	static [[nodiscard]] Result compileFile(std::string_view p_source_name, shaderc_shader_kind p_kind, std::string_view p_source, bool p_optimize);

	VkShaderModule m_vkHandle = VK_NULL_HANDLE;
	VkShaderStageFlagBits m_stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

	uint32_t m_device;

	friend class VulkanDevice;
	friend class VulkanPipeline;
	friend struct VulkanPipelineBuilder;
};

