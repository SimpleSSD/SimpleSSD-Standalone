// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __IGL_TRACE_CONFIG__
#define __IGL_TRACE_CONFIG__

#include "simplessd/sim/base_config.hh"

namespace Standalone::IGL {

class TraceConfig : public SimpleSSD::BaseConfig {
 public:
  enum Key : uint32_t {
    File,
    TimingMode,
    Depth,
    Limit,
    Regex,
    GroupOperation,
    GroupByteOffset,
    GroupByteLength,
    GroupLBAOffset,
    GroupLBALength,
    GroupSecond,
    GroupMiliSecond,
    GroupMicroSecond,
    GroupNanoSecond,
    GroupPicoSecond,
    LBASize,
    UseHexadecimal,
  };

  enum class TimingModeType : uint8_t {
    Synchronoous,
    Aynchronous,
    Strict,
  };

 private:
  std::string file;
  TimingModeType mode;
  uint32_t queueDepth;
  uint64_t iolimit;
  std::string regex;
  uint32_t groupOperation;
  uint32_t groupByteOffset;
  uint32_t groupByteLength;
  uint32_t groupLBAOffset;
  uint32_t groupLBALength;
  uint32_t groupSecond;
  uint32_t groupMiliSecond;
  uint32_t groupMicroSecond;
  uint32_t groupNanoSecond;
  uint32_t groupPicoSecond;
  uint32_t lbaSize;
  bool useHexadecimal;

 public:
  TraceConfig();

  const char *getSectionName() override { return "trace"; }

  void loadFrom(pugi::xml_node &) override;
  void storeTo(pugi::xml_node &) override;
  void update() override;

  uint64_t readUint(uint32_t) override;
  std::string readString(uint32_t) override;
  bool readBoolean(uint32_t) override;
  bool writeUint(uint32_t, uint64_t) override;
  bool writeString(uint32_t, std::string &) override;
  bool writeBoolean(uint32_t, bool) override;
};

}  // namespace Standalone::IGL

#endif
