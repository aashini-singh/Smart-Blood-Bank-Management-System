#include <stdio.h>
#include "blood.h"

int main(void)
{
    printf("=== INITIAL INVENTORY ===\n");
    print_inventory();

    printf("\n=== CHECK BLOOD ===\n");
    printf("A+ x 3: %s\n", check_blood("A+", 3) ? "AVAILABLE" : "UNAVAILABLE");
    printf("O- x 10: %s\n", check_blood("O-", 10) ? "AVAILABLE" : "UNAVAILABLE");

    printf("\n=== ALLOCATE BLOOD ===\n");
    printf("Allocating A+ x 3...\n");
    printf("Result: %s\n", allocate_blood("A+", 3) ? "SUCCESS" : "FAILED");

    printf("\n=== INVENTORY AFTER ALLOCATION ===\n");
    print_inventory();

    printf("\n=== SHORTAGE TEST ===\n");
    printf("Allocating O- x 10...\n");
    printf("Result: %s\n", allocate_blood("O-", 10) ? "SUCCESS" : "RESOURCE_UNAVAILABLE");

    printf("\n=== RELEASE BLOOD ===\n");
    printf("Releasing A+ x 2...\n");
    release_blood("A+", 2);

    printf("\n=== FINAL INVENTORY ===\n");
    print_inventory();

    return 0;
}