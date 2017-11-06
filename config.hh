/**
 * Copyright (C) 2017 CAMELab
 *
 * This file is part of SimpleSSD.
 *
 * SimpleSSD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSSD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SimpleSSD.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Donghyun Gouk <kukdh1@camelab.org>
 */

#ifndef __CONFIG_HH__
#define __CONFIG_HH__

#include <iostream>
#include <string>

#include "simplessd/base_config.hh"
#include "simplessd/ini.h"
#include "base/misc.hh"

class Config : public BaseConfig {
  private:
    static int parserHandler(void *, const char *, const char *, const char *);

  public:
    Config(std::string);

    /** Request Generator Configuration area **/
    int32_t MaxRequest;
    int32_t StartPPN;
    int32_t RequestSize;
    int32_t ReadFraction;
    int32_t ReadDenominator;
    int32_t RandomFraction;
    int32_t RandomDenominator;
    int32_t RandomSeed;
    int32_t IOGEN;
    int32_t QueueDepth;

    /** Trace Replayer Configuration area **/
    bool Enable;
    std::string TraceFile;
};

#endif
