#pragma once

#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

struct StringHash
{
    using is_transparent = void;

    std::size_t operator()(const std::string_view key) const noexcept
    {
        return std::hash<std::string_view>{}(key);
    }
};

struct StringEqual
{
    using is_transparent = void;

    bool operator()(const std::string_view lhs, const std::string_view rhs) const noexcept
    {
        return lhs == rhs;
    }
};

template <typename T>
using StringUMap = std::unordered_map<std::string, T, StringHash, StringEqual>;

template <typename T>
using StringMap = std::map<std::string, T, StringHash, StringEqual>;