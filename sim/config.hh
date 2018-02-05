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

#ifndef __SIM_CONFIG__
#define __SIM_CONFIG__

#include "util/base_config.hh"

namespace SimpleSSD {

typedef enum : uint32_t {
  SIM_OPERATION_MODE,
  SIM_LOG_PERIOD,
} SIM_CONFIG;

typedef enum : int {
  MODE_REQUEST_GENERATOR,
  MODE_TRACE_REPLAYER,
  SIM_MODE_NUM,
} OPERATION_MODE;

class Config : public BaseConfig {
 private:
  OPERATION_MODE mode;  //!< Default: MODE_REQUEST_GENERATOR
  uint64_t logPeriod;   //!< Default: 10000000000 (10ms)

 public:
  Config();

  bool setConfig(const char *, const char *) override;
  void update() override;

  int64_t readInt(uint32_t) override;
  uint64_t readUint(uint32_t) override;
};

}  // namespace SimpleSSD

#endif
