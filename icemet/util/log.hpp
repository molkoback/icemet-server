#ifndef ICEMET_LOG_H
#define ICEMET_LOG_H

#include "icemet/util/strfmt.hpp"

#include <string>

#define COLOR_BLACK          "\x1b[30m"
#define COLOR_RED            "\x1b[31m"
#define COLOR_GREEN          "\x1b[32m"
#define COLOR_YELLOW         "\x1b[33m"
#define COLOR_BLUE           "\x1b[34m"
#define COLOR_MAGENTA        "\x1b[35m"
#define COLOR_CYAN           "\x1b[36m"
#define COLOR_WHITE          "\x1b[37m"

#define COLOR_BRIGHT_BLACK   "\x1b[90m"
#define COLOR_BRIGHT_RED     "\x1b[91m"
#define COLOR_BRIGHT_GREEN   "\x1b[92m"
#define COLOR_BRIGHT_YELLOW  "\x1b[93m"
#define COLOR_BRIGHT_BLUE    "\x1b[94m"
#define COLOR_BRIGHT_MAGENTA "\x1b[95m"
#define COLOR_BRIGHT_CYAN    "\x1b[96m"
#define COLOR_BRIGHT_WHITE   "\x1b[97m"

#define COLOR_RESET          "\x1b[0m"

typedef enum _log_level {
	LOG_DEBUG = 10,
	LOG_INFO = 20,
	LOG_WARNING = 30,
	LOG_ERROR = 40,
	LOG_CRITICAL = 50
} LogLevel;

class Log {
private:
	std::string m_name;
	
	void logsend(LogLevel level, const std::string& str) const;

public:
	Log(const std::string& name);
	
	template <typename... Args>
	void debug(const std::string& fmt, Args... args) const { logsend(LOG_DEBUG, strfmt(fmt, args...)); }
	
	template <typename... Args>
	void info(const std::string& fmt, Args... args) const { logsend(LOG_INFO, strfmt(fmt, args...)); }
	
	template <typename... Args>
	void warning(const std::string& fmt, Args... args) const { logsend(LOG_WARNING, strfmt(fmt, args...)); }
	
	template <typename... Args>
	void error(const std::string& fmt, Args... args) const { logsend(LOG_ERROR, strfmt(fmt, args...)); }
	
	template <typename... Args>
	void critical(const std::string& fmt, Args... args) const { logsend(LOG_CRITICAL, strfmt(fmt, args...)); }
	
	static LogLevel level();
	static void setLevel(LogLevel level);
};

#endif
