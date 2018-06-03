#pragma once

#ifndef __UTIL_CONVERT_
#define __UTIL_CONVERT_

#include <cinttypes>

bool convertBoolean(const char *);
uint64_t convertInteger(const char *, bool * = nullptr);
uint64_t convertTime(const char *, bool * = nullptr);

#endif
