CC      := gcc
# MacOS
CFLAGS  := -I'../libyell/include' -pthread
LIBS    := ../libyell/lib/libyell.a -lncurses
# Linux
# CFLAGS  := -I'../libyell/include' -L'../libyell/lib' -pthread
# LIBS    := -llibyell -lncurses

SRC = ./src
OBJ = ./obj
BIN = ./bin

$(OBJ)/%.o: $(SRC)/%.c
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

whisper: $(OBJ)/main.o $(OBJ)/whisper.o
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) -o $(BIN)/whisper $^ $(LIBS)

.PHONY: clean
clean:
	rm -r $(OBJ) || true
	rm -r $(BIN) || true
