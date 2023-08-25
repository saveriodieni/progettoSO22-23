#include "fake_process.h"
#include "linked_list.h"
#pragma once


typedef struct {
  ListItem list;
  int pid;
  ListHead events;
  float prev_q;
} FakePCB;

struct FakeOS;
typedef void (*ScheduleFn)(struct FakeOS* os, void* args,int pos);

typedef struct FakeOS{
  FakePCB** running;  //modified, array of FakePCB*
  ListHead ready;
  ListHead waiting;
  int timer;
  ScheduleFn schedule_fn;
  void* schedule_args;

  int* current_time;      
  int n_cpus;

  ListHead processes;
} FakeOS;

void FakeOS_init(FakeOS* os,int n);
void FakeOS_simStep(FakeOS* os);
void FakeOS_destroy(FakeOS* os);

void FakeOS_printReadyList(struct FakeOS* os);