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

#include "trace.hh"

TraceReader::TraceReader(std::string filename)
{
  fh.open(filename.c_str());
  fn = filename;
  if (!fh)
  {
    printf("File %s open FAIL!\n", filename.c_str());
    std::cerr<<": ** CRITICAL ERROR: can NOT open trace file!\n";
    std::terminate();
  }
}

TraceReader::~TraceReader()
{
  fh.close();
}

bool TraceReader::ReadIO(uint64_t &address, uint8_t &rw, uint32_t &length)
{
  std::string timestamp;
  std::string read_write;
  std::string str_address;
  std::string str_length;
  if (fh.eof()) return false;
  fh >> timestamp >> read_write >> str_address >> str_length >> timestamp;

  address = (uint64_t)strtoul(str_address.c_str(), NULL, 10);
  length = (uint32_t)strtoul(str_length.c_str(), NULL, 10);

  address = (uint64_t)(address % (2 * 1024 * 1024 * 300)) * (uint64_t)512;
  if (length % 8 > 0) length = length / 8 + 1;
  else length = length / 8;
  if (read_write == "W") {
    rw = 1;
    printf("address = %llu\tlength = %u\n", address, length);
  }
  else if (read_write == "R") {
    rw = 0;
    printf("address = %llu\tlength = %u\n", address, length);
  }
  else {
    return this->ReadIO(address, rw, length);
  }
  return true;
}
