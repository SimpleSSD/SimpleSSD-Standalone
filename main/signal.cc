// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "main/signal.hh"

#include <cinttypes>
#include <csignal>
#include <cstring>
#include <iostream>

#ifdef _MSC_VER

#pragma comment(lib, "dbghelp.lib")

#include <Windows.h>
// Windows.h should be included before DbgHelp.h
#include <DbgHelp.h>

#ifndef _M_X64
#error "Only x86_64 (AMD64) is supported."
#endif

void (*closeHandler)(int) = nullptr;

void print_backtrace();

BOOL consoleHandler(DWORD type) {
  if (closeHandler) {
    closeHandler(0);
  }

  return TRUE;
}

LONG WINAPI exceptionHandler(LPEXCEPTION_POINTERS pExceptionInfo) {
  switch (pExceptionInfo->ExceptionRecord->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_IN_PAGE_ERROR:
      std::cerr << "Program has encountered a segmentation fault." << std::endl;
      break;
    case EXCEPTION_INT_OVERFLOW:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      std::cerr << "Program has encountered a floating point exception."
                << std::endl;
      break;
    default:
      std::cerr << "Program aborted." << std::endl;
  }

  print_backtrace();

  return EXCEPTION_EXECUTE_HANDLER;
}

#else

#include <cxxabi.h>
#include <dlfcn.h>
#include <signal.h>
#include <unistd.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

static uint8_t stack[SIGSTKSZ];

void print_backtrace();

static bool setupSignalStack() {
  stack_t st;

  st.ss_sp = stack;
  st.ss_size = sizeof(stack);
  st.ss_flags = 0;

  return sigaltstack(&st, nullptr) == 0;
}

static void raiseSignal(int sig) {
  pthread_kill(pthread_self(), sig);

  exit(0);
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

#endif

void print_backtrace() {
  std::cerr << "--- BEGIN BACKTRACE ---" << std::endl;

#ifdef _MSC_VER
  CONTEXT context;
  STACKFRAME64 frame;
  DWORD type;
  HANDLE hProcess = GetCurrentProcess();
  HANDLE hThread = GetCurrentThread();
  char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)];
  PSYMBOL_INFO symInfo = (PSYMBOL_INFO)buffer;
  IMAGEHLP_MODULE modInfo;
  IMAGEHLP_LINE64 lineInfo;
  DWORD64 displacement;
  DWORD dword;

  RtlCaptureContext(&context);

  memset(&frame, 0, sizeof(STACKFRAME64));
  type = IMAGE_FILE_MACHINE_AMD64;
  frame.AddrPC.Offset = context.Rip;
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrFrame.Offset = context.Rsp;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrStack.Offset = context.Rsp;
  frame.AddrStack.Mode = AddrModeFlat;

  symInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
  symInfo->MaxNameLen = MAX_SYM_NAME;
  modInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
  lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

  if (!SymInitialize(hProcess, nullptr, true)) {
    std::cerr << "Failed to initialize symbol table." << std::endl;
    std::cerr << "--- END LIBC BACKTRACE ---" << std::endl;

    return;
  }

  while (true) {
    if (!StackWalk64(type, hProcess, hThread, &frame, &context, nullptr,
                     SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
      break;
    }

    if (frame.AddrPC.Offset != 0) {
      // Get Module Name
      if (SymGetModuleInfo(hProcess, frame.AddrPC.Offset, &modInfo)) {
        std::cerr << modInfo.ModuleName << ": (";
      }
      else {
        std::cerr << "???: (";
      }

      // Get Symbol Name
      if (SymFromAddr(hProcess, frame.AddrPC.Offset, &displacement, symInfo)) {
        std::cerr << symInfo->Name << "+0x" << std::hex << displacement << ") ";
      }

      std::cerr << "[0x" << std::hex << frame.AddrPC.Offset << "] ";

      // Get File Name
      if (SymGetLineFromAddr(hProcess, frame.AddrPC.Offset, &dword,
                             &lineInfo)) {
        std::cerr << std::dec << "\n\t(" << lineInfo.FileName << ":"
                  << lineInfo.LineNumber << ")";
      }

      std::cerr << std::endl;
    }
    else {
      break;
    }
  }

  SymCleanup(hProcess);
  CloseHandle(hThread);
  CloseHandle(hProcess);
#else
  unw_cursor_t cursor;
  unw_context_t context;
  unw_word_t ip, offset;
  char func[256];
  char *cxxname;
  int ret;

  // Initialize libunwind backtrace
  ret = unw_getcontext(&context);

  if (ret == 0) {
    ret = unw_init_local(&cursor, &context);

    if (ret == 0) {
      // For each stack frame
      while (unw_step(&cursor) > 0) {
        cxxname = nullptr;

        // instruction pointer + stack pointer
        unw_get_reg(&cursor, UNW_REG_IP, &ip);

        // function name + CXXAPI function name parse
        if (unw_get_proc_name(&cursor, func, 256, &offset) == 0) {
          cxxname = abi::__cxa_demangle(func, nullptr, nullptr, &ret);

          if (ret != 0) {
            free(cxxname);
            cxxname = nullptr;
          }
        }
        else {
          strcpy(func, "???");
        }

        // Print module name
        std::cerr << "???: (";

        // Print function name
        if (cxxname) {
          std::cerr << cxxname;
        }
        else {
          std::cerr << func;
        }

        // Print offset + instruction pointer
        fprintf(stderr, "+0x%" PRIx64 ") [0x%" PRIx64 "] ", offset, ip);

        // Print filename + line number

        std::cerr << std::endl;
      }
    }
    else {
      std::cerr << "Error: libunwind: unw_init_local() failed (ret = " << ret
                << ")." << std::endl;
    }
  }
  else {
    std::cerr << "Error: libunwind: unw_getcontext() failed (ret = " << ret
              << ")." << std::endl;
  }
#endif

  std::cerr << "--- END BACKTRACE ---" << std::endl;
}

void installSignalHandler(void (*handler)(int)) {
#ifdef _MSC_VER
  closeHandler = handler;
  SetConsoleCtrlHandler((PHANDLER_ROUTINE)consoleHandler, TRUE);
  SetUnhandledExceptionFilter(exceptionHandler);
#else
  installHandler(SIGINT, handler);
  installHandler(SIGABRT, abortHandler, SA_RESETHAND | SA_NODEFER);

  setupSignalStack();

  installHandler(SIGSEGV, segfaultHandler,
                 SA_RESETHAND | SA_NODEFER | SA_ONSTACK);
  installHandler(SIGFPE, fpeHandler, SA_RESETHAND | SA_NODEFER | SA_ONSTACK);
#endif
}
