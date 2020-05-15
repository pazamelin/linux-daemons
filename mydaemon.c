#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>

_Bool flag_sigint = 1;
_Bool flag_sigalrm = 0;
_Bool terminate = 0;

#define LOG_FILE_NAME "log.txt"

static void sigint_handler(int signo)
{   /* caught SIGINT */
    flag_sigint = 1;
}

static void sigterm_handler(int signo)
{   /* caught SIGTERM */
    terminate = 1;
}

static void sigalrm_handler(int signo)
{   /* caught SIGALRM */
    flag_sigalrm = 1;
}

void logmessage(int priority, const char* message)
{
    int logfile_d = open(LOG_FILE_NAME, O_CREAT|O_APPEND|O_RDWR, S_IRWXU);
    write(logfile_d, message, strlen(message));
    close(logfile_d);

    syslog(priority, "%s", message);
}

void cdaemon(int argc, char* argv[])
{

    /* opening a connection to the system logger */
    openlog("DAEMON", LOG_PID, LOG_DAEMON);
    logmessage(LOG_INFO, "daemon started!\n");

    /* registering SIGINT handler */
    if (signal (SIGINT, sigint_handler) == SIG_ERR)
    {
        logmessage(LOG_ERR, "cannot handle SIGINT!\n");
        exit(EXIT_FAILURE);
    }
    /* registering SIGTERM handler */
    if (signal (SIGTERM, sigterm_handler) == SIG_ERR)
    {
        logmessage(LOG_ERR, "cannot handle SIGTERM!\n");
        exit(EXIT_FAILURE);
    }
    /* registering SIGALRM handler */
    if (signal (SIGALRM, sigalrm_handler) == SIG_ERR)
    {
        logmessage(LOG_ERR, "cannot handle SIGALRM!\n");
        exit(EXIT_FAILURE);
    }

    /* the daemon's endless loop */
    while(!terminate)
    {
        if(flag_sigint)
        {   /* caught SIGINT */
            logmessage(LOG_INFO, "caught SIGINT\n");
            flag_sigint = 0;

            /* ... */
        }
        if(flag_sigalrm)
        {   /* caught SIGALRM */
            flag_sigalrm = 0;

            /* ... */
        }

        /* waiting for an any signal */
        pause();
    }

    if(terminate)
    {
        logmessage(LOG_ERR, "Caught SIGTERM\n");
        exit(EXIT_SUCCESS);
    }
}

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
