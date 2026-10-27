#ifndef BLOOD_H
#define BLOOD_H

int check_blood(const char *group, int units);
int allocate_blood(const char *group, int units);
void release_blood(const char *group, int units);
void print_inventory(void);

/* Returns the currently available units for a blood group.
   Returns -1 if the blood group is invalid. */
int get_blood_units(const char *group);

#endif