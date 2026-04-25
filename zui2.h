#ifndef ZUI_H
#define ZUI_H

#include "nuklear.h"
#include <stdlib.h>
#include <stdio.h>

// Variables de estado
// static float vol_value = 0.6f;
// static int brightness = 80;
// static int current_resolution = 0;
// static const char *res_options[] = {"1920x1080", "1280x720", "800x600"};

// Definimos los colores aquí arriba para que todas las funciones los vean
static struct nk_color dark_bg;
static struct nk_color phosphor_green;
static struct nk_color dark_green;

static float vol_value = 0.6f;
// static float bright_value = 0.8f;

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
/* // 💡 BRILLO
static void zui_set_brightness(float v)
{
    int percent = (int)(v * 100.0f);
    char cmd[64];
    snprintf(cmd, sizeof(cmd),
             "brightnessctl set %d%%", percent);
    system(cmd);
} */

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

    /*  // SLIDERS (Estos campos son de tipo struct nk_color, no nk_style_item)
     ctx->style.slider.bar_normal = dark_green;
     ctx->style.slider.bar_active = phosphor_green;
     ctx->style.slider.cursor_size = nk_vec2(20, 20); // Un cursor más grande y cuadrado
     ctx->style.slider.rounding = 0;                  // Sin bordes redondeados, todo cuadrado
  */
    // SLIDERS (Corregido para ZaramagaOS)
    ctx->style.slider.bar_normal = dark_green;
    ctx->style.slider.bar_active = phosphor_green;        // Color de la barra "rellena"
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
    static float last_sys_vol = -1.0f;

    float sys_vol = GetSystemVolume() / 100.0f; // siempre leer sistema
    static int skip_frames = 0;
    if (skip_frames++ % 5 == 0)
    { // Solo lee el volumen real cada ~0.5 segundos
        vol_value = GetSystemVolume() / 100.0f;
    }

    // --- ZONAS ---
    float logo_h = win_height * 0.15f;
    float footer_h = win_height * 0.20f;
    float middle_h = win_height - logo_h - footer_h;
    vol_value = GetSystemVolume() / 100.0f; // Actualiza el volumen cada frame

    if (nk_begin(ctx, "ZaramagaDock",
                 nk_rect(0, 0, (float)win_width, (float)win_height),
                 NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);

        // --- POSICIONES ---
        float y = 0;

        // =========================
        // 🟢 LOGO ZONE (debug opcional)
        // =========================
        nk_fill_rect(canvas,
                     nk_rect(0, y, win_width, logo_h),
                     0,
                     // nk_rgb(0, 255, 0));
                     nk_rgba(40, 40, 40, 20)); // 👈 ALPHA

        y += logo_h;

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

        // SLIDER 1
        nk_layout_space_begin(ctx, NK_STATIC, middle_h, 8);

        // ICONO
        nk_layout_space_push(ctx,
                             nk_rect(paddingM, start_y, icon_w, row_h * 2));
        nk_label(ctx, "\uF028", NK_TEXT_CENTERED);

        // LABEL
        nk_layout_space_push(ctx,
                             nk_rect(paddingM + icon_w, start_y, label_w, row_h * 2));
        nk_label(ctx, "VOLUME", NK_TEXT_LEFT);

        // SLIDER
        nk_layout_space_push(ctx,
                             nk_rect(paddingM + icon_w + label_w + 10, start_y, slider_w, row_h * 2));
        if (nk_slider_float(ctx, 0.0f, &vol_value, 2.0f, 0.01f))
        {
            zui_set_volume(vol_value);
        }

        // VALOR
        char buffer[16];
        sprintf(buffer, "%d%%", (int)(vol_value * 100));

        nk_layout_space_push(ctx,
                             nk_rect(win_width - paddingM - value_w, start_y, value_w, row_h * 2));
        nk_label(ctx, buffer, NK_TEXT_RIGHT);

        /*  // SLIDER 2
         start_y += row_h + 20;

         // ICONO
         nk_layout_space_push(ctx,
                              nk_rect(paddingM, start_y, icon_w, row_h));
         nk_label(ctx, "\uF185", NK_TEXT_CENTERED);

         // LABEL
         nk_layout_space_push(ctx,
                              nk_rect(paddingM + icon_w, start_y, label_w, row_h));
         nk_label(ctx, "BRIGHT", NK_TEXT_LEFT);

         // SLIDER
         nk_layout_space_push(ctx,
                              nk_rect(paddingM + icon_w + label_w + 10, start_y, slider_w, row_h));
         if (nk_slider_float(ctx, 0.0f, &bright_value, 1.0f, 0.01f))
         {
             zui_set_brightness(bright_value);
         }

         // VALOR
         sprintf(buffer, "%d%%", (int)(bright_value * 100));

         nk_layout_space_push(ctx,
                              nk_rect(win_width - paddingM - value_w, start_y, value_w, row_h));
         nk_label(ctx, buffer, NK_TEXT_RIGHT);

         nk_layout_space_end(ctx); */

        // =========================
        // 🔴 FOOTER ZONE
        // =========================
        nk_fill_rect(canvas,
                     nk_rect(0, y, win_width, footer_h),
                     0,
                     // nk_rgb(40, 40, 40));

                     nk_rgba(40, 40, 40, 20)); // 👈 ALPHA

        // --- BOTONES ---
        float padding = 15.0f;
        float btn_h = 40.0f;

        float btn_w_full = win_width - (padding * 2);
        float btn_w_half = (win_width - (padding * 3)) / 2;

        // centrado vertical dentro del footer
        float total_h = (btn_h * 2) + 10;
        float y_btn = y + (footer_h - total_h) / 2;

        nk_layout_space_begin(ctx, NK_STATIC, footer_h, 3);

        // EXIT
        nk_layout_space_push(ctx,
                             nk_rect(padding, y_btn, btn_w_full, btn_h));

        if (nk_button_label(ctx, "[ EXIT ]"))
        {
            exit(0);
        }

        // REBOOT / OFF
        y_btn += btn_h + 10;

        nk_layout_space_push(ctx,
                             nk_rect(padding, y_btn, btn_w_half, btn_h));

        nk_button_label(ctx, "REBOOT");

        nk_layout_space_push(ctx,
                             nk_rect(padding * 2 + btn_w_half, y_btn, btn_w_half, btn_h));

        nk_button_label(ctx, "OFF");

        nk_layout_space_end(ctx);
    }

    nk_end(ctx);
}
#endif