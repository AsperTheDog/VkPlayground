#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_glsl.hpp>

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

    struct ReflectionManager
    {
        ReflectionManager() = default;
        explicit ReflectionManager(spirv_cross::CompilerGLSL* compiler, bool isCompilerLocal = true);
        ~ReflectionManager() { if (compiler != nullptr && isCompilerLocal) delete compiler; }
        
        ReflectionManager(const ReflectionManager&) = delete;
        ReflectionManager& operator=(const ReflectionManager&) = delete;

        ReflectionManager(ReflectionManager&& other) noexcept;
        ReflectionManager& operator=(ReflectionManager&& other) noexcept;


        [[nodiscard]] spirv_cross::ShaderResources getResources() const { return compiler->get_shader_resources(); }
        [[nodiscard]] bool isValid() const { return compiler != nullptr; }

        [[nodiscard]] std::string getName(spirv_cross::ID id, const std::string& nameField) const;

        spirv_cross::CompilerGLSL* compiler = nullptr;
        bool isCompilerLocal = false;

    };


	static [[nodiscard]] shaderc_shader_kind getKindFromStage(VkShaderStageFlagBits stage);

	VkShaderModule operator*() const;

    [[nodiscard]] VkShaderStageFlagBits getStage() const { return m_stage; }

    [[nodiscard]] bool hasReflection() const { return m_compiler != nullptr; }
    [[nodiscard]] ReflectionManager getReflectionData() const;

    void printReflectionData() const;

    static ReflectionManager getReflectionDataFromFile(const std::string& filepath, VkShaderStageFlagBits stage);

private:
	void free();

	struct Result
	{
		bool success = false;
		std::vector<uint32_t> code;
		std::string error;
	};

	VulkanShader(uint32_t device, VkShaderModule handle, VkShaderStageFlagBits stage);
    void reflect(const std::vector<uint32_t>& code);

	static std::string readFile(const std::string& p_filename);
	static [[nodiscard]] Result compileFile(const std::string& p_source_name, shaderc_shader_kind p_kind, const std::string& p_source, bool p_optimize, const std::vector<MacroDef>& macros);
    
	VkShaderModule m_vkHandle = VK_NULL_HANDLE;
	VkShaderStageFlagBits m_stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

	uint32_t m_device;

    spirv_cross::CompilerGLSL* m_compiler = nullptr;

	friend class VulkanDevice;
	friend class VulkanPipeline;
	friend struct VulkanPipelineBuilder;
};

