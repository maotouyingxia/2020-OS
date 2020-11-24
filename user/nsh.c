#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAXARGS 10
#define MAXCMDS 10
#define EXEC 1
#define REDIR 2
#define PIPE 3

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int cc;
struct cmd
{
    int type;
} command[MAXCMDS];

int ec;
struct execcmd
{
    int type;
    char *argv[MAXARGS];
    char *eargv[MAXARGS];
} exec_command[MAXCMDS];

int rc;
struct redircmd
{
    int type;
    struct cmd *cmd;
    char *file;
    char *efile;
    int mode;
    int fd;
} redir_command[MAXCMDS];

int pc;
struct pipecmd
{
    int type;
    struct cmd *left;
    struct cmd *right;
} pipe_command[MAXCMDS];

// 报错退出
void panic(char *s)
{
    fprintf(2, "%s\n", s);
    exit(1);
}

// 获得用户输入的命令
int getcmd(char *buf, int nbuf)
{
    fprintf(2, "@ ");
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if (buf[0] == 0) // EOF
        return -1;
    return 0;
}

// 检查命令中是否存在指定字符
int peek(char **ps, char *es, char *toks)
{
    char *s;
    s = *ps;
    while (s < es && strchr(whitespace, *s))
        s++;
    *ps = s;
    return *s && strchr(toks, *s);
}

// 生成execcmd结构体
struct cmd *execcmd(void)
{
    struct execcmd *p;
    if (ec < MAXCMDS)
    {
        p = &exec_command[ec];
        memset(p, 0, sizeof(*p));
        p->type = EXEC;
        ec++;
    }
    else
    {
        panic("too many exec command");
    }
    return (struct cmd *)p;
}

// 生成redircmd结构体
struct cmd *redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
    struct redircmd *p;
    if (rc < MAXCMDS)
    {
        p = &redir_command[rc];
        p->type = REDIR;
        p->cmd = subcmd;
        p->file = file;
        p->efile = efile;
        p->mode = mode;
        p->fd = fd;
        rc++;
    }
    else
    {
        panic("too many redir commands");
    }
    return (struct cmd *)p;
}

// 生成pipecmd结构体
struct cmd *pipecmd(struct cmd *left, struct cmd *right)
{
    struct pipecmd *p;
    if (pc < MAXCMDS)
    {
        p = &pipe_command[pc];
        p->type = PIPE;
        p->left = left;
        p->right = right;
        pc++;
    }
    else
    {
        panic("too many pipe commands");
    }
    return (struct cmd *)p;
}

// 获取标记
int gettoken(char **ps, char *es, char **q, char **eq)
{
    char *s;
    int ret;
    s = *ps;
    while (s < es && strchr(whitespace, *s))
        s++;
    if (q)
        *q = s;
    ret = *s;
    switch (*s)
    {
    case 0:
        break;
    case '|':
    case '<':
        s++;
        break;
    case '>':
        s++;
        if (*s == '>')
        {
            ret = '+';
            s++;
        }
        break;
    default:
        ret = 'a';
        while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
            s++;
        break;
    }
    if (eq)
        *eq = s;

    while (s < es && strchr(whitespace, *s))
        s++;
    *ps = s;
    return ret;
}

// 检查是否有重定向命令
struct cmd *parseredirs(struct cmd *cmd, char **ps, char *es)
{
    int tok;
    char *q, *eq;

    while (peek(ps, es, "<>"))
    {
        tok = gettoken(ps, es, 0, 0);
        if (gettoken(ps, es, &q, &eq) != 'a')
        {
            panic("missing file for redirection");
        }
        switch (tok)
        {
        case '<':
            cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
            break;
        case '>':
            cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 1);
            break;
        case '+': // >>
            cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 1);
            break;
        }
    }
    return cmd;
}

// 分析命令
struct cmd *parseexec(char **ps, char *es)
{
    char *q, *eq;
    int tok, argc;
    struct execcmd *cmd;
    struct cmd *ret;

    ret = execcmd();
    cmd = (struct execcmd *)ret;

    argc = 0;
    ret = parseredirs(ret, ps, es);
    while (!peek(ps, es, "|"))
    {
        if ((tok = gettoken(ps, es, &q, &eq)) == 0)
            break;
        if (tok != 'a')
            panic("syntax");
        cmd->argv[argc] = q;
        cmd->eargv[argc] = eq;
        argc++;
        if (argc >= MAXARGS)
            panic("too many args");
        ret = parseredirs(ret, ps, es);
    }
    cmd->argv[argc] = 0;
    cmd->eargv[argc] = 0;
    return ret;
}

// 检查是否有管道命令
struct cmd *parsepipe(char **ps, char *es)
{
    struct cmd *cmd;

    cmd = parseexec(ps, es);
    if (peek(ps, es, "|"))
    {
        gettoken(ps, es, 0, 0);
        cmd = pipecmd(cmd, parsepipe(ps, es));
    }
    return cmd;
}

// 分析一行命令
struct cmd *parseline(char **ps, char *es)
{
    struct cmd *cmd;

    cmd = parsepipe(ps, es);
    return cmd;
}

// NUL-terminate all the counted strings.
struct cmd *nulterminate(struct cmd *cmd)
{
    int i;
    struct execcmd *ecmd;
    struct pipecmd *pcmd;
    struct redircmd *rcmd;

    if (cmd == 0)
        return 0;

    switch (cmd->type)
    {
    case EXEC:
        ecmd = (struct execcmd *)cmd;
        for (i = 0; ecmd->argv[i]; i++)
            *ecmd->eargv[i] = 0;
        break;

    case REDIR:
        rcmd = (struct redircmd *)cmd;
        nulterminate(rcmd->cmd);
        *rcmd->efile = 0;
        break;

    case PIPE:
        pcmd = (struct pipecmd *)cmd;
        nulterminate(pcmd->left);
        nulterminate(pcmd->right);
        break;
    }
    return cmd;
}

// 解释输入的命令
struct cmd *parsecmd(char *s)
{
    char *es;
    struct cmd *cmd;

    es = s + strlen(s);
    cmd = parseline(&s, es);
    nulterminate(cmd);
    return cmd;
}

// Execute cmd.  Never returns.
void runcmd(struct cmd *cmd)
{
    int p[2];
    struct execcmd *ecmd;
    struct pipecmd *pcmd;
    struct redircmd *rcmd;

    if (cmd == 0)
        exit(-1);
    switch (cmd->type)
    {
    case EXEC:
        ecmd = (struct execcmd *)cmd;
        if (ecmd->argv[0] == 0)
            exit(1);
        exec(ecmd->argv[0], ecmd->argv);
        fprintf(2, "exec %s failed\n", ecmd->argv[0]);
        break;

    case REDIR:
        rcmd = (struct redircmd *)cmd;
        close(rcmd->fd);
        if (open(rcmd->file, rcmd->mode) < 0)
        {
            fprintf(2, "open %s failed\n", rcmd->file);
            exit(1);
        }
        runcmd(rcmd->cmd);
        break;

    case PIPE:
        pcmd = (struct pipecmd *)cmd;
        if (pipe(p) < 0)
            panic("pipe");
        if (fork() == 0)
        {
            close(1);
            dup(p[1]);
            close(p[0]);
            close(p[1]);
            runcmd(pcmd->left);
        }
        if (fork() == 0)
        {
            close(0);
            dup(p[0]);
            close(p[0]);
            close(p[1]);
            runcmd(pcmd->right);
        }
        close(p[0]);
        close(p[1]);
        wait(0);
        wait(0);
        break;
    default:
        panic("runcmd");
    }
    exit(1);
}

int main(int argc, char const *argv[])
{
    static char buf[100];
    int fd;

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
            runcmd(parsecmd(buf));
        wait(0);
    }
    exit(0);
}