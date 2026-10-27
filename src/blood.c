```c
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "blood.h"

typedef struct {
    const char *group;
    int units;
} BloodStock;

static BloodStock inventory[] = {
    {"A+", 10},
    {"A-", 10},
    {"B+", 10},
    {"B-", 10},
    {"AB+", 10},
    {"AB-", 10},
    {"O+", 10},
    {"O-", 5}
};

#define INVENTORY_SIZE (sizeof(inventory) / sizeof(inventory[0]))

/*
 * Windows mutex protects shared blood inventory.
 * Initialize lazily using InitOnceExecuteOnce.
 */
static HANDLE inventory_mutex = NULL;
static INIT_ONCE inventory_once = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK initialize_mutex(
    PINIT_ONCE once,
    PVOID parameter,
    PVOID *context)
{
    (void)once;
    (void)parameter;
    (void)context;

    inventory_mutex = CreateMutexA(NULL, FALSE, NULL);

    return inventory_mutex != NULL;
}

static int lock_inventory(void)
{
    if (!InitOnceExecuteOnce(
            &inventory_once,
            initialize_mutex,
            NULL,
            NULL)) {
        return 0;
    }

    return WaitForSingleObject(inventory_mutex, INFINITE)
           == WAIT_OBJECT_0;
}

static void unlock_inventory(void)
{
    ReleaseMutex(inventory_mutex);
}

static int find_group(const char *group)
{
    int i;

    if (group == NULL)
        return -1;

    for (i = 0; i < INVENTORY_SIZE; i++) {
        if (strcmp(inventory[i].group, group) == 0)
            return i;
    }

    return -1;
}

int check_blood(const char *group, int units)
{
    int index;
    int available;

    if (units <= 0 || !lock_inventory())
        return 0;

    index = find_group(group);
    available = (index >= 0 && inventory[index].units >= units);

    unlock_inventory();

    return available;
}

int allocate_blood(const char *group, int units)
{
    int index;
    int allocated = 0;

    if (units <= 0 || !lock_inventory())
        return 0;

    index = find_group(group);

    if (index >= 0 && inventory[index].units >= units) {
        inventory[index].units -= units;
        allocated = 1;
    }

    unlock_inventory();

    return allocated;
}

void release_blood(const char *group, int units)
{
    int index;

    if (units <= 0 || !lock_inventory())
        return;

    index = find_group(group);

    if (index >= 0)
        inventory[index].units += units;

    unlock_inventory();
}

void print_inventory(void)
{
    int i;

    if (!lock_inventory()) {
        printf("Unable to lock inventory.\n");
        return;
    }

    printf("%-10s %-10s\n", "GROUP", "UNITS");
    printf("--------------------\n");

    for (i = 0; i < INVENTORY_SIZE; i++) {
        printf("%-10s %-10d\n",
               inventory[i].group,
               inventory[i].units);
    }

    unlock_inventory();
}

int get_blood_units(const char *group)
{
    int units = -1;

    /* Use the same mutex as the other inventory functions. */
    ensure_mutex_initialized();
    WaitForSingleObject(blood_mutex, INFINITE);

    int index = find_group(group);

    if (index >= 0)
        units = inventory[index].units;

    ReleaseMutex(blood_mutex);

    return units;
}
```