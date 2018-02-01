
#echo thread >>temp.txt
#./shirt 10 500 1024 >>temp.txt
#./shirt 1000 500 1024 >>temp.txt
#./shirt 100000 500 1024 >>temp.txt
#echo single >>temp.txt
#./shirt 10 1 1 >>temp.txt
#./shirt 1000 5 1 >>temp.txt
#./shirt 100000 5 1 >>temp.txt

#echo "./shirt 10 10 65536" >>thread.txt
#./shirt 10 10 65536 >>thread.txt
#echo "./shirt 10 100 65536" >>thread.txt
#./shirt 10 100 65536 >>thread.txt
#echo "./shirt 10 500 65536" >>thread.txt
#./shirt 10 500 65536 >>thread.txt
#echo "./shirt 10 1000 65536" >>thread.txt
#./shirt 10 1000 65536 >>thread.txt

#top-k   thread   level
./shirt 10 10 15
./shirt 100 10 15
./shirt 10000 10 15
./shirt 1000000 10 15








