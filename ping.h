#ifndef PING_H
#define PING_H

#include <pthread.h>
#include <stdbool.h>

typedef struct
{
    pthread_t thread;
    bool running;

} PingWorker;

void ping_start(PingWorker *ping);
void ping_stop(PingWorker *ping);

#endif