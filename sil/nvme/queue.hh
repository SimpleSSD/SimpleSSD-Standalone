// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __DRIVERS_NVME_QUEUE__
#define __DRIVERS_NVME_QUEUE__

#include <cinttypes>

namespace Standalone::SIL::NVMe {

class Queue {
 private:
  uint8_t *memory;
  uint64_t capacity;

  uint16_t stride;
  uint16_t entries;
  uint16_t head;
  uint16_t tail;

 public:
  Queue(uint16_t, uint16_t);
  ~Queue();

  void getBaseAddress(uint64_t &);
  void getData(uint8_t *, uint16_t);  // Increase head
  void setData(uint8_t *, uint16_t);  // Increase tail
  uint16_t getHead();
  uint16_t getTail();
  void incrHead();
  void incrTail();
  void peekData(uint8_t *, uint16_t);  // Get data without increasing head
};

}  // namespace Standalone::SIL::NVMe

#endif
