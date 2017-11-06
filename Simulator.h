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
 */

#ifndef __Simulator_h__
#define __Simulator_h__

#include "simplessd/SimpleSSD_types.h"

#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
using namespace std;

/*==============================
    Simulator
==============================*/
class Simulator
{
  public:
    uint64_t TickGlobal;  //system time
    uint64_t TickNextEvent;  //Next Event Time

    Simulator();

    /* Simulator Seq:
       0) TickNextEvent => TickGlobal
       1) Unset Busy Resource
       2) Get Possible Request
       3) Set Busy Resource
       4) Set TickNextEvent

       Event Data:
       Reserver  | Action | CurStatus       | EndTime
       <Address> | <RWE>  | <DMA0/Mem/DMA1> | TickEnd
    */
    uint64_t GetTick();

    void TickGo();

    uint8_t TickReserve(uint64_t nextTick);
};

#endif //__Simulator_h__
