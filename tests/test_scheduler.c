#include <stdio.h>
#include "pcb.h"
#include "scheduler.h"

PCB process_table[MAX_PROCESSES];
int process_count = 0;

void set_state(PCB *p, ProcessState new_state)
{
    printf("[P%d] %s -> %s\n",
           p->pid,
           state_name(p->state),
           state_name(new_state));

    p->state = new_state;
}

int main(void)
{
    process_table[0] = (PCB){
        .pid = 101,
        .patient = "Amit",
        .blood_group = "A+",
        .units = 2,
        .priority = PRIO_NORMAL,
        .state = READY,
        .arrival_time = 0,
        .burst_time = 6
    };

    process_table[1] = (PCB){
        .pid = 102,
        .patient = "Karan",
        .blood_group = "O-",
        .units = 10,
        .priority = PRIO_CRITICAL,
        .state = READY,
        .arrival_time = 1,
        .burst_time = 30
    };

    process_count = 2;

    scheduler_set(SCHED_PRIORITY);

    printf("\n=== STARTING TEST ===\n");

    scheduler_run(1);

    printf("\n=== FINAL PROCESS STATES ===\n");

    for (int i = 0; i < process_count; i++) {
        printf("P%d: %s\n",
               process_table[i].pid,
               state_name(process_table[i].state));
    }

    printf("\n=== FINAL BLOOD INVENTORY ===\n");
    return 0;
}