#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <cairo.h>
#include "wlr-layer-shell-unstable-v1.h"

// --- GLOBALES PARA ACCESO FÁCIL ---
struct wl_display *display;
struct wl_compositor *compositor;
struct wl_shm *shm;
struct zwlr_layer_shell_v1 *layer_shell;
struct wl_seat *seat;
struct wl_buffer *buffer;

uint32_t *shm_data_global; // El famoso dato que faltaba
int win_width = 400;
int win_height = 100;

double mouse_x = 0;
double mouse_y = 0;
int button_hover = 0;

// --- FUNCIÓN DE DIBUJO ---
void draw_menu(struct wl_surface *surface) {
    int stride = win_width * 4;
    cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data(
        (unsigned char*)shm_data_global, CAIRO_FORMAT_ARGB32, win_width, win_height, stride);
    cairo_t *cr = cairo_create(cairo_surface);
    
    // Fondo negro translúcido
    cairo_set_source_rgba(cr, 0.05, 0.05, 0.05, 0.9);
    cairo_paint(cr);

    // Lógica del color del botón (Hover)
    if (button_hover) {
        cairo_set_source_rgb(cr, 1.0, 0.3, 0.3); // Rojo claro
    } else {
        cairo_set_source_rgb(cr, 0.7, 0.1, 0.1); // Rojo oscuro
    }
    
    // Dibujar botón
    cairo_rectangle(cr, 250, 25, 120, 50);
    cairo_fill(cr);

    // Texto EXIT
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 20.0);
    cairo_move_to(cr, 285, 57);
    cairo_show_text(cr, "EXIT");

    // Título Zaramaga OS
    cairo_set_source_rgb(cr, 0.0, 0.8, 1.0);
    cairo_set_font_size(cr, 24.0);
    cairo_move_to(cr, 20, 57);
    cairo_show_text(cr, "ZARAMAGA OS");

    cairo_destroy(cr);
    cairo_surface_destroy(cairo_surface);

    // Notificar a Wayland del cambio
    if (surface) {
        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_damage(surface, 0, 0, win_width, win_height);
        wl_surface_commit(surface);
    }
}

// --- CALLBACKS DEL RATÓN ---
static void noop() {}

static void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
    struct wl_surface *surface = (struct wl_surface *)data;
    mouse_x = wl_fixed_to_double(x);
    mouse_y = wl_fixed_to_double(y);

    int was_hover = button_hover;
    button_hover = (mouse_x >= 250 && mouse_x <= 370 && mouse_y >= 25 && mouse_y <= 75);

    if (was_hover != button_hover) {
        draw_menu(surface);
    }
}

static void pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    if (button == 272 && state == 1) { // 272 = Left Click
        if (button_hover) {
            printf("ZaramagaOS: Killing Gamescope...\n");
            system("pkill gamescope");
            exit(0);
        }
    }
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = (void*)noop, .leave = (void*)noop, .motion = pointer_motion,
    .button = pointer_button, .axis = (void*)noop, .frame = (void*)noop,
    .axis_source = (void*)noop, .axis_stop = (void*)noop, .axis_discrete = (void*)noop,
};

// --- LAYER SHELL & REGISTRY ---
static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *layer_surface, uint32_t serial, uint32_t width, uint32_t height) {
    zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
    draw_menu((struct wl_surface *)data); // Primer dibujo
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_configure, .closed = (void*)exit,
};

static void global_registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    else if (strcmp(interface, wl_shm_interface.name) == 0) shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) layer_shell = wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 1);
    else if (strcmp(interface, wl_seat_interface.name) == 0) seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
}

static const struct wl_registry_listener registry_listener = { global_registry_handler, NULL };

static int create_shm_file(off_t size) {
    int fd = memfd_create("shm", MFD_CLOEXEC);
    if (fd < 0) return -1;
    ftruncate(fd, size);
    return fd;
}

int main() {
    display = wl_display_connect(NULL);
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);

    int size = win_width * win_height * 4;
    int fd = create_shm_file(size);
    shm_data_global = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    buffer = wl_shm_pool_create_buffer(pool, 0, win_width, win_height, win_width * 4, WL_SHM_FORMAT_ARGB8888);
    
    struct wl_surface *surface = wl_compositor_create_surface(compositor);
    struct zwlr_layer_surface_v1 *layer_surface = zwlr_layer_shell_v1_get_layer_surface(layer_shell, surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "overlay");

    zwlr_layer_surface_v1_set_anchor(layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
    zwlr_layer_surface_v1_set_size(layer_surface, win_width, win_height);
    //zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface, 0);
    zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener, surface);
    zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface, 1);
//
    
    if (seat) {
        struct wl_pointer *pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &pointer_listener, surface);
    }

    wl_surface_commit(surface);

    while (wl_display_dispatch(display) != -1);
    return 0;
}