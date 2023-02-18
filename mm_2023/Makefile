CC = gcc
CFLAGS = -I'../libyell/include'
LIBYELL = '../libyell/lib/yell.a'

SRC = ./src
OBJ = ./obj
BIN = ./bin

MM_2023_NAME = mm

$(OBJ)/%.o: $(SRC)/%.c
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: $(MM_2023_NAME)
$(MM_2023_NAME): $(OBJ)/main.o
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) -o $(BIN)/$(MM_2023_NAME) $(LIBYELL) $(OBJ)/main.o

.PHONY: clean
clean:
	rm -r $(OBJ)
	rm -r $(BIN)
