#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char const *argv[])
{
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
