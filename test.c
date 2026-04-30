#include "metricas.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    ZMetrics m;
    metrics_init(&m);
    
    printf("\033[2J\033[H"); // Limpiar pantalla
    printf("ZaramagaOS Hardware Info\n");
    printf("------------------------\n");
    printf("CPU:   %s\n", m.cpu_model);
    printf("PLACA: %s\n", m.mobo_name);
    printf("GPU:   %s\n", m.gpu_name);
    printf("------------------------\n");

    while(1) {
        metrics_update(&m);
        // \r para sobrescribir la línea
        printf("\rCPU: %.1f%% | RAM: %.2f/%.1f GB | TEMP: %d°C    ", 
               m.cpu_usage, m.mem_used_gb, m.mem_total_gb, m.temp_c);
        fflush(stdout);
        sleep(2); // Tu refresco de 2 segundos
    }
    return 0;
}