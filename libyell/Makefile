CC = clang

INCLUDE = ./include
OBJ = ./obj
LIB = ./lib

$(OBJ)/%.o: $(INCLUDE)/%.c
	mkdir -p $(OBJ)
	$(CC) -c -o $@ $<

.PHONY: libyell
libyell: $(OBJ)/yell.o
	mkdir -p $(LIB)
	ar -cvq $(LIB)/libyell.a $(OBJ)/yell.o

.PHONY: clean
clean:
	rm -r obj || true
	rm -r lib || true
