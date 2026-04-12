#ifndef ZUI_H
#define ZUI_H

#include "nuklear.h"
#include <stdlib.h>
#include <stdio.h>

// Variables de estado
static float vol_value = 0.6f;
static int brightness = 80;
static int current_resolution = 0;
static const char *res_options[] = {"1920x1080", "1280x720", "800x600"};

// Definimos los colores aquí arriba para que todas las funciones los vean
static struct nk_color dark_bg;
static struct nk_color phosphor_green;
static struct nk_color dark_green;

void zui_init_colors()
{
    dark_bg = nk_rgba(10, 15, 10, 230);
    phosphor_green = nk_rgb(51, 255, 51);
    dark_green = nk_rgb(20, 60, 20);
}

void zui_set_style(struct nk_context *ctx)
{
    zui_init_colors();

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

    // SLIDERS (Estos campos son de tipo struct nk_color, no nk_style_item)
    ctx->style.slider.bar_normal = dark_green;
    ctx->style.slider.bar_active = phosphor_green;
    ctx->style.slider.cursor_size = nk_vec2(20, 20); // Un cursor más grande y cuadrado
    ctx->style.slider.rounding = 0;                  // Sin bordes redondeados, todo cuadrado

    // TEXTO
    ctx->style.text.color = phosphor_green;
}

void zui_render(struct nk_context *ctx, int win_width, int win_height)
{
    char buffer[32];
    struct nk_command_buffer *canvas;

    if (nk_begin(ctx, "ZaramagaDock", nk_rect(0, 0, (float)win_width, (float)win_height), NK_WINDOW_NO_SCROLLBAR))
    {

        canvas = nk_window_get_canvas(ctx);

        // --- CABECERA ---
        nk_layout_row_dynamic(ctx, 40, 1);
        nk_label(ctx, " [ ZARAMAGA OS V1 ] ", NK_TEXT_CENTERED);

        // Línea divisoria (Separador)
        nk_layout_row_dynamic(ctx, 5, 1);
        struct nk_rect bounds = nk_layout_widget_bounds(ctx);
        nk_fill_rect(canvas, bounds, 0, phosphor_green);

        // --- SECCIÓN: AUDIO ---
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, ">> AUDIO_CONTROL", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 25, 2);
        nk_label(ctx, "MASTER VOL:", NK_TEXT_LEFT);
        sprintf(buffer, "%d%%", (int)(vol_value * 100));
        nk_label(ctx, buffer, NK_TEXT_RIGHT);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_slider_float(ctx, 0.0f, &vol_value, 1.0f, 0.01f);

        // --- SECCIÓN: VIDEO ---
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, ">> VIDEO_OUTPUT", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 30, 1);
        current_resolution = nk_combo(ctx, res_options, 3, current_resolution, 25, nk_vec2((float)win_width - 40, 200));

        nk_layout_row_dynamic(ctx, 25, 2);
        nk_label(ctx, "BRIGHTNESS:", NK_TEXT_LEFT);
        sprintf(buffer, "%d", brightness);
        nk_label(ctx, buffer, NK_TEXT_RIGHT);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_progress(ctx, (nk_size *)&brightness, 100, NK_MODIFIABLE);

        // --- PIE DE PÁGINA (Botones abajo) ---
        // Calculamos un espaciador dinámico
        float content_height = 320.0f;
        float spacer = (float)win_height - content_height - 120.0f;
        if (spacer > 0)
            nk_layout_row_dynamic(ctx, spacer, 1);

        nk_layout_row_dynamic(ctx, 40, 1);
        if (nk_button_label(ctx, "[ EXIT TO SHELL ]"))
        {
            system("pkill -9 gamescope");
            exit(0);
        }

        nk_layout_row_dynamic(ctx, 35, 2);
        if (nk_button_label(ctx, "REBOOT"))
            system("reboot");
        if (nk_button_label(ctx, "OFF"))
            system("poweroff");
    }
    nk_end(ctx);
}

#endif