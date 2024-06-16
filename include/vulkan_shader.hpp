#pragma once
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

#include "spirv_reflect.h"
#include "utils/identifiable.hpp"

class VulkanDevice;

class VulkanShader : public Identifiable
{
public:
    struct MacroDef
    {
        std::string name;
        std::string value;
    };

	static [[nodiscard]] shaderc_shader_kind getKindFromStage(VkShaderStageFlagBits stage);

	VkShaderModule operator*() const;

    [[nodiscard]] std::vector<SpvReflectInterfaceVariable*> getInputs() const;
    [[nodiscard]] std::vector<SpvReflectInterfaceVariable*> getOutputs() const;
    [[nodiscard]] std::vector<SpvReflectEntryPoint> getEntryPoints() const;

    [[nodiscard]] std::vector<SpvReflectBlockVariable> getPushConstants() const;
    [[nodiscard]] std::vector<SpvReflectBlockVariable> getPushConstants(uint32_t set) const;
    [[nodiscard]] std::optional<SpvReflectBlockVariable> getPushConstant(uint32_t set, uint32_t binding) const;

    [[nodiscard]] std::vector<SpvReflectDescriptorSet> getDescriptorSets() const;
    [[nodiscard]] std::vector<SpvReflectDescriptorBinding> getDescriptorBindings() const;
    [[nodiscard]] std::vector<SpvReflectDescriptorBinding> getDescriptorBindings(uint32_t set) const;
    [[nodiscard]] std::vector<SpvReflectDescriptorBinding> getDescriptorBindings(VkDescriptorType type) const;
    [[nodiscard]] std::vector<SpvReflectDescriptorBinding> getDescriptorBindings(uint32_t set, VkDescriptorType type) const;

    [[nodiscard]] uint32_t getDescriptorBindingCount() const;
    [[nodiscard]] uint32_t getDescriptorSetCount() const;
    [[nodiscard]] uint32_t getPushConstantCount() const;

    [[nodiscard]] VkShaderStageFlagBits getStage() const { return m_stage; }

    [[nodiscard]] bool hasReflection() const { return m_hasReflection; }

private:
	void free();

	struct Result
	{
		bool success = false;
		std::vector<uint32_t> code;
		std::string error;
	};

	VulkanShader(uint32_t device, VkShaderModule handle, VkShaderStageFlagBits stage, const std::vector<uint32_t>& code = {});

	static std::string readFile(std::string_view p_filename);
	static [[nodiscard]] Result compileFile(std::string_view p_source_name, shaderc_shader_kind p_kind, std::string_view p_source, bool p_optimize, const std::vector<MacroDef>& macros);

	VkShaderModule m_vkHandle = VK_NULL_HANDLE;
	VkShaderStageFlagBits m_stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

	uint32_t m_device;

    bool m_hasReflection = false;
    SpvReflectShaderModule m_reflectModule;

	friend class VulkanDevice;
	friend class VulkanPipeline;
	friend struct VulkanPipelineBuilder;
};

