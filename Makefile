# macOS + Homebrew + Allegro 5
CC=gcc
SRC=$(wildcard src/*.c)
CFLAGS=-Iinclude $(shell pkg-config --cflags allegro-5 allegro_font-5 allegro_primitives-5)
LDFLAGS=$(shell pkg-config --libs allegro-5 allegro_main-5 allegro_font-5 allegro_primitives-5)
BIN=jogo

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(BIN)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)