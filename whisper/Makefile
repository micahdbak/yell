CC      := gcc
CFLAGS  := -I'../libyell/include' -pthread
LIBS    := -lncurses
LIBYELL := '../libyell/lib/yell.a'

SRC = ./src
OBJ = ./obj
BIN = ./bin

$(OBJ)/%.o: $(SRC)/%.c
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

whisper: $(OBJ)/main.o $(OBJ)/whisper.o
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) -o $(BIN)/whisper $(LIBYELL) $^ $(LIBS)

.PHONY: clean
clean:
	rm -r $(OBJ) || true
	rm -r $(BIN) || true
