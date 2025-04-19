#include "vulkan_shader.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "vulkan_context.hpp"
#include "vulkan_device.hpp"
#include "utils/call_on_destroy.hpp"
#include "utils/logger.hpp"

void printBlob(slang::IBlob* p_Blob)
{
    if (p_Blob)
    {
        LOG_ERR(static_cast<const char*>(p_Blob->getBufferPointer()));
        p_Blob->release();
    }
}

void VulkanShader::reset(VulkanShader& p_Shader)
{
    p_Shader.~VulkanShader();
    new (&p_Shader) VulkanShader{};
}

void VulkanShader::reinit(VulkanShader& p_Shader, const ThreadID p_CompilationThread, const bool p_Optimize, const std::span<const MacroDef> p_Macros)
{
    p_Shader.~VulkanShader();
    new (&p_Shader) VulkanShader{ p_CompilationThread, p_Optimize, p_Macros };
}

VulkanShader::VulkanShader(const ThreadID p_CompilationThread, const bool p_Optimize, const std::span<const MacroDef> p_Macros)
    : m_CompilationThread(p_CompilationThread), m_Optimize(p_Optimize), m_Macros(p_Macros.begin(), p_Macros.end())
{

}

VulkanShader::~VulkanShader()
{
    if (m_SlangProgram)
    {
        m_SlangProgram->release();
        m_SlangProgram = nullptr;
    }

    if (m_SlangSession)
    {
        m_SlangSession->release();
        m_SlangSession = nullptr;
    }
}

VulkanShader::VulkanShader(VulkanShader&& p_Other) noexcept
{
    m_SlangSession = p_Other.m_SlangSession;
    p_Other.m_SlangSession = nullptr;
    m_SlangProgram = p_Other.m_SlangProgram;
    p_Other.m_SlangProgram = nullptr;
    m_SlangComponents = std::move(p_Other.m_SlangComponents);
    m_Result = p_Other.m_Result;
    m_Macros = std::move(p_Other.m_Macros);
}

void VulkanShader::loadModule(const std::string_view p_Filename, const std::string_view p_ModuleName)
{
    if (m_Result.status == Result::FAILED)
    {
        return;
    }

    // Get data from file
    std::ifstream l_File(p_Filename.data());
    if (!l_File.is_open())
    {
        m_Result.error = "Failed to open shader file: " + std::string(p_Filename);
        m_Result.status = Result::FAILED;
        return;
    }

    addSearchPath(std::filesystem::path{p_Filename}.parent_path().string());

    std::stringstream l_Stream;
    l_Stream << l_File.rdbuf();
    l_File.close();

    loadModuleString(l_Stream.str(), p_ModuleName);
}

void VulkanShader::loadModuleString(const std::string_view p_Source, const std::string_view p_ModuleName)
{
    if (!m_SlangSession)
        buildSession();

    slang::IBlob* l_DiagnosticsBlob = nullptr;
    slang::IModule* l_Module = m_SlangSession->loadModuleFromSourceString(p_ModuleName.data(), p_ModuleName.data(), p_Source.data(), &l_DiagnosticsBlob);
    printBlob(l_DiagnosticsBlob);

    if (!l_Module)
    {
        m_Result.error = "Failed to load shader module: " + std::string(p_ModuleName);
        m_Result.status = Result::FAILED;
        return;
    }

    m_SlangComponents.push_back(l_Module);

    for (uint32_t i = 0;  i < l_Module->getDefinedEntryPointCount(); i++)
    {
        slang::IEntryPoint* l_EntryPoint = nullptr;
        if (SLANG_FAILED(l_Module->getDefinedEntryPoint(i, &l_EntryPoint)))
        {
            m_Result.error = "Failed to get entry point by index: " + std::to_string(i);
            m_Result.status = Result::FAILED;
            return;
        }
        m_SlangComponents.push_back(l_EntryPoint);
    }
}

void VulkanShader::linkAndFinalize()
{
    if (m_Result.status == Result::FAILED)
    {
        return;
    }

    slang::IBlob* l_DiagnosticsBlob = nullptr;
    if (SLANG_FAILED(m_SlangSession->createCompositeComponentType(m_SlangComponents.data(), m_SlangComponents.size(), &m_SlangProgram, &l_DiagnosticsBlob)))
    {
        printBlob(l_DiagnosticsBlob);
        m_Result.error = "Failed to link shader modules";
        m_Result.status = Result::FAILED;
    }

    m_SlangComponents.clear();
    m_Result.status = Result::COMPILED;
}

std::vector<uint32_t> VulkanShader::getSPIRVForStage(const VkShaderStageFlagBits p_Stage)
{
    if (m_Result.status != Result::COMPILED)
    {
        m_Result.error = "Shader compilation not finished";
        m_Result.status = Result::FAILED;
        return {};
    }

    uint32_t l_EntryPointIndex = 0;
    slang::IBlob* l_DiagnosticsBlob = nullptr;
    slang::ProgramLayout* l_Layout = m_SlangProgram->getLayout(0, &l_DiagnosticsBlob);
    printBlob(l_DiagnosticsBlob);

    const SlangStage l_SlangStage = getSlangStageFromVkStage(p_Stage);
    for (uint32_t i = 0; i < l_Layout->getEntryPointCount(); i++)
    {
        slang::EntryPointLayout* l_EntryPoint = l_Layout->getEntryPointByIndex(i);
        if (l_EntryPoint->getStage() == l_SlangStage)
        {
            l_EntryPointIndex = i;
            break;
        }
    }

    slang::IBlob* l_EntryPointBlob = nullptr;
    if (SLANG_FAILED(m_SlangProgram->getEntryPointCode(l_EntryPointIndex, 0, &l_EntryPointBlob, &l_DiagnosticsBlob)))
    {
        printBlob(l_DiagnosticsBlob);
        m_Result.error = "Failed to get SPIR-V for shader stage: " + std::to_string(p_Stage);
        m_Result.status = Result::FAILED;
        return {};
    }

    std::vector<uint32_t> l_SPIRV;
    l_SPIRV.resize(l_EntryPointBlob->getBufferSize() / sizeof(uint32_t));
    memcpy(l_SPIRV.data(), l_EntryPointBlob->getBufferPointer(), l_EntryPointBlob->getBufferSize());
    l_EntryPointBlob->release();

    return l_SPIRV;
}

std::vector<uint32_t> VulkanShader::getSPIRVFromName(const std::string_view p_Name)
{
    if (m_Result.status != Result::COMPILED)
    {
        m_Result.error = "Shader compilation not finished";
        m_Result.status = Result::FAILED;
        return {};
    }

    uint32_t l_EntryPointIndex = 0;
    slang::IBlob* l_DiagnosticsBlob = nullptr;
    slang::ProgramLayout* l_Layout = m_SlangProgram->getLayout(0, &l_DiagnosticsBlob);
    printBlob(l_DiagnosticsBlob);

    for (uint32_t i = 0; i < l_Layout->getEntryPointCount(); i++)
    {
        slang::EntryPointLayout* l_EntryPoint = l_Layout->getEntryPointByIndex(i);
        if (l_EntryPoint->getName() == p_Name)
        {
            l_EntryPointIndex = i;
            break;
        }
    }

    slang::IBlob* l_EntryPointBlob = nullptr;
    if (SLANG_FAILED(m_SlangProgram->getEntryPointCode(l_EntryPointIndex, 0, &l_EntryPointBlob, &l_DiagnosticsBlob)))
    {
        printBlob(l_DiagnosticsBlob);
        m_Result.error = "Failed to get SPIR-V for shader entry point: " + std::string(p_Name);
        m_Result.status = Result::FAILED;
        return {};
    }

    std::vector<uint32_t> l_SPIRV;
    l_SPIRV.resize(l_EntryPointBlob->getBufferSize() / sizeof(uint32_t));
    memcpy(l_SPIRV.data(), l_EntryPointBlob->getBufferPointer(), l_EntryPointBlob->getBufferSize());
    l_EntryPointBlob->release();

    return l_SPIRV;
}

VkShaderModule VulkanShaderModule::operator*() const
{
    return m_VkHandle;
}

slang::ProgramLayout* VulkanShader::getLayout() const
{
    if (m_Result.status != Result::COMPILED)
        throw std::runtime_error("Could not obtain shader layout, compilation not finished");

    return m_SlangProgram->getLayout(0, nullptr);
}

SlangStage VulkanShader::getSlangStageFromVkStage(const VkShaderStageFlagBits p_Stage)
{
    switch (p_Stage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT: return SLANG_STAGE_VERTEX;
    case VK_SHADER_STAGE_FRAGMENT_BIT: return SLANG_STAGE_FRAGMENT;
    case VK_SHADER_STAGE_COMPUTE_BIT: return SLANG_STAGE_COMPUTE;
    case VK_SHADER_STAGE_GEOMETRY_BIT: return SLANG_STAGE_GEOMETRY;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return SLANG_STAGE_HULL;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return SLANG_STAGE_DOMAIN;
    case VK_SHADER_STAGE_RAYGEN_BIT_KHR: return SLANG_STAGE_RAY_GENERATION;
    case VK_SHADER_STAGE_ANY_HIT_BIT_KHR: return SLANG_STAGE_ANY_HIT;
    case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return SLANG_STAGE_CLOSEST_HIT;
    case VK_SHADER_STAGE_MISS_BIT_KHR: return SLANG_STAGE_MISS;
    case VK_SHADER_STAGE_INTERSECTION_BIT_KHR: return SLANG_STAGE_INTERSECTION;
    case VK_SHADER_STAGE_CALLABLE_BIT_KHR: return SLANG_STAGE_CALLABLE;
    case VK_SHADER_STAGE_MESH_BIT_EXT: return SLANG_STAGE_MESH;
    }

    throw std::runtime_error("Unsupported shader stage");
}

VkShaderStageFlagBits VulkanShader::getVkStageFromSlangStage(const SlangStage p_Stage)
{
    switch (p_Stage)
    {
    case SLANG_STAGE_VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
    case SLANG_STAGE_FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
    case SLANG_STAGE_COMPUTE: return VK_SHADER_STAGE_COMPUTE_BIT;
    case SLANG_STAGE_GEOMETRY: return VK_SHADER_STAGE_GEOMETRY_BIT;
    case SLANG_STAGE_HULL: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case SLANG_STAGE_DOMAIN: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case SLANG_STAGE_RAY_GENERATION: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    case SLANG_STAGE_ANY_HIT: return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    case SLANG_STAGE_CLOSEST_HIT: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    case SLANG_STAGE_MISS: return VK_SHADER_STAGE_MISS_BIT_KHR;
    case SLANG_STAGE_INTERSECTION: return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    case SLANG_STAGE_CALLABLE: return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
    case SLANG_STAGE_MESH: return VK_SHADER_STAGE_MESH_BIT_EXT;
    }
    throw std::runtime_error("Unsupported shader stage");
}

bool VulkanShader::buildSession()
{
    if (!s_SlangSessions.contains(m_CompilationThread))
    {
        slang::IGlobalSession* l_SlangGlobalSession = nullptr;
        if (SLANG_FAILED(slang::createGlobalSession(&l_SlangGlobalSession)))
        {
            m_Result.error = "Failed to create Slang global session";
            m_Result.status = Result::FAILED;
            return false;
        }
        s_SlangSessions[m_CompilationThread] = l_SlangGlobalSession;
    }

    slang::SessionDesc sessionDesc = {};

    slang::TargetDesc targetDesc = {};
    targetDesc.format = SLANG_SPIRV;
    targetDesc.profile = s_SlangSessions[m_CompilationThread]->findProfile("spirv_1_5");

    sessionDesc.targetCount = 1;
    sessionDesc.targets = &targetDesc;

    std::array<slang::CompilerOptionEntry, 3> options;
    options[0] = { slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr} };
    options[1] = { slang::CompilerOptionName::Optimization, {slang::CompilerOptionValueKind::Int, static_cast<int>(m_Optimize ? SLANG_OPTIMIZATION_LEVEL_HIGH : SLANG_OPTIMIZATION_LEVEL_NONE), 0, nullptr, nullptr} };
    options[2] = { slang::CompilerOptionName::DebugInformation, {slang::CompilerOptionValueKind::Int, static_cast<int>(m_Optimize ? SLANG_DEBUG_INFO_LEVEL_NONE : SLANG_DEBUG_INFO_LEVEL_MAXIMAL), 0, nullptr, nullptr} };

    sessionDesc.compilerOptionEntryCount = options.size();
    sessionDesc.compilerOptionEntries = options.data();

    std::vector<const char*> l_SearchPaths;
    l_SearchPaths.reserve(m_SearchPaths.size());
    for (const std::string& l_Path : m_SearchPaths)
        l_SearchPaths.push_back(l_Path.c_str());

    sessionDesc.searchPathCount = l_SearchPaths.size();
    sessionDesc.searchPaths = l_SearchPaths.data();

    std::vector<slang::PreprocessorMacroDesc> l_Macros;
    l_Macros.reserve(m_Macros.size());
    for (const MacroDef& l_Macro : m_Macros)
    {
        slang::PreprocessorMacroDesc l_Desc = {};
        l_Desc.name = l_Macro.name.c_str();
        l_Desc.value = l_Macro.value.c_str();
        l_Macros.push_back(l_Desc);
    }

    sessionDesc.preprocessorMacroCount = l_Macros.size();
    sessionDesc.preprocessorMacros = l_Macros.data();

    if (SLANG_FAILED(s_SlangSessions[m_CompilationThread]->createSession(sessionDesc, &m_SlangSession)))
    {
        m_Result.error = "Failed to create Slang session";
        m_Result.status = Result::FAILED;
        return false;
    }
    return true;
}

void VulkanShaderModule::free()
{
    if (m_VkHandle != VK_NULL_HANDLE)
    {
        const VulkanDevice& l_Device = VulkanContext::getDevice(getDeviceID());

	    l_Device.getTable().vkDestroyShaderModule(l_Device.m_VkHandle, m_VkHandle, nullptr);
        LOG_DEBUG("Freed shader module (ID: ", m_ID, ")");
        m_VkHandle = VK_NULL_HANDLE;
    }
}

VulkanShaderModule::VulkanShaderModule(const ResourceID p_Device, const VkShaderModule p_Handle, const VkShaderStageFlagBits p_Stage)
    : VulkanDeviceSubresource(p_Device), m_VkHandle(p_Handle), m_Stage(p_Stage)
{
}