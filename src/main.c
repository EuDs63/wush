#include<stdio.h>
#include<sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include"../include/builtin.h"

void wush_print_prompt(void)
{
    printf("> ");
}

char *wush_read_line(void)
{
    char *line = NULL;
    // 0 means no limit
    ssize_t bufsize = 0;

    if(getline(&line,&bufsize,stdin) == -1)
    {
        // 当读到EOF时，退出
        if(feof(stdin))
        {
            exit(EXIT_SUCCESS);
        }
        else
        {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }

    return line;
}

#define WUSH_TOKEN_BUFFERSIZE 64
// 定义分割符，分别为tab,回车，换行，空格
#define WUSH_TOKEN_DELIMITER "\t\r\n "
char** wush_split_line(char* line)
{
    int bufsize = WUSH_TOKEN_BUFFERSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*)); 
    char *token;

    // 若申请空间失败，则报错
    if (!tokens) {
        fprintf(stderr, "wush: allocation tokens error\n");
        exit(EXIT_FAILURE);
    }

    // 分割line
    token = strtok(line, WUSH_TOKEN_DELIMITER);
    while (token){
        tokens[position] = token;
        position++;

        // 若超过空间，则重新申请空间
        if (position >= bufsize) {
            bufsize += WUSH_TOKEN_BUFFERSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "wush: allocation tokens error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, WUSH_TOKEN_DELIMITER);
    }

    // 循环结束后，最后一个位置置空
    tokens[position] = NULL;

    return tokens;
}

int wush_launch(char **args)
{
    pid_t pid, wpid;
    int status;

    //fork子进程
    pid = fork();
    if(pid == 0)
    {
        //子进程,执行命令
        if(execvp(args[0],args) == -1)
        {
            perror("wush");
            printf("the last input args are:\n");
            // 将参数打印出来
            int i = 0;
            while (args[i] != NULL)
            {
                printf("%s\n", args[i]);
                i++;
            }
        }
    }
    else if (pid < 0)
    {
        //出错
        perror("wush");
        
    }
    else
    {
        //父进程,等待，直到子进程结束或者被信号中断
        do {
            wpid = waitpid(pid,&status,WUNTRACED);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int wush_execute(char **args)
{
    int i;
    if (args[0] == NULL) {
        // 无命令
        return 1;
    }

    // 先判断是否为内置命令
    for(i=0;i < wush_num_builtins();i++)
    {
        if(strcmp(args[0],builtin_str[i]) == 0)
        {
            return (*builtin_func[i])(args);
        }
    }
    // 不是内置命令，调用launch
    return wush_launch(args);
}

void wush_loop(void)
{
    char *line;
    char **args;
    int status;

    do
    {
        wush_print_prompt();
        line = wush_read_line();
        args = wush_split_line(line);
        status = wush_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv)
{
    // loop
    wush_loop();

    return EXIT_SUCCESS;
}