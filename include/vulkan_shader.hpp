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

    struct ReflectionData
    {
        ReflectionData() : valid(false) {}
        explicit ReflectionData(const spirv_cross::ShaderResources& src): resources(src), valid(true) {}

        spirv_cross::ShaderResources resources{};
        bool valid;
    };


	static [[nodiscard]] shaderc_shader_kind getKindFromStage(VkShaderStageFlagBits stage);

	VkShaderModule operator*() const;

    [[nodiscard]] VkShaderStageFlagBits getStage() const { return m_stage; }

    [[nodiscard]] bool hasReflection() const { return m_compiler != nullptr; }
    [[nodiscard]] ReflectionData getReflectionData() const;

    void printReflectionData() const;

    static ReflectionData getReflectionDataFromFile(const std::string& filepath, VkShaderStageFlagBits stage);

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

	static std::string readFile(std::string_view p_filename);
	static [[nodiscard]] Result compileFile(std::string_view p_source_name, shaderc_shader_kind p_kind, std::string_view p_source, bool p_optimize, const std::vector<MacroDef>& macros);
    
	VkShaderModule m_vkHandle = VK_NULL_HANDLE;
	VkShaderStageFlagBits m_stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

	uint32_t m_device;

    spirv_cross::CompilerGLSL* m_compiler = nullptr;

	friend class VulkanDevice;
	friend class VulkanPipeline;
	friend struct VulkanPipelineBuilder;
};

