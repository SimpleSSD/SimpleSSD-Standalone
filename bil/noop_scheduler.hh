// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __BIL_NOOP_SCHEDULER__
#define __BIL_NOOP_SCHEDULER__

#include "bil/scheduler.hh"

namespace BIL {

class NoopScheduler : public Scheduler {
 public:
  NoopScheduler(Engine &, DriverInterface *);
  ~NoopScheduler();

  void init();
  void submitIO(BIO &);
};

}  // namespace BIL

#endif
