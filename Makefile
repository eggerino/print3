CC = gcc
SRC = src/args.c src/main.c src/scene.c src/viewer.c src/deserialize/file.c src/deserialize/stdin.c src/deserialize/stl.c src/deserialize/util.c
RAYLIB_VERSION = 5.0
RAYLIB_FLAGS = -I dep/raylib-$(RAYLIB_VERSION)_linux_amd64/include -L dep/raylib-$(RAYLIB_VERSION)_linux_amd64/lib/ -l:libraylib.a -lm

bin/print3: $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o bin/print3 $(RAYLIB_FLAGS)


.PHONY: raylib


raylib:
	wget -P dep https://github.com/raysan5/raylib/releases/download/$(RAYLIB_VERSION)/raylib-$(RAYLIB_VERSION)_linux_amd64.tar.gz
	tar -xf dep/raylib-$(RAYLIB_VERSION)_linux_amd64.tar.gz -C dep
