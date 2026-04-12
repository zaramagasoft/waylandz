#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <cairo.h>
#include "wlr-layer-shell-unstable-v1.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#include "nuklear.h"

// --- GLOBALES ---
struct wl_display *display;
struct wl_compositor *compositor;
struct wl_shm *shm;
struct zwlr_layer_shell_v1 *layer_shell;
struct wl_seat *seat;
struct wl_buffer *buffer;
struct nk_context ctx;
uint32_t *shm_data_global;
int win_width = 400;
int win_height = 120;
int cur_x = 0, cur_y = 0;

// --- PROTOTIPOS ---
static void render_frame(struct wl_surface *surface);

// --- FUNCIONES DE APOYO ---
static void noop() {}

static float text_get_width(nk_handle handle, float height, const char *text, int len)
{
    return len * (height * 0.5f);
}

// --- CALLBACKS DEL RATÓN ---
static void pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
    nk_input_begin(&ctx);
}

static void pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface)
{
    nk_input_end(&ctx);
}

static void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
    cur_x = wl_fixed_to_int(x);
    cur_y = wl_fixed_to_int(y);
    nk_input_motion(&ctx, cur_x, cur_y);
    if (data) render_frame((struct wl_surface *)data);
}

// Y ahora en pointer_button usa cur_x y cur_y
static void pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    if (button == 272)
    { 
        // PASAMOS LAS COORDENADAS REALES
        nk_input_button(&ctx, NK_BUTTON_LEFT, cur_x, cur_y, state == WL_POINTER_BUTTON_STATE_PRESSED);
    }
    if (data) render_frame((struct wl_surface *)data);
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter, .leave = pointer_leave, .motion = pointer_motion, .button = pointer_button, .axis = (void *)noop, .frame = (void *)noop, .axis_source = (void *)noop, .axis_stop = (void *)noop, .axis_discrete = (void *)noop};

// --- DIBUJO ---
void draw_nuklear_to_cairo(struct nk_context *ctx, cairo_t *cr)
{
    const struct nk_command *cmd;
    // Fondo del buffer SHM totalmente transparente al inicio
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0, 0, 0, 0); // 0 en el canal Alpha
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
        case NK_COMMAND_TEXT:
        {
            const struct nk_command_text *t = (const struct nk_command_text *)cmd;
            cairo_set_source_rgba(cr, t->foreground.r / 255.0, t->foreground.g / 255.0, t->foreground.b / 255.0, t->foreground.a / 255.0);
            cairo_move_to(cr, t->x, t->y + t->height - 5);
            cairo_show_text(cr, (const char *)t->string);
        }
        break;
        }
    }
}

static void render_frame(struct wl_surface *surface)
{
    if (nk_begin(&ctx, "Zaramaga", nk_rect(0, 0, win_width, win_height), NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_dynamic(&ctx, 30, 1);
        nk_label(&ctx, "ZARAMAGA OS", NK_TEXT_CENTERED);
        nk_layout_row_dynamic(&ctx, 40, 1);
        if (nk_button_label(&ctx, "EXIT"))
        {
            printf("Saliendo...\n");
            fflush(stdout);
            system("pkill -9 gamescope");
            exit(0);
        }
    }
    nk_end(&ctx);

    cairo_surface_t *c_surf = cairo_image_surface_create_for_data((unsigned char *)shm_data_global, CAIRO_FORMAT_ARGB32, win_width, win_height, win_width * 4);
    cairo_t *cr = cairo_create(c_surf);
    draw_nuklear_to_cairo(&ctx, cr);
    cairo_destroy(cr);
    cairo_surface_destroy(c_surf);
    nk_clear(&ctx);

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage(surface, 0, 0, win_width, win_height);
    wl_surface_commit(surface);
}

// --- WAYLAND SETUP ---
static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *ls, uint32_t serial, uint32_t width, uint32_t height)
{
    zwlr_layer_surface_v1_ack_configure(ls, serial);
    render_frame((struct wl_surface *)data);
}
static const struct zwlr_layer_surface_v1_listener ls_listener = {.configure = layer_surface_configure, .closed = (void *)exit};

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
static const struct wl_registry_listener reg_listener = {global_registry_handler, NULL};

int main()
{
    display = wl_display_connect(NULL);
    struct wl_registry *reg = wl_display_get_registry(display);
    wl_registry_add_listener(reg, &reg_listener, NULL);
    wl_display_roundtrip(display);

    nk_init_default(&ctx, 0);
    struct nk_color background_neon = nk_rgba(20, 10, 25, 180); // Fondo oscuro semi-transparente
    struct nk_color neon_magenta = nk_rgb(255, 0, 255);
    struct nk_color neon_cyan = nk_rgb(0, 255, 255);

    // Aplicar colores al estilo
    ctx.style.window.fixed_background = nk_style_item_color(background_neon);
    ctx.style.window.header.normal = nk_style_item_color(nk_rgba(40, 20, 50, 200));
    ctx.style.window.header.label_normal = neon_cyan;

    // Botones con borde neón
    ctx.style.button.normal = nk_style_item_color(nk_rgba(60, 0, 60, 255));
    ctx.style.button.hover = nk_style_item_color(nk_rgba(100, 0, 100, 255));
    ctx.style.button.active = nk_style_item_color(neon_magenta);
    ctx.style.button.border_color = neon_cyan;
    ctx.style.button.border = 2.0f;
    ctx.style.button.text_normal = neon_cyan;
    ctx.style.button.text_hover = nk_rgb(255, 255, 255);
    static struct nk_user_font font;
    font.height = 14;
    font.width = text_get_width;
    nk_style_set_font(&ctx, &font);

    int size = win_width * win_height * 4;
    int fd = memfd_create("shm", MFD_CLOEXEC);
    ftruncate(fd, size);
    shm_data_global = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    buffer = wl_shm_pool_create_buffer(pool, 0, win_width, win_height, win_width * 4, WL_SHM_FORMAT_ARGB8888);
    close(fd);

    struct wl_surface *surf = wl_compositor_create_surface(compositor);
    struct zwlr_layer_surface_v1 *ls = zwlr_layer_shell_v1_get_layer_surface(layer_shell, surf, NULL, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "overlay");
    zwlr_layer_surface_v1_set_anchor(ls, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
    zwlr_layer_surface_v1_set_size(ls, win_width, win_height);
    zwlr_layer_surface_v1_add_listener(ls, &ls_listener, surf);

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