#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *fmtname(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

void find(char *path, char *file)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type){
    case T_FILE:
        printf("Usage: find [dir] [file]\n");
        break;

    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if (stat(buf, &st) < 0)
            {
                printf("find: cannot stat %s\n", buf);
                continue;
            }
            if ((st.type == T_DIR)&&(fmtname(buf)[0]!='.'))
            {
                find(buf, file);
            }
            if (strcmp(fmtname(buf), file)==0)
            {
                printf("%s\n", buf);
            }
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    char file[DIRSIZ + 1];
    if (argc != 3)
    {
        printf("Usage: find [dir] [file]\n");
        exit();
    }
    memmove(file, argv[2], strlen(argv[2]));
    memset(file + strlen(argv[2]), ' ', DIRSIZ - strlen(argv[2]));
    find(argv[1], file);
    exit();
}