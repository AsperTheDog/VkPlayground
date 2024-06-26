#pragma once
#include <iostream>
#include <vector>
#include <sstream>

typedef uint8_t LoggerLevels;

class Logger
{
public:
    enum LevelBits
    {
        NONE = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 4,
        ERR = 8,
        ALL = DEBUG | INFO | WARN | ERR
    };

    static void setEnabled(bool enabled);
    static void setLevels(LoggerLevels levels);

	static void setRootContext(std::string_view context);

	static void pushContext(std::string_view context);
	static void popContext();

	static void print(std::string_view message, LevelBits level, bool printHeader = true);
    static void setThreadSafe(bool activate);

private:

	inline static std::vector<std::string> m_contexts{};
	inline static std::string m_rootContext = "ROOT";

    inline static bool m_enabled = true;
    inline static LoggerLevels m_levels = LevelBits::INFO | LevelBits::WARN | LevelBits::ERR;

    inline static bool m_threadSafeMode = false;

	Logger() = default;
};

inline void Logger::setEnabled(const bool enabled)
{
    m_enabled = enabled;
}

inline void Logger::setLevels(const LoggerLevels levels)
{
    m_levels = levels;
}

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

inline void Logger::print(const std::string_view message, const LevelBits level, const bool printHeader)
{
    if (!m_enabled || (m_levels & level) == 0) return;

    if (printHeader)
    {
        std::string levelStr;
        switch (level)
        {
            case LevelBits::DEBUG:
                levelStr = "DEBUG";
                break;
            case LevelBits::INFO:
                levelStr = "INFO";
                break;
            case LevelBits::WARN:
                levelStr = "WARNING";
                break;
            case LevelBits::ERR:
                levelStr = "ERROR";
                break;
            default: 
                levelStr = "UNKNOWN";
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
    else
    {
        std::cout << message << '\n';
    }
	
    if (m_threadSafeMode) std::cout.flush();
}

inline void Logger::setThreadSafe(const bool activate)
{
    m_threadSafeMode = activate;
}
