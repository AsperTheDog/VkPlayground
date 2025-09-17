#include "utils/logger.hpp"

#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#define SET_COLOR(color) SetConsoleTextAttribute(l_Console, color)
#define RESET_COLOR SetConsoleTextAttribute(l_Console, 7)
#else
#define SET_COLOR(color)
#define RESET_COLOR
#endif

void Logger::popContext()
{
    const std::string l_ContextStr = s_Contexts.back();
    s_Contexts.pop_back();
}

void Logger::printStream(const std::stringstream& p_Message, const LevelBits p_Level)
{
    if (!s_Enabled || (s_Levels & p_Level) == 0)
    {
        return;
    }

#ifdef _WIN32
    const HANDLE l_Console = GetStdHandle(STD_ERROR_HANDLE);
#endif

    std::string l_LevelStr;
    switch (p_Level)
    {
    case DEBUG:
        l_LevelStr = "DEBUG";
        SET_COLOR(8);
        break;
    case INFO:
        l_LevelStr = "INFO";
        SET_COLOR(7);
        break;
    case WARN:
        l_LevelStr = "WARNING";
        SET_COLOR(14);
        break;
    case ERR:
        l_LevelStr = "ERROR";
        SET_COLOR(12);
        break;
    default:
        l_LevelStr = "UNKNOWN";
    }

    std::stringstream l_Context;
    if (!s_Contexts.empty())
    {
        for (uint32_t i = 0; i < s_Contexts.size(); ++i)
        {
            l_Context << "  ";
        }
        l_Context << "[" << s_Contexts.back() << " | " << l_LevelStr << "]: ";
    }
    else
    {
        l_Context << "[" << s_RootContext << " | " << l_LevelStr << "]: ";
    }
    std::cout << l_Context.str() << p_Message.str() << '\n';

    RESET_COLOR;
    if (s_ThreadSafeMode)
    {
        std::cout.flush();
    }
}

bool Logger::isLevelActive(const LevelBits p_Level)
{
    return (s_Levels & p_Level) != 0;
}
