g++ -I../../buddy20/src -I../../src  -g  -c  detect.cc
g++  -g -W -Wtraditional -Wmissing-prototypes -Wall  -o detect detect.o   -L../../buddy20/src -L../../src -lclab -lfl -lm -lbdd -fopenmp
chmod u+x detect
./detect $1 $2 $3 > consistency_detect_topk_result.txt
