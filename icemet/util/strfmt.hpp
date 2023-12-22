#ifndef ICEMET_STRFMT_H
#define ICEMET_STRFMT_H

#include <fmt/core.h>

#include <string>

template <typename... Args>
inline std::string strfmt(const std::string& fmt, Args... args) { return fmt::format(fmt, args...); }

#endif
