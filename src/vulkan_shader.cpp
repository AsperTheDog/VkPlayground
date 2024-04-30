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

void VulkanShader::free()
{
	if (m_vkHandle != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(VulkanContext::getDevice(m_device).m_vkHandle, m_vkHandle, nullptr);
        Logger::print("Freed shader module (ID: " + std::to_string(m_id) + ")", Logger::DEBUG);
		m_vkHandle = VK_NULL_HANDLE;
	}
}

VulkanShader::VulkanShader(const uint32_t device, const VkShaderModule handle, const VkShaderStageFlagBits stage)
	:  m_vkHandle(handle), m_stage(stage), m_device(device)
{
}

std::string VulkanShader::readFile(const std::string_view p_filename, const std::vector<std::pair<std::string_view, std::string_view>>& replaceTags)
{
	std::ifstream shaderFile(p_filename.data());
	if (!shaderFile.is_open()) throw std::runtime_error("failed to open shader file " + std::string(p_filename));
	std::string str((std::istreambuf_iterator(shaderFile)), std::istreambuf_iterator<char>());
    for (const auto& [key, value] : replaceTags)
    {
        std::string tag = std::string("¿") + key.data() + "¿";
        size_t pos = str.find(tag);
        while (pos != std::string::npos)
        {
            str.replace(pos, tag.size(), value);
            pos = str.find(tag);
        }
    }
	return str;
}

VulkanShader::Result VulkanShader::compileFile(const std::string_view p_source_name, const shaderc_shader_kind p_kind, const std::string_view p_source, const bool p_optimize)
{
	const shaderc::Compiler compiler;
	shaderc::CompileOptions options;

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