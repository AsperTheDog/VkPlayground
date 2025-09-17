#pragma once

#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

struct StringHash
{
    using IsTransparent = void;

    std::size_t operator()(const std::string_view p_Key) const noexcept
    {
        return std::hash<std::string_view>{}(p_Key);
    }
};

struct StringEqual
{
    using IsTransparent = void;

    bool operator()(const std::string_view p_Lhs, const std::string_view p_Rhs) const noexcept
    {
        return p_Lhs == p_Rhs;
    }
};

template <typename T>
using StringUMap = std::unordered_map<std::string, T, StringHash, StringEqual>;

template <typename T>
using StringMap = std::map<std::string, T, StringHash, StringEqual>;
