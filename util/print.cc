// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "util/print.hh"

#include <cmath>

void print(std::ostream &out, std::string str, uint32_t width) {
  if (str.length() >= width) {
    out << str;
  }
  else {
    width -= (uint32_t)str.length();

    str.append(width, ' ');

    out << str;
  }
}

void print(std::ostream &out, double val, uint32_t width) {
  print(out, std::to_string(val), width);
}

void printBandwidth(std::ostream &os, uint64_t bytes, uint64_t period) {
  float bps = (float)bytes * 1000000000000. / period;

  printBandwidth(os, (uint64_t)bps);
}

void printBandwidth(std::ostream &os, uint64_t bps) {
  float bw = (float)bps;
  float digit = log10(bw);

  if (digit < 6.0) {
    os << bw << " B/s";
  }
  else if (digit < 9.0) {
    os << bw / 1000. << " KB/s";
  }
  else if (digit < 12.0) {
    os << std::to_string(bw / 1000000.) << " MB/s";
  }
  else {
    os << std::to_string(bw / 1000000000.) << " GB/s";
  }
}

void printLatency(std::ostream &os, uint64_t lat) {
  float latency = (float)lat;
  float digit = log10(latency);

  if (digit < 6.0) {
    os << latency << " ps";
  }
  else if (digit < 9.0) {
    os << latency / 1000. << " ns";
  }
  else if (digit < 12.0) {
    os << std::to_string(latency / 1000000.) << " us";
  }
  else if (digit < 15.0) {
    latency = (float)(lat / 1000);
    os << std::to_string(latency / 1000000.) << " ms";
  }
  else {
    latency = (float)(lat / 1000000);
    os << std::to_string(latency / 1000000.) << " sec.";
  }
}
