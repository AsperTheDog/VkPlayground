#include "vulkan_shader.hpp"

#include <fstream>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_context.hpp"
#include "vulkan_device.hpp"
#include "utils/logger.hpp"

#include "spirv_cross/spirv_glsl.hpp"

VulkanShader::ReflectionManager::ReflectionManager(spirv_cross::CompilerGLSL* p_Compiler, const bool p_IsCompilerLocal)
 : compiler(p_Compiler), isCompilerLocal(p_IsCompilerLocal) {}

VulkanShader::ReflectionManager::ReflectionManager(ReflectionManager&& p_Other) noexcept : compiler(p_Other.compiler), isCompilerLocal(p_Other.isCompilerLocal)
{
    p_Other.compiler = nullptr;
}

VulkanShader::ReflectionManager& VulkanShader::ReflectionManager::operator=(ReflectionManager&& p_Other) noexcept
{
    compiler = p_Other.compiler;
    isCompilerLocal = p_Other.isCompilerLocal;
    p_Other.compiler = nullptr;
    return *this;
}

std::string VulkanShader::ReflectionManager::getName(const spirv_cross::ID p_ID, const std::string_view p_NameField) const
{
    std::string l_Name = std::string(p_NameField);
    if (l_Name.empty())
        l_Name = compiler->get_name(p_ID);
    if (l_Name.empty())
        l_Name = compiler->get_fallback_name(p_ID);
    return l_Name;
}

shaderc_shader_kind VulkanShader::getKindFromStage(const VkShaderStageFlagBits p_Stage)
{
    switch (p_Stage)
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
    default: throw std::runtime_error(std::string("Unsupported shader stage ") + string_VkShaderStageFlagBits(p_Stage));
    }
}

VkShaderModule VulkanShader::operator*() const
{
    return m_VkHandle;
}

VulkanShader::ReflectionManager VulkanShader::getReflectionData() const
{
    if (m_Compiler == nullptr)
    {
        LOG_DEBUG("No reflection data available for shader (ID: ", m_ID, ")");
        return {};
    }

    return VulkanShader::ReflectionManager{m_Compiler, false};
}

void VulkanShader::printReflectionData() const
{
    if (!Logger::isLevelActive(Logger::DEBUG)) return;

    if (m_Compiler == nullptr)
    {
        LOG_DEBUG("No reflection data available for shader (ID: ", m_ID, ")");
        return;
    }

    LOG_DEBUG("Reflection data for shader (ID: ", m_ID, ")");
    LOG_DEBUG("Inputs:");
    for (const spirv_cross::Resource& l_Input : m_Compiler->get_shader_resources().stage_inputs)
    {
        LOG_DEBUG("  ", l_Input.name, " (", l_Input.type_id, ")");
    }

    LOG_DEBUG("Outputs:");
    for (const spirv_cross::Resource& l_Output : m_Compiler->get_shader_resources().stage_outputs)
    {
        LOG_DEBUG("  ", l_Output.name, " (", l_Output.type_id, ")");
    }

    LOG_DEBUG("Uniform buffers:");
    for (const spirv_cross::Resource& l_UniformBuffer : m_Compiler->get_shader_resources().uniform_buffers)
    {
        LOG_DEBUG("  ", l_UniformBuffer.name, " (", l_UniformBuffer.type_id, ")");
    }

    LOG_DEBUG("Storage buffers:");
    for (const spirv_cross::Resource& l_StorageBuffer : m_Compiler->get_shader_resources().storage_buffers)
    {
        LOG_DEBUG("  ", l_StorageBuffer.name, " (", l_StorageBuffer.type_id, ")");
    }

    LOG_DEBUG("Subpass inputs:");
    for (const spirv_cross::Resource& l_SubpassInput : m_Compiler->get_shader_resources().subpass_inputs)
    {
        LOG_DEBUG("  ", l_SubpassInput.name, " (", l_SubpassInput.type_id, ")");
    }

    LOG_DEBUG("Storage images:");
    for (const spirv_cross::Resource& l_StorageImage : m_Compiler->get_shader_resources().storage_images)
    {
        LOG_DEBUG("  ", l_StorageImage.name, " (", l_StorageImage.type_id, ")");
    }

    LOG_DEBUG("Push constants:");
    for (const spirv_cross::Resource& l_SampledImage : m_Compiler->get_shader_resources().sampled_images)
    {
        LOG_DEBUG("  ", l_SampledImage.name, " (", l_SampledImage.type_id, ")");
    }
}

VulkanShader::ReflectionManager VulkanShader::getReflectionDataFromFile(const std::string_view p_Filepath, const VkShaderStageFlagBits p_Stage)
{
    const VulkanShader::Result l_Result = VulkanShader::compileFile(p_Filepath, VulkanShader::getKindFromStage(p_Stage), VulkanShader::readFile(p_Filepath), false, {});
    if (!l_Result.success)
    {
        LOG_ERR("Failed to compile shader file ", p_Filepath, ": ", l_Result.error);
        return {};
    }
    spirv_cross::CompilerGLSL* l_Compiler = ARENA_ALLOC(spirv_cross::CompilerGLSL)(l_Result.code);
    return VulkanShader::ReflectionManager{l_Compiler};
}

void VulkanShader::free()
{
    if (m_VkHandle != VK_NULL_HANDLE)
    {
        const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

	    l_Device.getTable().vkDestroyShaderModule(l_Device.m_VkHandle, m_VkHandle, nullptr);
        LOG_DEBUG("Freed shader module (ID: ", m_ID, ")");
        m_VkHandle = VK_NULL_HANDLE;
    }

    if (m_Compiler != nullptr)
    {
        ARENA_FREE(m_Compiler, sizeof(spirv_cross::CompilerGLSL));
        m_Compiler = nullptr;
    }
}

VulkanShader::VulkanShader(const ResourceID p_Device, const VkShaderModule p_Handle, const VkShaderStageFlagBits p_Stage)
    : VulkanDeviceSubresource(p_Device), m_VkHandle(p_Handle), m_Stage(p_Stage)
{
}

void VulkanShader::reflect(const std::vector<uint32_t>& p_Code)
{
    m_Compiler = ARENA_ALLOC(spirv_cross::CompilerGLSL)(p_Code);
    printReflectionData();
}

std::string VulkanShader::readFile(const std::string_view p_Filename)
{
    std::ifstream l_ShaderFile(p_Filename.data());
    if (!l_ShaderFile.is_open()) throw std::runtime_error("failed to open shader file " + std::string(p_Filename));
    std::string str((std::istreambuf_iterator(l_ShaderFile)), std::istreambuf_iterator<char>());
    return str;
}

VulkanShader::Result VulkanShader::compileFile(const std::string_view p_Source_name, const shaderc_shader_kind p_Kind, const std::string_view p_Source, const bool p_Optimize, const std::span<const MacroDef> p_Macros)
{
    const shaderc::Compiler l_Compiler;
    shaderc::CompileOptions l_Options;

    for (const MacroDef& l_Macro : p_Macros)
        l_Options.AddMacroDefinition(l_Macro.name, l_Macro.value);

    if (p_Optimize)
        l_Options.SetOptimizationLevel(shaderc_optimization_level_performance);
    else
    {
        l_Options.SetOptimizationLevel(shaderc_optimization_level_zero);
        l_Options.SetGenerateDebugInfo();
    }

    const shaderc::SpvCompilationResult l_Module = l_Compiler.CompileGlslToSpv(p_Source.data(), p_Kind, p_Source_name.data(), l_Options);

    if (l_Module.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        return { false, {}, l_Module.GetErrorMessage() };
    }

    return { true, { l_Module.cbegin(), l_Module.cend() }, "" };
}