gcc mainN.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenu_nuklear     $(pkg-config --cflags --libs wayland-client wayland-cursor cairo)     -lm -O3

gcc mainNl.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenun     $(pkg-config --cflags --libs wayland-client cairo)     -lm -O2
[alb@zaramagaOS waylandz]$ 

xxd -i logo.png > logo_data.h
xxd -i zaramagaos.png > logo_data.h