#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include"../include/builtin.h"

void wush_print_prompt()
{
    // ANSI escape codes for colors
    const char *red_color = "\033[1;31m";
    const char *reset_color = "\033[0m";

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == -1)
    {
        perror("wush");
        exit(EXIT_FAILURE);
    }

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("wush");
        exit(EXIT_FAILURE);
    }

    printf("%s%s@%s%s:%s%s%s ",
           red_color, getenv("USER"), hostname, reset_color,
           red_color, cwd, reset_color);

    // Print "#" for root user, otherwise "$"
    if (geteuid() == 0)
    {
        printf("%s#%s ", red_color, reset_color);
    }
    else
    {
        printf("%s$%s ", red_color, reset_color);
    }

    fflush(stdout);
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

// Handle input and output redirection
void handle_redirection(char **command)
{
    int in_redirect_index = -1;
    int out_redirect_index = -1;

    // Find the input and output redirection indices
    for (int i = 0; command[i] != NULL; i++)
    {
        if (strcmp(command[i], "<") == 0)
        {
            in_redirect_index = i;
        }
        else if (strcmp(command[i], ">") == 0)
        {
            out_redirect_index = i;
        }
    }

    // Handle input redirection
    if (in_redirect_index != -1)
    {
        char *filename = command[in_redirect_index + 1];
        int fd = open(filename, O_RDONLY);
        if (fd == -1)
        {
            perror("wush");
            exit(EXIT_FAILURE);
        }

        dup2(fd, STDIN_FILENO);
        close(fd);

        // Remove the redirection symbols and the filename from the command
        command[in_redirect_index] = NULL;
        command[in_redirect_index + 1] = NULL;
    }

    // Handle output redirection
    if (out_redirect_index != -1)
    {
        char *filename = command[out_redirect_index + 1];
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1)
        {
            perror("wush");
            exit(EXIT_FAILURE);
        }

        dup2(fd, STDOUT_FILENO);
        close(fd);

        // Remove the redirection symbols and the filename from the command
        command[out_redirect_index] = NULL;
        command[out_redirect_index + 1] = NULL;
    }
}

int wush_handle_pipe(char **args)
{
    int status;
    pid_t pid;

    // 判断是否为pipe
    int pipe_index = -1;
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            pipe_index = i;
            args[i] = NULL; // Replace "|" with NULL to terminate the first command
            break;
        }
    }

    if (pipe_index == -1)
    {
        handle_redirection(args);
        // No pipe
        return wush_launch(args);
    }

    //printf("pipe_index:%d\n",pipe_index);

    // Pipe is present, execute commands on both sides of the pipe
    char **first_command = args;
    char **second_command = &args[pipe_index + 1];

    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("wush");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid == 0)
    {
        // Child process - execute the first command
        close(pipefd[0]);               // 关闭未使用的读取端，因为子进程只负责写入

        // close(STDOUT_FILENO);           // 关闭标准输出
        // dup(pipefd[1]);                 // 将标准输出重定向到管道的写入端

        dup2(pipefd[1], STDOUT_FILENO); // 将标准输出重定向到管道的写入端

        // Handle redirection for the first command
        handle_redirection(first_command);

        //close(pipefd[1]);               // Close the write end of the pipe
        wush_launch(first_command);     // 执行第一个命令
        exit(EXIT_SUCCESS);
    }
    else if (pid > 0)
    {
        // Parent process - execute the second command
        close(pipefd[1]);              // Close the write end of the pipe

        // close(STDIN_FILENO);           // 关闭标准输入
        // dup(pipefd[0]);                // 将标准输入重定向到管道的读取端

        dup2(pipefd[0], STDIN_FILENO); // Redirect standard input to the pipe
        
        //close(pipefd[0]);              // Close the read end of the pipe

        waitpid(pid, &status, 0); // Wait for the child to finish

        // Restore standard input to terminal
        dup2(STDIN_FILENO, STDIN_FILENO);

        // Handle redirection for the second command
        handle_redirection(second_command);

        //printf("now execute second command, %s\n", second_command[0]);
        wush_handle_pipe(second_command);
        exit(EXIT_SUCCESS);
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

    // 判断是否为pipe
    pid_t wu_pid;
    int wu_status;
    wu_pid = fork();
    if(wu_pid == 0)
    {
        wush_handle_pipe(args);
        exit(EXIT_SUCCESS);
    }
    else if(wu_pid < 0)
    {
        perror("wush");
    }
    else
    {
        do {
            wu_pid = waitpid(wu_pid,&wu_status,WUNTRACED);
        } while(!WIFEXITED(wu_status) && !WIFSIGNALED(wu_status));
    }
    return 1;

    // 不是内置命令，调用launch
    // return wush_launch(args);
}

int main(int argc, char **argv)
{
    char *line;
    char **args;
    int status;

    // loop
    do
    {
        wush_print_prompt();
        line = wush_read_line();
        args = wush_split_line(line);
        status = wush_execute(args);
        //printf("status:%d\n",status);

        free(line);
        free(args);
    } while (status);

    return EXIT_SUCCESS;
}