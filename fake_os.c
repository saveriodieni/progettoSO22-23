#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "fake_os.h"

void FakeOS_init(FakeOS* os,int n) {
  if(n<1){
    printf("ERROR: not acceptable number of cpus\n");
    exit(EXIT_FAILURE);
  }
  os->n_cpus=n;
  os->running=(FakePCB**)malloc(sizeof(FakePCB*)*n);
  for(int i=0;i<n;i++) os->running[i]=0;
  os->current_time=(int*)malloc(sizeof(int)*n);;
  for(int i=0;i<n;i++) os->current_time[i]=0;
  List_init(&os->ready);
  List_init(&os->waiting);
  List_init(&os->processes);
  os->timer=0;
  os->schedule_fn=0;
}

void FakeOS_createProcess(FakeOS* os, FakeProcess* p) {
  // sanity check
  assert(p->arrival_time==os->timer && "time mismatch in creation");
  // we check that in the list of PCBs there is no
  // pcb having the same pid
  for(int i=0;i<os->n_cpus;i++) assert( (!os->running[i] || os->running[i]->pid!=p->pid) && "pid taken");

  ListItem* aux=os->ready.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  aux=os->waiting.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  // all fine, no such pcb exists
  FakePCB* new_pcb=(FakePCB*) malloc(sizeof(FakePCB));
  new_pcb->list.next=new_pcb->list.prev=0;
  new_pcb->pid=p->pid;
  new_pcb->events=p->events;
  new_pcb->prev_q=0;

  assert(new_pcb->events.first && "process without events");

  // depending on the type of the first event
  // we put the process either in ready or in waiting
  ProcessEvent* e=(ProcessEvent*)new_pcb->events.first;
  switch(e->type){
  case CPU:
    List_pushBack(&os->ready, (ListItem*) new_pcb);
    break;
  case IO:
    List_pushBack(&os->waiting, (ListItem*) new_pcb);
    break;
  default:
    assert(0 && "illegal resource");
    ;
  }
}




void FakeOS_simStep(FakeOS* os){
  
  printf("************** TIME: %08d **************\n", os->timer);

  //scan process waiting to be started
  //and create all processes starting now
  ListItem* aux=os->processes.first;
  while (aux){
    FakeProcess* proc=(FakeProcess*)aux;
    FakeProcess* new_process=0;
    if (proc->arrival_time==os->timer){
      new_process=proc;
    }
    aux=aux->next;
    if (new_process) {
      printf("\tcreate pid:%d\n", new_process->pid);
      new_process=(FakeProcess*)List_detach(&os->processes, (ListItem*)new_process);
      FakeOS_createProcess(os, new_process);
      free(new_process);
    }
  }

  // scan waiting list, and put in ready all items whose event terminates
  aux=os->waiting.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    ProcessEvent* e=(ProcessEvent*) pcb->events.first;
    printf("\twaiting pid: %d\n", pcb->pid);
    assert(e->type==IO);
    e->duration--;
    printf("\t\tremaining time:%d\n",e->duration);
    if (e->duration==0){
      printf("\t\tend burst\n");
      List_popFront(&pcb->events);
      free(e);
      List_detach(&os->waiting, (ListItem*)pcb);
      if (! pcb->events.first) {
        // kill process
        printf("\t\tend process\n");
        free(pcb);
      } else {
        //handle next event
        e=(ProcessEvent*) pcb->events.first;
        switch (e->type){
        case CPU:
          printf("\t\tmove to ready\n");
          List_pushBack(&os->ready, (ListItem*) pcb);
          break;
        case IO:
          printf("\t\tmove to waiting\n");
          List_pushBack(&os->waiting, (ListItem*) pcb);
          break;
        }
      }
    }
  }

  

  // decrement the duration of running
  // if event over, destroy event
  // and reschedule process
  // if last event, destroy running
  for(int i=0;i<os->n_cpus;i++){
    FakePCB* running=os->running[i];
    
    int* current_time=os->current_time;
    printf("\trunning pid: %d\n", running?running->pid:-1);
    if (running) {
      ProcessEvent* e=(ProcessEvent*) running->events.first;
      assert(e->type==CPU);
      e->duration--;
      current_time[i]=current_time[i]+1;
      printf("\t\tremaining time:%d\n",e->duration);
      if (e->duration==0){
        printf("\t\tend burst\n");
        if(e->n_iteration==0){ // change the quantum of next process event

          (*os->schedule_fn)(os, os->schedule_args,i);
        }
        current_time[i]=0;
        List_popFront(&running->events);
        free(e);
        if (! running->events.first) {
          printf("\t\tend process\n");
          free(running); // kill process
        } else {
          e=(ProcessEvent*) running->events.first;
          if(e->type==CPU){
            printf("\t\tmove to ready\n");
            List_pushBack(&os->ready, (ListItem*) running);
            //break;
          }
          else if(e->type==IO){
            printf("\t\tmove to waiting\n");
            List_pushBack(&os->waiting, (ListItem*) running);
            //break;
          }
          else{
            printf("Event type not defined\n");
            exit(EXIT_FAILURE);
          }
        }
        os->running[i] = 0;
      }
    }


    // call schedule, if defined

    if (os->schedule_fn && ! os->running[i]){
      (*os->schedule_fn)(os, os->schedule_args,i); 
    }

    // if running not defined and ready queue not empty
    // put the first in ready to run
    if (! os->running[i] && os->ready.first) {
      os->running[i]=(FakePCB*) List_popFront(&os->ready);
      printf("\t\tSCHEDULER FAILED\n");
    }
  }

  ++os->timer;

}

void FakeOS_destroy(FakeOS* os) {
  free(os->running);
  free(os->current_time);
}


void FakeOS_printReadyList(struct FakeOS* os){
  if(os!=NULL){
    printf("\t\tReady List\n");
    ListHead ready=os->ready;
    if(ready.size<=0){
      printf("\t\tLista vuota");
      return;
    }
    else{
      printf("\t\t[\n");
      ListItem* aux=os->ready.first;
      while(aux){
        printf("\t\t\tpid : %d , prev_q : %f\n",((FakePCB*)aux)->pid,((FakePCB*)aux)->prev_q);
        aux=aux->next;
      }
      printf("\t\t]\n");
    }
  }
}