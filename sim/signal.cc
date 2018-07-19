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

/*
 * This file contains some codes from original gem5 project.
 * See src/sim/backtrace_glib.cc and src/sim/init_signals.cc of gem5.
 * Followings are copyright from gem5.
 */

/*
 * Copyright (c) 2015 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2000-2005 The Regents of The University of Michigan
 * Copyright (c) 2008 The Hewlett-Packard Development Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Andreas Sandberg, Nathan Binkert
 */

#include "sim/signal.hh"

#include <cinttypes>
#include <csignal>
#include <cstring>
#include <iostream>

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

static uint8_t stack[SIGSTKSZ * 2];

static bool setupStack() {
  stack_t st;

  st.ss_sp = stack;
  st.ss_size = sizeof(stack);
  st.ss_flags = 0;

  return sigaltstack(&st, nullptr) == 0;
}

static void raiseSignal(int sig) {
  pthread_kill(pthread_self(), sig);

  std::terminate();
}

static void installHandler(int sig, void (*handler)(int),
                           int flags = SA_RESTART) {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = handler;
  sa.sa_flags = flags;

  sigaction(sig, &sa, nullptr);
}

void print_backtrace() {
  void *buffer[32];
  int size;

  size = backtrace(buffer, sizeof(buffer) / sizeof(*buffer));

  std::cerr << "--- BEGIN LIBC BACKTRACE ---" << std::endl;

  backtrace_symbols_fd(buffer, size, STDERR_FILENO);

  if (size == sizeof(buffer))
    std::cerr << "Warning: Backtrace may have been truncated." << std::endl;

  std::cerr << "--- END LIBC BACKTRACE ---" << std::endl;
}

void abortHandler(int sig) {
  std::cerr << "Program aborted." << std::endl;

  print_backtrace();
  raiseSignal(sig);
}

void segfaultHandler(int sig) {
  std::cerr << "Program has encountered a segmentation fault." << std::endl;

  print_backtrace();
  raiseSignal(sig);
}

void fpeHandler(int sig) {
  std::cerr << "Program has encountered a floating point exception."
            << std::endl;

  print_backtrace();
  raiseSignal(sig);
}

void installSignalHandler(void (*handler)(int)) {
  installHandler(SIGINT, handler);
  installHandler(SIGABRT, abortHandler, SA_RESETHAND | SA_NODEFER);

  setupStack();

  installHandler(SIGSEGV, segfaultHandler,
                 SA_RESETHAND | SA_NODEFER | SA_ONSTACK);
  installHandler(SIGFPE, fpeHandler, SA_RESETHAND | SA_NODEFER | SA_ONSTACK);
}
