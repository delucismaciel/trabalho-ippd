compile:
	gcc -o main main.c
compileomp:
	gcc -o main main.c -fopenmp -w
execute:
	./main