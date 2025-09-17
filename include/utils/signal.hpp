# pragma once

#include <functional>
#include <vector>

#include "string_utils.hpp"
#include <ranges>

template <typename... Args>
class Signal
{
public:
    using FunctionType = std::function<void(Args...)>;

    void connect(FunctionType p_Func, std::string p_Name = "")
    {
        if (p_Name.empty())
        {
            m_Functions.push_back(p_Func);
        }
        else
        {
            m_NamedFunctions[p_Name] = p_Func;
        }
    }

    template <typename T>
    void connect(T* p_Instance, void (T::*p_Method)(Args...), std::string p_Name = "")
    {
        FunctionType l_Func = [=](Args... p_Args) { return (p_Instance->*p_Method)(p_Args...); };
        if (p_Name.empty())
        {
            m_Functions.push_back(l_Func);
        }
        else
        {
            m_NamedFunctions[p_Name] = l_Func;
        }
    }

    bool disconnect(std::string p_Name)
    {
        return m_NamedFunctions.erase(p_Name) != 0;
    }

    void emit(Args... p_Args)
    {
        for (const FunctionType& l_Func : m_Functions)
        {
            l_Func(p_Args...);
        }
        for (const FunctionType& l_Func : m_NamedFunctions | std::views::values)
        {
            l_Func(p_Args...);
        }
    }

    size_t isEmpty()
    {
        return m_Functions.empty() && m_NamedFunctions.empty();
    }

    void reset()
    {
        m_Functions.clear();
        m_NamedFunctions.clear();
    }

private:
    std::vector<FunctionType> m_Functions;
    StringUMap<FunctionType> m_NamedFunctions;
};
