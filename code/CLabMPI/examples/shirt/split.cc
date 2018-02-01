#include<stdlib.h>
#include<stdio.h>
#include<string> 
#include<string.h> 
#include<clab.hpp>
#include<math.h>  
#include<bdd.h>
#include<fstream>
#include<time.h>
#include<omp.h>
#include<iostream>
#include"mpi.h"
#include <limits>

using namespace std;
int domainSize[10000];
int domainSizeCount;
int Matrix[10000][100];
int maxWeight[10000];
int AttriLow[10000];
int AttriHigh[10000];
int varoffset[10000];
int var2Level[10000];
int res[2];
int partitionNum = 0;
void init()
{
	bdd_init(6000, 60000);
}
bdd loadTree(char* path)
{

	bdd tree;
	int flag = bdd_fnload(path, tree);
	return tree;
}

int partitionNumber;
void bdd_partition(int partitionVarNum, bdd root)
{
	if (partitionVarNum <0)
	{
		return;
	}
	bdd tree = root;
	/*double difference = DBL_MAX;*/
	double difference = 99999999;
	bdd Left_bdd;
	bdd Right_bdd;
	int var = -1;
	for (int i = 0; i < bdd_varnum(); i++)
	{
		bdd left = tree&bdd_biimp(bdd_ithvar(i), bddfalse);
		//int leftPathNum = bdd_pathcount(left);
		double leftPathNum = bdd_satcount(left);

		bdd right = tree&bdd_biimp(bdd_ithvar(i), bddtrue);
		//int rightPathNum = bdd_pathcount(right);
		double rightPathNum = bdd_satcount(right);
		
		//cout << abs(leftPathNum - rightPathNum) << ":" << difference << endl;
		/*
		if (difference < bdd_varnum())
		{
			break;
		}
		*/
		if (fabs(leftPathNum - rightPathNum)< difference)
		{
			//cout<< fabs(leftPathNum - rightPathNum) <<endl;
			difference = fabs(leftPathNum - rightPathNum);
			Left_bdd = left;
			Right_bdd = right;
			var = i;
		}
	}
	
	partitionVarNum = partitionVarNum - 1;
	if (partitionVarNum == 0)
	{
		//bdd_printset(Left_bdd);

		//bdd_printset(Right_bdd);
		
//		cout <<"var:"<<var << endl;
		char Left_bdd_name[50];
		sprintf(Left_bdd_name, "bdd_%d.txt", partitionNumber);
		bdd_fnsave(Left_bdd_name, Left_bdd);
		partitionNumber++;
		
		
		char Right_bdd_name[50];
		sprintf(Right_bdd_name, "bdd_%d.txt", partitionNumber);
		bdd_fnsave(Right_bdd_name, Right_bdd);
		partitionNumber++;
		
		
	}
	/*
	bdd_printset(LeftTemp);
	cout << "====" << endl;
	bdd_printset(RightTemp);
	cout << "*****" << endl;
	*/

	bdd_partition(partitionVarNum, Left_bdd);
	bdd_partition(partitionVarNum, Right_bdd);

}
main(int argc, char *argv[])  {
	int myid, numprocs;
	int nameLen;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	init();
        CPR shirt("shirt.cp");
	bdd root=shirt.compileRules(cm_dynamic);
	//bdd root = loadTree("bdd.txt");
	bdd_partition(atoi(argv[1]), root);
	MPI_Finalize();

}







