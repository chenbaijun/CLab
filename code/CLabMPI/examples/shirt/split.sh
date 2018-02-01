rm split.o split
mpic++ -I../../buddy20/src -I../../src  -g  -c  -fopenmp split.cc 
mpic++  -g -W -Wtraditional -Wmissing-prototypes -Wall  -o split split.o   -L../../buddy20/src -L../../src  -lclab -lfl -lm -lbdd -fopenmp
mpirun -np 5 ./split $1
