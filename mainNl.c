#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <cairo.h>
#include "wlr-layer-shell-unstable-v1.h"

// --- CONFIGURACIÓN NUKLEAR ---
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

// IMPORTANTE: Definimos la implementación y cargamos la librería
#define NK_IMPLEMENTATION
#include "nuklear.h"

// AHORA definimos una guarda para que zui.h no intente re-implementar nada
#undef NK_IMPLEMENTATION

#include "zui.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Tienes que bajar este .h y ponerlo en tu carpeta
#include "logo_data.h"

// ... resto de tu código (globales, render_frame, etc.) ...

// ... resto de tus variables globales y funciones (draw_nuklear_to_cairo, etc) ...

// --- GLOBALES ---
struct wl_display *display;
struct wl_compositor *compositor;
struct wl_shm *shm;
struct zwlr_layer_shell_v1 *layer_shell;
struct wl_seat *seat;
struct wl_buffer *buffer;
struct nk_context ctx;

uint32_t *shm_data_global;
int win_width = 300;
int win_height = 1080;
int cur_x = 0, cur_y = 0;

void draw_logo_shm(cairo_t *cr, int x, int y)
{
    int w, h, channels;
    unsigned char *pixels = stbi_load_from_memory(zaramagaos_png, zaramagaos_png_len, &w, &h, &channels, 4);

    if (pixels)
    {
        // --- ARREGLO DE PIXELES (RGBA -> BGRA Premultiplicado) ---
        for (int i = 0; i < w * h * 4; i += 4)
        {
            unsigned char r = pixels[i];
            unsigned char g = pixels[i + 1];
            unsigned char b = pixels[i + 2];
            unsigned char a = pixels[i + 3];

            // Cairo ARGB32 usa BGRA en memoria en sistemas Little Endian (x86)
            // Y necesita premultiplicación (color * alpha / 255)
            pixels[i] = (unsigned char)((b * a) / 255);
            pixels[i + 1] = (unsigned char)((g * a) / 255);
            pixels[i + 2] = (unsigned char)((r * a) / 255);
            pixels[i + 3] = a;
        }

        int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
        cairo_surface_t *surf = cairo_image_surface_create_for_data(
            pixels, CAIRO_FORMAT_ARGB32, w, h, stride);

        cairo_save(cr);
        cairo_translate(cr, x, y);

        float escala = 100.0f / (float)w;
        cairo_scale(cr, escala, escala);

        cairo_set_source_surface(cr, surf, 0, 0);
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_GOOD);
        cairo_paint(cr);
        cairo_restore(cr);

        cairo_surface_destroy(surf);
        stbi_image_free(pixels);
    }
}

// --- TRADUCTOR NUKLEAR A CAIRO ---
void draw_nuklear_to_cairo(struct nk_context *ctx, cairo_t *cr)
{
    const struct nk_command *cmd;
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    nk_foreach(cmd, ctx)
    {
        switch (cmd->type)
        {
        case NK_COMMAND_RECT_FILLED:
        {
            const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
            cairo_set_source_rgba(cr, r->color.r / 255.0, r->color.g / 255.0, r->color.b / 255.0, r->color.a / 255.0);
            cairo_rectangle(cr, r->x, r->y, r->w, r->h);
            cairo_fill(cr);
        }
        break;
        case NK_COMMAND_RECT:
        {
            const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
            cairo_set_source_rgba(cr, r->color.r / 255.0, r->color.g / 255.0, r->color.b / 255.0, r->color.a / 255.0);
            cairo_set_line_width(cr, r->line_thickness);
            cairo_rectangle(cr, r->x, r->y, r->w, r->h);
            cairo_stroke(cr);
        }
        break;
        case NK_COMMAND_TEXT:
        {
            const struct nk_command_text *t = (const struct nk_command_text *)cmd;
            cairo_set_source_rgba(cr, t->foreground.r / 255.0, t->foreground.g / 255.0, t->foreground.b / 255.0, t->foreground.a / 255.0);
            cairo_move_to(cr, t->x, t->y + t->height - 5);
            cairo_show_text(cr, (const char *)t->string);
        }
        break;
        case NK_COMMAND_SCISSOR:
        {
            const struct nk_command_scissor *s = (const struct nk_command_scissor *)cmd;
            cairo_reset_clip(cr);
            cairo_rectangle(cr, s->x, s->y, s->w, s->h);
            cairo_clip(cr);
        }
        break;
        }
    }
    draw_logo_shm(cr, 90, 10);
}

// --- RENDERIZADO ---
static void render_frame(struct wl_surface *surface)
{
    zui_render(&ctx, win_width, win_height);

    cairo_surface_t *c_surf = cairo_image_surface_create_for_data(
        (unsigned char *)shm_data_global, CAIRO_FORMAT_ARGB32, win_width, win_height, win_width * 4);
    cairo_t *cr = cairo_create(c_surf);

    draw_nuklear_to_cairo(&ctx, cr);

    cairo_destroy(cr);
    cairo_surface_destroy(c_surf);
    nk_clear(&ctx);

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage(surface, 0, 0, win_width, win_height);
    wl_surface_commit(surface);
}

static float text_get_width(nk_handle handle, float height, const char *text, int len)
{
    return len * (height * 0.55f);
}

static void pointer_motion(void *data, struct wl_pointer *ptr, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    cur_x = wl_fixed_to_int(x);
    cur_y = wl_fixed_to_int(y);
    nk_input_motion(&ctx, cur_x, cur_y);
    render_frame((struct wl_surface *)data);
}
static void noop() {}

static void pointer_button(void *data, struct wl_pointer *ptr, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    if (button == 272)
        nk_input_button(&ctx, NK_BUTTON_LEFT, cur_x, cur_y, state == WL_POINTER_BUTTON_STATE_PRESSED);
    render_frame((struct wl_surface *)data);
}
// 1. Crea estas funciones de apoyo para que no den problemas
static void pointer_enter(void *data, struct wl_pointer *ptr, uint32_t serial, struct wl_surface *surf, wl_fixed_t x, wl_fixed_t y)
{
    // No hacemos nada raro aquí, Nuklear se enterará en el próximo frame
}

static void pointer_leave(void *data, struct wl_pointer *ptr, uint32_t serial, struct wl_surface *surf)
{
    nk_input_begin(&ctx);
    nk_input_end(&ctx); // Esto limpia el estado de los botones al salir
}

// 2. Actualiza el listener
static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = (void *)noop, // Usa una función vacía, no 'free' ni 'noop' de sistema
    .frame = (void *)noop,
    .axis_source = (void *)noop,
    .axis_stop = (void *)noop,
    .axis_discrete = (void *)noop};

static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *ls, uint32_t serial, uint32_t width, uint32_t height)
{
    zwlr_layer_surface_v1_ack_configure(ls, serial);
    win_width = width;
    win_height = height;
    render_frame((struct wl_surface *)data);
}

static void global_registry_handler(void *data, struct wl_registry *reg, uint32_t id, const char *interface, uint32_t version)
{
    if (!strcmp(interface, wl_compositor_interface.name))
        compositor = wl_registry_bind(reg, id, &wl_compositor_interface, 1);
    else if (!strcmp(interface, wl_shm_interface.name))
        shm = wl_registry_bind(reg, id, &wl_shm_interface, 1);
    else if (!strcmp(interface, zwlr_layer_shell_v1_interface.name))
        layer_shell = wl_registry_bind(reg, id, &zwlr_layer_shell_v1_interface, 1);
    else if (!strcmp(interface, wl_seat_interface.name))
        seat = wl_registry_bind(reg, id, &wl_seat_interface, 1);
}

int main()
{
    display = wl_display_connect(NULL);
    struct wl_registry *reg = wl_display_get_registry(display);
    static const struct wl_registry_listener rl = {global_registry_handler, NULL};
    wl_registry_add_listener(reg, &rl, NULL);
    wl_display_roundtrip(display);

    nk_init_default(&ctx, 0);
    zui_set_style(&ctx);
    static struct nk_user_font font;
    font.height = 18;
    font.width = text_get_width;
    nk_style_set_font(&ctx, &font);

    struct wl_surface *surf = wl_compositor_create_surface(compositor);
    int size = win_width * win_height * 4;
    int fd = memfd_create("shm", MFD_CLOEXEC);
    ftruncate(fd, size);
    shm_data_global = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    buffer = wl_shm_pool_create_buffer(pool, 0, win_width, win_height, win_width * 4, WL_SHM_FORMAT_ARGB8888);
    close(fd);

    struct zwlr_layer_surface_v1 *ls = zwlr_layer_shell_v1_get_layer_surface(layer_shell, surf, NULL, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "dock");
    static const struct zwlr_layer_surface_v1_listener lsl = {layer_surface_configure, (void *)exit};
    zwlr_layer_surface_v1_set_anchor(ls, ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);
    zwlr_layer_surface_v1_set_size(ls, win_width, 0);
    zwlr_layer_surface_v1_add_listener(ls, &lsl, surf);

    if (seat)
    {
        struct wl_pointer *ptr = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(ptr, &pointer_listener, surf);
    }

    wl_surface_commit(surf);
    while (wl_display_dispatch(display) != -1)
        ;
    return 0;
}