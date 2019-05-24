#include "log.hpp"

#include "util/strfmt.hpp"

#include <cstdio>
#include <ctime>
#include <mutex>

const char *logfmt = COLOR_BRIGHT_BLACK "[" COLOR_RESET "%s" COLOR_BRIGHT_BLACK "]<" COLOR_BRIGHT_WHITE "%s" COLOR_BRIGHT_BLACK ">(" COLOR_RESET "%s" COLOR_BRIGHT_BLACK ")" COLOR_RESET " %s\n";

LogLevel loglevel = LOG_INFO;
std::mutex mutex; // Protects stdout

Log::Log(const std::string& name) :
	m_name(name) {}

const char* Log::levelstr(LogLevel level) const
{
	switch (level) {
	case LOG_DEBUG:
		return COLOR_BRIGHT_BLACK "DEBUG" COLOR_RESET;
		break;
	case LOG_INFO:
		return "INFO";
		break;
	case LOG_WARNING:
		return COLOR_YELLOW "WARNING" COLOR_RESET;
		break;
	case LOG_ERROR:
		return COLOR_BRIGHT_RED "ERROR" COLOR_RESET;
		break;
	case LOG_CRITICAL:
		return COLOR_RED "CRITICAL" COLOR_RESET;
		break;
	default:
		return "";
	}
}

void Log::logsend(LogLevel level, const std::string& fmt, va_list args) const
{
	if (level < loglevel)
		return;
	
	time_t rawtime;
	time(&rawtime);
	struct tm *info = localtime(&rawtime);
	char timebuf[9];
	strftime(timebuf, 9, "%H:%M:%S", info);
	
	mutex.lock();
	fprintf(stdout, logfmt, timebuf, m_name.c_str(), levelstr(level), vstrfmt(fmt, args).c_str());
	fflush(stdout);
	mutex.unlock();
}

void Log::debug(const std::string& fmt, ...) const
{
	va_list args;
	va_start(args, fmt);
	logsend(LOG_DEBUG, fmt, args);
	va_end(args);
}

void Log::info(const std::string& fmt, ...) const
{
	va_list args;
	va_start(args, fmt);
	logsend(LOG_INFO, fmt, args);
	va_end(args);
}

void Log::warning(const std::string& fmt, ...) const
{
	va_list args;
	va_start(args, fmt);
	logsend(LOG_WARNING, fmt, args);
	va_end(args);
}

void Log::error(const std::string& fmt, ...) const
{
	va_list args;
	va_start(args, fmt);
	logsend(LOG_ERROR, fmt, args);
	va_end(args);
}

void Log::critical(const std::string& fmt, ...) const
{
	va_list args;
	va_start(args, fmt);
	logsend(LOG_CRITICAL, fmt, args);
	va_end(args);
}

void Log::setLevel(LogLevel level)
{
	loglevel = level;
}

LogLevel Log::level()
{
	return loglevel;
}
