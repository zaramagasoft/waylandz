CC = gcc
CFLAGS = $(shell pkg-config --cflags wayland-client cairo)
LIBS = $(shell pkg-config --libs wayland-client cairo)

# Añadimos xdg-shell.c a la lista de archivos
zmenu: main.c wlr-layer-shell-unstable-v1.c xdg-shell.c
	$(CC) $(CFLAGS) -o zmenu main.c wlr-layer-shell-unstable-v1.c xdg-shell.c $(LIBS) -lrt