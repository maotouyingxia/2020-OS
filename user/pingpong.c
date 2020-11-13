#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char const *argv[])
{
    int pid;
    int parent_fc[2];
    int child_fc[2];
    char buf[100];
    pipe(parent_fc);
    pipe(child_fc);
    pid = fork();
    if (pid > 0)
    {
        write(parent_fc[1], "p", 1);
        close(parent_fc[1]);
        close(parent_fc[0]);
        close(child_fc[1]);
        read(child_fc[0], buf, 1);
        if (*buf == 'c')
        {
            printf("%d: received pong\n", getpid());
            close(child_fc[0]);
        }
        pid = wait();
    }
    else if (pid == 0)
    {
        close(parent_fc[1]);
        read(parent_fc[0], buf, 1);
        if (*buf == 'p')
        {
            close(parent_fc[0]);
            printf("%d: received ping\n", pid);
            write(child_fc[1], "c", 1);
            close(child_fc[1]);
            close(child_fc[0]);
        }
        exit();
    }
    else
    {
        printf("fork error\n");
    }
    exit();
}
