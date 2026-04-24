CC = gcc
CFLAGS = -O2 $(shell pkg-config --cflags wayland-client cairo)
LDFLAGS = $(shell pkg-config --libs wayland-client cairo) -lm

SRC = mainN2.c wlr-layer-shell-unstable-v1.c xdg-shell.c
OBJ = $(SRC:.c=.o)

zmenun2: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@