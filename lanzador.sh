#!/bin/bash

# Limpiamos posibles sockets muertos
rm -f /run/user/$(id -u)/gamescope-0

# 1. Lanzar Gamescope en el TTY (Backend DRM)
# -f: pantalla completa
# -W/H: resolución
# --: lo que va después es la app principal (Steam)
gamescope -W 1920 -H 1080 -f -e  -- steam  -gamepadui &

# 2. PAUSA DE SEGURIDAD (Los 10 segundos que pediste)
echo "ZaramagaOS: Gamescope arrancando... Esperando 10 segundos para inicializar DRM..."
sleep 10

# 3. LANZAR ZMENU (Versión Wayland)
# Forzamos que busque el socket interno de gamescope
export WAYLAND_DISPLAY=gamescope-0
export XDG_RUNTIME_DIR=/run/user/$(id -u)

if [ -S "$XDG_RUNTIME_DIR/gamescope-0" ]; then
    echo "Socket detectado. Lanzando zmenu..."
    ./zmenu
else
    echo "ERROR: Gamescope no creó el socket a tiempo."
fi
