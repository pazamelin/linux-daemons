#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fs.h>
#include "request.h"

#define SUCCESS 0
#define FAILURE -1

struct Request* readRequest(const char* pathname, size_t* seekshift)
{   /* reading a new request from the file */
    char buf[512] = "";
    int input_d = open(pathname, O_RDONLY, S_IRWXU);

    /* changing the current offset in the file */
    lseek(input_d, *seekshift, SEEK_SET);
    size_t sread = read(input_d, buf, sizeof(buf));
    if(sread <= 0)
    {
        return NULL;
    }

    char* comline = strtok(buf, "\n");
    comline[strlen(comline)] = '\0';

    /* changing a future offset in the file */
    *seekshift += strlen(comline) + 1;

    /* parsing the request */
    /* NOTICE: the following could be unsafe depending on input: */
    struct Request* newRequest = (struct Request*)malloc(sizeof(struct Request));
    newRequest->hour = atoi(strtok(comline, ":"));
    newRequest->min = atoi(strtok(NULL, " "));

    char* command = strtok(NULL, " ");
    size_t csize = strlen(command);
    newRequest->command = (char*)malloc(csize * sizeof(char));
    memcpy(newRequest->command, command, csize);
    newRequest->command[csize] = '\0';

    size_t argc = 0;
    size_t argc_reserved = 5;
    newRequest->argv = (char**)malloc(argc_reserved * sizeof(char**));
    char* arg = strtok(NULL, " ");
    while(arg != NULL)
    {
        if(argc == argc_reserved)
        {
            argc_reserved += 5;
            newRequest->argv = (char**)realloc(newRequest->argv, argc_reserved * sizeof(char**));
        }
        size_t args = strlen(arg);
        newRequest->argv[argc] = (char*)malloc(args * sizeof(char));
        memcpy(newRequest->argv[argc],arg, args);
        newRequest->argv[argc][args] = '\0';
        argc++;

        arg = strtok(NULL, " ");
    }
    argc++;
    newRequest->argv = (char**)realloc(newRequest->argv, argc * sizeof(char**));
    newRequest->argv[argc - 1] = NULL;
    newRequest->argc = argc;
    return newRequest;
}

int setRequest(struct Request** request, const char* filename, size_t* seekshift)
{
    *request = readRequest(filename, seekshift);
    if(*request == NULL)
    {
        return FAILURE;
    }
    return SUCCESS;
}

void freeRequest(struct Request* this)
{   /* request destructor */
    for(int arg = 0; arg < this->argc; arg++)
    {
        free(this->argv[arg]);
    }
    free(this->argv);
    free(this->command);
    free(this);
}