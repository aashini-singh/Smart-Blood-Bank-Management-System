// 

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "banker.h"
#include "blood.h"
#include "pcb.h"

static int allocation[MAX_PROCESSES] = {0};
static int maximum[MAX_PROCESSES] = {0};
static int registered[MAX_PROCESSES] = {0};

static int find_process_index(int pid)
{
    for (int i = 0; i < process_count; i++) {
        if (process_table[i].pid == pid)
            return i;
    }

    return -1;
}

int banker_register_process(int pid, const char *group, int max_units)
{
    int index = find_process_index(pid);

    if (index < 0 || max_units <= 0 || !group)
        return 0;

    int units = get_blood_units(group);

    if (units < 0)
        return 0;

    maximum[index] = max_units;
    allocation[index] = 0;
    registered[index] = 1;

    return 1;
}

int banker_is_safe(void)
{
    int work[8];
    int finish[MAX_PROCESSES] = {0};

    const char *groups[8] = {
        "A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"
    };

    for (int r = 0; r < 8; r++) {
        work[r] = get_blood_units(groups[r]);

        if (work[r] < 0)
            return 0;
    }

    for (int i = 0; i < process_count; i++) {
        if (!registered[i] ||
            process_table[i].state == TERMINATED) {
            finish[i] = 1;
        }
    }

    int completed = 0;
    int total = 0;

    for (int i = 0; i < process_count; i++) {
        if (!finish[i])
            total++;
    }

    printf("Checking system safety...\n");

    while (completed < total) {
        int progress = 0;

        for (int i = 0; i < process_count; i++) {
            if (finish[i])
                continue;

            int need = maximum[i] - allocation[i];
            int r = -1;

            for (int j = 0; j < 8; j++) {
                if (strcmp(process_table[i].blood_group, groups[j]) == 0) {
                    r = j;
                    break;
                }
            }

            if (r < 0)
                return 0;

            if (need <= work[r]) {
                work[r] += allocation[i];
                finish[i] = 1;
                completed++;
                progress = 1;

                printf("Process P%d can finish.\n",
                       process_table[i].pid);
            }
        }

        if (!progress) {
            printf("System is NOT in a safe state.\n");
            return 0;
        }
    }

    printf("System is in a safe state.\n");
    return 1;
}

int banker_request(int pid, const char *group, int units)
{
    int index = find_process_index(pid);

    if (index < 0 || !registered[index] ||
        !group || units <= 0)
        return 0;

    if (strcmp(process_table[index].blood_group, group) != 0)
        return 0;

    if (allocation[index] + units > maximum[index])
        return 0;

    if (!allocate_blood(group, units))
        return 0;

    allocation[index] += units;

    if (!banker_is_safe()) {
        allocation[index] -= units;
        release_blood(group, units);

        printf("Request denied: unsafe allocation.\n");
        return 0;
    }

    printf("Request approved for P%d.\n", pid);
    return 1;
}

void banker_release(int pid, const char *group, int units)
{
    int index = find_process_index(pid);

    if (index < 0 || !registered[index] ||
        !group || units <= 0)
        return;

    if (strcmp(process_table[index].blood_group, group) != 0)
        return;

    if (units > allocation[index])
        return;

    allocation[index] -= units;
    release_blood(group, units);

    printf("Released %d units for P%d.\n", units, pid);
}

int banker_get_allocation(int pid)
{
    int index = find_process_index(pid);

    if (index < 0 || !registered[index])
        return -1;

    return allocation[index];
}