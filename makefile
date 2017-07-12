EXE= controller
SRC= controller.c
FLAGS= -m32

compile : 
	gcc $(FLAGS) $(SRC) -o $(EXE)
