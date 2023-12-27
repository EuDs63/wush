/**
 * builtin.c
 * 内置命令
*/
#include"../include/builtin.h"

char *builtin_str[] = {
    "cd",
    "help",
    "exit"};

int (*builtin_func[])(char **) = {
    &wush_cd,
    &wush_help,
    &wush_exit};

int wush_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

int wush_cd(char **args)
{
    if (args[1] == NULL)
    {
        // 无参数
        fprintf(stderr, "wush: expected argument to \"cd\"\n");
    }
    else
    {
        // 改变目录
        if (chdir(args[1]) != 0)
        {
            perror("wush");
        }
    }

    return 1;
}

int wush_help(char **args)
{
    int i;

    printf("Welcome to use wush\n");
    printf("The following are built in:\n");

    for (i = 0; i < wush_num_builtins(); i++)
    {
        printf("  %s\n", builtin_str[i]);
    }

    return 1;
}

int wush_exit(char **args)
{
    return 0;
}