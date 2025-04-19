#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "vulkan_binding.hpp"
#include "vulkan_shader.hpp"

struct ShaderReflectionData
{
    ShaderReflectionData() = default;
    ShaderReflectionData(slang::ProgramLayout* p_Layout, VkShaderStageFlags p_Stages);
    
    enum FieldType: uint16_t
    {
        BOOL = 0,      BOOL2 = 2,       BOOL3 = 3,       BOOL4 = 4,       BOOL2X2 = 5,       BOOL3X3 = 10,      BOOL4X4 = 17,
        INT8 = 144,    INT8_2 = 146,    INT8_3 = 147,    INT8_4 = 148,    INT8_2X2 = 149,    INT8_3X3 = 154,    INT8_4X4 = 161,
        INT16 = 180,   INT16_2 = 182,   INT16_3 = 183,   INT16_4 = 184,   INT16_2X2 = 185,   INT16_3X3 = 190,   INT16_4X4 = 197,
        INT32 = 18,    INT32_2 = 20,    INT32_3 = 21,    INT32_4 = 22,    INT32_2X2 = 23,    INT32_3X3 = 28,    INT32_4X4 = 35,
        INT64 = 54,    INT64_2 = 56,    INT64_3 = 57,    INT64_4 = 58,    INT64_2X2 = 59,    INT64_3X3 = 64,    INT64_4X4 = 71,
        UINT8 = 162,   UINT8_2 = 164,   UINT8_3 = 165,   UINT8_4 = 166,   UINT8_2X2 = 167,   UINT8_3X3 = 172,   UINT8_4X4 = 179,
        UINT16 = 198,  UINT16_2 = 200,  UINT16_3 = 201,  UINT16_4 = 202,  UINT16_2X2 = 203,  UINT16_3X3 = 208,  UINT16_4X4 = 215,
        UINT32 = 36,   UINT32_2 = 38,   UINT32_3 = 39,   UINT32_4 = 40,   UINT32_2X2 = 41,   UINT32_3X3 = 46,   UINT32_4X4 = 53,
        UINT64 = 72,   UINT64_2 = 74,   UINT64_3 = 75,   UINT64_4 = 76,   UINT64_2X2 = 77,   UINT64_3X3 = 82,   UINT64_4X4 = 89,
        FLOAT16 = 90,  FLOAT16_2 = 92,  FLOAT16_3 = 93,  FLOAT16_4 = 94,  FLOAT16_2X2 = 95,  FLOAT16_3X3 = 100, FLOAT16_4X4 = 107,
        FLOAT32 = 108, FLOAT32_2 = 110, FLOAT32_3 = 111, FLOAT32_4 = 112, FLOAT32_2X2 = 113, FLOAT32_3X3 = 118, FLOAT32_4X4 = 125,
        FLOAT64 = 126, FLOAT64_2 = 128, FLOAT64_3 = 129, FLOAT64_4 = 130, FLOAT64_2X2 = 131, FLOAT64_3X3 = 136, FLOAT64_4X4 = 143,

        UNKNOWN = UINT16_MAX
    };

    struct TypeData
    {
        size_t numElements;
        FieldType type;
    };

    struct Field
    {
        virtual ~Field() = default;

        std::string name;
        size_t size;
    };
    using FieldPtr = std::shared_ptr<Field>;

    struct Variable final : Field
    {
        TypeData data;
        uint32_t binding;
    };
    using VariablePtr = std::shared_ptr<Variable>;

    struct Struct final : Field
    {
        std::vector<FieldPtr> members;
    };
    using StructPtr = std::shared_ptr<Struct>;

    struct PushConstantField
    {
        VkShaderStageFlagBits stage;
        FieldPtr field;
    };

    struct BindingRef
    {
        struct BindingField
        {
            VariablePtr field;
            uint32_t topLevelIndex;
        };

        VulkanBinding& binding;
        uint32_t fields;
    };

    std::vector<PushConstantField> pushConstants;
    std::vector<FieldPtr> vertexInputs;

    [[nodiscard]] std::vector<VkPushConstantRange> getPushConstantRanges() const;
    void populateBindings(std::span<BindingRef> p_Bindings, uint32_t p_StartingField = 0) const;

    bool isValid() const { return m_Valid; }

private:
    struct FieldData
    {
        enum Category : uint8_t { INPUT, PUSH, INVALID };
        Category category;
        FieldPtr field;
    };

    struct BindingFormat
    {
        VkFormat format;
        uint32_t bindingSize;
    };

    bool m_Valid = false;

    static FieldData createField(slang::VariableLayoutReflection* p_Variable, uint32_t p_BindingOffset);
    static TypeData getTypeData(slang::TypeLayoutReflection* p_Type);
    static FieldType getType(slang::TypeLayoutReflection* p_Type);
    static BindingFormat getBindingFormat(FieldType p_Type);

    [[nodiscard]] std::vector<BindingRef::BindingField> getFlatVertexInputs(uint32_t p_StartingPoint = 0) const;
    static void fillFlatVertexInputs(std::vector<BindingRef::BindingField>& p_Vector, const FieldPtr& p_Input, const uint32_t p_TopLevelIndex);

    static constexpr std::array<FieldType, 14> l_ScalarKindMapping = {
        UNKNOWN, UNKNOWN, BOOL, INT32, UINT32, INT64, UINT64, FLOAT16, FLOAT32, FLOAT64, INT8, UINT8, INT16, UINT16
    };
};

inline ShaderReflectionData::ShaderReflectionData(slang::ProgramLayout* p_Layout, const VkShaderStageFlags p_Stages)
    : m_Valid(true)
{
    for (uint32_t i = 0; i < p_Layout->getEntryPointCount(); i++)
    {
        slang::EntryPointLayout* l_EntryPoint = p_Layout->getEntryPointByIndex(i);
        if (!(VulkanShader::getVkStageFromSlangStage(l_EntryPoint->getStage()) & p_Stages))
            continue;

        for (uint32_t j = 0; j < l_EntryPoint->getParameterCount(); j++)
        {
            slang::VariableLayoutReflection* l_Variable = l_EntryPoint->getParameterByIndex(j);
            FieldData l_Field = createField(l_Variable, 0);
            if (l_Field.category == FieldData::INPUT && l_EntryPoint->getStage() == SLANG_STAGE_VERTEX)
                vertexInputs.push_back(l_Field.field);
            else if (l_Field.category == FieldData::PUSH)
                pushConstants.push_back({ VulkanShader::getVkStageFromSlangStage(l_EntryPoint->getStage()), l_Field.field });
        }
    }
}

inline std::vector<VkPushConstantRange> ShaderReflectionData::getPushConstantRanges() const
{
    std::vector<VkPushConstantRange> l_Ranges;
    VkShaderStageFlagBits l_Stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    uint32_t l_Offset = 0;
    for (const PushConstantField& l_PushConstant : pushConstants)
    {
        if (l_Stage != l_PushConstant.stage)
        {
            if (!l_Ranges.empty())
                l_Offset += l_Ranges.back().size;
            l_Stage = l_PushConstant.stage;
            l_Ranges.emplace_back(l_Stage, l_Offset, 0);
        }
        l_Ranges.back().size += l_PushConstant.field->size;
    }
    return l_Ranges;
}

inline void ShaderReflectionData::populateBindings(std::span<BindingRef> p_Bindings, const uint32_t p_StartingField) const
{
    const std::vector<BindingRef::BindingField> l_FlatVertexInputs = getFlatVertexInputs(p_StartingField);
    for (uint32_t i = 0; i < p_Bindings.size(); i++)
    {
        std::vector<BindingRef::BindingField> l_BindingFields;

        const uint32_t l_MaxFieldIndex = i + p_Bindings[i].fields - 1;
        for (const BindingRef::BindingField& l_Field : l_FlatVertexInputs)
        {
            if (l_Field.topLevelIndex >= i && l_Field.topLevelIndex <= l_MaxFieldIndex)
                l_BindingFields.push_back(l_Field);
        }

        std::ranges::sort(l_BindingFields, [](const BindingRef::BindingField& a, const BindingRef::BindingField& b) { return a.field->binding < b.field->binding; });

        uint32_t l_Offset = 0;
        for (const BindingRef::BindingField& l_Field : l_BindingFields)
        {
            if (l_Field.field->binding == UINT32_MAX)
                continue;
            BindingRef& l_Binding = p_Bindings[i];
            const BindingFormat l_BindingFormat = getBindingFormat(l_Field.field->data.type);
            l_Binding.binding.addAttribDescription(l_BindingFormat.format, l_Offset, l_Field.field->binding, l_BindingFormat.bindingSize);
            l_Offset += l_Field.field->size;
        }
    }
}

inline ShaderReflectionData::FieldData ShaderReflectionData::createField(slang::VariableLayoutReflection* p_Variable, const uint32_t p_BindingOffset)
{
    slang::TypeLayoutReflection* l_Type = p_Variable->getTypeLayout();
    const slang::ParameterCategory l_SlangCategory = p_Variable->getCategory();
    FieldData::Category l_category = FieldData::INVALID;
    if (l_SlangCategory == slang::ParameterCategory::VaryingInput || l_SlangCategory == slang::ParameterCategory::Uniform)
        l_category = l_SlangCategory == slang::ParameterCategory::VaryingInput ? FieldData::INPUT : FieldData::PUSH;

    Field* l_Field;
    if (l_Type->getKind() == slang::TypeReflection::Kind::Struct)
    {
        Struct* l_Struct = new Struct();
        for (uint32_t i = 0; i < l_Type->getFieldCount(); i++)
        {
            FieldData l_FieldData = createField(l_Type->getFieldByIndex(i), p_BindingOffset + p_Variable->getBindingIndex());
            l_Struct->members.push_back(l_FieldData.field);
        }

        l_Field = l_Struct;
    }
    else
    {
        Variable* l_Variable = new Variable();
        l_Variable->data = getTypeData(l_Type);
        l_Variable->binding = p_Variable->getBindingIndex() + p_BindingOffset;

        l_Field = l_Variable;
    }

    l_Field->name = p_Variable->getName();
    l_Field->size = l_Type->getSize();
    return { l_category, std::shared_ptr<Field>(l_Field) };
}

inline ShaderReflectionData::TypeData ShaderReflectionData::getTypeData(slang::TypeLayoutReflection* p_Type)
{
    slang::TypeLayoutReflection* l_Type = p_Type;

    const TypeData l_TypeData{
        .numElements = std::max(1ULL, l_Type->getTotalArrayElementCount()),
        .type = getType(l_Type->unwrapArray()),
    };

    return l_TypeData;
}

inline ShaderReflectionData::FieldType ShaderReflectionData::getType(slang::TypeLayoutReflection* p_Type)
{
    switch (p_Type->getKind())
    {
    case slang::TypeReflection::Kind::Scalar:
        return l_ScalarKindMapping[p_Type->getScalarType()];
    case slang::TypeReflection::Kind::Vector:
        return static_cast<FieldType>(getType(p_Type->getElementTypeLayout()) + p_Type->getElementCount());
    case slang::TypeReflection::Kind::Matrix:
        return static_cast<FieldType>(getType(p_Type->getElementTypeLayout()->getElementTypeLayout()) + p_Type->getRowCount() * p_Type->getColumnCount() + 1);
    default: 
        return FieldType::UNKNOWN;
    }
}

inline std::vector<ShaderReflectionData::BindingRef::BindingField> ShaderReflectionData::getFlatVertexInputs(const uint32_t p_StartingPoint) const
{
    using BindingField = ShaderReflectionData::BindingRef::BindingField;

    std::vector<BindingField> l_FlatVertexInputs;
    for (uint32_t i = p_StartingPoint; i < vertexInputs.size(); i++)
        fillFlatVertexInputs(l_FlatVertexInputs, vertexInputs[i], i);
    return l_FlatVertexInputs;
}

inline void ShaderReflectionData::fillFlatVertexInputs(std::vector<ShaderReflectionData::BindingRef::BindingField>& p_Vector, const FieldPtr& p_Input, const uint32_t p_TopLevelIndex)
{
    if (const VariablePtr l_Var = std::dynamic_pointer_cast<Variable>(p_Input))
    {
        p_Vector.push_back({l_Var, p_TopLevelIndex});
    }
    else if (const StructPtr l_Struct = std::dynamic_pointer_cast<Struct>(p_Input))
    {
        for (const FieldPtr l_Member : l_Struct->members)
            fillFlatVertexInputs(p_Vector, l_Member, p_TopLevelIndex);
    }
}

inline ShaderReflectionData::BindingFormat ShaderReflectionData::getBindingFormat(const FieldType p_Type)
{
    switch (p_Type)
    {
    case FieldType::BOOL: return {VK_FORMAT_R8_UINT, 1};
    case FieldType::BOOL2: return { VK_FORMAT_R8G8_UINT, 1 };
    case FieldType::BOOL3: return { VK_FORMAT_R8G8B8_UINT, 1 };
    case FieldType::BOOL4: return { VK_FORMAT_R8G8B8A8_UINT, 1 };
    case FieldType::BOOL2X2: return { VK_FORMAT_R8G8_UINT, 2 };
    case FieldType::BOOL3X3: return { VK_FORMAT_R8G8B8_UINT, 3 };
    case FieldType::BOOL4X4: return { VK_FORMAT_R8G8B8A8_UINT, 4 };
    case FieldType::INT8: return { VK_FORMAT_R8_SINT, 1 };
    case FieldType::INT8_2: return { VK_FORMAT_R8G8_SINT, 1 };
    case FieldType::INT8_3: return { VK_FORMAT_R8G8B8_SINT, 1 };
    case FieldType::INT8_4: return { VK_FORMAT_R8G8B8A8_SINT, 1 };
    case FieldType::INT8_2X2: return { VK_FORMAT_R8G8_SINT, 2 };
    case FieldType::INT8_3X3: return { VK_FORMAT_R8G8B8_SINT, 3 };
    case FieldType::INT8_4X4: return { VK_FORMAT_R8G8B8A8_SINT, 4 };
    case FieldType::INT16: return { VK_FORMAT_R16_SINT, 1 };
    case FieldType::INT16_2: return { VK_FORMAT_R16G16_SINT, 1 };
    case FieldType::INT16_3: return { VK_FORMAT_R16G16B16_SINT, 1 };
    case FieldType::INT16_4: return { VK_FORMAT_R16G16B16A16_SINT, 1 };
    case FieldType::INT16_2X2: return { VK_FORMAT_R16G16_SINT, 2 };
    case FieldType::INT16_3X3: return { VK_FORMAT_R16G16B16_SINT, 3 };
    case FieldType::INT16_4X4: return { VK_FORMAT_R16G16B16A16_SINT, 4 };
    case FieldType::INT32: return { VK_FORMAT_R32_SINT, 1 };
    case FieldType::INT32_2: return { VK_FORMAT_R32G32_SINT, 1 };
    case FieldType::INT32_3: return { VK_FORMAT_R32G32B32_SINT, 1 };
    case FieldType::INT32_4: return { VK_FORMAT_R32G32B32A32_SINT, 1 };
    case FieldType::INT32_2X2: return { VK_FORMAT_R32G32_SINT, 2 };
    case FieldType::INT32_3X3: return { VK_FORMAT_R32G32B32_SINT, 3 };
    case FieldType::INT32_4X4: return { VK_FORMAT_R32G32B32A32_SINT, 4 };
    case FieldType::INT64: return { VK_FORMAT_R64_SINT, 1 };
    case FieldType::INT64_2: return { VK_FORMAT_R64G64_SINT, 1 };
    case FieldType::INT64_3: return { VK_FORMAT_R64G64B64_SINT, 1 };
    case FieldType::INT64_4: return { VK_FORMAT_R64G64B64A64_SINT, 1 };
    case FieldType::INT64_2X2: return { VK_FORMAT_R64G64_SINT, 2 };
    case FieldType::INT64_3X3: return { VK_FORMAT_R64G64B64_SINT, 3 };
    case FieldType::INT64_4X4: return { VK_FORMAT_R64G64B64A64_SINT, 4 };
    case FieldType::UINT8: return { VK_FORMAT_R8_UINT, 1 };
    case FieldType::UINT8_2: return { VK_FORMAT_R8G8_UINT, 1 };
    case FieldType::UINT8_3: return { VK_FORMAT_R8G8B8_UINT, 1 };
    case FieldType::UINT8_4: return { VK_FORMAT_R8G8B8A8_UINT, 1 };
    case FieldType::UINT8_2X2: return { VK_FORMAT_R8G8_UINT, 2 };
    case FieldType::UINT8_3X3: return { VK_FORMAT_R8G8B8_UINT, 3 };
    case FieldType::UINT8_4X4: return { VK_FORMAT_R8G8B8A8_UINT, 4 };
    case FieldType::UINT16: return { VK_FORMAT_R16_UINT, 1 };
    case FieldType::UINT16_2: return { VK_FORMAT_R16G16_UINT, 1 };
    case FieldType::UINT16_3: return { VK_FORMAT_R16G16B16_UINT, 1 };
    case FieldType::UINT16_4: return { VK_FORMAT_R16G16B16A16_UINT, 1 };
    case FieldType::UINT16_2X2: return { VK_FORMAT_R16G16_UINT, 2 };
    case FieldType::UINT16_3X3: return { VK_FORMAT_R16G16B16_UINT, 3 };
    case FieldType::UINT16_4X4: return { VK_FORMAT_R16G16B16A16_UINT, 4 };
    case FieldType::UINT32: return { VK_FORMAT_R32_UINT, 1 };
    case FieldType::UINT32_2: return { VK_FORMAT_R32G32_UINT, 1 };
    case FieldType::UINT32_3: return { VK_FORMAT_R32G32B32_UINT, 1 };
    case FieldType::UINT32_4: return { VK_FORMAT_R32G32B32A32_UINT, 1 };
    case FieldType::UINT32_2X2: return { VK_FORMAT_R32G32_UINT, 2 };
    case FieldType::UINT32_3X3: return { VK_FORMAT_R32G32B32_UINT, 3 };
    case FieldType::UINT32_4X4: return { VK_FORMAT_R32G32B32A32_UINT, 4 };
    case FieldType::UINT64: return { VK_FORMAT_R64_UINT, 1 };
    case FieldType::UINT64_2: return { VK_FORMAT_R64G64_UINT, 1 };
    case FieldType::UINT64_3: return { VK_FORMAT_R64G64B64_UINT, 1 };
    case FieldType::UINT64_4: return { VK_FORMAT_R64G64B64A64_UINT, 1 };
    case FieldType::UINT64_2X2: return { VK_FORMAT_R64G64_UINT, 2 };
    case FieldType::UINT64_3X3: return { VK_FORMAT_R64G64B64_UINT, 3 };
    case FieldType::UINT64_4X4: return { VK_FORMAT_R64G64B64A64_UINT, 4 };
    case FieldType::FLOAT16: return { VK_FORMAT_R16_SFLOAT, 1 };
    case FieldType::FLOAT16_2: return { VK_FORMAT_R16G16_SFLOAT, 1 };
    case FieldType::FLOAT16_3: return { VK_FORMAT_R16G16B16_SFLOAT, 1 };
    case FieldType::FLOAT16_4: return { VK_FORMAT_R16G16B16A16_SFLOAT, 1 };
    case FieldType::FLOAT16_2X2: return { VK_FORMAT_R16G16_SFLOAT, 2 };
    case FieldType::FLOAT16_3X3: return { VK_FORMAT_R16G16B16_SFLOAT, 3 };
    case FieldType::FLOAT16_4X4: return { VK_FORMAT_R16G16B16A16_SFLOAT, 4 };
    case FieldType::FLOAT32: return { VK_FORMAT_R32_SFLOAT, 1 };
    case FieldType::FLOAT32_2: return { VK_FORMAT_R32G32_SFLOAT, 1 };
    case FieldType::FLOAT32_3: return { VK_FORMAT_R32G32B32_SFLOAT, 1 };
    case FieldType::FLOAT32_4: return { VK_FORMAT_R32G32B32A32_SFLOAT, 1 };
    case FieldType::FLOAT32_2X2: return { VK_FORMAT_R32G32_SFLOAT, 2 };
    case FieldType::FLOAT32_3X3: return { VK_FORMAT_R32G32B32_SFLOAT, 3 };
    case FieldType::FLOAT32_4X4: return { VK_FORMAT_R32G32B32A32_SFLOAT, 4 };
    case FieldType::FLOAT64: return { VK_FORMAT_R64_SFLOAT, 1 };
    case FieldType::FLOAT64_2: return { VK_FORMAT_R64G64_SFLOAT, 1 };
    case FieldType::FLOAT64_3: return { VK_FORMAT_R64G64B64_SFLOAT, 1 };
    case FieldType::FLOAT64_4: return { VK_FORMAT_R64G64B64A64_SFLOAT, 1 };
    case FieldType::FLOAT64_2X2: return { VK_FORMAT_R64G64_SFLOAT, 2 };
    case FieldType::FLOAT64_3X3: return { VK_FORMAT_R64G64B64_SFLOAT, 3 };
    case FieldType::FLOAT64_4X4: return { VK_FORMAT_R64G64B64A64_SFLOAT, 4 };
    case FieldType::UNKNOWN:
    default:
        return { VK_FORMAT_UNDEFINED, 0 };
    }
}
