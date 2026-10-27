#ifndef BANKER_H
#define BANKER_H

int banker_register_process(int pid, const char *group, int max_units);
int banker_is_safe(void);
int banker_request(int pid, const char *group, int units);
void banker_release(int pid, const char *group, int units);
int banker_get_allocation(int pid);

#endif  