#ifndef REQUEST_H
#define REQUEST_H

struct Request
{
    int hour;
    int min;
    char* command;
    int argc;
    char** argv;
};
struct Request* readRequest(const char* pathname, size_t* seekshift);
int setRequest(struct Request** request, const char* filename, size_t* seekshift);
void freeRequest(struct Request* this);

#endif //REQUEST_H
