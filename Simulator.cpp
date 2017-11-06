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

#include "Simulator.h"

Simulator::Simulator()
: TickGlobal( 0 ), TickNextEvent( 0 )
{
  printf("Init: TickGlobal = %" PRIu64 ", TickNextEvent = %" PRIu64 "\n", TickGlobal, TickNextEvent );
}

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
uint64_t Simulator::GetTick()
{
  return TickGlobal;
}

void Simulator::TickGo()
{
  if (TickGlobal > TickNextEvent)
  {
    printf("TickGlobal = %" PRIu64 ", Requested_TickNextEvent = %" PRIu64 "\n", TickGlobal, TickNextEvent);
    std::cerr<<GetTick()<<": ** CRITICAL ERROR: Tick Error - Trying to go back to past !!\n";
    std::terminate();
  }

  TickGlobal = TickNextEvent;
}

uint8_t Simulator::TickReserve(uint64_t nextTick)
{
  if (TickNextEvent == nextTick)
  {
    return 0;
  }

  if (nextTick < TickGlobal)
  {
    return 1;
  }

  if ( TickNextEvent == TickGlobal || (TickNextEvent > nextTick && nextTick > TickGlobal) )
  {
    TickNextEvent = nextTick;
  }

  return 1;
}
