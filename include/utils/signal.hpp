# pragma once

#include <functional>
#include <vector>

#include "string_utils.hpp"
#include <ranges>

template<typename... Args>
class Signal {
public:
    using FunctionType = std::function<void(Args...)>;

    void connect(FunctionType p_Func, std::string p_Name = "") {
        if (p_Name.empty())
            m_Functions.push_back(p_Func);
        else
            m_namedFunctions[p_Name] = p_Func;
    }

    template<typename T>
    void connect(T* p_Instance, void (T::*p_Method)(Args...), std::string p_Name = "") {
        FunctionType func = [=](Args... p_Args) { return (p_Instance->*p_Method)(p_Args...); };
        if (p_Name.empty())
            m_Functions.push_back(func);
        else
            m_namedFunctions[p_Name] = func;
    }

    bool disconnect(std::string p_Name) {
        return m_namedFunctions.erase(p_Name) != 0;
    }

    void emit(Args... p_Args) {
        for (const FunctionType& func : m_Functions) {
            func(p_Args...);
        }
        for (const FunctionType& func : m_namedFunctions | std::views::values) {
            func(p_Args...);
        }
    }

    size_t isEmpty()
    {
        return m_Functions.empty() && m_namedFunctions.empty();
    }

    void reset() {
        m_Functions.clear();
        m_namedFunctions.clear();
    }

private:
    std::vector<FunctionType> m_Functions;
    StringUMap<FunctionType> m_namedFunctions;
};
