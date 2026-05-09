#!/bin/bash

# Buscamos si el proceso ya existe
if pgrep -x "zmenun223" > /dev/null
then
    # Si existe, lo matamos (ocultar)
    pkill -x "zmenun223"
else
    # Si no existe, entramos a la carpeta y lo lanzamos (mostrar)
    cd /home/alb/waylanzN2 && GSK_RENDERER=cairo WAYLAND_DISPLAY=wayland-1 ./zmenun223 300 768 &
fi
