#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/menuos.sock"
int main()
{
    int server_fd;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0)
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
    unlink(SOCKET_PATH);

    if (bind(server_fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0)
    {
        perror("bind");
        exit(1);
    }
    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        exit(1);
    }
    printf("Esperando cliente...\n");

    int client_fd;

    client_fd = accept(server_fd, NULL, NULL);
    printf("Cliente conectado!\n");
    char buffer[256];
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));

        int n = read(client_fd,
                     buffer,
                     sizeof(buffer));
        if (n <= 0)
            break;
        printf("Cliente: %s\n", buffer);
        char response[256]= "Recibido... ";
        write(client_fd,
              response,
              strlen(response)-1);
    }
    close(client_fd);
    close(server_fd);

    unlink(SOCKET_PATH);

    return 0;
}