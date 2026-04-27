#include <stdio.h>
#include <string.h>

void print_converted_address(char *address);
void print_converted_state(char *state);

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
        char local_address[16];
        char rem_address[16];
        char state[16];
        if (sscanf(line, "%*s %15s %15s %15s", local_address, rem_address, state) == 3)
        {
            print_converted_address(local_address);
            fprintf(stdout, "  ");
            print_converted_address(rem_address);
            fprintf(stdout, "  ");
            print_converted_state(state);
            fprintf(stdout, "\n");
        }
    }
    fclose(f_tcp);
    return 0;
}

void print_converted_address(char *address)
{
    unsigned int ip, port;
    sscanf(address, "%X:%X", &ip, &port);

    char buf[32];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d:%d",
    (ip) & 0xFF,
    (ip >> 8)  & 0xFF,
    (ip >> 16) & 0xFF,
    (ip >> 24) & 0xFF,
    port);
    fprintf(stdout, "%-25s", buf);
}

void print_converted_state(char *state)
{
    if (strcmp(state,"01") == 0)
    {
        fprintf(stdout, "ESTABLISHED");
    }
    else if (strcmp(state,"0A") == 0)
    {
        fprintf(stdout, "LISTEN");
    }
    else if (strcmp(state,"06") == 0)
    {
        fprintf(stdout, "TIME_WAIT");
    }
    else { fprintf(stdout, "(%s)", state); }
}