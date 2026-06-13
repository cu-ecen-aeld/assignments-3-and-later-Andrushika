#include "systemcalls.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

bool do_system(const char *cmd)
{
    int rc = system(cmd);
    if(rc == -1)
        return false;
    return WIFEXITED(rc) && WEXITSTATUS(rc) == 0;
}

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    fflush(stdout);

    pid_t pid = fork();
    if(pid == -1){
        va_end(args);
        return false;
    }

    if(pid == 0){
        execv(command[0], command);
        exit(EXIT_FAILURE);
    }

    int wstatus;
    waitpid(pid, &wstatus, 0);
    va_end(args);

    return WIFEXITED(wstatus) && (WEXITSTATUS(wstatus) == 0);
}

bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    fflush(stdout);

    pid_t pid = fork();
    if(pid == -1){
        va_end(args);
        return false;
    }

    if(pid == 0){
        int fd = open(outputfile, O_WRONLY|O_CREAT|O_TRUNC, 0644);
        if(fd < 0)
            exit(EXIT_FAILURE);
        dup2(fd, STDOUT_FILENO);
        close(fd);
        execv(command[0], command);
        exit(EXIT_FAILURE);
    }

    int wstatus;
    waitpid(pid, &wstatus, 0);
    va_end(args);

    return WIFEXITED(wstatus) && (WEXITSTATUS(wstatus) == 0);
}
