#include "raylib.h"
#include <stdlib.h>

// --- CONFIGURACIÓN DEL DOCK ---
#define DOCK_WIDTH 600
#define DOCK_HEIGHT 60
#define MARGIN 10

// Estructura para gestionar el estado (puedes añadir batería, cpu, etc.)
typedef struct {
    float volumen;
    bool visible;
} DockState;

int main(void) {
    // 1. Configuración de flags de Wayland y transparencia
    // FLAG_WINDOW_UNDECORATED: Quita la barra de título
    // FLAG_WINDOW_TRANSPARENT: Permite ver el fondo (necesario en Wayland)
    // FLAG_MSAA_4X_HINT: Suaviza los bordes de los botones
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_MSAA_4X_HINT);

    // Inicializamos la ventana (con Wayland nativo cargará mucho más rápido)
    InitWindow(DOCK_WIDTH, DOCK_HEIGHT, "ZaramagaDock");

    // 2. Optimización de recursos
    SetTargetFPS(20); // El secreto para que no chupe RAM/CPU sin sentido
    
    DockState state = { .volumen = 0.5f, .visible = true };

    // Bucle principal
    while (!WindowShouldClose()) {
        // --- LÓGICA DE INTERACCIÓN ---
        Vector2 mouse = GetMousePosition();
        
        // Área de la barra de volumen (relativa al dock)
        Rectangle volBar = { 200, 25, 200, 10 };

        if (CheckCollisionPointRec(mouse, volBar)) {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                state.volumen = (mouse.x - volBar.x) / volBar.width;
                if (state.volumen < 0) state.volumen = 0;
                if (state.volumen > 1) state.volumen = 1;
                // AQUÍ: Podrías llamar a un system("pactl set-sink-volume @DEFAULT_SINK@ ...")
            }
        }

        // --- DIBUJO ---
        BeginDrawing();
            ClearBackground(BLANK); // Transparencia total del framebuffer

            // 1. Cuerpo del Dock (Glassmorphism style)
            DrawRectangleRounded((Rectangle){0, 0, DOCK_WIDTH, DOCK_HEIGHT}, 0.4f, 12, Fade(BLACK, 0.8f));
            //DrawRectangleRoundedLines((Rectangle){0, 0, DOCK_WIDTH, DOCK_HEIGHT}, 0.4f, 12, 1, Fade(RAYWHITE, 0.2f));

            // 2. Logo / Indicador Zaramaga
            DrawCircle(40, 30, 18, MAROON);
            DrawText("Z", 33, 18, 25, WHITE);

            // 3. Etiqueta de Sistema
            DrawText("ZARAMAGA OS", 80, 22, 18, LIGHTGRAY);

            // 4. Widget de Volumen
            DrawText("VOL", 195, 10, 10, GRAY);
            // Fondo de la barra
            DrawRectangleRounded(volBar, 0.5f, 5, DARKGRAY);
            // Progreso de la barra (Verde neón)
            DrawRectangleRounded((Rectangle){volBar.x, volBar.y, volBar.width * state.volumen, volBar.height}, 0.5f, 5, GREEN);

            // 5. Reloj simple (opcional pero rellena bien)
            DrawText(TextFormat("%02i:%02i", GetTime() > 3600 ? 0 : 0, (int)GetTime() % 60), 520, 20, 20, RAYWHITE);

        EndDrawing();
    }

    // 3. Limpieza
    CloseWindow();

    return 0;
}