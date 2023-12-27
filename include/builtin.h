/**
 * @file builtin.h
 * @brief Header file for builtin.c
 * @details Contains the function prototypes for the builtin commands
*/

#ifndef BUILTIN_H
#define BUILTIN_H
#include <stdlib.h>
#include <stdio.h>

/*
 * 内置命令
 * 1. cd
 * 2. help
 * 3. exit
 */
extern char *builtin_str[];            // 使用 extern 进行声明
extern int (*builtin_func[])(char **); // 使用 extern 进行声明

int wush_cd(char **args);
int wush_help(char **args);
int wush_exit(char **args);
int wush_num_builtins();



#endif // BUILTIN_H