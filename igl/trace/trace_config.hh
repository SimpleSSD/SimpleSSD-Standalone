// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __IGL_TRACE_CONFIG_HH__
#define __IGL_TRACE_CONFIG_HH__

#include "simplessd/sim/base_config.hh"

namespace Standalone::IGL {

class TraceConfig : public SimpleSSD::BaseConfig {
 public:
  enum Key : uint32_t {
    File,
    TimingMode,
    Depth,
    CountLimit,
    SizeLimit,
    WrapTrace,
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
    Synchronous,
    Asynchronous,
    Strict,
  };

 private:
  std::string file;
  TimingModeType mode;
  uint32_t queueDepth;
  uint64_t iocountlimit;
  uint64_t iosizelimit;
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
  bool wrap;

 public:
  TraceConfig();

  const char *getSectionName() noexcept override { return "trace"; }

  void loadFrom(pugi::xml_node &) noexcept override;
  void storeTo(pugi::xml_node &) noexcept override;
  void update() noexcept override;

  uint64_t readUint(uint32_t) const noexcept override;
  std::string readString(uint32_t) const noexcept override;
  bool readBoolean(uint32_t) const noexcept override;
  bool writeUint(uint32_t, uint64_t) noexcept override;
  bool writeString(uint32_t, std::string &) noexcept override;
  bool writeBoolean(uint32_t, bool) noexcept override;
};

}  // namespace Standalone::IGL

#endif
