#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/zmetrics.sock"
typedef struct
{
    char magic[4];
    unsigned char version;
    unsigned char type;
    unsigned short size;
} ZHeader;

typedef struct
{
    float cpu_usage;
    float mem_used_gb;
    float mem_total_gb;
    int temp_c;
    char cpu_model[64];
    char mobo_name[64];
    char gpu_name[64];
} ZMetrics;

static int read_full(int sock, void *buf, size_t len)
{
    size_t off = 0;

    while (off < len)
    {
        ssize_t r = read(sock, (char *)buf + off, len - off);

        if (r == 0)
            return 0; // server closed

        if (r < 0)
            return -1;

        off += r;
    }

    return 1;
}
void *hilo_funcion(void *arg)
{

    int valor = *(int *)arg;
    printf("Hola desde el hilo!\n");

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        exit(1);
    }
    while (1)
    {
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            perror("connect");
            close(sock);
            sleep(1);
            continue;
        }

        ZHeader h;
        if (read_full(sock, &h, sizeof(h)) <= 0)
        {
            close(sock);
            continue;
        }

        ZMetrics m;
        if (read_full(sock, &m, sizeof(m)) <= 0)
        {
            close(sock);
            continue;
        }

        printf("\033[H\033[J");
        printf("CPU: %.1f\n", m.cpu_usage);
        printf("RAM: %.2f / %.2f GB\n", m.mem_used_gb, m.mem_total_gb);
        printf("TEMP: %d°C\n", m.temp_c);
        fflush(stdout);
        close(sock);
        usleep(200000);
    }

    close(sock);
    // return 0;
}

void *mi_funcion(void *arg)
{
    int valor = *(int *)arg; // Convertimos el argumento a un entero
    printf("Valor recibido en el hilo: %d\n", valor);
    return NULL;
}

int main()
{
    pthread_t hilo; // Declaramos la variable del hilo
    int valor = 42; // Valor que pasaremos a la función

    // Creamos el hilo, pasándole la función y el argumento
    if (pthread_create(&hilo, NULL, hilo_funcion, &valor))
    {
        fprintf(stderr, "Error creando el hilo\n");
        return 1;
    }

    // pthread_detach(hilo);
    //  Esperamos a que el hilo termine
    // pthread_join(hilo, NULL);
    while (1)
    {
        printf("Hilo vive\n");
        // tu lógica principal aquí
        sleep(2);
        printf("programa sigue ejecutandose\n");
    }
    
    return 0;
}