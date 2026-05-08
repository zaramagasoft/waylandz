#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>

#include <poll.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <cairo.h>
#include "wlr-layer-shell-unstable-v1.h"
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <pthread.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/timerfd.h>
// --- CONFIGURACIÓN NUKLEAR ---
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_SOFTWARE_FONT

// IMPORTANTE: Definimos la implementación y cargamos la librería
#define NK_IMPLEMENTATION
#include "nuklear.h"

// AHORA definimos una guarda para que zui.h no intente re-implementar nada
#undef NK_IMPLEMENTATION
#include "zmetrics.h"
#include "zui22.h"

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
struct wl_surface *surf;
struct wl_cursor_theme *cursor_theme;
struct wl_cursor *default_cursor;
struct wl_surface *cursor_surface;

bool configured = false;
static int frame_count = 0;
uint32_t *shm_data_global;
static int retFlag = 0;
static bool needs_redraw = false;
char *mi_buffer[256];

ZMetrics datos_compartidos;
pthread_mutex_t mutex_metricas = PTHREAD_MUTEX_INITIALIZER;
bool hay_datos_nuevos = false;
#define SOCKET_PATH "/tmp/zmetrics.sock"
typedef struct
{
    char magic[4];
    unsigned char version;
    unsigned char type;
    unsigned short size;
} ZHeader;


ZMetrics *metricasZui = NULL;
bool frame_callback_pending = false; // Para evitar múltiples callbacks pendientes
struct shared_metrics *m_shared; // Variable globalz
// ojo a estudiar bien esto, es la clave para no hacer render cada vez que recibimos un configure, sino solo cuando realmente haya que redibujar
pid_t pid = -1; // Variable global al principio del archivo
int win_width = 300;
int win_height = 550;
int cur_x = 0, cur_y = 0;
static void on_frame_done(void *data, struct wl_callback *cb, uint32_t time) {
    wl_callback_destroy(cb);
    frame_callback_pending = false; // Ya podemos volver a dibujar
}

static const struct wl_callback_listener frame_listener = { .done = on_frame_done };
static int read_full(int sock, void *buf, size_t len)
{
    size_t off = 0;

    while (off < len)
    {
        ssize_t r = read(sock, (char *)buf + off, len - off);

        if (r == 0)
            return 0; // server closed

        if (r < 0)
            return -1;

        off += r;
    }

    return 1;
}
void *hilo_funcion(void *arg)
{

    int valor = *(int *)arg;
    printf("Hola desde el hilo!\n");

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        exit(1);
    }
    while (1)
    {
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            perror("connect");
            close(sock);
            sleep(1);
            continue;
        }

        ZHeader h;
        if (read_full(sock, &h, sizeof(h)) <= 0)
        {
            close(sock);
            continue;
        }

        ZMetrics m;
        if (read_full(sock, &m, sizeof(m)) > 0)
        {
            pthread_mutex_lock(&mutex_metricas);
            datos_compartidos = m; // Copiamos los datos, no el puntero
            hay_datos_nuevos = true;
            pthread_mutex_unlock(&mutex_metricas);
        }
        close(sock);
        usleep(2000000); // 2 segundos para no saturar

        metricasZui = &m; // Asignamos el puntero a la estructura ZMetrics
        close(sock);
        usleep(200000);
    }

    close(sock);
    // return 0;
}
void handle_vol_signal(int sig)
{
    needs_redraw = true; // ← seguro, solo escribe un bool

 
}

char *kernelinfo(char *buffer, size_t size)
{
    struct utsname u;

    if (uname(&u) == 0)
    {
        snprintf(buffer, size, "Kernel: %s %s", u.sysname, u.release);
        return buffer;
    }
    else
    {
        snprintf(buffer, size, "Kernel: Unknown");
        return buffer;
    }
}

int refesco(struct wl_surface *surf);
int wayinit(int win_width, int win_height, int *retFlag);
void start_zui_metrics_monitor()
{
    pid_t m_pid = fork();
    if (m_pid < 0)
        return;

    if (m_pid == 0) // HIJO
    {
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        signal(SIGUSR1, SIG_IGN);

        FILE *fp = popen("./zmetrics-client", "r");
        if (!fp)
            exit(1);

        char linea[1024];
        while (fgets(linea, sizeof(linea), fp) != NULL)
        {
            // 1. CPU -> a la memoria compartida
            if (strstr(linea, "CPU:"))
            {
                if (sscanf(strstr(linea, "CPU:"), "CPU: %f", &m_shared->cpu) == 1)
                { // <--- CAMBIO AQUÍ
                    printf("Z-DEBUG: CPU EN SHARED -> %.1f\n", m_shared->cpu);
                }
            }

            // 2. RAM -> a la memoria compartida
            if (strstr(linea, "RAM:"))
            {
                if (sscanf(strstr(linea, "RAM:"), "RAM: %f / %f", &m_shared->mem_u, &m_shared->mem_t) == 2)
                { // <--- CAMBIO AQUÍ
                    printf("Z-DEBUG: RAM EN SHARED -> %.2f\n", m_shared->mem_u);
                }
            }

            // 3. TEMP -> a la memoria compartida
            if (strstr(linea, "TEMP:"))
            {
                if (sscanf(strstr(linea, "TEMP:"), "TEMP: %d", &m_shared->temp) == 1)
                { // <--- CAMBIO AQUÍ
                    printf("Z-DEBUG: TEMP EN SHARED -> %d. Avisando...\n", m_shared->temp);
                    kill(getppid(), SIGUSR1);
                    usleep(2000000);
                }
            }
        }
        pclose(fp);
        exit(0);
    }
}
void start_zui_monitor()
{
    pid = fork();
    if (pid < 0)
        return;

    if (pid == 0)
    {
#include <sys/prctl.h>
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        // HIJO: Solo vigila y avisa
        signal(SIGUSR1, SIG_IGN);
        FILE *fp = popen("pactl subscribe", "r");
        if (!fp)
            exit(1);

        char linea[1024];
        while (fgets(linea, sizeof(linea), fp) != NULL)
        {
            if (strstr(linea, "change") && strstr(linea, "sink"))
            {
                printf("ZaramagaOS: Volumen cambiado, avisando al padre...\n");
                // EL CODAZO: Avisa al padre para que se despierte
                kill(getppid(), SIGUSR1);
                usleep(500000);
            }
        }
        pclose(fp);
        exit(0);
    }
    // PADRE: Continúa su ejecución normal
}

void draw_logo_shm(cairo_t *cr,
                   int x, int y,
                   int max_w, int max_h)
{
    int w, h, channels;
    unsigned char *pixels = stbi_load_from_memory(
        zaramagaos_png, zaramagaos_png_len,
        &w, &h, &channels, 4);

    if (!pixels)
        return;

    // --- RGBA → BGRA premultiplicado ---
    for (int i = 0; i < w * h * 4; i += 4)
    {
        unsigned char r = pixels[i];
        unsigned char g = pixels[i + 1];
        unsigned char b = pixels[i + 2];
        unsigned char a = pixels[i + 3];

        pixels[i] = (b * a) / 255;
        pixels[i + 1] = (g * a) / 255;
        pixels[i + 2] = (r * a) / 255;
        pixels[i + 3] = a;
    }

    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
    cairo_surface_t *surf =
        cairo_image_surface_create_for_data(
            pixels, CAIRO_FORMAT_ARGB32, w, h, stride);

    // 🔥 escala proporcional (clave)
    float scale_x = (float)max_w / (float)w;
    float scale_y = (float)max_h / (float)h;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;

    cairo_save(cr);

    cairo_translate(cr, x, y);
    cairo_scale(cr, scale, scale);

    cairo_set_source_surface(cr, surf, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_GOOD);
    cairo_paint(cr);

    cairo_restore(cr);

    cairo_surface_destroy(surf);
    stbi_image_free(pixels);
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
        { /*
             const struct nk_command_text *t = (const struct nk_command_text *)cmd;
             cairo_set_source_rgba(cr, t->foreground.r / 255.0, t->foreground.g / 255.0, t->foreground.b / 255.0, t->foreground.a / 255.0);
             cairo_move_to(cr, t->x, t->y + t->height - 5);
             cairo_show_text(cr, (const char *)t->string); */
            const struct nk_command_text *t = (const struct nk_command_text *)cmd;

            cairo_set_source_rgba(cr,
                                  t->foreground.r / 255.0,
                                  t->foreground.g / 255.0,
                                  t->foreground.b / 255.0,
                                  t->foreground.a / 255.0);

            // 👇 AQUÍ eliges la fuente
            cairo_select_font_face(cr, "3270 Nerd Font Propo",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_NORMAL);

            cairo_set_font_size(cr, t->height);

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
        case NK_COMMAND_CIRCLE_FILLED:
        {
            const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;

            cairo_set_source_rgba(cr,
                                  c->color.r / 255.0,
                                  c->color.g / 255.0,
                                  c->color.b / 255.0,
                                  c->color.a / 255.0);

            cairo_arc(cr,
                      c->x + c->w / 2.0,
                      c->y + c->h / 2.0,
                      c->w / 2.0,
                      0, 2 * 3.1416);

            cairo_fill(cr);
        }
        break;
        }
    }
    // draw_logo_shm(cr, 90, 10);
    int logo_height = win_height * 0.15f; // 15% arriba

    draw_logo_shm(
        cr,
        (win_width / 2 + (logo_height / 3)) - (logo_height), // Centrado horizontalmente
        0,
        win_width,
        logo_height);
}

// --- RENDERIZADO ---
static void render_frame(struct wl_surface *surface)
{
    static struct timespec last_draw = {0, 0};
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // Calculamos la diferencia en milisegundos
    long delta_ms = (now.tv_sec - last_draw.tv_sec) * 1000 + 
                    (now.tv_nsec - last_draw.tv_nsec) / 1000000;

    // --- EL FILTRO ZARAMAGA ---
    // Si han pasado menos de 33ms (aprox 30 FPS), ignoramos el render
    // Puedes subirlo a 100ms si quieres que sea aún más ahorrador
    if (delta_ms < 200) {
        return; 
    }
    last_draw = now;
    zui_render(&ctx, win_width, win_height);
    needs_redraw = false; // 🔥 IMPORTANTE: Solo renderizamos cuando realmente haya que hacerlo
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
    // 🔥 LA MAGIA: Pedimos el callback antes del commit
    struct wl_callback *cb = wl_surface_frame(surface);
    wl_callback_add_listener(cb, &frame_listener, NULL);
    frame_callback_pending = true; 

    wl_surface_commit(surface);
    needs_redraw = false;
}

static float text_get_width(nk_handle handle, float height, const char *text, int len)
{
    return len * (height * 0.55f);
}

static void pointer_motion(void *data, struct wl_pointer *ptr, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    cur_x = wl_fixed_to_int(x);
    cur_y = wl_fixed_to_int(y);
    
    // 1. Informamos a Nuklear de la nueva posición
    nk_input_motion(&ctx, cur_x, cur_y);

    // 2. Comprobamos si el ratón está sobre algo que Nuklear reconozca
    // Esto evita que redibujes cuando el ratón está en el "espacio vacío"
    if (nk_window_is_any_hovered(&ctx)) {
        needs_redraw = true; 
    }else {
        needs_redraw = false; // No hay interacción, no redibujamos
    }
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
    printf("winheight en layer configre antes:%f \n", win_height);

    win_width = width;
    win_height = height;
    //needs_redraw = true;
    // ESTO ES VITAL: Sin esto el padre nunca dibujará
    configured = true;
    printf(" despues layer configrewinheight:%f \n", win_height);

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

int main(int argc, char **argv)
{

    int win_width = 300;
    int win_height = 1080;

    if (argc >= 3)
    {
        win_width = atoi(argv[1]);
        win_height = atoi(argv[2]);
    }
    signal(SIGUSR1, handle_vol_signal);
    
    m_shared = mmap(NULL, sizeof(struct shared_metrics), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (m_shared == MAP_FAILED)
    {
        perror("mmap falló");
        exit(1);
    }
    char su_buffer[256];
    //[[[[[[[[[printf("%s\n", kernelinfo(su_buffer, sizeof(su_buffer)));
    // --- 2. LANZAR EL MONITOR (FORK) ---
    *mi_buffer = kernelinfo(su_buffer, sizeof(su_buffer));
    // monitor_pactl_simple(); // Iniciamos el monitor de volumen en un proceso aparte
    start_zui_monitor();
    // start_zui_metrics_monitor(); // Iniciamos el monitor de volumen en un proceso aparte
    //  --- CONEXIÓN WAYLAND ---
    int retVal = wayinit(win_width, win_height, &retFlag);
    if (retFlag == 1)
        return retVal;

    // --- EL BUCLE DE ACERO (30 FPS) ---
    printf("ZawayinitramagaOS: Motor de refresco sólido iniciado.\n");
    // iniciohilo
    pthread_t hilo; // Declaramos la variable del hilo
    int valor = 42; // Valor que pasaremos a la función

    // Creamos el hilo, pasándole la función y el argumento
    if (pthread_create(&hilo, NULL, hilo_funcion, &valor))
    {
        fprintf(stderr, "Error creando el hilo\n");
        return 1;
    }
    int frame_count = refesco(surf);

    return 0;
}
int wayinit(int win_width, int win_height, int *retFlag)
{

    *retFlag = 1;
    display = wl_display_connect(NULL);
    if (!display)
        return 1;

    struct wl_registry *reg = wl_display_get_registry(display);
    static const struct wl_registry_listener rl = {global_registry_handler, NULL};
    wl_registry_add_listener(reg, &rl, NULL);
    wl_display_roundtrip(display);

    // --- INICIALIZAR NUKLEAR & FUENTES ---
    nk_init_default(&ctx, 0);
    zui_set_style(&ctx);

    struct nk_font_atlas atlas;
    int w, h;
    nk_font_atlas_init_default(&atlas);
    nk_font_atlas_begin(&atlas);
    struct nk_font *jetbrains = nk_font_atlas_add_from_file(&atlas, "JetBrainsMonoNerdFont-Regular.ttf", 18, NULL);
    nk_font_atlas_bake(&atlas, &w, &h, NK_FONT_ATLAS_ALPHA8);
    nk_font_atlas_end(&atlas, nk_handle_id(0), NULL);

    if (jetbrains)
    {
        nk_style_set_font(&ctx, &jetbrains->handle);
    }

    // --- CONFIGURAR SUPERFICIE & LAYER SHELL ---
    surf = wl_compositor_create_surface(compositor);

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
    zwlr_layer_surface_v1_set_size(ls, win_width, win_height);
    zwlr_layer_surface_v1_add_listener(ls, &lsl, surf);
    struct wl_cursor_theme *cursor_theme;
    struct wl_cursor *default_cursor;
    struct wl_surface *cursor_surface;
    if (seat)
    {
        struct wl_pointer *ptr = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(ptr, &pointer_listener, surf);
    }

    wl_surface_commit(surf);
    // render_frame(surf);
    *retFlag = 0;
    return 0;
}
int refesco(struct wl_surface *surf)
{
    printf("ZaramagaOS: Motor de refresco optimizado (CPU 0%%).\n");
    fflush(stdout);

    while (1)
    {
        while (wl_display_prepare_read(display) != 0)
        {
            wl_display_dispatch_pending(display);
        }
        wl_display_flush(display);

        struct pollfd pfd = {.fd = wl_display_get_fd(display), .events = POLLIN};

        // --- CAMBIO AQUÍ: Cálculo del Timeout para el Reloj ---
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        // Despertar cada minuto (60s - segundos actuales)
        // Añadimos 500ms de margen para asegurar que el sistema ya cambió el minuto
        int timeout_ms = ((60 - (now.tv_sec % 60)) * 1000) + 500;

        int ret = poll(&pfd, 1, timeout_ms);

        if (ret == 0)
        {
            // ¡TIMEOUT! Ha pasado un minuto.
            wl_display_cancel_read(display);
            needs_redraw = true; // Forzamos el render para actualizar la hora
        }
        else if (ret < 0)
        {
            if (errno == EINTR)
            {
                wl_display_cancel_read(display);
                if (configured)
                    render_frame(surf);
                continue;
            }
            wl_display_cancel_read(display);
            break;
        }
        else if (pfd.revents & POLLIN)
        {
            wl_display_read_events(display);
        }
        else
        {
            wl_display_cancel_read(display);
        }

        wl_display_dispatch_pending(display);
        if (configured && needs_redraw)
        {
            render_frame(surf);
            needs_redraw = false;
        }
    }
    return 0;
}