// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __DRIVER_NVME_PRP_HH__
#define __DRIVER_NVME_PRP_HH__

#include <cinttypes>
#include <vector>

#define PAGE_SIZE 4096

namespace Standalone::Driver::NVMe {

class PRP {
 private:
  uint8_t *memory;
  uint64_t capacity;

  uint64_t ptr1;
  uint64_t ptr2;

  std::vector<uint64_t> ptrList;  // For faster access

 public:
  PRP(uint64_t);
  ~PRP();

  void getPointer(uint64_t &, uint64_t &);
  void readData(uint64_t, uint64_t, uint8_t *);
  void writeData(uint64_t, uint64_t, uint8_t *);
};

}  // namespace Standalone::Driver::NVMe

#endif
