// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "bil/noop_scheduler.hh"

namespace BIL {

NoopScheduler::NoopScheduler(Engine &e, DriverInterface *i) : Scheduler(e, i) {}

NoopScheduler::~NoopScheduler() {}

void NoopScheduler::init() {}

void NoopScheduler::submitIO(BIO &bio) {
  pInterface->submitIO(bio);
}

}  // namespace BIL
