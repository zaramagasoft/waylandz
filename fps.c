#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <xf86drm.h>
#include <bits/fcntl-linux.h>
#include <asm-generic/fcntl.h>

int main() {
    // Abrimos el dispositivo de la GPU (agnóstico para cualquier GPU)
    int fd = open("/dev/dri/card1", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        perror("Error al abrir /dev/dri/card1");
        return 1;
    }

    drmVBlank vbl;
    uint64_t last_sequence;
    struct timeval start, end;

    // Obtener el punto de partida
    vbl.request.type = DRM_VBLANK_RELATIVE;
    vbl.request.sequence = 0;
    if (drmWaitVBlank(fd, &vbl) != 0) {
        perror("drmWaitVBlank falló");
        close(fd);
        return 1;
    }
    
    last_sequence = vbl.reply.sequence;
    gettimeofday(&start, NULL);

    printf("Midiendo FPS del sistema (VBlank)... Presiona Ctrl+C para salir.\n");

    while (1) {
        // Esperamos 1 segundo
        sleep(1);

        // Pedimos el contador actual
        vbl.request.type = DRM_VBLANK_RELATIVE;
        vbl.request.sequence = 0;
        drmWaitVBlank(fd, &vbl);

        uint64_t current_sequence = vbl.reply.sequence;
        gettimeofday(&end, NULL);

        // Calculamos el tiempo exacto transcurrido
        double elap = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        
        // FPS = Diferencia de frames / tiempo transcurrido
        double fps = (current_sequence - last_sequence) / elap;

        printf("\rFPS Reales: %.2f (Sec: %llu)", fps, current_sequence);
        fflush(stdout);

        last_sequence = current_sequence;
        start = end;
    }

    close(fd);
    return 0;
}