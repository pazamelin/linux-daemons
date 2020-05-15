#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


void cdaemon(int argc, char* argv[]);

void daemonize()
{
    /* setting files permissions */
    umask(0);

    /* giving the daemon a new process group and session */
    pid_t sid = setsid();
    if(sid == -1)
    {
        exit(EXIT_FAILURE);
    }

    /* setting the working directory to the root directory */
    if(chdir("/") == -1)
    {
        exit(EXIT_FAILURE);
    }

    /* closing all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }

    /* redirecting file descriptors 0,1,2 to /dev/null */
    open ("/dev/null", O_RDWR);  /* 0 - stdin     */
    dup (0);                      /* 1 - stdout    */
    dup (0);                      /* 2 - stderror  */
}

int main(int argc, char* argv[])
{
    /* daemon usage: */
    if (argc != 2)
    {
        printf("use $ ./my_daemon input.txt\n");
        exit(EXIT_FAILURE);
    }

    /* creating a new process */
    pid_t pid = fork();
    if(pid  > 0)
    {
        /* parent is calling exit */
        exit(EXIT_SUCCESS);
    }
    else if(pid == 0)
    {
        /* daemonizing the child process */
        daemonize();

        /* daemon prototype: */
        cdaemon(argc, argv);
    }
    else
    {
        perror("Cannot fork process");
    }
    return 0;
}