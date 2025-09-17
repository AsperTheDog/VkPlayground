#pragma once
#include <functional>

class CallOnDestroyList
{
public:
    CallOnDestroyList() = default;

    ~CallOnDestroyList()
    {
        for (auto& l_Func : m_Functions)
        {
            l_Func();
        }
    }

    void add(std::function<void()> p_Func)
    {
        m_Functions.push_back(std::move(p_Func));
    }

    void cancel()
    {
        m_Functions.clear();
    }

private:
    std::vector<std::function<void()>> m_Functions;
};

class CallOnDestroy
{
public:
    explicit CallOnDestroy(std::function<void()> p_Func) : m_Function(std::move(p_Func)) {}
    ~CallOnDestroy()
    {
        if (m_Function)
        {
            m_Function();
        }
    }

    void cancel()
    {
        m_Function = nullptr;
    }

private:
    std::function<void()> m_Function;
};
