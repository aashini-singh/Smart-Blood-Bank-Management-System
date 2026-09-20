#include <stdio.h>
#include <string.h>

#include "pcb.h"

/* -------------------------------------------------
 * Process table
 * Owned by Process Manager
 * ------------------------------------------------- */

PCB process_table[MAX_PROCESSES];

int process_count = 0;

/* PID starts from 101 */
static int next_pid = 101;

/* Arrival time for new processes */
static int next_arrival_time = 0;


/* -------------------------------------------------
 * Create a new process
 * ------------------------------------------------- */

int create_process(
    const char *patient,
    const char *blood_group,
    int units,
    Priority priority
)
{
    /* Check if process table is full */
    if (process_count >= MAX_PROCESSES) {
        printf("Process table is full.\n");
        return -1;
    }

    /* Validate input */
    if (patient == NULL ||
        blood_group == NULL ||
        units <= 0) {

        printf("Invalid process information.\n");
        return -1;
    }

    /* Get the next empty PCB */
    PCB *p = &process_table[process_count];

    /* -------------------------------
     * Assign PID
     * ------------------------------- */

    p->pid = next_pid++;


    /* -------------------------------
     * Store patient information
     * ------------------------------- */

    strncpy(
        p->patient,
        patient,
        sizeof(p->patient) - 1
    );

    p->patient[sizeof(p->patient) - 1] = '\0';


    /* -------------------------------
     * Store blood group
     * ------------------------------- */

    strncpy(
        p->blood_group,
        blood_group,
        sizeof(p->blood_group) - 1
    );

    p->blood_group[sizeof(p->blood_group) - 1] = '\0';


    /* -------------------------------
     * Store request information
     * ------------------------------- */

    p->units = units;

    p->priority = priority;


    /* -------------------------------
     * Initial process state
     * ------------------------------- */

    p->state = NEW;


    /* -------------------------------
     * Arrival time
     * ------------------------------- */

    p->arrival_time = next_arrival_time++;


    /* -------------------------------
     * Burst time
     *
     * Burst = units × 3
     * ------------------------------- */

    p->burst_time = units * 3;


    /* -------------------------------
     * Remaining time
     * ------------------------------- */

    p->remaining_time = p->burst_time;


    /* -------------------------------
     * Scheduler fields
     * Initially zero
     * ------------------------------- */

    p->start_time = 0;
    p->completion_time = 0;
    p->waiting_time = 0;
    p->turnaround_time = 0;


    /* -------------------------------
     * Add process to process table
     * ------------------------------- */

    process_count++;


    /* -------------------------------
     * Move NEW → READY
     *
     * Temporary direct assignment.
     * Later this can use set_state()
     * from states.c.
     * ------------------------------- */

    p->state = READY;


    /* -------------------------------
     * Print PCB information
     * ------------------------------- */

    printf("\n========== PROCESS CREATED ==========\n");

    printf("PID          : %d\n", p->pid);
    printf("Patient      : %s\n", p->patient);
    printf("Blood Group  : %s\n", p->blood_group);
    printf("Units        : %d\n", p->units);
    printf("Priority     : %s\n", priority_name(p->priority));
    printf("State        : %s\n", state_name(p->state));
    printf("Arrival Time : %d\n", p->arrival_time);
    printf("Burst Time   : %d\n", p->burst_time);

    printf("=====================================\n");


    return p->pid;
}