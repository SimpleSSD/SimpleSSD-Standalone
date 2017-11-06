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

#ifndef __TRACE_READER_HH__
#define __TRACE_READER_HH__

#include <iostream>
#include <fstream>
#include <string>

class TraceReader{
  private:
    std::ifstream fh;
    std::string fn;
  public:
    TraceReader(std::string filename);
    ~TraceReader();
    bool ReadIO(uint64_t &address, uint8_t &rw, uint32_t &length);
};

#endif
