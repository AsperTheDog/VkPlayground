#pragma once
#include <iostream>
#include <vector>
#include <sstream>

class Logger
{
public:
    enum Levels
    {
        DEBUG = 1,
        INFO = 2,
        WARN = 4,
        ERR = 8
    };

	static void setRootContext(std::string_view context);

	static void pushContext(std::string_view context);
	static void popContext();

	static void print(std::string_view message, Levels level);

private:

	inline static std::vector<std::string> m_contexts{};
	inline static std::string m_rootContext = "ROOT";

    inline static bool m_enabled = true;
    inline static uint8_t m_levels = Levels::INFO | Levels::WARN | Levels::ERR;

	Logger() = default;
};

inline void Logger::setRootContext(const std::string_view context)
{
	m_rootContext = context;
}

inline void Logger::pushContext(const std::string_view context)
{
	const std::string contextStr = static_cast<std::string>(context);
	m_contexts.push_back(contextStr);
}

inline void Logger::popContext()
{
	const std::string contextStr = m_contexts.back();
	m_contexts.pop_back();
}

inline void Logger::print(const std::string_view message, const Levels level)
{
    if (!m_enabled || (m_levels & level) == 0) return;

    std::string levelStr;
    switch (level)
    {
        case Levels::DEBUG:
            levelStr = "DEBUG";
            break;
        case Levels::INFO:
            levelStr = "INFO";
            break;
        case Levels::WARN:
            levelStr = "WARNING";
            break;
        case Levels::ERR:
            levelStr = "ERROR";
            break;
    }

	std::stringstream context;
	if (!m_contexts.empty())
	{
		for (uint32_t i = 0; i < m_contexts.size(); ++i)
		{
			context << "  ";
		}
		context << "[" << m_contexts.back() << " | " << levelStr << "]: ";
	}
	else
	{
		context << "[" << m_rootContext << " | " << levelStr << "]: ";
	}

	std::cout << context.str() << message << '\n';
}
