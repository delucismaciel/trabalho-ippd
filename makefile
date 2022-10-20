compile:
	gcc -o main main.c -fopenmp -w
execute0:
	gcc -o main main.c -fopenmp -w
	time ./main 0 0 > out
execute1:
	gcc -o main main.c -fopenmp -w
	time ./main 0 1 > out