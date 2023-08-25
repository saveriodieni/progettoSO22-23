#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "fake_os.h"

FakeOS os;
int verbose=0;

void modifyPrevQuantum(FakeOS* os,void* args_,int pos);
void schedSJF(FakeOS* os,void* args_,int pos);

typedef struct {
  int quantum;
  float a;  // 0<=a<=1
} SchedSJFArgs;

int isRunning(FakeOS* os){
  for(int i=0;i<os->n_cpus;i++){
    if(os->running[i]!=0) return 1;
  }
  return 0;
}

void modifyPrevQuantum(FakeOS* os,void* args_,int pos){
  
  FakePCB* running=os->running[pos];
  if(!running) return schedSJF(os,args_,pos);
  
  SchedSJFArgs* args=(SchedSJFArgs*)args_;
  running->prev_q=
    (1-(args->a))*running->prev_q+args->a*(((ProcessEvent*)(running->events.first))->paused*args->quantum+os->current_time[pos]);
  printf("\t\tcambiato prev_q del processo %d, %f\n",running->pid,running->prev_q);
  return;
}

void schedSJF(FakeOS* os,void* args_,int pos){
  SchedSJFArgs* args=(SchedSJFArgs*)args_;
  if (! os->ready.first)
    return;
  
  FakePCB* pcb=(FakePCB*)os->ready.first;
  ListItem* aux=(os->ready.first)->next;
  while(aux){
    if(((FakePCB*)aux)->prev_q<pcb->prev_q){
      pcb=(FakePCB*)aux;
    }
    aux=aux->next;
  }

  // stampa la lista dei processi con il loro prev_q

  if(verbose) FakeOS_printReadyList(os);

  List_detach(&os->ready,(ListItem*)pcb);
  os->running[pos]=pcb;

  if(verbose) printf("\t\tprocesso scelto : %d\n",pcb->pid);
  
  assert(pcb->events.first);
  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  assert(e->type==CPU);

  // look at the first event
  // if duration>quantum
  // push front in the list of event a CPU event of duration quantum
  // alter the duration of the old event subtracting quantum
  if (e->duration>args->quantum) {
    ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
    qe->list.prev=qe->list.next=0;
    qe->type=CPU;
    qe->duration=args->quantum;
    e->duration-=args->quantum;
    qe->paused=e->paused;
    qe->n_iteration=e->n_iteration-1;
    List_pushFront(&pcb->events, (ListItem*)qe);
  }

}

int main(int argc, char** argv) {
  if(argc<5){
    printf("usage ./sched_sim <n_cpus> <quantum> <a> <processes> verbose{optional}\n");
    exit(EXIT_FAILURE);
  }

  if(strcmp(argv[argc-1],"verbose")==0){
    verbose=1;
    argc--;
  }

  FakeOS_init(&os,atoi(argv[1]));
  SchedSJFArgs ssjf_args;
  ssjf_args.quantum=atoi(argv[2]);
  if(ssjf_args.quantum<1){
    printf("ERROR: invalid value for quantum");
    exit(EXIT_FAILURE);
  }
  ssjf_args.a=atof(argv[3]);
  if(ssjf_args.a<0 || ssjf_args.a>1){
    printf("ERROR: invalid value for a");
    exit(EXIT_FAILURE);
  }
  os.schedule_args=&ssjf_args;
  os.schedule_fn=modifyPrevQuantum;
  
  for (int i=4; i<argc; ++i){   
    FakeProcess new_process;
    int num_events=FakeProcess_load(&new_process, argv[i]);
    printf("loading [%s], pid: %d, events:%d\n",
          argv[i], new_process.pid, num_events);
    if (num_events) {
      FakeProcess* new_process_ptr=(FakeProcess*)malloc(sizeof(FakeProcess));
      *new_process_ptr=new_process;
      ListItem* aux=(ListItem*)new_process_ptr->events.first;
      int size=new_process_ptr->events.size;
      for(int i=0;i<size;i++){
        ProcessEvent* pe=(ProcessEvent*) aux;
        if(pe->type==CPU) pe->paused=(pe->duration-1)/ssjf_args.quantum;
        aux=aux->next;
      }
      List_pushBack(&os.processes, (ListItem*)new_process_ptr);
    }
  }

  printf("num processes in queue %d\n", os.processes.size);
  printf("num cpus %d\n", os.n_cpus);
  while(isRunning(&os)
        || os.ready.first
        || os.waiting.first
        || os.processes.first){
    FakeOS_simStep(&os);
  }
  FakeOS_destroy(&os);
}
