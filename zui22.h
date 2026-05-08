extern char *mi_buffer[256];
#include "zmetrics.h"

extern ZMetrics *metricasZui;

#ifndef ZUI22_H
#define ZUI22_H

#include "nuklear.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <time.h>   // Para time, localtime, strftime
#include <stdint.h> // Para uint32_t
#include <signal.h> // Para kill y SIGTERM
#include <unistd.h> // Para getpgrp
#include <string.h>
#include <sys/socket.h> // Para socket(), setsockopt(), SOL_SOCKET...
#include <sys/un.h>     // Para la estructura sockaddr_un y AF_UNIX
#include <arpa/inet.h>  // Opcional, pero ayuda con estructuras de red    // Para strcat
#define SOCKET_PATH "/tmp/zmetrics.sock"
// Variables de fecha/hora
char time_str[10];
char date_str[20];
// --- Métricas del sistema (ZaramagaOS) ---
static float sys_cpu = 0.0f;
static float sys_mem_u = 0.0f;
static float sys_mem_t = 0.0f;
static int sys_temp = 0;
struct shared_metrics
{
    float cpu;
    float mem_u;
    float mem_t;
    int temp;
};

// LA CLAVE: Esto dice "m_shared existe fuera de este archivo"
extern struct shared_metrics *m_shared;
static int show_confirm = 0; // 0: nada, 1: reboot, 2: poweroff
// Definimos los colores aquí arriba para que todas las funciones los vean
static struct nk_color dark_bg;
static struct nk_color phosphor_green;
static struct nk_color dark_green;
struct nk_style_button estilo_original;
struct nk_style_button miestilo; // ✅ Copia directa
int contador = 0;

static float vol_value = 0.6f;
// static float bright_value = 0.8f;
int logoDraw(struct nk_command_buffer *canvas, float y, float win_width, float logo_h);
int datedraw(struct nk_context *ctx, float y, float win_width);
int voldraw(struct nk_context *ctx, float y, float win_width, float middle_h);
int kernelraw(struct nk_context *ctx, float y, float win_width, float middle_h);
int metricsDraw(struct nk_context *ctx, float y, float win_width, float footer_h);
#include <errno.h>

// Copiamos tu función ganadora del cliente.c
static int zui_read_full(int sock, void *buf, size_t len)
{
    size_t off = 0;
    while (off < len)
    {
        ssize_t r = read(sock, (char *)buf + off, len - off);
        if (r <= 0)
            return 0;
        off += r;
    }
    return 1;
}

#include <errno.h>

// 🔊 VOLUMEN
static void zui_set_volume(float v)
{
    int vol = (int)(v * 100.0f);
    char cmd[64];
    snprintf(cmd, sizeof(cmd),
             "pactl set-sink-volume @DEFAULT_SINK@ %d%%", vol);
    system(cmd);
}
// --- LÓGICA DE AUDIO (ABSTRACCIÓN) ---
int GetSystemVolume()
{
    int volume = 0;
    // Este comando de pactl es estándar y muy rápido
    FILE *fp = popen("pactl get-sink-volume @DEFAULT_SINK@ | grep -Po '\\d+(?=%)' | head -n 1", "r");

    if (fp != NULL)
    {
        char buffer[16];
        if (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            volume = atoi(buffer);
        }
        pclose(fp);
    }
    return volume;
}

void UpdateVolume(int delta)
{
    vol_value = GetSystemVolume();
    vol_value += delta;
    if (vol_value < 0)
        vol_value = 0;
    if (vol_value >= 150)
        vol_value = 150;

    // Ejecución en segundo plano para no congelar el frame
    char cmd[64];
    sprintf(cmd, "pactl set-sink-volume @DEFAULT_SINK@ %d%% &", (int)(vol_value * 100));
    system(cmd);
}

void zui_init_colors()
{
    dark_bg = nk_rgba(10, 15, 10, 230);
    phosphor_green = nk_rgb(51, 255, 51);
    dark_green = nk_rgb(20, 60, 20);
}

void zui_set_style(struct nk_context *ctx)
{
    zui_init_colors();

    ctx->style.slider.bar_height = 30.0f;
    ctx->style.slider.cursor_size = nk_vec2(46, 46);

    // VENTANA
    ctx->style.window.fixed_background = nk_style_item_color(dark_bg);
    ctx->style.window.border_color = phosphor_green;
    ctx->style.window.border = 2.0f;

    // BOTONES
    ctx->style.button.normal = nk_style_item_color(dark_bg);
    ctx->style.button.hover = nk_style_item_color(dark_green);
    ctx->style.button.active = nk_style_item_color(phosphor_green);
    ctx->style.button.border_color = phosphor_green;

    // El texto del botón sí es un nk_color, no un style_item
    ctx->style.button.text_normal = phosphor_green;
    ctx->style.button.text_hover = nk_rgb(255, 255, 255);

    // SLIDERS (Corregido para ZaramagaOS)
    ctx->style.slider.bar_normal = dark_green;
    ctx->style.slider.bar_active = phosphor_green;                            // Color de la barra "rellena"
    ctx->style.slider.cursor_normal = nk_style_item_color(nk_rgb(0, 205, 0)); // 👈 EL TIRADOR
    ctx->style.slider.cursor_hover = nk_style_item_color(phosphor_green);
    ctx->style.slider.cursor_active = nk_style_item_color(phosphor_green);

    ctx->style.slider.cursor_size = nk_vec2(25, 25); // Tamaño del cuadrado
    ctx->style.slider.bar_height = 6.0f;             // Un poco más gruesa para que se vea industrial
    ctx->style.slider.rounding = 0;                  // Cuadrado puro
    // TEXTO
    ctx->style.text.color = phosphor_green;
}
void zui_render(struct nk_context *ctx, int win_width, int win_height)
{
    estilo_original = ctx->style.button; // Guardamos el estilo original del botón
    miestilo = estilo_original;          // Inicializamos mi_estilo con el original
    // printf("winheightzUI:%f \n", win_height);
    printf("zui_render %d\n", contador++);
    // fflush(stdout); // Esto te ayudará a ver cuándo se llama a zui_render
    static float last_sys_vol = -1.0f;
    // --- LÓGICA DE TIEMPO ---
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Formateamos HH:MM y la fecha (ej: 29 Abr)
    strftime(time_str, sizeof(time_str), "%H:%M", timeinfo);
    strftime(date_str, sizeof(date_str), "%d %b %Y", timeinfo);
    // strftime(date_str, sizeof(date_str), "%d %b", timeinfo);
    //////zmetrics////////////
    while (metricasZui == NULL)
    {
        printf("Esperando a que metricasZui esté disponible...\n");
        usleep(2000000); // Espera 100ms antes de volver a comprobar
    }
    printf("Métricas en zui_render: CPU=%.1f%%, RAM=%.2f/%.2fGB, Temp=%d°C\n",
           metricasZui->cpu_usage, metricasZui->mem_used_gb, metricasZui->mem_total_gb, metricasZui->temp_c);

    float sys_vol = GetSystemVolume() / 100.0f; // siempre leer sistema
    // Dentro de tu zui_render o donde leas el volumen:
    static uint32_t frame_count = 0;
    frame_count++;

    float v = vol_value;
    // --- ZONAS ---
    float logo_h = win_height * 0.15f;
    float footer_h = win_height * 0.150f;
    // printf("CCCOMOOOOwinheight:%f \n", win_height);

    float middle_h = win_height - logo_h - footer_h;
    vol_value = GetSystemVolume() / 100.0f; // Actualiza el volumen cada frame

    if (nk_begin(ctx, "ZaramagaDock",
                 nk_rect(0, 0, (float)win_width, (float)win_height),
                 NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);

        // --- POSICIONES ---
        float y = 0;
        y = logoDraw(canvas, y, win_width, logo_h);
        printf("Después de logoDraw, y = %f\n", y);
        y = kernelraw(ctx, y, win_width, middle_h);
        printf("Después de kernelraw, y = %f\n", y);
        y = datedraw(ctx, y, win_width);
        printf("Después de datedraw, y = %f\n", y);
        y = voldraw(ctx, y, win_width, middle_h);
        printf("Después de voldraw, y = %f\n", y);
        // printf("cpuZui %f\n", m_shared->cpu);

        int pos = metricsDraw(ctx, win_height - footer_h, win_width, footer_h);

        // =========================
        // 🔵 MIDDLE ZONE (debug opcional)
        // =========================
        nk_fill_rect(canvas,
                     nk_rect(0, y, win_width, middle_h),
                     0,
                     // nk_rgb(0, 0, 255));
                     nk_rgba(40, 40, 40, 20)); // 👈 ALPHA
        // nk_button_label(ctx, "\uF028"); // volumen
        float paddingM = 20.0f;
        float row_h = 30.0f;
        float start_y = y + 40;

        // columnas
        float icon_w = 30.0f;
        float label_w = 60.0f;
        float value_w = 40.0f;
        float slider_w = win_width - (paddingM * 2 + icon_w + label_w + value_w + 20);
        // float paddingM = 20.0f;
        float slider_h = 55.0f;

        paddingM = 20.0f;
        row_h = 30.0f;
        // Ajustamos start_y un poco para dejar sitio a la hora
        start_y = y + 40;

        // =========================
        // 🔴 FOOTER ZONE
        // =========================
        miestilo.text_normal = nk_rgb(255, 0, 0); // Rojo para el texto del botón
        nk_fill_rect(canvas,
                     nk_rect(0, win_height - footer_h, win_width, footer_h),
                     0,
                     // nk_rgb(40, 40, 40));

                     nk_rgba(0, 0, 40, 0)); // 👈 ALPHA

        // =========================
        // 🔴 FOOTER ZONE (UNA FILA - 3 BOTONES)
        // =========================

        // --- CÁLCULO DE MEDIDAS ---
        float padding = 10.0f; // Un pelín menos de padding para que quepan bien
        float btn_h = 25.0f;

        // Dividimos el ancho entre 3, restando los 4 huecos de padding (izq, entre-1, entre-2, der)
        float btn_w_third = (win_width - (padding * 4)) / 3;

        // Centrado vertical en el footer (una sola fila)
        float y_btn = win_height - footer_h + (footer_h - btn_h) / 2;
        // middle_h= win_height - footer_h;
        printf("Medidas middleh:%f \n", middle_h);
        printf("Medidas foother:%f \n", footer_h);
        printf("Medidas middleh:%f \n", middle_h);
        printf("winheightdESPUESLOGO:%f \n", win_height);

        // Iniciamos el layout para 3 widgets
        nk_layout_space_begin(ctx, NK_STATIC, footer_h, 3);

        // Definimos el mismo espacio de 3 huecos para ambos estados
        nk_layout_space_begin(ctx, NK_STATIC, btn_h, 3);
        int semaforo = 0;

        if (show_confirm == 0)
        {
            // --- MODO NORMAL (Los 3 iconos) ---

            // REBOOT
            nk_layout_space_push(ctx, nk_rect(padding, middle_h, btn_w_third, btn_h));
            // cambiodecolor(ctx);

            if (nk_button_label(ctx, "\uf01e"))

                show_confirm = 1;

            // ctx->style.button = estilo_original;
            //  EXIT
            nk_layout_space_push(ctx, nk_rect(padding * 2 + btn_w_third, middle_h, btn_w_third, btn_h));
            if (nk_button_label(ctx, "\uf08b"))
            {
                kill(-getpgrp(), SIGTERM);
                exit(0);
            }

            // POWER
            nk_layout_space_push(ctx, nk_rect(padding * 3 + btn_w_third * 2, middle_h, btn_w_third, btn_h));
            if (nk_button_label(ctx, "\uf011"))
                show_confirm = 2;
        }
        else
        {
            // --- MODO CONFIRMACIÓN (Ocupamos el mismo espacio pero con 2 botones anchos) ---
            // Calculamos un ancho para que dos botones ocupen lo que antes ocupaban tres
            float btn_w_confirm = (win_width - (padding * 3)) / 2;

            // BOTÓN SÍ (A la izquierda)
            nk_layout_space_push(ctx, nk_rect(padding, middle_h, btn_w_confirm, btn_h));
            const char *msg = (show_confirm == 1) ? "pc-REBOOT\uf00c" : "pc-OFF\uf00c";
            if (nk_button_label(ctx, msg))
            {
                if (show_confirm == 1)
                    system("reboot");
                else
                    system("poweroff");
                exit(0);
            }

            // BOTÓN CANCELAR (A la derecha)
            nk_layout_space_push(ctx, nk_rect(padding * 2 + btn_w_confirm, middle_h, btn_w_confirm, btn_h));
            if (nk_button_label(ctx, "Cancel\uf00d"))
            {
                show_confirm = 0;
                // Limpiamos el input para evitar que el clic "atraviese" al modo normal
                nk_input_begin(ctx);
                nk_input_end(ctx);
            }
        }

        nk_layout_space_end(ctx);
    }

    nk_end(ctx);
}
int logoDraw(struct nk_command_buffer *canvas, float y, float win_width, float logo_h)
{
    // =========================
    // 🟢 LOGO ZONE (debug opcional)
    // =========================
    nk_fill_rect(canvas,
                 nk_rect(0, y, win_width, logo_h),
                 0,
                 // nk_rgb(0, 255, 0));
                 nk_rgba(40, 40, 40, 20)); // 👈 ALPHA

    y += logo_h;
    return y;
}
int datedraw(struct nk_context *ctx, float y, float win_width)
{
    float row_height = 1.0f; // La altura que reservamos para este bloque

    // =========================
    // 🕒 BLOQUE RELOJ
    // =========================
    nk_layout_space_begin(ctx, NK_STATIC, row_height, 1);

    // Empujamos el rect en la posición 'y' actual
    nk_layout_space_push(ctx, nk_rect(5, y, win_width, row_height));

    char icoReloj[60] = " \uf017 ";
    char icoCalendario[30] = "  \uf073 ";

    strcat(icoReloj, time_str);
    strcat(icoReloj, " / ");
    strcat(icoCalendario, date_str);
    strcat(icoReloj, icoCalendario);

    nk_label(ctx, icoReloj, NK_TEXT_LEFT);

    nk_layout_space_end(ctx);

    // DEVOLVEMOS 'y' + la altura de lo que hemos dibujado
    // Así, el siguiente elemento sabrá que debe empezar más abajo.
    return (int)(y + row_height);
}
int voldraw(struct nk_context *ctx, float y, float win_width, float middle_h)
{

    // SLIDER 1
    int offset = 30;          // Un pequeño offset para que el slider no toque los bordes
    float row_height = 20.0f; // La altura que reservamos para este bloque
    float icon_w = 30.0f;
    float label_w = 60.0f;
    float value_w = 40.0f;
    float slider_w = win_width - (1 * 2 + icon_w + label_w + value_w);
    // float paddingM = 20.0f;
    float slider_h = 55.0f;
    nk_layout_space_begin(ctx, NK_STATIC, row_height, 8);
    y += offset; // Un pequeño espacio antes de empezar a dibujar el bloque

    // ICONO
    nk_layout_space_push(ctx,
                         nk_rect(0, y, icon_w, row_height * 2));
    nk_label(ctx, "\uF028", NK_TEXT_CENTERED);

    // LABEL
    nk_layout_space_push(ctx,
                         nk_rect(1 + icon_w, y, label_w, row_height * 2));
    nk_label(ctx, "VOLUME", NK_TEXT_CENTERED);

    // SLIDER
    nk_layout_space_push(ctx,
                         nk_rect(1 + icon_w + label_w, y, slider_w - offset, row_height * 2));
    if (nk_slider_float(ctx, 0.0f, &vol_value, 2.0f, 0.01f))
    {
        zui_set_volume(vol_value);
    }

    // VALOR
    char buffer[16];
    sprintf(buffer, "%d%%", (int)(vol_value * 100));

    nk_layout_space_push(ctx,
                         nk_rect(1 + icon_w + label_w + slider_w - offset, y, value_w, row_height * 2));
    nk_label(ctx, buffer, NK_TEXT_CENTERED);

    return (int)(y + row_height); // Devolvemos la posición final después de dibujar el bloque
}
int kernelraw(struct nk_context *ctx, float y, float win_width, float middle_h)
{
    float row_height = 15.0f; // La altura que reservamos para este bloque

    // =========================
    // 🕒 labelKernel
    // =========================
    nk_layout_space_begin(ctx, NK_STATIC, row_height, 1);

    // Empujamos el rect en la posición 'y' actual
    nk_layout_space_push(ctx, nk_rect(15, y, win_width * 0.75, row_height));

    nk_label(ctx, *mi_buffer, NK_TEXT_CENTERED);

    nk_layout_space_end(ctx);

    // DEVOLVEMOS 'y' + la altura de lo que hemos dibujado
    // Así, el siguiente elemento sabrá que debe empezar más abajo.
    return (int)(y + row_height);
}
int metricsDraw(struct nk_context *ctx, float y, float win_width, float footer_h)
{
    printf("Entrando a metricsDraw, footer_h = %f\n", y);
    float row_height = 20.0f; // La altura que reservamos para este bloque

    // =========================
    // 📊 BLOQUE MÉTRICAS
    // =========================
    nk_layout_space_begin(ctx, NK_STATIC, row_height, 3);

    // Empujamos el rect en la posición 'y' actual
    nk_layout_space_push(ctx, nk_rect(15, y, win_width * 0.75, row_height));
    /*   printf("Métricas en zui_render: CPU=%.1f%%, RAM=%.2f/%.2fGB, Temp=%d°C\n",
             metricasZui->cpu_usage, metricasZui->mem_used_gb, metricasZui->mem_total_gb, metricasZui->temp_c);
  */
    /* char icoReloj[60] = " \uf017 ";
    char icoCalendario[30] = "  \uf073 ";

    strcat(icoReloj, time_str);
    strcat(icoReloj, " / ");
    strcat(icoCalendario, date_str);
    strcat(icoReloj, icoCalendario);
     */
    //char metricasall[100];
    //char cpu_str[20] = "", mem_str[30] = "\uefc5 ", temp_str[30] = "\uef2b ";
    // cpu_str="CPU: %.1f%%";
    char metricasall[128]; // Asegúrate de que sea lo bastante grande

    // Formateamos todo de una sola vez
    snprintf(metricasall, sizeof(metricasall),
             "%.1f%%  %.2f/%.2fGB  %d°C",
             metricasZui->cpu_usage,
             metricasZui->mem_used_gb,
             metricasZui->mem_total_gb,
             metricasZui->temp_c);

    // Ahora Nuklear lo recibirá perfecto
    nk_label(ctx, metricasall, NK_TEXT_LEFT);
    nk_label(ctx, metricasall, NK_TEXT_LEFT);

    nk_layout_space_end(ctx);
}
#endif