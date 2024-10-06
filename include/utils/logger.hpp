#pragma once
#include <string>
#include <vector>

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
    
    static void setEnabled(const bool enabled) { m_enabled = enabled; }
    static void setLevels(const LoggerLevels levels) { m_levels = levels; }
    static void setRootContext(const std::string& context) { m_rootContext = context; }
    static void setThreadSafe(const bool activate) { m_threadSafeMode = activate; }

    static void pushContext(const std::string& context) { m_contexts.push_back(context); }
	static void popContext();

	static void print(const std::string& message, LevelBits level, bool printHeader = true);

private:

	inline static std::vector<std::string> m_contexts{};
	inline static std::string m_rootContext = "ROOT";

    inline static bool m_enabled = true;
    inline static LoggerLevels m_levels = LevelBits::INFO | LevelBits::WARN | LevelBits::ERR;

    inline static bool m_threadSafeMode = false;

	Logger() = default;
};