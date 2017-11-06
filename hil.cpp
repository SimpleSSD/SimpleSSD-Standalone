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
 *          Donghyun Gouk <kukdh1@camelab.org>
 */

#include <cinttypes>
#include <string>
#include <vector>
#include <map>

#include "hil.h"
#include "base/types.hh"
#include "simplessd/SimpleSSD_types.h"
#include "simplessd/ftl.hh"
#include "simplessd/Latency.h"
#include "simplessd/LatencySLC.h"
#include "simplessd/LatencyMLC.h"
#include "simplessd/LatencyTLC.h"
#include "simplessd/PAL2.h"
#include "simplessd/PALStatistics.h"

Latency* lat;

HIL::HIL(int dn, int SSDenable, BaseConfig SSDConfig ) : cfg(SSDConfig)
{
  disk_number = dn;

  if(SSDenable != 0){
    switch (cfg.NANDType) {
      case NAND_SLC:  lat = new LatencySLC(cfg.DMAMHz, cfg.SizePage); break;
      case NAND_MLC:  lat = new LatencyMLC(cfg.DMAMHz, cfg.SizePage); break;
      case NAND_TLC:  lat = new LatencyTLC(cfg.DMAMHz, cfg.SizePage); break;
    }

    lbaratio = cfg.SizePage / 512;  // Sector Size
    int sbd = cfg.NumChannel * cfg.NumPackage * cfg.NumDie * cfg.NumPlane;

    param = new Parameter();
    param->over_provide = cfg.FTLOP;
    param->page_per_block = cfg.NumPage * sbd;
    param->physical_block_number = cfg.NumBlock;
    param->logical_block_number = (double)param->physical_block_number * (1 - param->over_provide);
    param->physical_page_number = param->physical_block_number * param->page_per_block;
    param->logical_page_number = param->logical_block_number * param->page_per_block;
    param->warmup = cfg.Warmup;
    param->gc_threshold = cfg.FTLGCThreshold;
    param->page_size = cfg.SizePage / 512;
    param->mapping_K = cfg.FTLMapK;
    param->mapping_N = cfg.FTLMapN;
    param->erase_cycle = cfg.FTLEraseCycle;
    param->page_byte = cfg.SizePage;

    stats = new PALStatistics(&cfg, lat);
    pal2 = new PAL2(stats, &cfg, lat);
    ftl = new FTL(param, pal2);

    ftl->initialize();
  }
  hdd_next_available_time = 0;
  sample_time[0] = sample_time[1] = 0;

  Min_size[0] = Min_size[1] = (unsigned)-1;
  Max_size[0] = Max_size[1] = 0;
  access_count[0] = access_count[1]=0;
  total_volume[0] = total_volume[1] = 0;
  total_time[0] = total_time[1] = 0;
  Min_latency[0] = Min_latency[1] = (unsigned long long)-1;
  Max_latency[0] = Max_latency[1] = 0;
  accumulated_latency[0] = accumulated_latency[1] = 0;
  Min_bandwidth[0] = Min_bandwidth[1] = 1000000000; //give a very big value
  Min_bandwidth_woidle[0] = Min_bandwidth_woidle[1] = 1000000000; //give a very big value
  Min_bandwidth_only[0] = Min_bandwidth_only[1] = 1000000000; //give a very big value
  Max_bandwidth[0] = Max_bandwidth[1] = 0;
  Max_bandwidth_woidle[0] = Max_bandwidth_woidle[1] = 0;
  Max_bandwidth_only[0] = Max_bandwidth_only[1] = 0;
  Min_IOPS[0] = Min_IOPS[1] = 1000000000; //give a very big value
  Min_IOPS_woidle[0] = Min_IOPS_woidle[1] = 1000000000; //give a very big value
  Min_IOPS_only[0] = Min_IOPS_only[1] = 1000000000; //give a very big value
  Max_IOPS[0] = Max_IOPS[1] = 0;
  Max_IOPS_woidle[0] = Max_IOPS_woidle[1] = 0;
  Max_IOPS_only[0] = Max_IOPS_only[1] = 0;
  sample_access_count[0] = sample_access_count[1] = 0;
  sample_total_volume[0] = sample_total_volume[1] = 0;
  //sample_period = 100000000000; //never used then
  last_record = 0;
}
Tick HIL::getSSDBusyTime(){
  return stats->SampledExactBusyTime;
}

void
HIL::setSSD(int SSDparam)
{
  SSD = SSDparam;
}

void
HIL::SSDoperation(Addr address, int pgNo, Tick TransTick, bool writeOp)
{
  Addr sector_address = address / 512; // Address is the byte address, sector size is 512B
  Tick delay;
  uint64_t slpn;
  uint16_t nlp, off;

  slpn = sector_address / lbaratio;
  off = sector_address % lbaratio;
  nlp = (pgNo + off + lbaratio - 1) / lbaratio;

  if(writeOp){
    #if DBG_PRINT_REQUEST
    printf( "HIL: disk[%d] Write operation Tick: %" PRIu64 " Address: %#" PRIx64 " size: %d Bytes: %u\n", disk_number, TransTick, address, pgNo, pgNo*512);
    #endif
    //************************ statistics ************************************************//
    access_count[OPER_WRITE]++; total_volume[OPER_WRITE]+=pgNo*512;
    if (pgNo*512 < Min_size[OPER_WRITE]) Min_size[OPER_WRITE] = pgNo*512;
    if (pgNo*512 > Max_size[OPER_WRITE]) Max_size[OPER_WRITE] = pgNo*512;
    sample_access_count[OPER_WRITE]++; sample_total_volume[OPER_WRITE] += pgNo*512;
    //*************************************************************************************//
    if(SSD != 0){
      delay = ftl->write(slpn, nlp, TransTick);
      setLatency(sector_address, delay);
    } else {
      Tick arrived_time = TransTick;
      Tick waiting_time = (hdd_next_available_time > arrived_time)? hdd_next_available_time - arrived_time : 0;
      Tick service_time = ((uint64_t)3600000000 + ((uint64_t)pgNo * (uint64_t)8)*(uint64_t)3450000);
      setLatency(sector_address, waiting_time + service_time); // changed to 2ms
      hdd_next_available_time = arrived_time + waiting_time + service_time;

      accumulated_latency[OPER_WRITE] += waiting_time + service_time;
      total_time[OPER_WRITE] += service_time;
      sample_time[OPER_WRITE] += service_time;
      if((waiting_time + service_time) < Min_latency[OPER_WRITE]) Min_latency[OPER_WRITE] = waiting_time + service_time;
      if((waiting_time + service_time) > Max_latency[OPER_WRITE]) Max_latency[OPER_WRITE] = waiting_time + service_time;
      //*************************************************************************************//
    }
  } else {
    #if DBG_PRINT_REQUEST
    printf( "HIL: disk[%d] Read operation Tick: %" PRIu64 " Address: %#" PRIx64 " size: %d Bytes: %u\n", disk_number, TransTick, address, pgNo, pgNo*512);
    #endif
    //************************ statistics ************************************************//
    access_count[OPER_READ]++; total_volume[OPER_READ]+=pgNo*512;
    if (pgNo*512 < Min_size[OPER_READ]) Min_size[OPER_READ] = pgNo*512;
    if (pgNo*512 > Max_size[OPER_READ]) Max_size[OPER_READ] = pgNo*512;
    sample_access_count[OPER_READ]++; sample_total_volume[OPER_READ] += pgNo*512;
    //*************************************************************************************//
    if(SSD != 0){
      delay = ftl->read(slpn, nlp, TransTick);
      setLatency(sector_address, delay);
    } else {
      Tick arrived_time = TransTick;
      Tick waiting_time = (hdd_next_available_time > arrived_time)? hdd_next_available_time - arrived_time : 0;
      Tick service_time = ((uint64_t)3600000000 + ((uint64_t)pgNo * (uint64_t)8)*(uint64_t)3450000);
      setLatency(sector_address, waiting_time + service_time); // changed to 2ms
      hdd_next_available_time = arrived_time + waiting_time + service_time;

      accumulated_latency[OPER_READ] += waiting_time + service_time;
      total_time[OPER_READ] += service_time;
      sample_time[OPER_READ] += service_time;
      if((waiting_time + service_time) < Min_latency[OPER_READ]) Min_latency[OPER_READ] = waiting_time + service_time;
      if((waiting_time + service_time) > Max_latency[OPER_READ]) Max_latency[OPER_READ] = waiting_time + service_time;
      //*************************************************************************************//
    }
  }
  if (SSD == 0){
    //************************ statistics ************************************************//
    if (hdd_next_available_time - last_record > EPOCH_INTERVAL){
      for (unsigned i = 0; i < 2; i++){
        if (1.0 * sample_total_volume[i] / 1024 / 1024 / (hdd_next_available_time - last_record) * 1000000000000 < Min_bandwidth[i])
        Min_bandwidth[i] = 1.0 * sample_total_volume[i] / 1024 / 1024 / (hdd_next_available_time - last_record) * 1000000000000;
        if (1.0 * sample_total_volume[i] / 1024 / 1024 / (hdd_next_available_time - last_record) * 1000000000000 > Max_bandwidth[i])
        Max_bandwidth[i] = 1.0 * sample_total_volume[i] / 1024 / 1024 / (hdd_next_available_time - last_record) * 1000000000000;
        if (1.0 * sample_access_count[i] / (hdd_next_available_time - last_record) * 1000000000000 < Min_IOPS[i])
        Min_IOPS[i] = 1.0 * sample_access_count[i] / (hdd_next_available_time - last_record) * 1000000000000;
        if (1.0 * sample_access_count[i] / (hdd_next_available_time - last_record) * 1000000000000 > Max_IOPS[i])
        Max_IOPS[i] = 1.0 * sample_access_count[i] / (hdd_next_available_time - last_record) * 1000000000000;

        if(sample_time[OPER_READ] + sample_time[OPER_WRITE] > 0){
          if (1.0 * sample_total_volume[i] / 1024 / 1024 / (sample_time[OPER_READ] + sample_time[OPER_WRITE]) * 1000000000000 < Min_bandwidth_woidle[i])
          Min_bandwidth_woidle[i] = 1.0 * sample_total_volume[i] / 1024 / 1024 / (sample_time[OPER_READ] + sample_time[OPER_WRITE]) * 1000000000000;
          if (1.0 * sample_total_volume[i] / 1024 / 1024 / (sample_time[OPER_READ] + sample_time[OPER_WRITE]) * 1000000000000 > Max_bandwidth_woidle[i])
          Max_bandwidth_woidle[i] = 1.0 * sample_total_volume[i] / 1024 / 1024 / (sample_time[OPER_READ] + sample_time[OPER_WRITE]) * 1000000000000;
          if (1.0 * sample_access_count[i] / (sample_time[OPER_READ] + sample_time[OPER_WRITE]) * 1000000000000 < Min_IOPS_woidle[i])
          Min_IOPS_woidle[i] = 1.0 * sample_access_count[i] / (sample_time[OPER_READ] + sample_time[OPER_WRITE]) * 1000000000000;
          if (1.0 * sample_access_count[i] / (sample_time[OPER_READ] + sample_time[OPER_WRITE]) * 1000000000000 > Max_IOPS_woidle[i])
          Max_IOPS_woidle[i] = 1.0 * sample_access_count[i] / (sample_time[OPER_READ] + sample_time[OPER_WRITE]) * 1000000000000;
        }

        if((i==OPER_READ && sample_time[OPER_READ] > 0) || (i==OPER_WRITE && sample_time[OPER_WRITE] > 0)){
          if (1.0 * sample_total_volume[i] / 1024 / 1024 / (sample_time[i]) * 1000000000000 < Min_bandwidth_only[i])
          Min_bandwidth_only[i] = 1.0 * sample_total_volume[i] / 1024 / 1024 / (sample_time[i]) * 1000000000000;
          if (1.0 * sample_total_volume[i] / 1024 / 1024 / (sample_time[i]) * 1000000000000 > Max_bandwidth_only[i])
          Max_bandwidth_only[i] = 1.0 * sample_total_volume[i] / 1024 / 1024 / (sample_time[i]) * 1000000000000;
          if (1.0 * sample_access_count[i] / (sample_time[i]) * 1000000000000 < Min_IOPS_only[i])
          Min_IOPS_only[i] = 1.0 * sample_access_count[i] / (sample_time[i]) * 1000000000000;
          if (1.0 * sample_access_count[i] / (sample_time[i]) * 1000000000000 > Max_IOPS_only[i])
          Max_IOPS_only[i] = 1.0 * sample_access_count[i] / (sample_time[i]) * 1000000000000;
        }
      }

      sample_access_count[OPER_READ] = 0; sample_total_volume[OPER_READ] = 0;
      sample_access_count[OPER_WRITE] = 0; sample_total_volume[OPER_WRITE] = 0;
      sample_time[OPER_READ] = 0; sample_time[OPER_WRITE] = 0;
      last_record = hdd_next_available_time;
    }
  }
}

void HIL::SyncOperation(Addr prev_address, Tick prev_Tick, Addr address, int pgNo, bool writeOp){
  Tick delay = getLatency(prev_address);
  currentTick = prev_Tick + delay;
  SSDoperation(address, pgNo, currentTick, writeOp);
}

void HIL::updateFinishTick(Addr prev_address) {
  Tick delay = getLatency(prev_address);
  if (finishTick < currentTick + delay ) {
    finishTick = currentTick + delay;
  }
}

Tick HIL::updateDelay(Addr prev_address) {
  return getLatency(prev_address);
}

void
HIL::setLatency(Addr sector_address, Tick delay)
{
  Addr address = sector_address * 512; // Each sector is 512B
  std::map<Addr, Tick>::iterator iDelay = delayMap.find(address);
  if(iDelay == delayMap.end()){
    delayMap.insert(std::pair<Addr, Tick>(address, delay));
    #if DBG_PRINT_REQUEST
    printf( "HIL: Delay duration: %" PRIu64 " |  Address: %#" PRIx64 "\n", delay, address);
    #endif
  }else {
    iDelay->second += delay;
    #if DBG_PRINT_REQUEST
    printf( "HIL: Delay duration: %" PRIu64 " |  Address: %#" PRIx64 "\n", delay, address);
    #endif
  }
  dlMap[address] = delay;
}

Tick HIL::getLatency(Addr address){
  std::map<Addr, Tick>::iterator iDelay = dlMap.find(address);
  assert(iDelay != dlMap.end());
  return iDelay->second;
}

void HIL::print_sample(Tick& sampled_period){
  if (currentTick >= sampled_period) {
    printStats();
    sampled_period = sampled_period + EPOCH_INTERVAL;
  }
}



void
HIL::printStats()
{
  if(SSD != 0){
    pal2->InquireBusyTime( sampled_period );
    stats->PrintStats( sampled_period );
    ftl->PrintStats( sampled_period );
  }
  else{
    printf( "*********************Info of Access Capacity*********************\n");
    printf( "HDD OPER, AVERAGE(B), COUNT, TOTAL(B), MIN(B), MAX(B)\n");
    if (access_count[OPER_READ])
    printf( "HDD %s, %.4lf, %llu, %llu, %u, %u\n",
    operation[OPER_READ], total_volume[OPER_READ]*1.0/access_count[OPER_READ], access_count[OPER_READ], total_volume[OPER_READ],Min_size[OPER_READ], Max_size[OPER_READ]);
    if (access_count[OPER_WRITE])
    printf( "HDD %s, %.4lf, %llu, %llu, %u, %u\n",
    operation[OPER_WRITE], total_volume[OPER_WRITE]*1.0/access_count[OPER_WRITE], access_count[OPER_WRITE], total_volume[OPER_WRITE],Min_size[OPER_WRITE], Max_size[OPER_WRITE]);
    printf( "HDD: Total execution time (ms), Total HDD active time (ms)\n");
    printf( "HDD: %.4f\t\t\t, %.4f\n", hdd_next_available_time * 1.0 / 1000000000, (total_time[OPER_READ]+total_time[OPER_WRITE]) * 1.0 / 1000000000);
    if (total_time[OPER_READ])
    printf( "HDD read bandwidth including idle time (min, max, average): (%.4lf, %.4lf, %.4lf) MB/s\n", Min_bandwidth[OPER_READ], Max_bandwidth[OPER_READ], total_volume[OPER_READ] * 1.0 / 1024 / 1024 *1000 * 1000 * 1000 * 1000 / hdd_next_available_time);
    if(total_time[OPER_WRITE])
    printf( "HDD write bandwidth including idle time (min, max, average): (%.4lf, %.4lf, %.4lf) MB/s\n", Min_bandwidth[OPER_WRITE], Max_bandwidth[OPER_WRITE], total_volume[OPER_WRITE] * 1.0 / 1024 / 1024 *1000 * 1000 * 1000 * 1000 / hdd_next_available_time);
    if (total_time[OPER_READ])
    printf( "HDD read bandwidth excluding idle time (min, max, average): (%.4lf, %.4lf, %.4lf) MB/s\n", Min_bandwidth_woidle[OPER_READ], Max_bandwidth_woidle[OPER_READ], total_volume[OPER_READ] * 1.0 / 1024 / 1024 *1000 * 1000 * 1000 * 1000 / (total_time[OPER_READ]+total_time[OPER_WRITE]));
    if(total_time[OPER_WRITE])
    printf( "HDD write bandwidth excluding idle time (min, max, average): (%.4lf, %.4lf, %.4lf) MB/s\n", Min_bandwidth_woidle[OPER_WRITE], Max_bandwidth_woidle[OPER_WRITE], total_volume[OPER_WRITE] * 1.0 / 1024 / 1024 *1000 * 1000 * 1000 * 1000 / (total_time[OPER_READ]+total_time[OPER_WRITE]));
    if (total_time[OPER_READ])
    printf( "HDD read-only bandwidth (min, max, average): (%.4lf, %.4lf, %.4lf) MB/s\n", Min_bandwidth_only[OPER_READ], Max_bandwidth_only[OPER_READ], total_volume[OPER_READ] * 1.0 / 1024 / 1024 *1000 * 1000 * 1000 * 1000 / (total_time[OPER_READ]));
    if(total_time[OPER_WRITE])
    printf( "HDD write-only bandwidth (min, max, average): (%.4lf, %.4lf, %.4lf) MB/s\n", Min_bandwidth_only[OPER_WRITE], Max_bandwidth_only[OPER_WRITE], total_volume[OPER_WRITE] * 1.0 / 1024 / 1024 *1000 * 1000 * 1000 * 1000 / (total_time[OPER_WRITE]));

    if (access_count[OPER_READ])
    printf( "HDD read request latency (min, max, average): (%llu, %llu, %llu)us\n",Min_latency[OPER_READ]/1000000, Max_latency[OPER_READ]/1000000,accumulated_latency[OPER_READ]/access_count[OPER_READ]/1000000);
    if (access_count[OPER_WRITE])
    printf( "HDD write request latency (min, max, average): (%llu, %llu, %llu)us\n",Min_latency[OPER_WRITE]/1000000, Max_latency[OPER_WRITE]/1000000,accumulated_latency[OPER_WRITE]/access_count[OPER_WRITE]/1000000);
    if (total_time[OPER_READ])
    printf( "HDD read IOPS including idle time (min, max, average): (%.4lf, %.4lf, %.4lf)\n", Min_IOPS[OPER_READ], Max_IOPS[OPER_READ], access_count[OPER_READ] * 1.0 *1000 * 1000 * 1000 * 1000 / hdd_next_available_time);
    if(total_time[OPER_WRITE])
    printf( "HDD write IOPS including idle time (min, max, average): (%.4lf, %.4lf, %.4lf)\n", Min_IOPS[OPER_WRITE], Max_IOPS[OPER_WRITE], access_count[OPER_WRITE] * 1.0 *1000 * 1000 * 1000 * 1000 / hdd_next_available_time);
    if (total_time[OPER_READ])
    printf( "HDD read IOPS excluding idle time (min, max, average): (%.4lf, %.4lf, %.4lf)\n", Min_IOPS_woidle[OPER_READ], Max_IOPS_woidle[OPER_READ], access_count[OPER_READ] * 1.0 *1000 * 1000 * 1000 * 1000 / (total_time[OPER_READ]+total_time[OPER_WRITE]));
    if(total_time[OPER_WRITE])
    printf( "HDD write IOPS excluding idle time (min, max, average): (%.4lf, %.4lf, %.4lf)\n", Min_IOPS_woidle[OPER_WRITE], Max_IOPS_woidle[OPER_WRITE], access_count[OPER_WRITE] * 1.0 *1000 * 1000 * 1000 * 1000 / (total_time[OPER_READ]+total_time[OPER_WRITE]));
    if (total_time[OPER_READ])
    printf( "HDD read-only IOPS (min, max, average): (%.4lf, %.4lf, %.4lf)\n", Min_IOPS_only[OPER_READ], Max_IOPS_only[OPER_READ], access_count[OPER_READ] * 1.0 *1000 * 1000 * 1000 * 1000 / (total_time[OPER_READ]));
    if(total_time[OPER_WRITE])
    printf( "HDD write-only IOPS (min, max, average): (%.4lf, %.4lf, %.4lf)\n", Min_IOPS_only[OPER_WRITE], Max_IOPS_only[OPER_WRITE], access_count[OPER_WRITE] * 1.0 *1000 * 1000 * 1000 * 1000 / (total_time[OPER_WRITE]));

    printf( "***************************************************************\n");
  }
}

void HIL::get_parameter(enum layer_type layer, struct output_result &output)
{
  #define MIN(a,b) (((a)<(b))?(a):(b))
  #define MAX(a,b) (((a)>(b))?(a):(b))
  printf("finishTick = %" PRIu64 "\n", finishTick);
  if (SSD != 0 ) {
    if (layer == PAL_LAYER){
      while (finishTick >= sampled_period) {
        printStats();
        sampled_period = sampled_period + EPOCH_INTERVAL;
      }

      pal2->InquireBusyTime( finishTick );
      stats->PrintFinalStats( finishTick );
    }
    if (layer == FTL_HOST_LAYER){
      ftl->PrintStats(finishTick);  // collect, and print stats for the last epoch
    }
  }
  switch(layer){
    case HDD_LAYER:
    access_count[OPER_READ] > 0 ?
    output.statistics[RD][CAPACITY][AVG] = total_volume[OPER_READ]*1.0/access_count[OPER_READ] :
    output.statistics[RD][CAPACITY][AVG] = 0;
    output.statistics[RD][CAPACITY][MIN] = Min_size[OPER_READ];
    output.statistics[RD][CAPACITY][MAX] = Max_size[OPER_READ];
    output.statistics[RD][CAPACITY][TOT] = total_volume[OPER_READ];
    output.statistics[RD][CAPACITY][COUNT] = access_count[OPER_READ];
    access_count[OPER_WRITE] > 0?
    output.statistics[WR][CAPACITY][AVG] = total_volume[OPER_WRITE]*1.0/access_count[OPER_WRITE] :
    output.statistics[WR][CAPACITY][AVG] = 0;
    output.statistics[WR][CAPACITY][MIN] = Min_size[OPER_WRITE];
    output.statistics[WR][CAPACITY][MAX] = Max_size[OPER_WRITE];
    output.statistics[WR][CAPACITY][TOT] = total_volume[OPER_WRITE];
    output.statistics[WR][CAPACITY][COUNT] = access_count[OPER_WRITE];
    //HIL layer does not contain erase operation
    (access_count[OPER_READ] + access_count[OPER_WRITE]) > 0 ?
    output.statistics[ALL][CAPACITY][AVG] = (total_volume[OPER_READ] + total_volume[OPER_WRITE])*1.0/(access_count[OPER_READ] + access_count[OPER_WRITE]) :
    output.statistics[ALL][CAPACITY][AVG] = 0;
    output.statistics[ALL][CAPACITY][MIN] = MIN(Min_size[OPER_READ], Min_size[OPER_WRITE]);
    output.statistics[ALL][CAPACITY][MAX] = MAX(Max_size[OPER_READ], Max_size[OPER_WRITE]);
    output.statistics[ALL][CAPACITY][TOT] = total_volume[OPER_READ]+total_volume[OPER_WRITE];
    output.statistics[ALL][CAPACITY][COUNT] = access_count[OPER_READ] + access_count[OPER_WRITE];

    total_time[OPER_READ] > 0 ?
    output.statistics[RD][BANDWIDTH][AVG] = total_volume[OPER_READ] * 1.0 / 1024 / 1024 *1000 * 1000 * 1000 * 1000 / (total_time[OPER_READ]+total_time[OPER_WRITE]):
    output.statistics[RD][BANDWIDTH][AVG] = 0;
    output.statistics[RD][BANDWIDTH][MIN] = Min_bandwidth_woidle[OPER_READ];
    output.statistics[RD][BANDWIDTH][MAX] = Max_bandwidth_woidle[OPER_READ];
    total_time[OPER_WRITE] > 0 ?
    output.statistics[WR][BANDWIDTH][AVG] = total_volume[OPER_WRITE] * 1.0 / 1024 / 1024 *1000 * 1000 * 1000 * 1000 / (total_time[OPER_READ]+total_time[OPER_WRITE]) :
    output.statistics[WR][BANDWIDTH][AVG] = 0;
    output.statistics[WR][BANDWIDTH][MIN] = Min_bandwidth_woidle[OPER_WRITE];
    output.statistics[WR][BANDWIDTH][MAX] = Max_bandwidth_woidle[OPER_WRITE];

    total_time[OPER_READ] > 0 ?
    output.statistics[RD][BANDWIDTH_WIDLE][AVG] = total_volume[OPER_READ] * 1.0 / 1024 / 1024 *1000 * 1000 * 1000 * 1000 / (currentTick):
    output.statistics[RD][BANDWIDTH_WIDLE][AVG] = 0;
    output.statistics[RD][BANDWIDTH_WIDLE][MIN] = Min_bandwidth[OPER_READ];
    output.statistics[RD][BANDWIDTH_WIDLE][MAX] = Max_bandwidth[OPER_READ];
    total_time[OPER_WRITE] > 0 ?
    output.statistics[WR][BANDWIDTH_WIDLE][AVG] = total_volume[OPER_WRITE] * 1.0 / 1024 / 1024 *1000 * 1000 * 1000 * 1000 / (currentTick):
    output.statistics[WR][BANDWIDTH_WIDLE][AVG] = 0;
    output.statistics[WR][BANDWIDTH_WIDLE][MIN] = Min_bandwidth[OPER_WRITE];
    output.statistics[WR][BANDWIDTH_WIDLE][MAX] = Max_bandwidth[OPER_WRITE];

    total_time[OPER_READ] > 0 ?
    output.statistics[RD][BANDWIDTH_OPER][AVG] = total_volume[OPER_READ] * 1.0 / 1024 / 1024 *1000 * 1000 * 1000 * 1000 / total_time[OPER_READ]:
    output.statistics[RD][BANDWIDTH_OPER][AVG] = 0;
    output.statistics[RD][BANDWIDTH_OPER][MIN] = Min_bandwidth_only[OPER_READ];
    output.statistics[RD][BANDWIDTH_OPER][MAX] = Max_bandwidth_only[OPER_READ];
    total_time[OPER_WRITE] > 0 ?
    output.statistics[WR][BANDWIDTH_OPER][AVG] = total_volume[OPER_WRITE] * 1.0 / 1024 / 1024 *1000 * 1000 * 1000 * 1000 / total_time[OPER_WRITE]:
    output.statistics[WR][BANDWIDTH_OPER][AVG] = 0;
    output.statistics[WR][BANDWIDTH_OPER][MIN] = Min_bandwidth_only[OPER_WRITE];
    output.statistics[WR][BANDWIDTH_OPER][MAX] = Max_bandwidth_only[OPER_WRITE];

    access_count[OPER_READ] > 0 ?
    output.statistics[RD][LATENCY][AVG] = accumulated_latency[OPER_READ]/access_count[OPER_READ]/1000000 :
    output.statistics[RD][LATENCY][AVG] = 0;
    output.statistics[RD][LATENCY][MIN] = Min_latency[OPER_READ]/1000000;
    output.statistics[RD][LATENCY][MAX] = Max_latency[OPER_READ]/1000000;
    access_count[OPER_WRITE] > 0 ?
    output.statistics[WR][LATENCY][AVG] = accumulated_latency[OPER_WRITE]/access_count[OPER_WRITE]/1000000 :
    output.statistics[WR][LATENCY][AVG] = 0;
    output.statistics[WR][LATENCY][MIN] = Min_latency[OPER_WRITE]/1000000;
    output.statistics[WR][LATENCY][MAX] = Max_latency[OPER_WRITE]/1000000;

    total_time[OPER_READ] > 0 ?
    output.statistics[RD][IOPS][AVG] = access_count[OPER_READ] * 1.0 *1000 * 1000 * 1000 * 1000 / (total_time[OPER_READ] + total_time[OPER_WRITE]) :
    output.statistics[RD][IOPS][AVG] = 0;
    output.statistics[RD][IOPS][MIN] = Min_IOPS_woidle[OPER_READ];
    output.statistics[RD][IOPS][MAX] = Max_IOPS_woidle[OPER_READ];
    total_time[OPER_WRITE] > 0 ?
    output.statistics[WR][IOPS][AVG] = access_count[OPER_WRITE] * 1.0 *1000 * 1000 * 1000 * 1000 / (total_time[OPER_READ] + total_time[OPER_WRITE]) :
    output.statistics[WR][IOPS][AVG] = 0;
    output.statistics[WR][IOPS][MIN] = Min_IOPS_woidle[OPER_WRITE];
    output.statistics[WR][IOPS][MAX] = Max_IOPS_woidle[OPER_WRITE];

    total_time[OPER_READ] > 0 ?
    output.statistics[RD][IOPS_WIDLE][AVG] = access_count[OPER_READ] * 1.0 *1000 * 1000 * 1000 * 1000 / (currentTick) :
    output.statistics[RD][IOPS_WIDLE][AVG] = 0;
    output.statistics[RD][IOPS_WIDLE][MIN] = Min_IOPS[OPER_READ];
    output.statistics[RD][IOPS_WIDLE][MAX] = Max_IOPS[OPER_READ];
    total_time[OPER_WRITE] > 0 ?
    output.statistics[WR][IOPS_WIDLE][AVG] = access_count[OPER_WRITE] * 1.0 *1000 * 1000 * 1000 * 1000 / (currentTick) :
    output.statistics[WR][IOPS_WIDLE][AVG] = 0;
    output.statistics[WR][IOPS_WIDLE][MIN] = Min_IOPS[OPER_WRITE];
    output.statistics[WR][IOPS_WIDLE][MAX] = Max_IOPS[OPER_WRITE];

    total_time[OPER_READ] > 0 ?
    output.statistics[RD][IOPS_OPER][AVG] = access_count[OPER_READ] * 1.0 *1000 * 1000 * 1000 * 1000 /total_time[OPER_READ] :
    output.statistics[RD][IOPS_OPER][AVG] = 0;
    output.statistics[RD][IOPS_OPER][MIN] = Min_IOPS_only[OPER_READ];
    output.statistics[RD][IOPS_OPER][MAX] = Max_IOPS_only[OPER_READ];
    total_time[OPER_WRITE] > 0 ?
    output.statistics[WR][IOPS_OPER][AVG] = access_count[OPER_WRITE] * 1.0 *1000 * 1000 * 1000 * 1000 / total_time[OPER_WRITE] :
    output.statistics[WR][IOPS_OPER][AVG] = 0;
    output.statistics[WR][IOPS_OPER][MIN] = Min_IOPS_only[OPER_WRITE];
    output.statistics[WR][IOPS_OPER][MAX] = Max_IOPS_only[OPER_WRITE];

    output.statistics[TOT][DEVICE_IDLE][TOT] = (currentTick - total_time[OPER_READ] - total_time[OPER_WRITE]) / 1000 / 1000 / 1000;
    output.statistics[TOT][DEVICE_BUSY][TOT] = (total_time[OPER_READ] + total_time[OPER_WRITE]) / 1000 / 1000 / 1000;

    break;

    case FTL_HOST_LAYER:
    output.statistics[RD][CAPACITY][TOT] = ftl->ftl_statistics.host_sim_read_capacity;
    output.statistics[RD][CAPACITY][COUNT] = ftl->ftl_statistics.host_sim_read_count;

    output.statistics[WR][CAPACITY][TOT] = ftl->ftl_statistics.host_sim_write_capacity;
    output.statistics[WR][CAPACITY][COUNT] = ftl->ftl_statistics.host_sim_write_count;

    output.statistics[RD][LATENCY][AVG] = ftl->ftl_statistics.host_sim_read_latency.avg_value;
    ftl->ftl_statistics.host_sim_read_latency.min_value == DBL_MAX ?
    output.statistics[RD][LATENCY][MIN] = 0 :
    output.statistics[RD][LATENCY][MIN] = ftl->ftl_statistics.host_sim_read_latency.min_value;
    output.statistics[RD][LATENCY][MAX] = ftl->ftl_statistics.host_sim_read_latency.max_value;

    output.statistics[WR][LATENCY][AVG] = ftl->ftl_statistics.host_sim_write_latency.avg_value;
    ftl->ftl_statistics.host_sim_write_latency.min_value == DBL_MAX ?
    output.statistics[WR][LATENCY][MIN] = 0 :
    output.statistics[WR][LATENCY][MIN] = ftl->ftl_statistics.host_sim_write_latency.min_value;
    output.statistics[WR][LATENCY][MAX] = ftl->ftl_statistics.host_sim_write_latency.max_value;

    output.statistics[RD][BANDWIDTH_WIDLE][AVG] = ftl->ftl_statistics.host_sim_read_BW_total.Get();
    output.statistics[RD][BANDWIDTH][AVG] = ftl->ftl_statistics.host_sim_read_BW_active.Get();
    output.statistics[RD][BANDWIDTH_OPER][AVG] = ftl->ftl_statistics.host_sim_read_BW_only.Get();
    output.statistics[WR][BANDWIDTH_WIDLE][AVG] = ftl->ftl_statistics.host_sim_write_BW_total.Get();
    output.statistics[WR][BANDWIDTH][AVG] = ftl->ftl_statistics.host_sim_write_BW_active.Get();
    output.statistics[WR][BANDWIDTH_OPER][AVG] = ftl->ftl_statistics.host_sim_write_BW_only.Get();
    output.statistics[TOT][BANDWIDTH_WIDLE][AVG] = ftl->ftl_statistics.host_sim_rw_BW_total.Get();
    output.statistics[TOT][BANDWIDTH][AVG] = ftl->ftl_statistics.host_sim_rw_BW_active.Get();

    output.statistics[RD][IOPS_WIDLE][AVG] = ftl->ftl_statistics.host_sim_read_IOPS_total.Get();
    output.statistics[WR][IOPS_WIDLE][AVG] = ftl->ftl_statistics.host_sim_write_IOPS_total.Get();
    output.statistics[TOT][IOPS_WIDLE][AVG] = ftl->ftl_statistics.host_sim_rw_IOPS_total.Get();
    output.statistics[RD][IOPS][AVG] = ftl->ftl_statistics.host_sim_read_IOPS_active.Get();
    output.statistics[WR][IOPS][AVG] = ftl->ftl_statistics.host_sim_write_IOPS_active.Get();
    output.statistics[TOT][IOPS][AVG] = ftl->ftl_statistics.host_sim_rw_IOPS_active.Get();
    output.statistics[RD][IOPS_OPER][AVG] = ftl->ftl_statistics.host_sim_read_IOPS_only.Get();
    output.statistics[WR][IOPS_OPER][AVG] = ftl->ftl_statistics.host_sim_write_IOPS_only.Get();

    break;
    case FTL_MAP_LAYER:

    output.statistics[GC][CAPACITY][COUNT] = ftl->FTLmapping->map_total_gc_count;  // FIXME
    output.statistics[ER][CAPACITY][COUNT] = ftl->FTLmapping->map_block_erase_count; // FIXME
    // BAD BLOCK COUNT
    output.statistics[GC][LATENCY][AVG] = ftl->FTLmapping->map_gc_lat_avg;
    output.statistics[GC][LATENCY][MAX] = ftl->FTLmapping->map_gc_lat_max;
    ftl->FTLmapping->map_gc_lat_min == DBL_MAX ?
    output.statistics[GC][LATENCY][MIN] = 0 :
    output.statistics[GC][LATENCY][MIN] = ftl->FTLmapping->map_gc_lat_min;

    break;

    case PAL_LAYER:
    for(unsigned i = 0; i < 4; i++){
      output.statistics[i][CAPACITY][AVG] = stats->Access_Capacity.vals[i].avg();
      stats->Access_Capacity.vals[i].minval == 0xFFFFFFFFFFFFFFFF ?
      output.statistics[i][CAPACITY][MIN] = 0 :
      output.statistics[i][CAPACITY][MIN] = stats->Access_Capacity.vals[i].minval;
      output.statistics[i][CAPACITY][MAX] = stats->Access_Capacity.vals[i].maxval;
      output.statistics[i][CAPACITY][TOT] = stats->Access_Capacity.vals[i].sum;
      output.statistics[i][CAPACITY][COUNT] = stats->Access_Capacity.vals[i].cnt;

      output.statistics[i][LATENCY][AVG] = stats->Ticks_Total.vals[i].avg();
      stats->Ticks_Total.vals[i].minval == 0xFFFFFFFFFFFFFFFF ?
      output.statistics[i][LATENCY][MIN] = 0 :
      output.statistics[i][LATENCY][MIN] = stats->Ticks_Total.vals[i].minval;
      output.statistics[i][LATENCY][MAX] = stats->Ticks_Total.vals[i].maxval;

      output.statistics[i][BANDWIDTH][AVG] = (stats->Access_Capacity.vals[i].sum)*1.0/MBYTE/((stats->SampledExactBusyTime)*1.0/PSEC);
      output.statistics[i][BANDWIDTH][MIN] = stats->Access_Bandwidth.vals[i].minval;
      output.statistics[i][BANDWIDTH][MAX] = stats->Access_Bandwidth.vals[i].maxval;

      output.statistics[i][BANDWIDTH_WIDLE][AVG] = (stats->Access_Capacity.vals[i].sum)*1.0/MBYTE/((currentTick-stats->sim_start_time_ps)*1.0/PSEC);
      output.statistics[i][BANDWIDTH_WIDLE][MIN] = stats->Access_Bandwidth_widle.vals[i].minval;
      output.statistics[i][BANDWIDTH_WIDLE][MAX] = stats->Access_Bandwidth_widle.vals[i].maxval;

      if (i < 3){
        output.statistics[i][BANDWIDTH_OPER][AVG] = (stats->Access_Capacity.vals[i].sum)*1.0/MBYTE/((stats->OpBusyTime[i])*1.0/PSEC);
        output.statistics[i][BANDWIDTH_OPER][MIN] = stats->Access_Oper_Bandwidth.vals[i].minval;
        output.statistics[i][BANDWIDTH_OPER][MAX] = stats->Access_Oper_Bandwidth.vals[i].maxval;
      }
      output.statistics[i][IOPS][AVG] = (stats->Access_Capacity.vals[i].cnt)*1.0/((stats->SampledExactBusyTime)*1.0/PSEC);
      output.statistics[i][IOPS][MIN] = stats->Access_Iops.vals[i].minval;
      output.statistics[i][IOPS][MAX] = stats->Access_Iops.vals[i].maxval;

      output.statistics[i][IOPS_WIDLE][AVG] = (stats->Access_Capacity.vals[i].cnt)*1.0/((currentTick-stats->sim_start_time_ps)*1.0/PSEC);
      output.statistics[i][IOPS_WIDLE][MIN] = stats->Access_Iops_widle.vals[i].minval;
      output.statistics[i][IOPS_WIDLE][MAX] = stats->Access_Iops_widle.vals[i].maxval;

      if (i < 3){
        output.statistics[i][IOPS_OPER][AVG] = (stats->Access_Capacity.vals[i].cnt)*1.0/((stats->OpBusyTime[i])*1.0/PSEC);
        output.statistics[i][IOPS_OPER][MIN] = stats->Access_Oper_Iops.vals[i].minval;
        output.statistics[i][IOPS_OPER][MAX] = stats->Access_Oper_Iops.vals[i].maxval;
      }
    }
    output.statistics[TOT][DEVICE_IDLE][TOT] = (finishTick + 1 - stats->SampledExactBusyTime) *1.0 / 1000 / 1000 / 1000;
    output.statistics[TOT][DEVICE_BUSY][TOT] = (stats->SampledExactBusyTime) *1.0 / 1000 / 1000 / 1000;
    break;
    default:
    printf("input layer_type is not correct!\n");
    abort();
  }
}

Tick HIL::getMinNANDLatency(){
  Tick min_latency = 0xFFFFFFFFFFFFFFFF;
  for (unsigned i = 0; i < 3; i++){
    if (stats->Ticks_TotalOpti.vals[i].minval != 0xFFFFFFFFFFFFFFFF){
      if (min_latency == 0xFFFFFFFFFFFFFFFF) min_latency = stats->Ticks_TotalOpti.vals[i].minval;
      else if (min_latency > stats->Ticks_TotalOpti.vals[i].minval) min_latency = stats->Ticks_TotalOpti.vals[i].minval;
    }
  }
  return min_latency;
}
HIL::~HIL()
{
  delete pal2;
  delete stats;
  delete ftl;
  delete param;
}
