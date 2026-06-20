#include <stdio.h>
#include <stdlib.h>
#include <string.h> // <-- ESTA ES LA QUE FALTABA
#include <wayland-client.h>
#include "gamma-control-client-protocol.h"

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    if (strcmp(interface, "zwlr_gamma_control_manager_v1") == 0) {
        printf("¡Éxito! Gestor de gamma encontrado en el nombre: %u\n", name);
    }
}

static const struct wl_registry_listener registry_listener = { .global = registry_handle_global, .global_remove = NULL };

int main() {
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "No se pudo conectar a Wayland\n");
        return 1;
    }
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    
    // roundtrip es fundamental: solicita al compositor que envíe todos los eventos pendientes
    wl_display_roundtrip(display); 
    
    wl_display_disconnect(display);
    return 0;
}