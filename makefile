EXE= controller.o
SRC= controller.c
FLAGS= -m32

compile : 
	gcc $(FLAGS) $(SRC) -o $(EXE)
