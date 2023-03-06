CC      := clang
CFLAGS  := -I'../../include'
LIBS    := ../../lib/yell.a -pthread

SRC = ./src
OBJ = ./obj
BIN = ./bin

$(OBJ)/%.o: $(SRC)/%.c
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

chat: $(OBJ)/main.o
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) -o $(BIN)/chat $^ $(LIBS)

.PHONY: clean
clean:
	rm -r $(OBJ) || true
	rm -r $(BIN) || true
