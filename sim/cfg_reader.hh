// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __SIM_CONFIG_READER_2__
#define __SIM_CONFIG_READER_2__

#include <cinttypes>
#include <string>

#include "igl/request/request_config.hh"
#include "igl/trace/trace_config.hh"
#include "sim/global_config.hh"
#include "simplessd/lib/inih/ini.h"

typedef enum {
  CONFIG_GLOBAL,
  CONFIG_TRACE,
  CONFIG_REQ_GEN,
} CONFIG_SECTION;

class ConfigReader {
 private:
  Config globalConfig;
  IGL::TraceConfig traceConfig;
  IGL::RequestConfig requestConfig;

  static int parserHandler(void *, const char *, const char *, const char *);

 public:
  bool init(std::string);

  int64_t readInt(CONFIG_SECTION, uint32_t);
  uint64_t readUint(CONFIG_SECTION, uint32_t);
  float readFloat(CONFIG_SECTION, uint32_t);
  std::string readString(CONFIG_SECTION, uint32_t);
  bool readBoolean(CONFIG_SECTION, uint32_t);
};

#endif
