```c

#include "banker.h"
#include "states.h"
#include <stdio.h>
#include "deadlock.h"
#include "pcb.h"
#include "blood.h"

int detect_deadlock(void)
{
    int waiting_count = 0;
    int deadlocked_count = 0;

    printf("\n--- Deadlock Detection ---\n");

    for (int i = 0; i < process_count; i++) {
        if (process_table[i].state != WAITING)
            continue;

        waiting_count++;

        int available = get_blood_units(
            process_table[i].blood_group
        );

        if (available >= 0 &&
            available < process_table[i].units) {
            deadlocked_count++;
            printf("P%d is blocked: needs %d units of %s, "
                   "but only %d available.\n",
                   process_table[i].pid,
                   process_table[i].units,
                   process_table[i].blood_group,
                   available);
        }
    }

    if (waiting_count == 0) {
        printf("No waiting processes.\n");
        return 0;
    }

    if (deadlocked_count > 0) {
        printf("Blocked processes detected. "
               "This alone does NOT prove a deadlock.\n");
        return 0;
    }

    printf("No resource shortage detected for waiting processes.\n");
    return 0;
}

 ```c
 void recover_deadlock(void)
{
    int victim = -1;

    printf("\n--- Deadlock Recovery ---\n");

    for (int i = 0; i < process_count; i++) {
        PCB *p = &process_table[i];

        if (p->state != WAITING)
            continue;

        if (victim == -1 ||
            p->priority > process_table[victim].priority) {
            victim = i;
        }
    }

    if (victim == -1) {
        printf("No waiting process available for recovery.\n");
        return;
    }

    PCB *p = &process_table[victim];
    int held = banker_get_allocation(p->pid);

    printf("Selected victim: P%d (%s)\n", p->pid, p->patient);

    if (held < 0) {
        printf("Recovery aborted: allocation information unavailable.\n");
        return;
    }

    if (held > 0) {
        banker_release(p->pid, p->blood_group, held);
    }

    printf("Victim resources released: %d unit(s).\n", held);
    printf("Process remains WAITING until its request can be retried.\n");
}
```
```