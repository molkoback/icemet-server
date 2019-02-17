#include "strfmt.hpp"

#include <cstring>

std::vector<char> vvecfmt(const std::string& fmt, va_list args)
{
	va_list args_count;
	va_copy(args_count, args);
	
	size_t len = std::vsnprintf(NULL, 0, fmt.c_str(), args_count);
	std::vector<char> vec(len+1);
	std::vsnprintf(&vec[0], len+1, fmt.c_str(), args);
	
	va_end(args_count);
	return vec;
}

std::vector<char> vecfmt(const std::string& fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	std::vector<char> vec = vvecfmt(fmt, args);
	va_end(args);
	return vec;
}

std::string vstrfmt(const std::string& fmt, va_list args)
{
	std::vector<char> vec = vvecfmt(fmt, args);
	return std::string(&vec[0]);
}

std::string strfmt(const std::string& fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	std::vector<char> vec = vvecfmt(fmt, args);
	std::string str = std::string(&vec[0]);
	va_end(args);
	return str;
}
