#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "error: pid required\n");
        return 1;
    }

    char path[128];
    snprintf(path, sizeof(path), "/proc/%s/fd", argv[1]);

    DIR *pid_fd_dir = opendir(path);
    if (pid_fd_dir == NULL)
    {
        fprintf(stderr, "error: opening %s\n", path);
        return 1;
    }

    struct dirent *curr_fd;
    while ((curr_fd = readdir(pid_fd_dir)) != NULL)
    {
        if (strcmp(curr_fd->d_name, ".") == 0 || strcmp(curr_fd->d_name, "..") == 0)
            continue;

        char full_path[128];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, curr_fd->d_name);

        char target[256];
        ssize_t len = readlink(full_path, target, sizeof(target) - 1);
        if (len == -1) continue;
        target[len] = '\0';

        fprintf(stdout, "fd %s -> %s\n", curr_fd->d_name, target);
    }
    closedir(pid_fd_dir);
    return 0;
}