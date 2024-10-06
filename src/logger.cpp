#include "utils/logger.hpp"

#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#define SET_COLOR(color) SetConsoleTextAttribute(hConsole, color)
#define RESET_COLOR SetConsoleTextAttribute(hConsole, 7)
#else
#define SET_COLOR(color)
#define RESET_COLOR
#endif

void Logger::popContext()
{
	const std::string contextStr = m_contexts.back();
	m_contexts.pop_back();
}

void Logger::print(const std::string& message, const LevelBits level, const bool printHeader)
{
    if (!m_enabled || (m_levels & level) == 0) return;

#ifdef _WIN32
    const HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
#endif

    if (printHeader)
    {
        std::string levelStr;
        switch (level)
        {
            case LevelBits::DEBUG:
                levelStr = "DEBUG";
                SET_COLOR(8);
                break;
            case LevelBits::INFO:
                levelStr = "INFO";
                SET_COLOR(7);
                break;
            case LevelBits::WARN:
                levelStr = "WARNING";
                SET_COLOR(14);
                break;
            case LevelBits::ERR:
                levelStr = "ERROR";
                SET_COLOR(12);
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

    RESET_COLOR;
    if (m_threadSafeMode) std::cout.flush();
}
