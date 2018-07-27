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

#include "sil/nvme/prp.hh"

#include <cstdlib>

#include "simplessd/sim/trace.hh"
#include "simplessd/util/algorithm.hh"

namespace SIL {

namespace NVMe {

PRP::PRP(uint64_t size) : memory(nullptr), capacity(0), ptr1(0), ptr2(0) {
  uint32_t listCount = 1;
  uint32_t lastEntryCount = 0;
  const uint32_t maxEntryCount = PAGE_SIZE / 8;

  // Calculate size to allocate
  if (size <= PAGE_SIZE) {
    // PRP1 is PRP pointer, PRP2 is not used
    capacity = PAGE_SIZE;
    ptr1 = 1;
  }
  else if (size <= PAGE_SIZE * 2) {
    // Both are PRP pointer
    capacity = PAGE_SIZE * 2;
    ptr1 = 2;
  }
  else {
    // We need to create PRPList
    // PRP1 is PRP pointer (always)
    ptr1 = 3;
    size -= PAGE_SIZE;
    capacity = PAGE_SIZE;

    while (true) {
      if (size > PAGE_SIZE * maxEntryCount) {
        listCount++;

        // Last entry is pointing next PRP list
        size -= PAGE_SIZE * (maxEntryCount - 1);
        capacity += PAGE_SIZE * maxEntryCount;
      }
      else {
        lastEntryCount = DIVCEIL(size, PAGE_SIZE);
        capacity += PAGE_SIZE * lastEntryCount;

        break;
      }
    }
  }

  memory = (uint8_t *)aligned_alloc(16, capacity);

  if (memory == nullptr) {
    SimpleSSD::panic("Failed to allocate memory for PRP region");
  }

  if (ptr1 == 1) {
    ptr1 = (uint64_t)memory;
  }
  else {
    ptr1 = (uint64_t)memory;
    ptr2 = (uint64_t)(memory + PAGE_SIZE);

    if (ptr1 == 3) {
      uint8_t *listPtr = (uint8_t *)ptr2;
      uint8_t *page = memory + PAGE_SIZE * 2;
      uint32_t entry = 0;

      while (true) {
        *((uint64_t *)listPtr + entry++) = (uint64_t)page;
        page += PAGE_SIZE;

        if (listCount == 1 && entry == lastEntryCount) {
          break;
        }
        else if (listCount > 1 && entry == maxEntryCount - 1) {
          *((uint64_t *)listPtr + entry) = (uint64_t)page;
          listPtr = page;
          page += PAGE_SIZE;
          listCount--;
          entry = 0;
        }
      }
    }
  }
}

PRP::~PRP() {
  free(memory);
}

void PRP::getPointer(uint64_t &prp1, uint64_t &prp2) {
  prp1 = ptr1;
  prp2 = ptr2;
}

}  // namespace NVMe

}  // namespace SIL
