#ifndef ICEMET_STRFMT_H
#define ICEMET_STRFMT_H

#include <cstdarg>
#include <string>
#include <vector>

std::vector<char> vvecfmt(const std::string& fmt, va_list args);
std::vector<char> vecfmt(const std::string& fmt, ...);
std::string vstrfmt(const std::string& fmt, va_list args);
std::string strfmt(const std::string& fmt, ...);

#endif
