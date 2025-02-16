#pragma once
#include <sstream>
#include <string>
#include <vector>

typedef uint8_t LoggerLevels;

#define LOG_DEBUG(...) if (Logger::isLevelActive(Logger::DEBUG)) Logger::print(Logger::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) if (Logger::isLevelActive(Logger::INFO)) Logger::print(Logger::INFO, __VA_ARGS__)
#define LOG_WARN(...) if (Logger::isLevelActive(Logger::WARN)) Logger::print(Logger::WARN, __VA_ARGS__)
#define LOG_ERR(...) if (Logger::isLevelActive(Logger::ERR)) Logger::print(Logger::ERR, __VA_ARGS__)

class Logger
{
public:
    enum LevelBits : uint8_t
    {
        NONE = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 4,
        ERR = 8,
        ALL = DEBUG | INFO | WARN | ERR
    };
    
    static void setEnabled(const bool p_Enabled) { m_Enabled = p_Enabled; }
    static void setLevels(const LoggerLevels p_Levels) { m_Levels = p_Levels; }
    static void setRootContext(const std::string_view p_Context) { m_RootContext = p_Context; }
    static void setThreadSafe(const bool p_Activate) { m_ThreadSafeMode = p_Activate; }

    static void pushContext(const std::string_view p_Context) { m_Contexts.emplace_back(p_Context); }
	static void popContext();

    static bool isLevelActive(LevelBits p_Level);

    template<typename ... Args>
	static void print(LevelBits l_Level, Args&&... p_Args);
private:
    static void printStream(const std::stringstream& p_Message, LevelBits l_Level);

	inline static std::vector<std::string> m_Contexts{};
	inline static std::string m_RootContext = "ROOT";

    inline static bool m_Enabled = true;
    inline static LoggerLevels m_Levels = LevelBits::INFO | LevelBits::WARN | LevelBits::ERR;

    inline static bool m_ThreadSafeMode = false;

	Logger() = default;
};

template <typename... Args>
void Logger::print(const LevelBits l_Level, Args&&... p_Args)
{
    std::stringstream l_Message;
    (l_Message << ... << std::forward<Args>(p_Args));
    printStream(l_Message, l_Level);
}
