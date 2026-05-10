#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/menuos.sock"

int main()
{
    int sock;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;

    strncpy(addr.sun_path,
            SOCKET_PATH,
            sizeof(addr.sun_path) - 1);

    if (connect(sock,
                (struct sockaddr *)&addr,
                sizeof(addr)) < 0)
    {
        perror("connect");
        exit(1);
    }

    printf("Conectado al servidor\n");

    char buffer[256];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));

        printf("Mensaje: ");

        fgets(buffer, sizeof(buffer), stdin);

        if (strncmp(buffer, "exit", 4) == 0)
            break;

        write(sock,
              buffer,
              strlen(buffer));

        memset(buffer, 0, sizeof(buffer));

        int n = read(sock,
                     buffer,
                     sizeof(buffer));

        if (n <= 0)
        {
            printf("Servidor desconectado\n");
            break;
        }

        printf("Servidor: %s\n", buffer);
    }

    close(sock);

    return 0;
}