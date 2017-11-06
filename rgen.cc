/**
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
 *
 * Authors: Jie Zhang <jie@camelab.org>
 *          Miryoung Kwon <mkwon@camelab.org>
 *          Donghyun Gouk <kukdh1@camelab.org>
 */

#include "rgen.hh"

RequestGenerator::RequestGenerator(Config &cfg) {
  cur_page = cfg.MaxRequest;
  MAX_REQ = cfg.MaxRequest;
  cur_REQ = 0;

  start_PPN = cfg.StartPPN;
  REQ_SIZE = cfg.RequestSize;
  curRequest.PPN = 0;
  curRequest.REQ_SIZE = 0;
  curRequest.Oper = OPER_READ;
  curRequest.IOGEN = 0;
  readratio.fraction = cfg.ReadFraction;
  readratio.denominator = cfg.ReadDenominator;
  randomratio.fraction = cfg.RandomFraction;
  randomratio.denominator = cfg.RandomDenominator;
  srand(cfg.RandomSeed);
  IOGEN = cfg.IOGEN;

  if (cfg.Enable) {
    fn = cfg.TraceFile;
    tr = new TraceReader(fn);
  }
  else {
    tr = nullptr;
  }
}

RequestGenerator::~RequestGenerator() {
  if (tr) {
    delete tr;
  }
}

bool RequestGenerator::generate_request(){
  if (!tr) {
    if (cur_REQ >= MAX_REQ) {
      return false;
    }

    if (cur_REQ % randomratio.denominator < randomratio.fraction) { //random access
      int cmd = rand() % 100; //for testing
      curRequest.PPN = (uint64_t)((uint64_t)(rand() % (MAX_REQ * REQ_SIZE / 2)) * (uint64_t)8192);
    }
    else {
      curRequest.PPN = start_PPN; //8192 is default page size
      start_PPN += 4096*REQ_SIZE;
    }

    if (cur_REQ % readratio.denominator < readratio.fraction) {
      curRequest.Oper = OPER_READ;
    }
    else {
      curRequest.Oper = OPER_WRITE;
    }

    curRequest.REQ_SIZE = REQ_SIZE * 8;
    curRequest.IOGEN = IOGEN;
    cur_REQ++;
  }
  else {
    return tr->ReadIO(curRequest.PPN, curRequest.Oper, curRequest.REQ_SIZE);
  }
  return true;
}
