#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <execinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <wait.h>
#include <linux/fs.h>
#include <time.h>

#include <semaphore.h>
#define SEMAPHORE_NAME "/mutex1"

#include "request.h"

_Bool flag_sigint = 1;
_Bool flag_sigalrm = 0;
_Bool terminate = 0;
#define SUCCESS 0
#define FAILURE -1

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

int setTimer(const struct Request* this)
{
    /* getting the current time */
    time_t rawtime = time(NULL);
    struct tm * curtime;
    curtime = localtime(&rawtime);

    /* calculating the difference between the request's time and the current time */
    size_t curtime_min = curtime->tm_min + curtime->tm_hour * 60;
    size_t reqtime_min = this->min+ this->hour * 60;
    ssize_t diff_min = reqtime_min - curtime_min;

    if(diff_min < 0)
    {   /* it could be more properly handled somehow */
        diff_min += 60*24;
    }
    /* setting the timer in seconds */
    alarm(1);
    return SUCCESS;
}

void execute(const struct Request* request, _Bool* flag_fork, _Bool* flag_execution)
{   /* request execution */
    pid_t pid = fork();
    if(pid == 0)
    {   /* child process */

        int log_fd = open(LOG_FILE_NAME, O_CREAT|O_APPEND|O_RDWR, S_IRWXU);
        /* changing std file descriptors to log_fd*/
        dup2(log_fd, STDOUT_FILENO);
        dup2(log_fd, STDERR_FILENO);
        close(log_fd);

        /* executing the request's programme */
        if(execve(request->command, request->argv, NULL) == -1)
        {
            /* command cannot be executed */
            *flag_execution = 0;
        }
    }
    else if(pid > 0)
    {   /* parent */
        /* waiting the child process to end*/
        wait(0);
        return;
    }
    else
    {   /* cannot fork process */
        *flag_fork = 0;
        return;
    }
}

void executionMonitor(const struct Request* request)
{
    pid_t pid = fork();
    if(pid == 0)
    {
        /* the child is to monitor execution */

        sem_t* sem = sem_open(SEMAPHORE_NAME, 0);
        sem_wait(sem);

        char* logbuf = (char*)malloc(sizeof(char) * 256);
        sprintf(logbuf, "\nCommand %s is being executed:\n", request->command);
        logmessage(LOG_INFO, logbuf);

        _Bool flag_fork = 1;
        _Bool flag_execution = 1;

        execute(request, &flag_fork, &flag_execution);

        if(flag_fork != 1)
        {
            logmessage(LOG_ERR, "Cannot fork process for execution!\n");
            exit(EXIT_FAILURE);
        }

        if(flag_execution != 1)
        {
            logmessage(LOG_ERR, "Cannot execute the command!\n");
        }
        else
        {
            logmessage(LOG_ERR, "Command has been executed!\n\n");
        }

        sem_post(sem);
        sem_close(sem);

        /* monitor's job is done */
        exit(EXIT_SUCCESS);
    }
    else if(pid > 0)
    {
        /* parent process is the daemon itself */
        return;
    }
    else
    {
        logmessage(LOG_ERR, "Cannot fork process for monitor!\n");
        exit(EXIT_FAILURE);
    }
}



void cdaemon(int argc, char* argv[])
{
    /* creating a named semaphore */
    sem_t *sem = sem_open(SEMAPHORE_NAME, O_CREAT, S_IRWXU, 1);

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

     /* position in the input file and a ptr to current request */
    size_t seekshift = 0;
    struct Request* request = NULL;

    /* the daemon's endless loop */
    while(!terminate)
    {
        if(flag_sigint)
        {   /* caught SIGINT */
            /* set new request from the very start of the config file */
            logmessage(LOG_INFO, "caught SIGINT\n");
            flag_sigint = 0;

            /* setting a new request */
            seekshift = 0;
            if(setRequest(&request, argv[1], &seekshift) == FAILURE)
            {
                logmessage(LOG_INFO, "no requests!\n");
                exit(EXIT_FAILURE);
            }
            /* a new request has been set */
            /* setting a timer for the request */
            setTimer(request);
        }
        if(flag_sigalrm)
        {   /* caught SIGALRM */
            /* execute current request and set a new one */
            flag_sigalrm = 0;


            executionMonitor(request);
            /* freeing memory occupied by recently executed request */
            freeRequest(request);

            /* setting a new request */
            if(setRequest(&request, argv[1], &seekshift) == FAILURE)
            {
                /* entering the critical section - locking the binary semaphore */
                sem_wait(sem);

                logmessage(LOG_INFO, "no new requests!\n");

                /*exiting the critical section - unlocking the binary semaphore */
                sem_post(sem);
                terminate = 1;
                break;
            }

            /* a new request has been set */
            /* setting a timer for the request */
            setTimer(request);
        }

        /* waiting for an any signal */
        pause();
    }

    if(terminate)
    {
        /* waiting for all child processes to finish */
        int status = 0;
        while (wait(&status) > 0);

        /* closing the semaphore and removing it's name from the system */
        sem_close(sem);
        sem_unlink(SEMAPHORE_NAME);

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
    /*
    if(chdir("/") == -1)
    {
        exit(EXIT_FAILURE);
    }
     */
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
        printf("use $ ./my_daemon filename.cfg\n");
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

        cdaemon(argc, argv);
    }
    else
    {
        perror("Cannot fork process");
    }
    return 0;
}








