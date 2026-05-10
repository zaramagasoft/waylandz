#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/zmetrics.sock"

typedef struct
{
    char magic[4];
    uint8_t version;
    uint8_t type;
    uint16_t size;
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

// ASEGÚRATE DE QUE ESTO SEA IGUAL EN EL SERVER
typedef struct
{
    int opcode;
    char payload[256];
} ZCommand;

static int read_full(int sock, void *buf, size_t len)
{
    size_t off = 0;
    while (off < len)
    {
        ssize_t r = read(sock, (char *)buf + off, len - off);
        if (r <= 0)
            return r;
        off += r;
    }
    return 1;
}

int main()
{
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    while (1)
    {
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0)
        {
            perror("socket");
            sleep(1);
            continue;
        }

        // DENTRO DEL WHILE(1) del cliente
        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) >= 0)
        {
            printf("[CLIENT] 1. Conectado. Enviando comando...\n");

            ZCommand cmd = {.opcode = 0};
            int w = write(sock, &cmd, sizeof(ZCommand));
            printf("[CLIENT] 2. Comando enviado (%d bytes). Esperando cabecera...\n", w);

            ZHeader h;
            if (read_full(sock, &h, sizeof(h)) > 0)
            {
                printf("[CLIENT] 3. Cabecera recibida. Leyendo métricas...\n");
                ZMetrics m;
                if (read_full(sock, &m, sizeof(m)) > 0)
                {
                    printf("[CLIENT] 4. Todo OK. CPU: %.1f\n", m.cpu_usage);
                }
            }
        }
        close(sock);
        usleep(200000);
    }
    return 0;
}