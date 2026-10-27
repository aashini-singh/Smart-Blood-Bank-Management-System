```c
#include <stdio.h>
#include <windows.h>
#include "blood.h"

#define THREAD_COUNT 2

DWORD WINAPI request_blood(LPVOID arg)
{
    int units = *(int *)arg;

    if (allocate_blood("A+", units)) {
        printf("Thread requested %d units: SUCCESS\n", units);
    } else {
        printf("Thread requested %d units: FAILED (insufficient stock)\n",
               units);
    }

    return 0;
}

int main(void)
{
    HANDLE threads[THREAD_COUNT];
    int requests[THREAD_COUNT] = {7, 5};
    int i;

    printf("=== CONCURRENT BLOOD ALLOCATION TEST ===\n");
    printf("Initial A+ inventory: 10 units\n");
    printf("Two threads request 7 and 5 units simultaneously.\n\n");

    for (i = 0; i < THREAD_COUNT; i++) {
        threads[i] = CreateThread(
            NULL,
            0,
            request_blood,
            &requests[i],
            0,
            NULL
        );

        if (threads[i] == NULL) {
            printf("Failed to create thread %d.\n", i + 1);
            return 1;
        }
    }

    WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, INFINITE);

    for (i = 0; i < THREAD_COUNT; i++) {
        CloseHandle(threads[i]);
    }

    printf("\n=== INVENTORY AFTER CONCURRENT REQUESTS ===\n");
    print_inventory();

    return 0;
}
```
