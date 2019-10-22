// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "bil/noop_scheduler.hh"

namespace Standalone::BIL {

NoopScheduler::NoopScheduler(ObjectData &o, DriverInterface *i)
    : Scheduler(o, i) {}

NoopScheduler::~NoopScheduler() {}

void NoopScheduler::init() {}

void NoopScheduler::submitIO(BIO &bio) {
  pInterface->submitIO(bio);
}

}  // namespace Standalone::BIL
