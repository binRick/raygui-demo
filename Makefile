CC      := clang
CFLAGS  := -std=c11 -O2 -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-missing-braces
INCLUDES:= -Ivendor $(shell pkg-config --cflags raylib)
LIBS    := $(shell pkg-config --libs raylib) -lm -framework Cocoa -framework IOKit -framework CoreVideo

SRC     := src/main.c src/ui.c src/scenes.c
BIN     := build/raygui-menus

all: $(BIN)

$(BIN): $(SRC) vendor/raygui.h src/ui.h | build
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRC) $(LIBS)

build:
	@mkdir -p build

run: $(BIN)
	./$(BIN)

clean:
	rm -rf build

.PHONY: all run clean
