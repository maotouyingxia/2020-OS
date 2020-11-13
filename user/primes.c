#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define MAX 35
#define COUNT 11

int main(int argc, char const *argv[])
{
    int pid;
    int prime;
    int pip_id;
    int pip[COUNT][2];
    int buf[100];
    pip_id = 0;
    pipe(pip[pip_id]);
    for (int i = 0; i < MAX - 1; i++)
    {
        *buf = i + 2;
        write(pip[pip_id][1], buf, 4);
    }
    while (pip_id < COUNT)
    {
        pip_id++;
        pipe(pip[pip_id]);
        pid = fork();
        if (pid > 0)
        {
            close(pip[pip_id-1][1]);
            close(pip[pip_id-1][0]);
            pid = wait();
        }
        else if (pid == 0)
        {
            close(pip[pip_id-1][1]);
            read(pip[pip_id-1][0], buf, 4);
            prime = *buf;
            printf("prime %d\n", prime);
            while (read(pip[pip_id-1][0], buf, 4) != 0)
            {
                if (*buf % prime != 0)
                {
                    write(pip[pip_id][1], buf, 4);
                }
            }
            close(pip[pip_id-1][0]);
            exit();
        }
        else
        {
            printf("fork error\n");
        }
    }
    exit();
}