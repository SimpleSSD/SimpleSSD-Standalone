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

void print(std::ostream &, std::string, uint32_t);
void print(std::ostream &, double, uint32_t);

#endif
