#pragma once
#include <functional>

class CallOnDestroyList
{
public:
    CallOnDestroyList() = default;
    ~CallOnDestroyList()
    {
        for (auto& func : m_Functions)
            func();
    }
    void add(std::function<void()> func)
    {
        m_Functions.push_back(std::move(func));
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
    explicit CallOnDestroy(std::function<void()> func) : m_Function(std::move(func)) {}
    ~CallOnDestroy()
    {
        if (m_Function)
            m_Function();
    }

    void cancel()
    {
        m_Function = nullptr;
    }

private:
    std::function<void()> m_Function;
};