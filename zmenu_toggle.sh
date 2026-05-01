#!/bin/bash

# Buscamos si el proceso ya existe
if pgrep -x "zmenun22" > /dev/null
then
    # Si existe, lo matamos (ocultar)
    pkill -x "zmenun22"
else
    # Si no existe, entramos a la carpeta y lo lanzamos (mostrar)
    cd /home/alb/waylandz && GSK_RENDERER=cairo WAYLAND_DISPLAY=wayland-1 ./zmenun22 &
fi
