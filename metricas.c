#include "metricas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static char temp_path[256] = "";
static long last_total = 0, last_idle = 0;

// Función auxiliar para leer strings de archivos del sistema
void read_sys_file(const char *path, char *dest, int size)
{
    FILE *f = fopen(path, "r");
    if (f)
    {
        fgets(dest, size, f);
        dest[strcspn(dest, "\n")] = 0; // Quitar el salto de línea
        fclose(f);
    }
    else
    {
        strncpy(dest, "Desconocido", size);
    }
}

void metrics_init(ZMetrics *m)
{
    // 1. Modelo de CPU (de /proc/cpuinfo)
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f)
    {
        char line[256];
        while (fgets(line, sizeof(line), f))
        {
            if (strncmp(line, "model name", 10) == 0)
            {
                char *name = strchr(line, ':') + 2;
                strncpy(m->cpu_model, name, 63);
                m->cpu_model[strcspn(m->cpu_model, "\n")] = 0;
                break;
            }
        }
        fclose(f);
    }

    // 2. Placa Base (DMI)
    read_sys_file("/sys/class/dmi/id/board_name", m->mobo_name, 64);

    // 3. Gráfica (GPU) - Buscamos en PCI
    // Esto es un resumen, suele estar en /sys/class/drm/card0/device/device
    // Por simplicidad en modo texto, usamos una ruta común de identificación
    // read_sys_file("/sys/class/drm/card0/device/uevent", m->gpu_name, 64);
    // Nota: El uevent es sucio, mañana si quieres lo pulimos con libpci
    // Buscamos el nombre de la GPU en el subsistema DRM
    struct dirent *de_gpu;
    DIR *dr_gpu = opendir("/sys/class/drm");
    if (dr_gpu)
    {
        while ((de_gpu = readdir(dr_gpu)) != NULL)
        {
            // Buscamos "card0" o "card1", evitando los "renderD128"
            if (strncmp(de_gpu->d_name, "card", 4) == 0 && strlen(de_gpu->d_name) < 7)
            {
                char vendor_path[256], device_path[256];
                char vendor_id[16] = {0}, device_id[16] = {0};

                snprintf(vendor_path, sizeof(vendor_path), "/sys/class/drm/%s/device/vendor", de_gpu->d_name);
                snprintf(device_path, sizeof(device_path), "/sys/class/drm/%s/device/device", de_gpu->d_name);

                FILE *fv = fopen(vendor_path, "r");
                FILE *fd = fopen(device_path, "r");

                if (fv && fd)
                {
                    fscanf(fv, "%s", vendor_id);
                    fscanf(fd, "%s", device_id);

                    // Mapeo rápido de Vendors comunes
                    const char *v_name = "GPU";
                    if (strstr(vendor_id, "0x1002"))
                        v_name = "AMD";
                    else if (strstr(vendor_id, "0x10de"))
                        v_name = "NVIDIA";
                    else if (strstr(vendor_id, "0x8086"))
                        v_name = "Intel";

                    snprintf(m->gpu_name, 64, "%s (%s:%s)", v_name, vendor_id, device_id);
                    
                    fclose(fv);
                    fclose(fd);
                    break; // Ya tenemos la primaria
                }
                if (fv)
                    fclose(fv);
                if (fd)
                    fclose(fd);
            }
        }
        closedir(dr_gpu);
    }

    // 4. Buscar sensor de temperatura (Agnóstico)
    struct dirent *de;
    DIR *dr = opendir("/sys/class/hwmon");
    if (dr)
    {
        while ((de = readdir(dr)) != NULL)
        {
            if (de->d_name[0] == 'h')
            {
                char name_path[256], name[64];
                snprintf(name_path, sizeof(name_path), "/sys/class/hwmon/%s/name", de->d_name);
                FILE *fn = fopen(name_path, "r");
                if (fn)
                {
                    fscanf(fn, "%s", name);
                    fclose(fn);
                    if (strcmp(name, "k10temp") == 0 || strcmp(name, "coretemp") == 0)
                    {
                        snprintf(temp_path, sizeof(temp_path), "/sys/class/hwmon/%s/temp1_input", de->d_name);
                    }
                }
            }
        }
        closedir(dr);
    }
}

void metrics_update(ZMetrics *m)
{
    // RAM en GB
    FILE *f_mem = fopen("/proc/meminfo", "r");
    if (f_mem)
    {
        long total = 0, avail = 0;
        char buf[256];
        while (fgets(buf, sizeof(buf), f_mem))
        {
            if (sscanf(buf, "MemTotal: %ld", &total) == 1)
                m->mem_total_gb = total / 1024.0 / 1024.0;
            if (sscanf(buf, "MemAvailable: %ld", &avail) == 1)
            {
                m->mem_used_gb = m->mem_total_gb - (avail / 1024.0 / 1024.0);
                break;
            }
        }
        fclose(f_mem);
    }

    // CPU %
    FILE *f_stat = fopen("/proc/stat", "r");
    if (f_stat)
    {
        long u, n, s, i, iw, irq, sirq;
        fscanf(f_stat, "cpu  %ld %ld %ld %ld %ld %ld %ld", &u, &n, &s, &i, &iw, &irq, &sirq);
        fclose(f_stat);
        long cur_i = i + iw;
        long cur_t = u + n + s + i + iw + irq + sirq;
        m->cpu_usage = 100.0 * (1.0 - ((float)(cur_i - last_idle) / (float)(cur_t - last_total)));
        last_idle = cur_i;
        last_total = cur_t;
    }

    // Temp
    if (temp_path[0])
    {
        FILE *ft = fopen(temp_path, "r");
        if (ft)
        {
            int t;
            fscanf(ft, "%d", &t);
            m->temp_c = t / 1000;
            fclose(ft);
        }
    }
}