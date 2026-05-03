TO-DO aniadir cliente a zmenun22
 gcc mainN22.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenun222     $(pkg-config --cflags --libs wayland-client cairo)     -lm -O2
[alb@zaramagaOS waylandz]$ peleando
///server ojo

  gcc test.c metricas.c -o zmetrics-server
//cliente metricas

  gcc cliente.c -o zmetrics-client
  ./zmetrics-client 
 gcc mainN21.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenun221     $(pkg-config --cflags --libs wayland-client cairo)     -lm -O2

gcc mainN2.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenun22     $(pkg-config --cflags --libs wayland-client cairo)     -lm -O2

gcc test.c metricas.c -o tmetrics
//ToDo cockpit y menuos
//TODO ajustar zmenun22 a resolucion minima vrtical 768 y maxima 4k

gcc mainN3.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenun3 \
$(pkg-config --cflags --libs wayland-client cairo) \
-lpthread -lm -O2
//////////////fuentes///////////////
https://www.nerdfonts.com/font-downloads

sudo cp 3270NerdFontPropo-Regular.ttf /usr/share/fonts/TTF/
sudo cp /ruta/a/tu/fuente.ttf /usr/share/fonts/TTF/
sudo fc-cache -fv //////////recargar 
fc-list | grep -i nombre_fuente//// y eso en cairo
ver linea 
fc-list | grep -i nombre_fuente
cairo_select_font_face(cr, "JetBrainsMono Nerd Font Mono",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_NORMAL);

3270 Nerd Font Propo
cairo_select_font_face(cr, "3270 Nerd Font Propo",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_NORMAL);                      
/////////                                               
gcc mainN3.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenun3     $(pkg-config --cflags --libs libsystemd wayland-client cairo)     -lm -O2

gcc mainN.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenu_nuklear     $(pkg-config --cflags --libs wayland-client wayland-cursor cairo)     -lm -O3

gcc mainNl.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenun     $(pkg-config --cflags --libs wayland-client cairo)     -lm -O2
[alb@zaramagaOS waylandz]$ 

xxd -i logo.png > logo_data.h
xxd -i zaramagaos.png > logo_data.h


¡No te preocupes! En Arch Linux, si la carpeta no existe, simplemente la creamos. Es el lugar estándar y más limpio para guardar tus propios scripts sin ensuciar las carpetas del sistema.
Sigue estos pasos rápidos para dejarlo funcionando:
1. Crear la carpeta y el script
Copia y pega esto en tu terminal:
bash
# Crear la carpeta por si no existe
mkdir -p ~/.local/bin

# Crear el script con el comando de apagado
echo -e '#!/bin/bash\nsteam -shutdown' > ~/.local/bin/steamos-session-select

# Darle permisos de ejecución
chmod +x ~/.local/bin/steamos-session-select
Usa el código con precaución.
2. Añadir la carpeta a tu PATH
Para que Sway y el sistema encuentren el script, tu terminal debe saber que ~/.local/bin existe.
Si usas bash, añade esto al final de tu ~/.bashrc:
export PATH="$HOME/.local/bin:$PATH"
Si usas zsh, añádelo a tu ~/.zshrc:
export PATH="$HOME/.local/bin:$PATH"
Importante: Después de guardar el archivo, reinicia la terminal o escribe source ~/.bashrc (o .zshrc) para que surta efecto.
3. Prueba de fuego
Para verificar que el sistema lo reconoce, escribe en la terminal:
which steamos-session-select
Si te responde con la ruta /home/tu_usuario/.local/bin/..., ¡estás listo! Ahora, cuando Steam (con el flag -steamos3) llame a la función de "Salir al escritorio", ejecutará tu script y cerrará Steam por completo.
¿Quieres que revisemos también cómo tienes puesto el comando de lanzamiento en Sway para asegurar que use el flag correcto?
