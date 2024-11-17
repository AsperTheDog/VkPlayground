#include "vulkan_shader.hpp"

#include <fstream>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_context.hpp"
#include "vulkan_device.hpp"
#include "utils/logger.hpp"

#include "spirv_cross/spirv_glsl.hpp"

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

VulkanShader::ReflectionData VulkanShader::getReflectionData() const
{
    if (m_compiler == nullptr)
    {
        Logger::print("No reflection data available for shader (ID: " + std::to_string(m_id) + ")", Logger::DEBUG, false);
        return {};
    }

    return ReflectionData(m_compiler->get_shader_resources());

}

void VulkanShader::printReflectionData() const
{
        if (m_compiler == nullptr)
    {
        Logger::print("No reflection data available for shader (ID: " + std::to_string(m_id) + ")", Logger::DEBUG, false);
        return;
    }

    Logger::print("Reflection data for shader (ID: " + std::to_string(m_id) + ")", Logger::DEBUG, false);
    Logger::print("Inputs:", Logger::DEBUG, false);
    for (const auto& input : m_compiler->get_shader_resources().stage_inputs)
    {
        Logger::print("  " + input.name + " (" + std::to_string(input.type_id) + ")", Logger::DEBUG, false);
    }

    Logger::print("Outputs:", Logger::DEBUG, false);
    for (const auto& output : m_compiler->get_shader_resources().stage_outputs)
    {
        Logger::print("  " + output.name + " (" + std::to_string(output.type_id) + ")", Logger::DEBUG, false);
    }

    Logger::print("Uniform buffers:", Logger::DEBUG, false);
    for (const auto& uniformBuffer : m_compiler->get_shader_resources().uniform_buffers)
    {
        Logger::print("  " + uniformBuffer.name + " (" + std::to_string(uniformBuffer.type_id) + ")", Logger::DEBUG, false);
    }

    Logger::print("Storage buffers:", Logger::DEBUG, false);
    for (const auto& storageBuffer : m_compiler->get_shader_resources().storage_buffers)
    {
        Logger::print("  " + storageBuffer.name + " (" + std::to_string(storageBuffer.type_id) + ")", Logger::DEBUG, false);
    }

    Logger::print("Subpass inputs:", Logger::DEBUG, false);
    for (const auto& subpassInput : m_compiler->get_shader_resources().subpass_inputs)
    {
        Logger::print("  " + subpassInput.name + " (" + std::to_string(subpassInput.type_id) + ")", Logger::DEBUG, false);
    }

    Logger::print("Storage images:", Logger::DEBUG, false);
    for (const auto& storageImage : m_compiler->get_shader_resources().storage_images)
    {
        Logger::print("  " + storageImage.name + " (" + std::to_string(storageImage.type_id) + ")", Logger::DEBUG, false);
    }

    Logger::print("Sampled images:", Logger::DEBUG, false);
    for (const auto& sampledImage : m_compiler->get_shader_resources().sampled_images)
    {
        Logger::print("  " + sampledImage.name + " (" + std::to_string(sampledImage.type_id) + ")", Logger::DEBUG, false);
    }
}

VulkanShader::ReflectionData VulkanShader::getReflectionDataFromFile(const std::string& filepath, const VkShaderStageFlagBits stage)
{
    const VulkanShader::Result result = VulkanShader::compileFile(filepath, VulkanShader::getKindFromStage(stage), VulkanShader::readFile(filepath), false, {});
    if (!result.success)
    {
        Logger::print("Failed to compile shader file " + filepath + ": " + result.error, Logger::ERR);
        return {};
    }
    const spirv_cross::CompilerGLSL compiler(result.code);
    return ReflectionData(compiler.get_shader_resources());
}

void VulkanShader::free()
{
    if (m_vkHandle != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(VulkanContext::getDevice(m_device).m_vkHandle, m_vkHandle, nullptr);
        Logger::print("Freed shader module (ID: " + std::to_string(m_id) + ")", Logger::DEBUG);
        m_vkHandle = VK_NULL_HANDLE;
    }

    if (m_compiler != nullptr)
    {
        delete m_compiler;
        m_compiler = nullptr;
    }
}

VulkanShader::VulkanShader(const uint32_t device, const VkShaderModule handle, const VkShaderStageFlagBits stage)
    : m_vkHandle(handle), m_stage(stage), m_device(device)
{
}

void VulkanShader::reflect(const std::vector<uint32_t>& code)
{
    m_compiler  = new spirv_cross::CompilerGLSL(code);
    printReflectionData();
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

    if (module.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        return { false, {}, module.GetErrorMessage() };
    }

    return { true, { module.cbegin(), module.cend() }, "" };
}