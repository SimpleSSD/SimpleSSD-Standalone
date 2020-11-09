// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __UTIL_PRINT__
#define __UTIL_PRINT__

#include <cinttypes>
#include <iostream>
#include <string>

// Functions for statistics aligned print
void print(std::ostream &, std::string, uint32_t);
void print(std::ostream &, double, uint32_t);

// Functions for progress print
void printBandwidth(std::ostream &, uint64_t, uint64_t);
void printBandwidth(std::ostream &, uint64_t);
void printBandwidth(int, uint64_t);
void printLatency(std::ostream &, uint64_t);
void printLatency(int, uint64_t);

#endif
