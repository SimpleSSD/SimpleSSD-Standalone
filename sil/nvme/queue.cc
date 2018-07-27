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

#include "sil/nvme/queue.hh"

#include <cstdlib>
#include <cstring>

#include "simplessd/sim/trace.hh"

namespace SIL {

namespace NVMe {

Queue::Queue(uint16_t e, uint16_t s)
    : memory(nullptr), entries(e), stride(s), head(0), tail(0) {
  capacity = stride * entries;

  memory = (uint8_t *)aligned_alloc(16, capacity);

  if (memory == nullptr) {
    SimpleSSD::panic("Failed to allocate memory for PRP region");
  }
}

Queue::~Queue() {
  free(memory);
}

void Queue::getBaseAddress(uint64_t &ptr) {
  ptr = (uint64_t)memory;
}

void Queue::getData(uint8_t *buffer, uint16_t length) {
  memcpy(buffer, memory + head * stride, length);

  head++;

  if (head == entries) {
    head = 0;
  }
}

void Queue::setData(uint8_t *buffer, uint16_t length) {
  memcpy(memory + tail * stride, buffer, length);

  tail++;

  if (tail == entries) {
    tail = 0;
  }
}

uint16_t Queue::getHead() {
  return head;
}

uint16_t Queue::getTail() {
  return tail;
}

}  // namespace NVMe

}  // namespace SIL
