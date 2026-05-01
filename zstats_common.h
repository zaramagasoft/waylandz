#ifndef ZSTATS_COMMON_H
#define ZSTATS_COMMON_H

#include <stdbool.h>
#include <stdint.h>

#define ZSHM_NAME "/zaramaga_stats"

typedef enum { Z_OFF = 0, Z_OK, Z_WARN, Z_FAIL } ZStatus;

typedef struct {
    // 1. Pilotos (Checklist)
    struct {
        ZStatus krnl, drm, vkn, ogl, snd, net;
    } dashboard;

    struct {
        bool mouse;
        bool keyboard;
        bool gamepad;
        int gamepad_battery; 
        char audio_out_name[64];
        int volume;
    } io;

    // 2. Telemetría
    struct {
        float cpu_load;
        float mem_used, mem_total;
        int temp_cpu, temp_gpu;
        int net_latency; // ms
    } metrics;

    // 3. Info de Sistema (Arch Rolling)
    char kernel_ver[64];
    bool headers_ok;

    // 4. Control (AQUÍ ESTÁ EL CAMBIO)
    struct {
        bool request_ping;
        uint64_t last_update; 
    } control; // <--- Ahora ya puedes usar stats->control.xxx

} ZSharedStats;

#endif