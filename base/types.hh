/**
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
 *
 * Authors: Jie Zhang <jie@camelab.org>
 *          Donghyun Gouk <kukdh1@camelab.org>
 */

#ifndef __TYPES_HH__
#define __TYPES_HH__

#include <inttypes.h>

#include <cassert>
#include <memory>
#include <ostream>
#include <stdexcept>

typedef int64_t Counter;
typedef uint64_t Tick;

extern Tick currentTick;
extern Tick finishTick;
extern Tick sampled_period;

const Tick MaxTick = 0xffffffffffffffffull;

Tick curTick();

typedef uint64_t Addr;

#endif // __TYPES_HH__
