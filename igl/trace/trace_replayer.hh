/*
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
 */

#pragma once

#ifndef __IGL_TRACE_REPLAYER__
#define __IGL_TRACE_REPLAYER__

#include <fstream>
#include <mutex>
#include <regex>
#include <thread>

#include "bil/entry.hh"
#include "igl/io_gen.hh"
#include "sim/cfg_reader.hh"
#include "sim/engine.hh"

namespace IGL {

class TraceReplayer : public IOGenerator {
 private:
  enum {
    ID_OPERATION,
    ID_BYTE_OFFSET,
    ID_BYTE_LENGTH,
    ID_LBA_OFFSET,
    ID_LBA_LENGTH,
    ID_TIME_SEC,
    ID_TIME_MS,
    ID_TIME_US,
    ID_TIME_NS,
    ID_TIME_PS,
    ID_NUM
  };

  std::mutex m;

  std::ifstream file;
  std::regex regex;

  uint64_t fileSize;

  TIMING_MODE mode;
  uint64_t syncBreak;
  uint64_t asyncBreak;
  uint32_t maxQueueDepth;

  bool useLBA;
  uint32_t lbaSize;
  uint32_t groupID[ID_NUM];
  bool timeValids[5];
  bool useHex;

  uint64_t ssdSize;
  uint32_t blocksize;

  uint64_t initTime;
  uint64_t firstTick;

  bool reserveTermination;
  int64_t iodepth;

  uint64_t max_io;
  uint64_t io_submitted;
  uint64_t io_count;
  uint64_t read_count;
  uint64_t write_count;

  BIL::BIO bio;

  uint64_t mergeTime(std::smatch &);
  BIL::BIO_TYPE getType(std::string);
  void handleNextLine(bool = false);

  SimpleSSD::Event submitEvent;
  SimpleSSD::EventFunction submitIO;
  SimpleSSD::EventFunction iocallback;

  void _submitIO(uint64_t);
  void _iocallback(uint64_t);

 public:
  TraceReplayer(Engine &, BIL::BlockIOEntry &, std::function<void()> &,
                ConfigReader &);
  ~TraceReplayer();

  void init(uint64_t, uint32_t) override;
  void begin() override;
  void printStats(std::ostream &) override;
  void getProgress(float &) override;
};

}  // namespace IGL

#endif
