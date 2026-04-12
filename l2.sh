#!/bin/bash

# Limpiamos posibles sockets muertos
rm -f /run/user/$(id -u)/gamescope-0

# 1. Lanzar Gamescope en el TTY (Backend DRM)
# -f: pantalla completa
# -W/H: resolución
# --: lo que va después es la app principal (Steam)
gamescope -W 1920 -H 1080  -f  -- bash -c "steam -gamepadui & sleep 10; WAYLAND_DISPLAY=gamescope-0 ./zmenu"

