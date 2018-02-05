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

#include "sim/config.hh"

namespace SimpleSSD {

const char NAME_SIM_OPERATION_MODE = "Mode";
const char NAME_SIM_LOG_PERIOD = "LogPeriod";

Config::Config() {
  mode = MODE_REQUEST_GENERATOR;
  logPeriod = 10000000000;
}

bool Config::setConfig(const char *name, const char *value) {
  bool ret = true;

  if (MATCH_NAME(NAME_SIM_OPERATION_MODE)) {
    int tmp = strtoul(value, nullptr, 10);

    if (tmp < SIM_MODE_NUM) {
      mode = (OPERATION_MODE)tmp;
    }
    else {
      ret = false;
    }
  }
  else if (MATCH_NAME(NAME_SIM_LOG_PERIOD)) {
    logPeriod = strtoul(value, nullptr, 10) * 1000000000;
  }
  else {
    ret = false;
  }

  return ret;
}

void Config::update() {
  // DO NOTHING
}

int64_t Config::readInt(uint32_t idx) {
  int64_t ret = 0;

  switch (idx) {
    case SIM_OPERATION_MODE:
      ret = mode;
      break;
  }

  return ret;
}

uint64_t Config::readUint(uint32_t idx) {
  uint64_t ret = 0;

  switch (idx) {
    case SIM_LOG_PERIOD:
      ret = logPeriod;
      break;
  }

  return ret;
}

}  // namespace SimpleSSD
