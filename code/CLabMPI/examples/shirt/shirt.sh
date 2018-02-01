
#ssh slave1 "rm S*"
#ssh slave2 "rm S*"
#ssh slave3 "rm S*"
#ssh slave4 "rm S*"
#rm S*

#cd ~/CLabMPI/buddy20/src
#make
#cd ~/CLabMPI/examples/shirt
#make clean
#make
#cd ~
#$sh scp.sh CLabMPI/
cd ../../buddy20/src/
make
cd ../../examples/shirt/
make clean
make

mpirun -np $3 ./shirt $1  $2
#mpiexec -n $3 -f ~/conf/machinefile ./shirt $1 $2
