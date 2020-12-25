#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char const *argv[])
{
    // 对于非数字字符，atoi返回0
    if (argc == 2 && atoi(argv[1]) != 0)
    {
        sleep(atoi(argv[1]));
    }
    else
    {
        printf("usage: sleep [number]\n");
    }
    exit();
}
