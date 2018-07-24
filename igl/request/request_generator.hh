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

#pragma once

#ifndef __IGL_REQUEST_GENERATOR__
#define __IGL_REQUEST_GENERATOR__

#include "bil/entry.hh"
#include "igl/io_gen.hh"
#include "sim/cfg_reader.hh"
#include "sim/engine.hh"

namespace IGL {

class RequestGenerator : public IOGenerator {
 public:
  RequestGenerator(Engine &, ConfigReader &, BIL::BlockIOEntry &);
  ~RequestGenerator();

  void init(uint64_t, uint32_t) override;
  void begin() override;
};

}  // namespace IGL

#endif
