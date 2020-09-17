// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "driver/nvme/queue.hh"

#include <cstdlib>
#include <cstring>

#include "driver/nvme/prp.hh"

namespace Standalone::Driver::NVMe {

Queue::Queue(uint16_t e, uint16_t s)
    : memory(nullptr), stride(s), entries(e), head(0), tail(0) {
  capacity = stride * entries;

#ifdef _MSC_VER
  memory = (uint8_t *)_aligned_malloc(capacity, PAGE_SIZE);
#else
  memory = (uint8_t *)aligned_alloc(PAGE_SIZE, capacity);
#endif

  memset(memory, 0, capacity);
}

Queue::~Queue() {
#ifdef _MSC_VER
  _aligned_free(memory);
#else
  free(memory);
#endif
}

void Queue::getBaseAddress(uint64_t &ptr) {
  ptr = (uint64_t)memory;
}

void Queue::peekData(uint8_t *buffer, uint16_t length) {
  memcpy(buffer, memory + head * stride, length);
}

void Queue::getData(uint8_t *buffer, uint16_t length) {
  peekData(buffer, length);

  incrHead();
}

void Queue::setData(uint8_t *buffer, uint16_t length) {
  memcpy(memory + tail * stride, buffer, length);

  incrTail();
}

uint16_t Queue::getHead() {
  return head;
}

uint16_t Queue::getTail() {
  return tail;
}

void Queue::incrHead() {
  head++;

  if (head == entries) {
    head = 0;
  }
}

void Queue::incrTail() {
  tail++;

  if (tail == entries) {
    tail = 0;
  }
}

}  // namespace Standalone::Driver::NVMe
