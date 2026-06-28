#include "ping.h"

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

static void *ping_thread(void *arg)
{
    PingWorker *ping = arg;
    int last_ping_ms = -1; // Variable local para almacenar el último ping
    while (ping->running)
    {
        printf("Ping...\n");
        sleep(1);
        FILE *fp = popen("LC_ALL=C ping -c 1 -W 1 8.8.8.8 | grep time= | cut -d '=' -f 4 | cut -d ' ' -f 1", "r");
        if (fp) {
            printf("Ping command executed successfully.\n");
            char buf[16];
            if (fgets(buf, sizeof(buf), fp)) {
                last_ping_ms = atoi(buf);
                printf("Último ping: %d ms\n", last_ping_ms);
                ping->running = false;
                //ping_stop(ping); // Aseguramos que el hilo siga corriendo
                ping->last_ping_ms = last_ping_ms; // Guardamos el último ping en la estructura
            }
            pclose(fp);
        }
        sleep(1);
    }

    printf("Fin del hilo\n");

    return NULL;
}
void ping_start(PingWorker *ping)
{
    ping->running = true;

    pthread_create(
        &ping->thread,
        NULL,
        ping_thread,
        ping
    );
}
void ping_stop(PingWorker *ping)
{
    ping->running = false;

    pthread_join(ping->thread, NULL);
}
int main(void)
{
    PingWorker ping;

    ping_start(&ping);


    //sleep(1);

    //ping_stop(&ping);
    while (ping.running)
    {
        while (ping.last_ping_ms <= 0)
        {
            //printf("Último ping registrado: %d ms\n", ping.last_ping_ms);
            sleep(1);
            
        }
        
        printf("Esperando a que el ping termine...\n");
        printf("Último ping registrado: %d ms\n", ping.last_ping_ms);
        //sleep(1);
    }

    return 0;
}