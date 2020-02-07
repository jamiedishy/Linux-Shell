#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LINE 80

int main(void)
{
    pid_t child_pid, pid, pid2;
    int child_status;
    int should_run = 1;
    char *separator = " ";
    char temp[MAX_LINE / 2 + 1];
    int bground = 0;
    int count = 0;

    while (should_run)
    {

        // These values are reinitialized for each loop
        char *args[MAX_LINE / 2 + 1];
        char **token = malloc(8 * (sizeof(char *) + 1));  // tokenized array passed to execvp
        char **token2 = malloc(8 * (sizeof(char *) + 1)); // tokenized array passed to execvp
        int index = 0;
        int in = 0;
        int out = 0;
        int thereIsPipe = 0;
        int indexOfPipe = 0;
        int status;
        int indexToken2 = 0;
        int pipes[2];

        printf("mysh:~$ ");
        fflush(stdout);

        fgets(args, MAX_LINE, stdin);
        if (strcmp(args, "!!\n") != 0)
        {
            strcpy(temp, args);
        }
        else
        {
            if (count == 0)
            {
                printf("No previous command. First run a command to use !!.\n");
                continue;
            }
            else
            {
                strcpy(args, temp);
            }
        }

        char *pointer = strtok(args, separator);

        while (pointer != NULL)
        {
            if (strcmp(pointer, "&\n") == 0)
            {
                bground = 1;
            }
            else
            {
                if (thereIsPipe)
                {
                    token2[indexToken2] = pointer;
                    indexToken2 = indexToken2 + 1;
                }

                else
                {

                    token[index] = pointer;

                    if (strcmp(pointer, "|") == 0)
                    {
                        indexOfPipe = index;
                        thereIsPipe = 1;
                    }
                    if (strcmp(pointer, "<") == 0)
                    {
                        in = index; // stores the index where < occurs
                    }
                    else if (strcmp(pointer, ">") == 0)
                    {
                        out = index;
                    }
                }
                index++;
            }
            pointer = strtok(NULL, separator);
        }

        if (thereIsPipe)
        {
            token[indexOfPipe] = NULL;
            token2[indexToken2 - 1][strlen(token2[indexToken2 - 1]) - 1] = '\0';
            pipe(pipes);
        }

        /*
        An issue occurred where the last character in the last index was a new line. For this reason, execvp didn't execute the specified input command.
        Resolution: remove the last character from the last index in token
        token = array
        token[index-1] gets last token
        strlen(token[index-1])-1 is index of last character for last token
        token[index-1][strlen(token[index-1])-1] = '\0'
        */
        if (bground != 1 && thereIsPipe != 1)
        {
            token[index - 1][strlen(token[index - 1]) - 1] = '\0';
        }

        if (strcmp(token[0], "exit") == 0)
        {
            should_run = 0;
            exit(0);
        }

        if (strcmp(token[0], "cd") == 0)
        {
            if (cd(token[1]) < 0)
            {
                perror(token[1]);
            }
            continue;
        }

        child_pid = fork();

        if (child_pid == 0)
        {
            //printf("Child: executing\n");

            if (thereIsPipe)
            {
                dup2(pipes[0], 0); // replace ls' stdout with write part of 1st pipe
                close(pipes[1]);
                execvp(token2[0], token2); // execute command after the pipe symbol
                if (execvp(token2[0], token2) < 0)
                {
                    perror("execvp failed");
                    exit(1);
                }
            }

            if (bground)
            {
                setpgid(0, 0);
            }

            if (in)
            {
                int fd0;
                if ((fd0 = open(token[in + 1], O_RDONLY, 0)) == -1)
                { // set token[in+1] b/c this index is where text file which we need to open
                    perror(token[in]);
                    exit(EXIT_FAILURE);
                }
                dup2(fd0, 0);
                close(fd0);
                token[in] = NULL;
            }

            if (out)
            {
                int fd1; // file descriptor, add append
                if ((fd1 = open(token[out + 1], O_WRONLY | O_CREAT | O_TRUNC | O_CREAT,
                                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
                {
                    perror(token[out]);
                    exit(EXIT_FAILURE);
                }
                dup2(fd1, 1);
                close(fd1);
                token[out] = NULL;
            }

            execvp(token[0], token);

            if (execvp(token[0], token) < 0)
            {
                perror("execvp failed");
                exit(1);
            }
        }

        if (thereIsPipe)
        {
            pid2 = fork();

            if (pid2 == 0)
            {
                dup2(pipes[1], 1);
                close(pipes[0]);
                execvp(token[0], token); // execute command before the pipe symbol
                if (execvp(token[0], token) < 0)
                {
                    perror("execvp failed");
                    exit(1);
                }
            }
        }

        if (child_pid > 0)
        {

            if (bground != 1 && thereIsPipe)
            {
                close(pipes[0]);
                close(pipes[1]);
                wait(NULL);
                wait(NULL);
            }

            if (bground != 1 && thereIsPipe != 1)
            {
                wait(NULL);
            }

            bground = 0;
        }
        else
        {
            perror("fork failed");
            exit(1);
        }
        count++;
    }
    return 0;
}

int cd(char *directory)
{
    return chdir(directory);
}