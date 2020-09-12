// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "igl/request/request_config.hh"

namespace Standalone::IGL {

const char NAME_IO_SIZE[] = "io_size";
const char NAME_IO_TYPE[] = "readwrite";
const char NAME_IO_MIX_RATIO[] = "rwmixread";
const char NAME_BLOCK_SIZE[] = "blocksize";
const char NAME_BLOCK_ALIGN[] = "blockalign";
const char NAME_IO_MODE[] = "iomode";
const char NAME_IO_DEPTH[] = "iodepth";
const char NAME_OFFSET[] = "offset";
const char NAME_SIZE[] = "size";
const char NAME_THINKTIME[] = "thinktime";
const char NAME_RANDOM_SEED[] = "randseed";
const char NAME_TIME_BASED[] = "time_based";
const char NAME_RUN_TIME[] = "runtime";

RequestConfig::RequestConfig() {
  io_size = 0;
  type = IOType::Read;
  rwmixread = 0;
  blocksize = 0;
  blockalign = 0;
  mode = IOMode::Synchronous;
  iodepth = 0;
  offset = 0;
  size = 0;
  thinktime = 0;
  randseed = 0;
  time_based = false;
  runtime = 0;
}

void RequestConfig::loadFrom(pugi::xml_node &section) noexcept {
  for (auto node = section.first_child(); node; node = node.next_sibling()) {
    LOAD_NAME_UINT(node, NAME_IO_SIZE, io_size);
    LOAD_NAME_STRING(node, NAME_IO_TYPE, _type);
    LOAD_NAME_FLOAT(node, NAME_IO_MIX_RATIO, rwmixread);
    LOAD_NAME_UINT(node, NAME_BLOCK_SIZE, blocksize);
    LOAD_NAME_UINT(node, NAME_BLOCK_ALIGN, blockalign);
    LOAD_NAME_STRING(node, NAME_IO_MODE, _mode);
    LOAD_NAME_UINT(node, NAME_IO_DEPTH, iodepth);
    LOAD_NAME_UINT(node, NAME_OFFSET, offset);
    LOAD_NAME_UINT(node, NAME_SIZE, size);
    LOAD_NAME_TIME(node, NAME_THINKTIME, thinktime);
    LOAD_NAME_UINT(node, NAME_RANDOM_SEED, randseed);
    LOAD_NAME_BOOLEAN(node, NAME_TIME_BASED, time_based);
    LOAD_NAME_TIME(node, NAME_RUN_TIME, runtime);
  }
}

void RequestConfig::storeTo(pugi::xml_node &section) noexcept {
  // Re-generate _type and _mode
  switch (type) {
    case IOType::Read:
      _type = "read";
      break;
    case IOType::Write:
      _type = "write";
      break;
    case IOType::RandRead:
      _type = "randread";
      break;
    case IOType::RandWrite:
      _type = "randwrite";
      break;
    case IOType::ReadWrite:
      _type = "readwrite";
      break;
    case IOType::RandRW:
      _type = "randrw";
      break;
    default:
      _type = "?";
      panic_if(true, "Unexpected readwrite.");
      break;
  }

  if (iodepth == 1) {
    _mode = "sync";
  }
  else {
    _mode = "async";
  }

  STORE_NAME_UINT(section, NAME_IO_SIZE, io_size);
  STORE_NAME_STRING(section, NAME_IO_TYPE, _type);
  STORE_NAME_FLOAT(section, NAME_IO_MIX_RATIO, rwmixread);
  STORE_NAME_UINT(section, NAME_BLOCK_SIZE, blocksize);
  STORE_NAME_UINT(section, NAME_BLOCK_ALIGN, blockalign);
  STORE_NAME_STRING(section, NAME_IO_MODE, _mode);
  STORE_NAME_UINT(section, NAME_IO_DEPTH, iodepth);
  STORE_NAME_UINT(section, NAME_OFFSET, offset);
  STORE_NAME_UINT(section, NAME_SIZE, size);
  STORE_NAME_TIME(section, NAME_THINKTIME, thinktime);
  STORE_NAME_UINT(section, NAME_RANDOM_SEED, randseed);
  STORE_NAME_BOOLEAN(section, NAME_TIME_BASED, time_based);
  STORE_NAME_TIME(section, NAME_RUN_TIME, runtime);
}

void RequestConfig::update() noexcept {
  // Convert type
  if (_type.compare("read") == 0) {
    type = IOType::Read;
  }
  else if (_type.compare("write") == 0) {
    type = IOType::Write;
  }
  else if (_type.compare("randread") == 0) {
    type = IOType::RandRead;
  }
  else if (_type.compare("randwrite") == 0) {
    type = IOType::RandWrite;
  }
  else if (_type.compare("readwrite") == 0) {
    type = IOType::ReadWrite;
  }
  else if (_type.compare("randrw") == 0) {
    type = IOType::RandRW;
  }
  else {
    panic_if(true, "Unexpected %s in readwrite.", _type.c_str());
  }

  // Convert mode
  if (_mode.compare("sync") == 0) {
    mode = IOMode::Synchronous;
  }
  else if (_mode.compare("async") == 0) {
    mode = IOMode::Asynchronous;
  }
  else {
    panic_if(true, "Unexpected %s in iomode.", _mode.c_str());
  }

  panic_if(rwmixread < 0 || rwmixread > 1, "Invalid rwmixread.");
}

uint64_t RequestConfig::readUint(uint32_t idx) const noexcept {
  uint64_t ret = 0;

  switch (idx) {
    case Key::Size:
      ret = io_size;
      break;
    case Key::Type:
      ret = (uint64_t)type;
      break;
    case Key::BlockSize:
      ret = blocksize;
      break;
    case Key::BlockAlign:
      ret = blockalign;
      break;
    case Key::Mode:
      ret = (uint64_t)mode;
      break;
    case Key::Depth:
      ret = iodepth;
      break;
    case Key::Offset:
      ret = offset;
      break;
    case Key::Limit:
      ret = size;
      break;
    case Key::ThinkTime:
      ret = thinktime;
      break;
    case Key::RandomSeed:
      ret = randseed;
      break;
    case Key::RunTime:
      ret = runtime;
      break;
  }

  return ret;
}

float RequestConfig::readFloat(uint32_t idx) const noexcept {
  float ret = 0.f;

  switch (idx) {
    case Key::ReadMix:
      ret = rwmixread;
      break;
  }

  return ret;
}

bool RequestConfig::readBoolean(uint32_t idx) const noexcept {
  bool ret = false;

  switch (idx) {
    case Key::TimeBased:
      ret = time_based;
      break;
  }

  return ret;
}

bool RequestConfig::writeUint(uint32_t idx, uint64_t value) noexcept {
  bool ret = true;

  switch (idx) {
    case Key::Size:
      io_size = value;
      break;
    case Key::Type:
      type = (IOType)value;
      break;
    case Key::BlockSize:
      blocksize = value;
      break;
    case Key::BlockAlign:
      blockalign = value;
      break;
    case Key::Mode:
      mode = (IOMode)value;
      break;
    case Key::Depth:
      iodepth = value;
      break;
    case Key::Offset:
      offset = value;
      break;
    case Key::Limit:
      size = value;
      break;
    case Key::ThinkTime:
      thinktime = value;
      break;
    case Key::RandomSeed:
      randseed = value;
      break;
    case Key::RunTime:
      runtime = value;
      break;
    default:
      ret = false;
      break;
  }

  return ret;
}

bool RequestConfig::writeFloat(uint32_t idx, float value) noexcept {
  bool ret = true;

  switch (idx) {
    case Key::ReadMix:
      rwmixread = value;
      break;
    default:
      ret = false;
      break;
  }

  return ret;
}

bool RequestConfig::writeBoolean(uint32_t idx, bool value) noexcept {
  bool ret = true;

  switch (idx) {
    case Key::TimeBased:
      time_based = value;
      break;
    default:
      ret = false;
      break;
  }

  return ret;
}

}  // namespace Standalone::IGL
