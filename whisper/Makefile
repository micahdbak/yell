CC      := clang
CFLAGS  := -I'../libyell/include'
LIBS    := ../libyell/lib/libyell.a -lncurses -pthread

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
