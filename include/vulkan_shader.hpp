#pragma once
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <slang/slang.h>
#include <Volk/volk.h>

#include "utils/identifiable.hpp"

class VulkanDevice;

class VulkanShader
{
public:
    struct MacroDef
    {
        std::string name;
        std::string value;
    };

	struct Result
	{
        enum Status : uint8_t { NOT_READY, FAILED, COMPILED };

		Status status = NOT_READY;
		std::string error;
	};

    static void reset(VulkanShader& p_Shader);
    static void reinit(VulkanShader& p_Shader, ThreadID p_CompilationThread, bool p_Optimize = true, std::span<const MacroDef> p_Macros = {});

    VulkanShader() = default;

    explicit VulkanShader(ThreadID p_CompilationThread, bool p_Optimize = true, std::span<const MacroDef> p_Macros = {});
    ~VulkanShader();
    VulkanShader(const VulkanShader&) = delete;
    VulkanShader(VulkanShader&&) noexcept;

    void loadModule(std::string_view p_Filename, std::string_view p_ModuleName);
    void loadModuleString(std::string_view p_Source, std::string_view p_ModuleName);

    void linkAndFinalize();

    [[nodiscard]] const Result& getStatus() const { return m_Result; }
    [[nodiscard]] std::vector<uint32_t> getSPIRVForStage(VkShaderStageFlagBits p_Stage);
    [[nodiscard]] std::vector<uint32_t> getSPIRVFromName(std::string_view p_Name);
    [[nodiscard]] const std::vector<MacroDef>& getMacros() const { return m_Macros; }
    [[nodiscard]] slang::ProgramLayout* getLayout() const;

    static SlangStage getSlangStageFromVkStage(VkShaderStageFlagBits p_Stage);
    static VkShaderStageFlagBits getVkStageFromSlangStage(SlangStage p_Stage);

    void addSearchPath(const std::string& p_Path) { m_SearchPaths.insert(p_Path); }

private:
    bool buildSession();

    ThreadID m_CompilationThread = 0;
    bool m_Optimize = true;
    std::vector<MacroDef> m_MacroDefs;

    std::vector<MacroDef> m_Macros;
    std::unordered_set<std::string> m_SearchPaths;

    Result m_Result;
    
    inline static std::unordered_map<ThreadID, slang::IGlobalSession*> s_SlangSessions;

    slang::ISession* m_SlangSession = nullptr;

    std::vector<slang::IComponentType*> m_SlangComponents;
    slang::IComponentType* m_SlangProgram = nullptr;

    friend class VulkanShaderModule;
};

class VulkanShaderModule final : public VulkanDeviceSubresource
{
public:

	VkShaderModule operator*() const;

    [[nodiscard]] VkShaderStageFlagBits getStage() const { return m_Stage; }

private:
	void free() override;

	VulkanShaderModule(ResourceID p_Device, VkShaderModule p_Handle, VkShaderStageFlagBits p_Stage);

	VkShaderModule m_VkHandle = VK_NULL_HANDLE;
	VkShaderStageFlagBits m_Stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

	friend class VulkanDevice;
	friend class VulkanPipeline;
	friend struct VulkanPipelineBuilder;
};

