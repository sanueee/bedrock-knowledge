#include <stdio.h>
#include <dirent.h>
#include <string.h>

int main(void)
{
    DIR *d1 = opendir("/no/such/path");
    if (d1 == NULL)
    {
        perror("opendir");
    }

    DIR *d2 = opendir("/tmp");
    struct dirent *entry;
    while ((entry = readdir(d2)) != NULL)
    {
        printf("%s\n", entry->d_name);
    }
    closedir(d2);
    return 0;
}