/* scheduler.c - CPU Scheduler for RaktSetu Mini OS Prototype
 * Owner: Aditya Upadhyay (Group 2)
 *
 * Algorithms (both non-preemptive in this prototype):
 *   FCFS     : earliest arrival first, ties by PID
 *   PRIORITY : CRITICAL > URGENT > NORMAL, ties by arrival, then PID
 *
 * At every decision point the scheduler only considers processes that
 * have already arrived. If none has arrived, the CPU stays IDLE until
 * the next arrival.
 *
 * Output: ASCII Gantt chart, per-process table (AT, BT, ST, CT, WT, TAT)
 *         and average waiting / turnaround time.
 */
#include <stdio.h>
#include "banker.h"
#include <ctype.h>
#include "pcb.h"
#include "blood.h"
#include "scheduler.h"

#define MAX_SLOTS (MAX_PROCESSES * 2)

typedef struct { int pid; int start; int end; } GanttSlot;   /* pid 0 = IDLE */

static SchedAlgo current_algo = SCHED_FCFS;

/* ---------- configuration ---------- */

void scheduler_set(SchedAlgo algo) {
    current_algo = algo;
    printf("Scheduler: %s\n", scheduler_name(algo));
}

SchedAlgo scheduler_get(void) { return current_algo; }

const char *scheduler_name(SchedAlgo algo) {
    return algo == SCHED_PRIORITY ? "PRIORITY" : "FCFS";
}

static int equals_ignore_case(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

int scheduler_parse(const char *text, SchedAlgo *out) {
    if (!text) return -1;
    if (equals_ignore_case(text, "fcfs"))     { *out = SCHED_FCFS;     return 0; }
    if (equals_ignore_case(text, "priority")) { *out = SCHED_PRIORITY; return 0; }
    return -1;
}

/* ---------- selection ---------- */

/* returns 1 if a should run before b */
static int runs_before(const PCB *a, const PCB *b, SchedAlgo algo) {
    if (algo == SCHED_PRIORITY && a->priority != b->priority)
        return a->priority < b->priority;
    if (a->arrival_time != b->arrival_time)
        return a->arrival_time < b->arrival_time;
    return a->pid < b->pid;
}

/* ---------- output ---------- */

static void print_gantt(const GanttSlot *s, int n) {
    int i;
    printf("\nGantt Chart:\n");
    for (i = 0; i < n; i++) printf("+-------");
    printf("+\n");
    for (i = 0; i < n; i++) {
        if (s[i].pid == 0) printf("| IDLE  ");
        else               printf("| P%-5d", s[i].pid);
    }
    printf("|\n");
    for (i = 0; i < n; i++) printf("+-------");
    printf("+\n");
    for (i = 0; i < n; i++) printf("%-8d", s[i].start);
    printf("%d\n", s[n - 1].end);
}

static void print_table(PCB **done, int n) {
    int i;
    double total_wt = 0, total_tat = 0;
    printf("\nPID   Patient      Group  Units  Priority   AT   BT   ST   CT   WT   TAT\n");
    printf("--------------------------------------------------------------------------\n");
    for (i = 0; i < n; i++) {
        PCB *p = done[i];
        printf("%-5d %-12s %-6s %-6d %-9s %4d %4d %4d %4d %4d %5d\n",
               p->pid, p->patient, p->blood_group, p->units, priority_name(p->priority),
               p->arrival_time, p->burst_time, p->start_time, p->completion_time,
               p->waiting_time, p->turnaround_time);
        total_wt  += p->waiting_time;
        total_tat += p->turnaround_time;
    }
    printf("--------------------------------------------------------------------------\n");
    printf("Average Waiting Time    : %.2f\n", total_wt / n);
    printf("Average Turnaround Time : %.2f\n", total_tat / n);
}

/* ---------- execution ---------- */

int scheduler_run(int commit) {
    PCB  copies[MAX_PROCESSES];
    PCB *cand[MAX_PROCESSES];
    PCB *done[MAX_PROCESSES];
    int  picked[MAX_PROCESSES] = {0};
    GanttSlot slots[MAX_SLOTS];
    int n = 0, n_done = 0, n_slots = 0, n_waiting = 0;
    int i, left, clock;

    /* collect READY processes */
    for (i = 0; i < process_count; i++) {
        if (process_table[i].state != READY) continue;
        if (commit) {
            cand[n] = &process_table[i];
        } else {
            copies[n] = process_table[i];
            cand[n] = &copies[n];
        }
        n++;
    }

    printf("\n========== Scheduler: %s %s==========\n",
           scheduler_name(current_algo), commit ? "" : "(preview) ");
    if (n == 0) {
        printf("No READY processes to schedule.\n");
        return 0;
    }
    if (!commit)
        printf("Preview mode: blood check skipped, process states unchanged.\n");

    /* CPU clock starts at the earliest arrival */
    clock = cand[0]->arrival_time;
    for (i = 1; i < n; i++)
        if (cand[i]->arrival_time < clock) clock = cand[i]->arrival_time;

    left = n;
    while (left > 0) {
        int best = -1;
        PCB *p;

        /* pick best among arrived, not yet picked */
        for (i = 0; i < n; i++) {
            if (picked[i] || cand[i]->arrival_time > clock) continue;
            if (best < 0 || runs_before(cand[i], cand[best], current_algo)) best = i;
        }

        /* nothing arrived yet -> CPU idle until next arrival */
        if (best < 0) {
            int next = -1;
            for (i = 0; i < n; i++)
                if (!picked[i] && (next < 0 || cand[i]->arrival_time < next))
                    next = cand[i]->arrival_time;
            slots[n_slots].pid = 0;
            slots[n_slots].start = clock;
            slots[n_slots].end = next;
            n_slots++;
            clock = next;
            continue;
        }

        p = cand[best];
        picked[best] = 1;
        left--;

        if (commit) {
            set_state(p, RUNNING);
            if (!banker_request(p->pid, p->blood_group, p->units)) {
    set_state(p, WAITING);

    printf("        Reason: RESOURCE_UNAVAILABLE or UNSAFE_REQUEST (%s x %d units)\n",
           p->blood_group, p->units);

    n_waiting++;
    continue;
}

printf("        %d unit(s) of %s allocated to P%d\n",
       p->units, p->blood_group, p->pid);

        //     if (!allocate_blood(p->blood_group, p->units)) {
        //         set_state(p, WAITING);
        //         printf("        Reason: RESOURCE_UNAVAILABLE (%s x %d units)\n",
        //                p->blood_group, p->units);
        //         n_waiting++;
        //         continue;
        //     }
        //     printf("        %d unit(s) of %s allocated to P%d\n",
        //            p->units, p->blood_group, p->pid);
        // }

        p->start_time      = clock;
        clock             += p->burst_time;
        p->remaining_time  = 0;
        p->completion_time = clock;
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time    = p->start_time - p->arrival_time;

        slots[n_slots].pid = p->pid;
        slots[n_slots].start = p->start_time;
        slots[n_slots].end = p->completion_time;
        n_slots++;
        done[n_done++] = p;

        if (commit) {
    banker_release(p->pid, p->blood_group, p->units);
    set_state(p, TERMINATED);
}
        // if (commit) set_state(p, TERMINATED);
    }

    if (n_done > 0) {
        printf("\nExecution Order: ");
        for (i = 0; i < n_done; i++)
            printf("P%d%s", done[i]->pid, i < n_done - 1 ? " -> " : "\n");
        print_gantt(slots, n_slots);
        print_table(done, n_done);
    } else {
        printf("No process could be executed.\n");
    }
    if (n_waiting > 0)
        printf("\n%d process(es) moved to WAITING due to blood shortage.\n", n_waiting);

    return n_done;
}
