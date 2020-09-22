// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __MAIN_SIGNAL_HH__
#define __MAIN_SIGNAL_HH__

void installSignalHandler(void (*)(int), void (*)(int));
void blockSIGINT();

#endif
