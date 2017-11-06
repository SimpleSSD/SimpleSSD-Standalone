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
 * Authors: Jie Zhang <jie@camelab.org>
 *          Miryoung Kwon <mkwon@camelab.org>
 *          Donghyun Gouk <kukdh1@camelab.org>
 */

#ifndef __RGEN_HH__
#define __RGEN_HH__

#include <cinttypes>
#include <iostream>

#include "trace.hh"
#include "config.hh"

typedef struct _Request
{
  uint64_t PPN;
  uint8_t  Oper;
  uint32_t REQ_SIZE;
  int IOGEN;
}Request;

class RequestGenerator
{
  public:
    uint32_t MAX_REQ;
    uint32_t cur_REQ;
    uint32_t cur_page;
    uint64_t start_PPN;
    uint32_t REQ_SIZE;
    Request curRequest;
    std::string fn;
    TraceReader* tr;
    struct Read_Ratio {
      uint32_t fraction;
      uint32_t denominator;
    } readratio;
    struct Random_Ratio {
      uint32_t fraction;
      uint32_t denominator;
    } randomratio;
    int IOGEN;
    RequestGenerator(Config &cfg);
    ~RequestGenerator();
    bool generate_request();
};
#endif
