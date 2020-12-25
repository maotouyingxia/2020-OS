#include <kernel/types.h>
#include <kernel/stat.h>
#include <kernel/param.h>
#include "kernel/fcntl.h"
#include <user/user.h>

int getcmd(char *buf, int nbuf)
{
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if (buf[0] == 0) // EOF
        return -1;
    return 0;
}

int main(int argc, char const *argv[])
{
    int fd;
    int cnt;
    static char buf[100];
    char *head;
    char *tail;
    char *vec[MAXARG];

    // Ensure that three file descriptors are open.
    while ((fd = open("console", O_RDWR)) >= 0)
    {
        if (fd >= 3)
        {
            close(fd);
            break;
        }
    }

    // Read and run input commands.
    while (getcmd(buf, sizeof(buf)) >= 0)
    {
        if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ')
        {
            // Chdir must be called by the parent, not the child.
            buf[strlen(buf) - 1] = 0; // chop \n
            if (chdir(buf + 3) < 0)
                fprintf(2, "cannot cd %s\n", buf + 3);
            continue;
        }
        if (fork() == 0)
        {
            head = buf;
            tail = buf;
            cnt = argc;
            // 复制已有的参数
            for (int i = 0; i < argc; i++)
            {
                memmove(vec, argv, argc*sizeof(char*));
            }
            // 添加额外的参数
            while (*tail != '\n')
            {
                tail++;
                if (*tail == ' ')
                {
                    *tail = 0;
                    vec[cnt] = head;
                    cnt++;
                    tail++;
                    head = tail;
                }
            }
            *tail = 0;
            vec[cnt] = head;
            // 执行命令
            exec(vec[1], vec+1);
        }
        wait();
    }
    exit();
}