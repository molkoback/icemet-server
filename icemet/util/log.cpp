#include "log.hpp"

#include "icemet/icemet.hpp"
#include "icemet/util/time.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

static const char* logfmt = COLOR_BRIGHT_BLACK "[" COLOR_RESET "{}" COLOR_BRIGHT_BLACK "]<" COLOR_BRIGHT_WHITE "{}" COLOR_BRIGHT_BLACK ">(" COLOR_RESET "{}" COLOR_BRIGHT_BLACK ")" COLOR_RESET " {}";
static LogLevel loglevel = LOG_INFO;
static std::mutex mutex;

Log::Log(const std::string& name) :
	m_name(name) {}

static const char* levelstr(LogLevel level)
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

void Log::logsend(LogLevel level, const std::string& str) const
{
	mutex.lock();
	if (level >= loglevel)
		std::cout << strfmt(logfmt, DateTime::now().str("%H:%M:%S", true), m_name, levelstr(level), str) << std::endl;
	mutex.unlock();
}

void Log::setLevel(LogLevel level)
{
	mutex.lock();
	loglevel = level;
	mutex.unlock();
}

LogLevel Log::level()
{
	mutex.lock();
	LogLevel ret = loglevel;
	mutex.unlock();
	return ret;
}
