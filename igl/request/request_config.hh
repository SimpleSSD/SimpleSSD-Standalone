// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __IGL_REQUEST_CONFIG__
#define __IGL_REQUEST_CONFIG__

#include "simplessd/sim/base_config.hh"

namespace IGL {

typedef enum {
  REQUEST_IO_SIZE,
  REQUEST_IO_TYPE,
  REQUEST_IO_MIX_RATIO,
  REQUEST_BLOCK_SIZE,
  REQUEST_BLOCK_ALIGN,
  REQUEST_IO_MODE,
  REQUEST_IO_DEPTH,
  REQUEST_OFFSET,
  REQUEST_SIZE,
  REQUEST_THINKTIME,
  REQUEST_RANDOM_SEED,
  REQUEST_TIME_BASED,
  REQUEST_RUN_TIME,
} REQUEST_CONFIG;

typedef enum {
  IO_READ,
  IO_WRITE,
  IO_RANDREAD,
  IO_RANDWRITE,
  IO_READWRITE,
  IO_RANDRW,
  IO_TYPE_NUM,
} IO_TYPE;

typedef enum {
  IO_SYNC,
  IO_ASYNC,
  IO_MODE_NUM,
} IO_MODE;

class RequestConfig : public SimpleSSD::BaseConfig {
 private:
  uint64_t io_size;
  IO_TYPE type;
  float rwmixread;
  uint64_t blocksize;
  uint64_t blockalign;
  IO_MODE mode;
  uint64_t iodepth;
  uint64_t offset;
  uint64_t size;
  uint64_t thinktime;
  uint64_t randseed;
  bool time_based;
  uint64_t runtime;

 public:
  RequestConfig();

  bool setConfig(const char *, const char *) override;
  void update() override;

  uint64_t readUint(uint32_t) override;
  float readFloat(uint32_t) override;
  bool readBoolean(uint32_t) override;
};

}  // namespace IGL

#endif
