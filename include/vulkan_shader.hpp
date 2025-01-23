#pragma once
#include <string>
#include <vector>
#include <Volk/volk.h>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include "utils/identifiable.hpp"

class VulkanDevice;

class VulkanShader final : public VulkanDeviceSubresource
{
public:
    struct MacroDef
    {
        std::string name;
        std::string value;
    };

    struct ReflectionManager
    {
        ReflectionManager() = default;
        explicit ReflectionManager(spirv_cross::CompilerGLSL* p_Compiler, bool p_IsCompilerLocal = true);
        ~ReflectionManager() { if (compiler != nullptr && isCompilerLocal) delete compiler; }
        
        ReflectionManager(ReflectionManager&& p_Other) noexcept;
        ReflectionManager& operator=(ReflectionManager&& p_Other) noexcept;

        [[nodiscard]] spirv_cross::ShaderResources getResources() const { return compiler->get_shader_resources(); }
        [[nodiscard]] bool isValid() const { return compiler != nullptr; }

        [[nodiscard]] std::string getName(spirv_cross::ID p_ID, const std::string& p_NameField) const;

        spirv_cross::CompilerGLSL* compiler = nullptr;
        bool isCompilerLocal = false;
    };


	static [[nodiscard]] shaderc_shader_kind getKindFromStage(VkShaderStageFlagBits p_Stage);

	VkShaderModule operator*() const;

    [[nodiscard]] VkShaderStageFlagBits getStage() const { return m_Stage; }

    [[nodiscard]] bool hasReflection() const { return m_Compiler != nullptr; }
    [[nodiscard]] ReflectionManager getReflectionData() const;

    void printReflectionData() const;

    static ReflectionManager getReflectionDataFromFile(const std::string& p_Filepath, VkShaderStageFlagBits p_Stage);

private:
	void free() override;

	struct Result
	{
		bool success = false;
		std::vector<uint32_t> code;
		std::string error;
	};

	VulkanShader(ResourceID p_Device, VkShaderModule p_Handle, VkShaderStageFlagBits p_Stage);
    void reflect(const std::vector<uint32_t>& p_Code);

	static std::string readFile(const std::string& p_Filename);
	static [[nodiscard]] Result compileFile(const std::string& p_Source_name, shaderc_shader_kind p_Kind, const std::string& p_Source, bool p_Optimize, const std::vector<MacroDef>& p_Macros);
    
	VkShaderModule m_VkHandle = VK_NULL_HANDLE;
	VkShaderStageFlagBits m_Stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

    spirv_cross::CompilerGLSL* m_Compiler = nullptr;

	friend class VulkanDevice;
	friend class VulkanPipeline;
	friend struct VulkanPipelineBuilder;
};

