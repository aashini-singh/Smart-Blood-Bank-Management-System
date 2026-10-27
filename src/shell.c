#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "deadlock.h"
#include "banker.h"
#include "pcb.h"
#include "blood.h"
#include "scheduler.h"

/*
 * process.c will provide this function.
 *
 * If your teammate uses a different function signature,
 * we will change this declaration to match it.
 */
extern int create_process(const char *patient,
                          const char *blood_group,
                          int units,
                          Priority priority);


/* -------------------------------------------------
 * Utility functions
 * ------------------------------------------------- */

static Priority parse_priority(const char *text)
{
    if (strcasecmp(text, "critical") == 0)
        return PRIO_CRITICAL;

    if (strcasecmp(text, "urgent") == 0)
        return PRIO_URGENT;

    return PRIO_NORMAL;
}


static void print_help(void)

{

    printf("\n");
    printf("========== RaktSetu Commands ==========\n");
    printf("help                         Show commands\n");
    printf("request <patient> <group> <units> <priority>\n");
    printf("                             Create blood request\n");
    printf("ps                           Show process table\n");
    printf("blood                        Show blood inventory\n");
    printf("donate <group> <units>       Add donated blood\n");
    printf("schedule <fcfs|priority>     Select scheduler\n");
    printf("deadlock                     Check for blocked processes\n");
printf("recover                      Show recovery status\n");
    printf("run                          Execute READY processes\n");
    printf("exit                         Exit program\n");

    printf("========================================\n");
}


/* -------------------------------------------------
 * Process table
 * ------------------------------------------------- */

static void print_process_table(void)
{
    int i;

    printf("\n================ PROCESS TABLE ================\n");

    if (process_count == 0) {
        printf("No processes created.\n");
        return;
    }

    printf("%-5s %-15s %-6s %-6s %-10s %-12s\n",
           "PID",
           "PATIENT",
           "GROUP",
           "UNITS",
           "PRIORITY",
           "STATE");

    printf("-------------------------------------------------------------\n");

    for (i = 0; i < process_count; i++) {

        PCB *p = &process_table[i];

        printf("%-5d %-15s %-6s %-6d %-10s %-12s\n",
               p->pid,
               p->patient,
               p->blood_group,
               p->units,
               priority_name(p->priority),
               state_name(p->state));
    }

    printf("=============================================================\n");
}


/* -------------------------------------------------
 * REQUEST command
 * ------------------------------------------------- */

static void handle_request(char *args)
{
    char patient[32];
    char group[4];
    char priority_text[16];
    int units;

    /*
     * Expected:
     *
     * request Rahul O- 2 critical
     */

    if (sscanf(args,
               "%31s %3s %d %15s",
               patient,
               group,
               &units,
               priority_text) != 4) {

        printf("Usage:\n");
        printf("request <patient> <group> <units> <priority>\n");
        printf("Example:\n");
        printf("request Rahul O- 2 critical\n");
        return;
    }

    if (units <= 0) {
        printf("Units must be greater than 0.\n");
        return;
    }

    Priority priority = parse_priority(priority_text);

    int pid = create_process(
        patient,
        group,
        units,
        priority
    );

    if (pid < 0) {
        printf("Failed to create process.\n");
        return;
    }

    if (!banker_register_process(pid, group, units)) {
    printf("Warning: Banker registration failed for PID %d.\n", pid);
}

    printf("\nBlood request created successfully.\n");
    printf("PID      : %d\n", pid);
    printf("Patient  : %s\n", patient);
    printf("Group    : %s\n", group);
    printf("Units    : %d\n", units);
    printf("Priority : %s\n", priority_name(priority));
}


```c
static void wake_waiting_processes(const char *group)
{
    for (int i = 0; i < process_count; i++) {
        PCB *p = &process_table[i];

        if (p->state == WAITING &&
            strcmp(p->blood_group, group) == 0 &&
            get_blood_units(group) >= p->units) {

            set_state(p, READY);
            printf("P%d (%s) moved from WAITING to READY.\n",
                   p->pid, p->patient);
        }
    }
}
```
/* -------------------------------------------------
 * DONATE command
 * ------------------------------------------------- */

static void handle_donate(char *args)
{
    char group[4];
    int units;

    if (sscanf(args, "%3s %d", group, &units) != 2 || units <= 0) {
        printf("Usage: donate <group> <units>\n");
        printf("Example: donate O- 5\n");
        return;
    }

    /* Validate the blood group before changing inventory. */
    if (check_blood(group, 0) < 0) {
        printf("Invalid blood group.\n");
        return;
    }

    release_blood(group, units);

    printf("Donation recorded: %d units of %s.\n", units, group);
    wake_waiting_processes(group);
    print_inventory();
}

/* -------------------------------------------------
 * SCHEDULE command
 * ------------------------------------------------- */

static void handle_schedule(char *args)
{
    SchedAlgo algorithm;

    if (scheduler_parse(args, &algorithm) != 0) {
        printf("Unknown scheduling algorithm.\n");
        printf("Available algorithms:\n");
        printf("  fcfs\n");
        printf("  priority\n");
        return;
    }

    scheduler_set(algorithm);

    printf("Selected scheduler: %s\n",
           scheduler_name(algorithm));
}


/* -------------------------------------------------
 * RUN command
 * ------------------------------------------------- */

static void handle_run(void)
{
    printf("\nStarting scheduler...\n");

    int executed = scheduler_run(1);

    printf("\nScheduler finished.\n");
    printf("Processes executed: %d\n", executed);
}


/* -------------------------------------------------
 * Command loop
 * ------------------------------------------------- */

int main(void)
{
    char input[256];

    printf("\n");
    printf("========================================\n");
    printf("          RAKTSETU PROTOTYPE\n");
    printf("       Smart Blood Bank OS Model\n");
    printf("========================================\n");

    printf("\nType 'help' to see available commands.\n");

    while (1) {

        printf("\nraktsetu> ");

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        /* Remove newline */
        input[strcspn(input, "\n")] = '\0';

        /* Ignore empty input */
        if (strlen(input) == 0)
            continue;


        /* ---------------- HELP ---------------- */

        if (strcmp(input, "help") == 0) {

            print_help();
        }


        /* ---------------- PS ---------------- */

        else if (strcmp(input, "ps") == 0) {

            print_process_table();
        }

        else if (strcmp(input, "deadlock") == 0) {
    detect_deadlock();
}
else if (strcmp(input, "recover") == 0) {
    recover_deadlock();
}


        /* ---------------- BLOOD ---------------- */

        else if (strcmp(input, "blood") == 0) {

            printf("\n========== BLOOD INVENTORY ==========\n");
            print_inventory();
            printf("=====================================\n");
        }

        /* ---------------- DONATE ---------------- */

else if (strncmp(input, "donate ", 7) == 0) {
    handle_donate(input + 7);
}


        /* ---------------- RUN ---------------- */

        else if (strcmp(input, "run") == 0) {

            handle_run();
        }


        /* ---------------- EXIT ---------------- */

        else if (strcmp(input, "exit") == 0) {

            printf("Shutting down RaktSetu prototype...\n");
            break;
        }


        /* ---------------- REQUEST ---------------- */

        else if (strncmp(input, "request ", 8) == 0) {

            handle_request(input + 8);
        }


        /* ---------------- SCHEDULE ---------------- */

        else if (strncmp(input, "schedule ", 9) == 0) {

            handle_schedule(input + 9);
        }


        /* ---------------- UNKNOWN ---------------- */

        else {

            printf("Unknown command: %s\n", input);
            printf("Type 'help' for available commands.\n");
        }
    }

    return 0;
}