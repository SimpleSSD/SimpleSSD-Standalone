// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __IGL_REQUEST_GENERATOR_HH__
#define __IGL_REQUEST_GENERATOR_HH__

#include <list>
#include <mutex>
#include <random>
#include <thread>

#include "igl/abstract_io_generator.hh"

namespace Standalone::IGL {

class RequestGenerator : public AbstractIOGenerator {
 private:
  std::mutex m;

  uint64_t io_size;
  uint64_t io_submitted;

  RequestConfig::IOType type;
  RequestConfig::IOMode mode;
  bool time_based;
  bool reserveTermination;

  float rwmixread;

  uint64_t iodepth;   //!< I/O depth from configuration
  uint64_t io_depth;  //!< Current I/O depth

  uint64_t io_count;
  uint64_t read_count;

  uint64_t offset;
  uint64_t size;

  uint64_t thinktime;

  uint64_t blocksize;
  uint64_t blockalign;

  uint64_t randseed;
  std::mt19937_64 randengine;
  std::uniform_int_distribution<uint64_t> randgen;

  uint64_t runtime;   //!< Only used when time_based == true
  uint64_t initTime;  //!< I/O generation begin at

  void generateAddress(uint64_t &, uint64_t &);
  bool nextIOIsRead();

  Event submitEvent;
  Event completionEvent;

  void submitIO(uint64_t);
  void iocallback(uint64_t, uint64_t);

 public:
  RequestGenerator(ObjectData &, BlockIOLayer &, Event);
  ~RequestGenerator();

  void initialize() override;

  void begin() override;

  void printStats(std::ostream &) override;
  void getProgress(float &) override;
};

}  // namespace Standalone::IGL

#endif
