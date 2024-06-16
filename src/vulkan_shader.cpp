#include "vulkan_shader.hpp"

#include <fstream>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_context.hpp"
#include "vulkan_device.hpp"
#include "utils/logger.hpp"

shaderc_shader_kind VulkanShader::getKindFromStage(const VkShaderStageFlagBits stage)
{
	switch (stage)
	{
	case VK_SHADER_STAGE_VERTEX_BIT: return shaderc_vertex_shader;
	case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return shaderc_tess_control_shader;
	case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return shaderc_tess_evaluation_shader;
	case VK_SHADER_STAGE_GEOMETRY_BIT: return shaderc_geometry_shader;
	case VK_SHADER_STAGE_FRAGMENT_BIT: return shaderc_fragment_shader;
	case VK_SHADER_STAGE_COMPUTE_BIT: return shaderc_compute_shader;
	case VK_SHADER_STAGE_RAYGEN_BIT_KHR: return shaderc_raygen_shader;
	case VK_SHADER_STAGE_ANY_HIT_BIT_KHR: return shaderc_anyhit_shader;
	case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return shaderc_closesthit_shader;
	case VK_SHADER_STAGE_MISS_BIT_KHR: return shaderc_miss_shader;
	case VK_SHADER_STAGE_INTERSECTION_BIT_KHR: return shaderc_intersection_shader;
	case VK_SHADER_STAGE_CALLABLE_BIT_KHR: return shaderc_callable_shader;
	case VK_SHADER_STAGE_TASK_BIT_EXT: return shaderc_task_shader;
	case VK_SHADER_STAGE_MESH_BIT_EXT: return shaderc_mesh_shader;
    default: throw std::runtime_error(std::string("Unsupported shader stage ") + string_VkShaderStageFlagBits(stage));
	}
}

VkShaderModule VulkanShader::operator*() const
{
	return m_vkHandle;
}

SpvReflectInterfaceVariable** VulkanShader::getInputs() const
{
    if (!hasReflection()) return nullptr;

    uint32_t inputCount = 0;
    spvReflectEnumerateInputVariables(&m_reflectModule, &inputCount, nullptr);
    SpvReflectInterfaceVariable** inputs = new SpvReflectInterfaceVariable*[inputCount];
    spvReflectEnumerateInputVariables(&m_reflectModule, &inputCount, inputs);
    return inputs;
}

SpvReflectInterfaceVariable** VulkanShader::getOutputs() const
{
    if (!hasReflection()) return nullptr;

    uint32_t outputCount = 0;
    spvReflectEnumerateOutputVariables(&m_reflectModule, &outputCount, nullptr);
    SpvReflectInterfaceVariable** outputs = new SpvReflectInterfaceVariable*[outputCount];
    spvReflectEnumerateOutputVariables(&m_reflectModule, &outputCount, outputs);
    return outputs;

}

SpvReflectBlockVariable** VulkanShader::getPushConstants() const
{
    if (!hasReflection()) return nullptr;

    uint32_t pushConstantCount = 0;
    spvReflectEnumeratePushConstantBlocks(&m_reflectModule, &pushConstantCount, nullptr);
    SpvReflectBlockVariable** pushConstants = new SpvReflectBlockVariable*[pushConstantCount];
    spvReflectEnumeratePushConstantBlocks(&m_reflectModule, &pushConstantCount, pushConstants);
    return pushConstants;
}

SpvReflectDescriptorBinding** VulkanShader::getDescriptorBindings() const
{
    if (!hasReflection()) return nullptr;

    uint32_t descriptorBindingCount = 0;
    spvReflectEnumerateDescriptorBindings(&m_reflectModule, &descriptorBindingCount, nullptr);
    SpvReflectDescriptorBinding** descriptorBindings = new SpvReflectDescriptorBinding*[descriptorBindingCount];
    spvReflectEnumerateDescriptorBindings(&m_reflectModule, &descriptorBindingCount, descriptorBindings);
    return descriptorBindings;
}

SpvReflectDescriptorSet** VulkanShader::getDescriptorSets() const
{
    if (!hasReflection()) return nullptr;

    uint32_t descriptorSetCount = 0;
    spvReflectEnumerateDescriptorSets(&m_reflectModule, &descriptorSetCount, nullptr);
    SpvReflectDescriptorSet** descriptorSets = new SpvReflectDescriptorSet*[descriptorSetCount];
    spvReflectEnumerateDescriptorSets(&m_reflectModule, &descriptorSetCount, descriptorSets);
    return descriptorSets;
}

uint32_t VulkanShader::getDescriptorBindingCount() const
{
    if (!hasReflection()) return 0;
    return m_reflectModule.descriptor_binding_count;
}

uint32_t VulkanShader::getDescriptorSetCount() const
{
    if (!hasReflection()) return 0;
    return m_reflectModule.descriptor_set_count;
}

uint32_t VulkanShader::getPushConstantCount() const
{
    if (!hasReflection()) return 0;
    return m_reflectModule.push_constant_block_count;
}

void VulkanShader::free()
{
	if (m_vkHandle != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(VulkanContext::getDevice(m_device).m_vkHandle, m_vkHandle, nullptr);
        Logger::print("Freed shader module (ID: " + std::to_string(m_id) + ")", Logger::DEBUG);
		m_vkHandle = VK_NULL_HANDLE;
	}

    if (m_hasReflection)
    {
        spvReflectDestroyShaderModule(&m_reflectModule);
        m_hasReflection = false;
    }
}

VulkanShader::VulkanShader(const uint32_t device, const VkShaderModule handle, const VkShaderStageFlagBits stage, const std::vector<uint32_t>& code)
	:  m_vkHandle(handle), m_stage(stage), m_device(device)
{
    if (!code.empty())
    {
        m_hasReflection = true;
        if (const SpvReflectResult result = spvReflectCreateShaderModule(4 * code.size(), code.data(), &m_reflectModule); result != SPV_REFLECT_RESULT_SUCCESS)
            throw std::runtime_error("Failed to reflect shader module: " + std::to_string(result));
    }
}

std::string VulkanShader::readFile(const std::string_view p_filename)
{
	std::ifstream shaderFile(p_filename.data());
	if (!shaderFile.is_open()) throw std::runtime_error("failed to open shader file " + std::string(p_filename));
	std::string str((std::istreambuf_iterator(shaderFile)), std::istreambuf_iterator<char>());
	return str;
}

VulkanShader::Result VulkanShader::compileFile(const std::string_view p_source_name, const shaderc_shader_kind p_kind, const std::string_view p_source, const bool p_optimize, const std::vector<MacroDef>& macros)
{
	const shaderc::Compiler compiler;
	shaderc::CompileOptions options;

    for (const MacroDef& macro : macros)
        options.AddMacroDefinition(macro.name, macro.value);
    
	if (p_optimize) 
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
    else
    {
        options.SetOptimizationLevel(shaderc_optimization_level_zero);
        options.SetGenerateDebugInfo();
    }

	const shaderc::SpvCompilationResult module =
		compiler.CompileGlslToSpv(p_source.data(), p_kind, p_source_name.data(), options);

	if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
		return {false, {}, module.GetErrorMessage()};
	}

	return {true, { module.cbegin(), module.cend() }, ""};
}