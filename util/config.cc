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

#include "util/config.hh"

#include "util/base_config.hh"
#include "simplessd/lib/ini/ini.h"
#include "simplessd/log/trace.hh"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

namespace SimpleSSD {

const char sectionName[SECTION_NUM][] = {"global", "generator", "trace"};

bool BaseConfig::convertBool(const char *value) {
  bool ret = false;

  if (strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") ||
      strcasecmp(value, "T") || strcasecmp(value, "Y") ||
      strtoul(value, nullptr, 10)) {
    ret = true;
  }

  return ret;
}

uint64_t BaseConfig::convertInt(const char *value) {
  uint64_t multipler = 1;
  std::string str(value);

  switch (str.back()) {
    case 'k':
      str.pop_back();
      multipler = 1000;
      break;
    case 'm':
      str.pop_back();
      multipler = 1000000;
      break;
    case 'g':
      str.pop_back();
      multipler = 1000000000;
      break;
    case 't':
      str.pop_back();
      multipler = 1000000000000;
      break;
    case 'p':
      str.pop_back();
      multipler = 1000000000000000;
      break;
    case 'K':
      str.pop_back();
      multipler = 1024;
      break;
    case 'M':
      str.pop_back();
      multipler = 1048576;
      break;
    case 'G':
      str.pop_back();
      multipler = 1073741824;
      break;
    case 'T':
      str.pop_back();
      multipler = 1099511627776;
      break;
    case 'P':
      str.pop_back();
      multipler = 1125899906842624;
      break;
  }

  if (str.at(0) == '0' && (str.at(1) == 'x' || str.at(1) == 'X')) {
    multipler *= strtoul(str.c_str(), nullptr, 16);
  }
  else {
    multipler *= strtoul(str.c_str(), nullptr, 10);
  }

  return multipler;
}

bool ConfigReader::init(std::string file) {
  if (ini_parse(file.c_str(), parserHandler, this) < 0) {
    return false;
  }

  // Update all

  return true;
}

int ConfigReader::parserHandler(void *context, const char *section,
                                const char *name, const char *value) {
  ConfigReader *pThis = (ConfigReader *)context;
  bool handled = true;

  if (MATCH_SECTION(sectionName[SECTION_GLOBAL])) {
  }
  else if (MATCH_SECTION(sectionName[SECTION_GENERATOR])) {
  }
  else if (MATCH_SECTION(sectionName[SECTION_TRACE])) {
  }

  if (!handled) {
    Logger::warn("Config [%s] %s = %s not handled", section, name, value);
  }

  return 1;
}

}  // namespace SimpleSSD
