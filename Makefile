CC      := clang
INCLUDE := ./include
OBJ     := ./obj
LIB     := ./lib

$(OBJ)/%.o: $(INCLUDE)/%.c
	mkdir -p $(OBJ)
	$(CC) -c -o $@ $<

.PHONY: yell
yell: $(OBJ)/yell.o $(OBJ)/LL.o
	mkdir -p $(LIB)
	ar -cvq $(LIB)/yell.a $(OBJ)/yell.o $(OBJ)/LL.o

.PHONY: clean
clean:
	rm -r obj || true
	rm -r lib || true
