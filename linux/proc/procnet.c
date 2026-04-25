#include <stdio.h>

int main(void)
{
    FILE *f_tcp = fopen("/proc/net/tcp", "r");
    if (f_tcp == NULL)
    {
        fprintf(stderr, "error: opening file /proc/net/tcp\n");
        return 1;
    }
    char line[512];
    while (fgets(line, sizeof(line), f_tcp) != NULL)
    {
        char local_address[64];
        char rem_address[64];
        char state[32];
        if (sscanf(line, "%*s %63s %63s %31s", local_address, rem_address, state) == 3)
        {
            fprintf(stdout, "%s %s %s\n", local_address, rem_address, state);
        }
    }
    fclose(f_tcp);
    return 0;
}