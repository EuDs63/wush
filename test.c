#include <stdio.h>
#include <unistd.h>

int main()
{
    char *command = "ls";                       // 要执行的命令
    char *argument_list[] = {"ls", "-l", NULL}; // 命令的参数列表

    printf("调用 execvp() 执行 ls -l 命令\n");
    execvp(command, argument_list);

    // 如果 execvp() 执行成功，下面的代码不会被执行
    printf("这行代码不会被打印出来\n");

    return 0;
}