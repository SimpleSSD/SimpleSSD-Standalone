// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "igl/trace/trace_config.hh"

namespace IGL {

const char NAME_FILE[] = "File";
const char NAME_TIMING_MODE[] = "TimingMode";
const char NAME_IO_DEPTH[] = "IODepth";
const char NAME_IO_LIMIT[] = "IOLimit";
const char NAME_LINE_REGEX[] = "Regex";
const char NAME_GROUP_OPERATION[] = "Operation";
const char NAME_GROUP_BYTE_OFFSET[] = "ByteOffset";
const char NAME_GROUP_BYTE_LENGTH[] = "ByteLength";
const char NAME_GROUP_LBA_OFFSET[] = "LBAOffset";
const char NAME_GROUP_LBA_LENGTH[] = "LBALength";
const char NAME_GROUP_SEC[] = "Second";
const char NAME_GROUP_MILI_SEC[] = "Millisecond";
const char NAME_GROUP_MICRO_SEC[] = "Microsecond";
const char NAME_GROUP_NANO_SEC[] = "Nanosecond";
const char NAME_GROUP_PICO_SEC[] = "Picosecond";
const char NAME_LBA_SIZE[] = "LBASize";
const char NAME_USE_HEX[] = "UseHexadecimal";

TraceConfig::TraceConfig() {
  mode = TimingModeType::Synchronoous;
  queueDepth = 1;
  iolimit = 0;
  groupOperation = 0;
  groupByteOffset = 0;
  groupByteLength = 0;
  groupLBAOffset = 0;
  groupLBALength = 0;
  groupSecond = 0;
  groupMiliSecond = 0;
  groupMicroSecond = 0;
  groupNanoSecond = 0;
  groupPicoSecond = 0;
  lbaSize = 512;
  useHexadecimal = false;
}

void TraceConfig::loadFrom(pugi::xml_node &section) {
  for (auto node = section.first_child(); node; node = node.next_sibling()) {
    LOAD_NAME_STRING(node, NAME_FILE, file);
    LOAD_NAME_UINT_TYPE(node, NAME_TIMING_MODE, TimingModeType, mode);
    LOAD_NAME_UINT(node, NAME_IO_DEPTH, queueDepth);
    LOAD_NAME_UINT(node, NAME_IO_LIMIT, iolimit);
    LOAD_NAME_STRING(node, NAME_LINE_REGEX, regex);
    LOAD_NAME_UINT(node, NAME_GROUP_OPERATION, groupOperation);
    LOAD_NAME_UINT(node, NAME_GROUP_BYTE_OFFSET, groupByteOffset);
    LOAD_NAME_UINT(node, NAME_GROUP_BYTE_LENGTH, groupByteLength);
    LOAD_NAME_UINT(node, NAME_GROUP_LBA_OFFSET, groupLBAOffset);
    LOAD_NAME_UINT(node, NAME_GROUP_LBA_LENGTH, groupLBALength);
    LOAD_NAME_UINT(node, NAME_GROUP_SEC, groupSecond);
    LOAD_NAME_UINT(node, NAME_GROUP_MILI_SEC, groupMiliSecond);
    LOAD_NAME_UINT(node, NAME_GROUP_MICRO_SEC, groupMicroSecond);
    LOAD_NAME_UINT(node, NAME_GROUP_NANO_SEC, groupNanoSecond);
    LOAD_NAME_UINT(node, NAME_GROUP_PICO_SEC, groupPicoSecond);
    LOAD_NAME_UINT(node, NAME_LBA_SIZE, lbaSize);
    LOAD_NAME_BOOLEAN(node, NAME_USE_HEX, useHexadecimal);
  }
}

void TraceConfig::storeTo(pugi::xml_node &section) {
  STORE_NAME_STRING(section, NAME_FILE, file);
  STORE_NAME_UINT(section, NAME_TIMING_MODE, mode);
  STORE_NAME_UINT(section, NAME_IO_DEPTH, queueDepth);
  STORE_NAME_UINT(section, NAME_IO_LIMIT, iolimit);
  STORE_NAME_STRING(section, NAME_LINE_REGEX, regex);
  STORE_NAME_UINT(section, NAME_GROUP_OPERATION, groupOperation);
  STORE_NAME_UINT(section, NAME_GROUP_BYTE_OFFSET, groupByteOffset);
  STORE_NAME_UINT(section, NAME_GROUP_BYTE_LENGTH, groupByteLength);
  STORE_NAME_UINT(section, NAME_GROUP_LBA_OFFSET, groupLBAOffset);
  STORE_NAME_UINT(section, NAME_GROUP_LBA_LENGTH, groupLBALength);
  STORE_NAME_UINT(section, NAME_GROUP_SEC, groupSecond);
  STORE_NAME_UINT(section, NAME_GROUP_MILI_SEC, groupMiliSecond);
  STORE_NAME_UINT(section, NAME_GROUP_MICRO_SEC, groupMicroSecond);
  STORE_NAME_UINT(section, NAME_GROUP_NANO_SEC, groupNanoSecond);
  STORE_NAME_UINT(section, NAME_GROUP_PICO_SEC, groupPicoSecond);
  STORE_NAME_UINT(section, NAME_LBA_SIZE, lbaSize);
  STORE_NAME_BOOLEAN(section, NAME_USE_HEX, useHexadecimal);
}

void TraceConfig::update() {
  uint64_t idx = 0;

  // Remove leading spaces
  for (auto c : regex) {
    if (c != ' ') {
      break;
    }

    idx++;
  }

  // Check double quotes
  if (regex[0] == '"') {
    regex = regex.substr(idx + 1);
  }
  else {
    regex = regex.substr(idx);
  }

  // Remove trailing spaces
  while (regex.back() == ' ') {
    regex.pop_back();
  }

  if (regex.back() == '"') {
    regex.pop_back();
  }

  panic_if((uint8_t)mode > (uint8_t)TimingModeType::Strict,
           "Invalid timing mode specified");
}

uint64_t TraceConfig::readUint(uint32_t idx) {
  uint64_t ret = 0;

  switch (idx) {
    case Key::TimingMode:
      ret = (uint64_t)mode;
      break;
    case Key::Depth:
      ret = queueDepth;
      break;
    case Key::Limit:
      ret = iolimit;
      break;
    case Key::GroupOperation:
      ret = groupOperation;
      break;
    case Key::GroupByteOffset:
      ret = groupByteOffset;
      break;
    case Key::GroupByteLength:
      ret = groupByteLength;
      break;
    case Key::GroupLBAOffset:
      ret = groupLBAOffset;
      break;
    case Key::GroupLBALength:
      ret = groupLBALength;
      break;
    case Key::GroupSecond:
      ret = groupSecond;
      break;
    case Key::GroupMiliSecond:
      ret = groupMiliSecond;
      break;
    case Key::GroupMicroSecond:
      ret = groupMicroSecond;
      break;
    case Key::GroupNanoSecond:
      ret = groupNanoSecond;
      break;
    case Key::GroupPicoSecond:
      ret = groupPicoSecond;
      break;
    case Key::LBASize:
      ret = lbaSize;
      break;
  }

  return ret;
}

std::string TraceConfig::readString(uint32_t idx) {
  std::string ret = "";

  switch (idx) {
    case Key::File:
      ret = file;
      break;
    case Key::Regex:
      ret = regex;
      break;
  }

  return ret;
}

bool TraceConfig::readBoolean(uint32_t idx) {
  bool ret = false;

  switch (idx) {
    case Key::UseHexadecimal:
      ret = useHexadecimal;
      break;
  }

  return ret;
}

bool TraceConfig::writeUint(uint32_t idx, uint64_t value) {
  bool ret = true;

  switch (idx) {
    case Key::TimingMode:
      mode = (TimingModeType)value;
      break;
    case Key::Depth:
      queueDepth = value;
      break;
    case Key::Limit:
      iolimit = value;
      break;
    case Key::GroupOperation:
      groupOperation = value;
      break;
    case Key::GroupByteOffset:
      groupByteOffset = value;
      break;
    case Key::GroupByteLength:
      groupByteLength = value;
      break;
    case Key::GroupLBAOffset:
      groupLBAOffset = value;
      break;
    case Key::GroupLBALength:
      groupLBALength = value;
      break;
    case Key::GroupSecond:
      groupSecond = value;
      break;
    case Key::GroupMiliSecond:
      groupMiliSecond = value;
      break;
    case Key::GroupMicroSecond:
      groupMicroSecond = value;
      break;
    case Key::GroupNanoSecond:
      groupNanoSecond = value;
      break;
    case Key::GroupPicoSecond:
      groupPicoSecond = value;
      break;
    case Key::LBASize:
      lbaSize = value;
      break;
    default:
      ret = false;
      break;
  }

  return ret;
}

bool TraceConfig::writeString(uint32_t idx, std::string &value) {
  bool ret = true;

  switch (idx) {
    case Key::File:
      file = value;
      break;
    case Key::Regex:
      regex = value;
      break;
    default:
      ret = false;
      break;
  }

  return ret;
}

bool TraceConfig::writeBoolean(uint32_t idx, bool value) {
  bool ret = true;

  switch (idx) {
    case Key::UseHexadecimal:
      useHexadecimal = value;
      break;
    default:
      ret = false;
      break;
  }

  return ret;
}

}  // namespace IGL
