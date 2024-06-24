# pragma once

#include <functional>
#include <vector>
#include <ranges>

template<typename... Args>
class Signal {
public:
    using FunctionType = std::function<void(Args...)>;

    void connect(FunctionType func, std::string name = "") {
        if (name.empty())
            m_functions.push_back(func);
        else
            m_namedFunctions[name] = func;
    }

    template<typename T>
    void connect(T* instance, void (T::*method)(Args...), std::string name = "") {
        FunctionType func = [=](Args... args) { return (instance->*method)(args...); };
        if (name.empty())
            m_functions.push_back(func);
        else
            m_namedFunctions[name] = func;
    }

    bool disconnect(std::string name) {
        return m_namedFunctions.erase(name) != 0;
    }

    void emit(Args... args) {
        for (const FunctionType& func : m_functions) {
            func(args...);
        }
        for (const FunctionType& func : m_namedFunctions | std::views::values) {
            func(args...);
        }
    }

    size_t isEmpty()
    {
        return m_functions.empty() && m_namedFunctions.empty();
    }

    void reset() {
        m_functions.clear();
        m_namedFunctions.clear();
    }

private:
    std::vector<FunctionType> m_functions;
    std::unordered_map<std::string, FunctionType> m_namedFunctions;
};