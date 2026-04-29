#!/bin/bash
PROCESO="zmenun21"

# Usamos -E para expresiones regulares y buscamos 'change' seguido de 'sink'
pactl subscribe | grep --line-buffered -E "change.*sink" | while read line; do
    echo "¡Cambio detectado!: $line" # Esto te servirá para ver si funciona
    pkill -USR1 "$PROCESO"
done