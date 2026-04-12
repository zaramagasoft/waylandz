gcc mainN.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenu_nuklear     $(pkg-config --cflags --libs wayland-client wayland-cursor cairo)     -lm -O3
