#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include "zstats_common.h"

ZSharedStats *stats = NULL;
void do_ping_test(ZSharedStats *s)
{
    printf("[Z] 📡 Ejecutando test de latencia a 8.8.8.8...\n");

    // Co/home/alb/zmenu_gtk/zmenu_toggle.shmando simplificado para asegurar que captura el dato
    // Esta versión es más compatible con el ping moderno de Arch
    FILE *fp = popen("ping -c 1 -W 1 8.8.8.8 | awk -F'tiempo=' '/tiempo=/{print $2}' | cut -d' ' -f1", "r");
    {
        float ms = 0;
        if (fscanf(fp, "%f", &ms) == 1)
        {
            s->metrics.net_latency = (int)ms;
            s->dashboard.net = Z_OK;
            printf("[Z] Latencia capturada: %d ms\n", s->metrics.net_latency);
        }
        else
        {
            s->metrics.net_latency = -1; // Para saber que falló el parseo
            s->dashboard.net = Z_FAIL;
            printf("[Z] Error: No se pudo leer el tiempo del ping.\n");
        }
        pclose(fp);
    }
    s->control.request_ping = false;
}
int setup_netlink()
{
    struct sockaddr_nl sa = {.nl_family = AF_NETLINK, .nl_groups = 1};
    int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
    if (sock < 0)
        return -1;
    bind(sock, (struct sockaddr *)&sa, sizeof(sa));
    return sock;
}

void cleanup(int sig)
{
    printf("\n[Z] Apagando Cockpit y limpiando SHM...\n");
    shm_unlink(ZSHM_NAME);
    exit(0);
}

int main()
{
    int nl_sock = setup_netlink();
    char buffer[2048];
    // Manejo de señales para no dejar basura en /dev/shm
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    // 1. Crear Memoria Compartida
    int fd = shm_open(ZSHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(ZSharedStats));
    stats = mmap(NULL, sizeof(ZSharedStats), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    memset(stats, 0, sizeof(ZSharedStats));
    printf("[Z] Zaramaga System Engine ONLINE\n");

    // 2. CHECKLIST INICIAL (Los Pilotos)
    printf("[Z] Ejecutando Checklist...\n");

    // Simulación de detección (luego meteremos los comandos reales)
    stats->dashboard.krnl = Z_OK;
    stats->dashboard.drm = Z_OK; // Habría que chequear /dev/dri/card0
    stats->dashboard.vkn = (access("/usr/share/vulkan/icd.d/", F_OK) == 0) ? Z_OK : Z_FAIL;

    // Obtener versión de Kernel
    FILE *fp = fopen("/proc/sys/kernel/osrelease", "r");
    fgets(stats->kernel_ver, 64, fp);
    fclose(fp);
    stats->control.request_ping = true; // Forzamos un ping al arrancar
    while (1)
    {
        // Usamos un truco: recv con MSG_DONTWAIT para que no bloquee el bucle
        int len = recv(nl_sock, buffer, sizeof(buffer), MSG_DONTWAIT);
        if (len > 0)
        {
            // Buscamos si es un Joystick/Gamepad
            if (strstr(buffer, "input/js") || strstr(buffer, "js0"))
            {
                if (strstr(buffer, "add"))
                {
                    printf("[Z] 🎮 MANDO DETECTADO\n");
                    stats->io.gamepad = true;
                }
                else if (strstr(buffer, "remove"))
                {
                    printf("[Z] 🎮 MANDO DESCONECTADO\n");
                    stats->io.gamepad = false;
                }
            }
            // Buscamos si es un Ratón
            else if (strstr(buffer, "mouse"))
            {
                if (strstr(buffer, "add"))
                    stats->io.mouse = true;
                else if (strstr(buffer, "remove"))
                    stats->io.mouse = false;
            }
        }

        // Actualizamos las métricas normales
        stats->metrics.cpu_load = 5.0; // Aquí llamaremos a tu lógica de antes
        stats->control.last_update = (uint64_t)time(NULL);
        if (stats->control.request_ping)
        {
            do_ping_test(stats);
        }

        usleep(500000); // Bajamos a 0.5s para que sea más fluido
    }

    return 0;
}