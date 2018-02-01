#include<stdlib.h>
#include<stdio.h>
#include<string> 
#include<string.h> 
#include<math.h>  
#include<bdd.h>
#include<fstream>
#include<time.h>
#include<omp.h>
#include<iostream>
#include<signal.h>
#include"mpi.h"
#include<clab.hpp>
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
double slaveComputingTime = 0;
void init()
{
	bdd_init(6000, 60000);
	bdd_setvarnum(1);
}
bdd loadTree(char* path)
{

       bdd tree;
	   int flag= bdd_fnload(path,tree);
       return tree;
}

void bdd_loadinfo(char* bddinfotext)
{
	FILE *stream;
	stream = fopen(bddinfotext, "ab+");
	int initWeight;
	int* set = (int*)malloc(sizeof(bdd_varnum()));
	int* position = (int*)malloc(sizeof(domainSizeCount));
	fread(&initWeight, sizeof(int), 1, stream);
	fread(set, bdd_varnum()*sizeof(int), 1, stream);
	fread(position, sizeof(int)*domainSizeCount, 1, stream);
	fclose(stream);
}
void bdd_saveinfo(char* bddinfotext, int initWeight, int* set, int* position)
{

	FILE *stream;
	stream = fopen(bddinfotext, "ab+");
	fwrite(&initWeight, sizeof(int), 1, stream);
	fwrite(set, bdd_varnum()*sizeof(int), 1, stream);
	fwrite(position, sizeof(int)*domainSizeCount, 1, stream);
	fclose(stream);

}


int *countNumsHelp(int i, char *str)
{
	res[0] = 0;
	res[1] = 0;
	int j;
	for (j = i;; j++)
	{
		if (str[j] != NULL&&str[j] - 48 >= 0 && str[j] - 48 <= 9)
		{
			res[0] = (res[0] + str[j] - 48) * 10;

			res[1]++;
		}
		else
			break;
	}
	res[0] /= 10;
	return res;
}
int countNums(char *str)
{
	static int index_row = 0;
	int i = 0;
	int count = 0;
	int index_col = 1;
	while (str[i] != '{')
	{
		i++;
	}
	while (str[i] != '\0')
	{
		if (str[i - 1] == ':'&&str[i] >= '0'&&str[i] <= '9')
		{
			int *num = countNumsHelp(i, str);
			Matrix[index_row][index_col] = num[0];
			i += num[1];
			index_col++;
			count++;
		}
		i++;
	}
	index_row++;
	return count;
}

void loadWeightProfile(const char *str)
{
	if (Matrix[0][0] != 0)
		return;
	int step[200];
	char line[1000];
	FILE *fp = fopen(str, "r");
	int varNums = 0;
	while (!feof(fp) && fgets(line, 1000, fp) != NULL)
	{
		if (line[0] != '\n'&&line[0] != '\0'&&line[0] != '\r')
		{
			domainSize[domainSizeCount] = countNums(line);
			int count = ceil(log(domainSize[domainSizeCount]) / log(2));
			Matrix[domainSizeCount][0] = count;
			varNums += count;
			step[domainSizeCount] = count;
			domainSizeCount++;
		}
	}
	fclose(fp);
	/*计算最大权重*/
	int i = 0;
	while (Matrix[i][0] != 0)
	{
		int j = i;
		maxWeight[i] = 0;
		while (Matrix[j][0] != 0)
		{
			maxWeight[i] += Matrix[j][1];
			j++;
		}
		i++;
	}
	i = 0;
	int val = 0;
	while (Matrix[i][0] != 0)
	{
		val += Matrix[i][0];
		AttriHigh[i] = val - 1;
		AttriLow[i] = val - Matrix[i][0];
		i++;
	}
	i = 0;
	int j = 0;
	while (Matrix[i][0] != 0)
	{
		int k = 0;
		for (k = AttriHigh[i]; k >= AttriLow[i]; k--)
		{
			var2Level[j] = i;
			j++;
		}
		i++;
	}
	/*初始化offet数组*/
	i = 0; int offsetIndex = 0;
	while (Matrix[i][0] != 0)
	{
		int j;
		for (j = Matrix[i][0] - 1; j >= 0; j--)
		{
			varoffset[offsetIndex] = pow(2, j);
			offsetIndex++;
		}
		i++;
	}
}

char subBDDName[50];
bdd loadSubBDD(int index)
{
	bdd subBDD;
	memset(subBDDName, '\0', sizeof(subBDDName));
	FILE *stream;
	sprintf(subBDDName, "%s%d%s", "bdd_", index, ".txt");
	bdd_fnload(subBDDName, subBDD);
	return subBDD;
}



int topkth_weight=0;
int minHeapVal=0;
int bestBcastWeight=0;
int partitionNum;
bdd subBDD;
int initWeight;

int top_k;
SolutionSpace* space;
int solutionsetNum=0;
double slaveGetTopkTime=0;
double loadFileTime=0;

void *getTopK(void *ptr)
{
	
	int taskID=*(int *)ptr;
	double t1=omp_get_wtime();
	subBDD=loadSubBDD(taskID);
	double t2=omp_get_wtime();
	loadFileTime+=(t2-t1);
	
	int* position=(int*) malloc(sizeof(int)*1000);
	int* set;
	memset(position,0,sizeof(int)*1000);
	set =(int*) malloc(sizeof(int)*bdd_varnum());
	memset(set,-1,sizeof(int)*bdd_varnum());
	initWeight=maxWeight[0];
	
	double start = omp_get_wtime();
	
	bdd_printset_topk(subBDD, top_k, initWeight, position, set, space, &solutionsetNum, &topkth_weight);
	double end = omp_get_wtime();
	
	slaveGetTopkTime+=(end-start);
	
	//释放空间
	free(set);
	free(position);
	MPI_Send(&topkth_weight, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
	
	pthread_exit(0); 
	
	
}
void *communication(void *ptr)
{
	int myid=*(int *)ptr;
	//int message[3]={fromID,taskid,topkth_weight};
	pthread_t id;
	while(true)
	{
		//sleep(2);
		MPI_Status status;
		int message[3];
		MPI_Recv(message, 3, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		//收到消息第一步 先设置
		minHeapVal=message[2];
		if(minHeapVal>bestBcastWeight)
		{
			bestBcastWeight=minHeapVal;
			setBound(bestBcastWeight);//
		}
		int taskid=message[1];
		if(taskid==-1)
		{
			//int kill_rc = pthread_kill(id,0);
			//0表示存活中
			//while(kill_rc == 0)
			//{
			//	kill_rc = pthread_kill(id,0);
			//	pthread_join(id,NULL);
			//	kill_rc = pthread_kill(id,0);
			//}
			//if(kill_rc == 0)
			//{
			//	cout<<"running!!"<<endl;
			//	pthread_join(id,NULL);
			//}
			break;
		}
		else
		{
			if(myid==message[0])
			{
				/*做任务*/
				//TaskID 线程私有
		        int ret;
				int TaskID=taskid;
				ret=pthread_create(&id,NULL,getTopK,(void *)&TaskID);
				//pthread_join(id,NULL);
			}
		}
	}
	
	double start = omp_get_wtime();
	maxHeapSort(space, top_k);
	double end = omp_get_wtime();
	slaveGetTopkTime+=(end-start);
	cout<<"Process-"<<myid<<"getTopkTime="<<slaveGetTopkTime <<"  loadFileTime="<<loadFileTime<<endl;
	
	pthread_exit(0);

}

main(int argc, char *argv[])  {
	
	int myid, numprocs;
	int nameLen;
	int provided;
	MPI_Status status;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	
	/*读取权重信息*/
	loadWeightProfile("shirt_profile.txt");
	
	top_k = atoi(argv[1]);
	partitionNum = atoi(argv[2]);

	/*所有进程 申请空间 存放top_k 结果 初始化*/
	space = (SolutionSpace*)malloc(sizeof(SolutionSpace)* top_k);
	for (int i = 0; i < top_k; i++)
	{
		space[i].maxWeight = -1;
	}
	
	int firstSendSpaceNum = ceil((double)top_k / (numprocs - 1));
	int firstSendRealNum;
	/*master 中的个数*/
	int count = 0;
	
	/*slave 中的个数*/

	MPI_Barrier(MPI_COMM_WORLD);
	/*负载均衡*/
	/*slave */
	
	if (myid != 0)
	{
		init();
		//1计算线程 
		pthread_t id1;
		int ret1;
		int taskID=myid-1;
		ret1=pthread_create(&id1,NULL,getTopK,(void *)&taskID);
		pthread_join(id1,NULL);
		
		//2通信线程
		
		pthread_t id2;
		int ret2;
		ret2=pthread_create(&id2,NULL,communication,(void *)&myid);
		pthread_join(id2,NULL);
		
	}
	
	//master
	
	if(myid == 0)
	{
		double start,end;
		double masterZeroTime=0;
		
		int taskid;
		
		//i表示taskid
		for(int i=numprocs-1;i<partitionNum;i++)
		{
			
			MPI_Status status;
			int byteNum;
			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_BYTE, &byteNum);
			
			int topkth_weight=0;
			MPI_Recv(&topkth_weight, byteNum, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		
			int fromID=status.MPI_SOURCE;
			//下一个任务的id号
			taskid=i;
			//cout<<"top-k th weight="<< topkth_weight<<endl;
			//cout<<"taskid="<<taskid<<endl;
			//int message[3]={fromID,taskid,topkth_weight};
			//封装一下信息
			int message[3]={fromID,taskid,topkth_weight};
			//将消息发送给所有的进程
			for(int slaveId=1;slaveId<numprocs;slaveId++)
			{
				start = omp_get_wtime();
				MPI_Send(message,3,MPI_INT,slaveId,myid,MPI_COMM_WORLD);
				end = omp_get_wtime();
				masterZeroTime+=end-start;
			}
		}
		
		for (int i = 0; i < numprocs-1;i++)
		{
			MPI_Status status;
			int topkth_weight=0;
			int byteNum;
			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_BYTE, &byteNum);
			
			MPI_Recv(&topkth_weight, byteNum, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			int fromID=status.MPI_SOURCE;
			
			//int message[3]={fromID,taskid,topkth_weight};
			int message[3]={-1,-1,topkth_weight};
			
			//标记该结点是否结束
			int* flag=(int *)malloc(sizeof(int)*(numprocs-1));
			memset(flag, 1, sizeof(int)*(numprocs-1));
			for(int slaveId=1;slaveId<numprocs;slaveId++)
			{
				if(slaveId==fromID)
				{
					//该进程终止
					flag[slaveId-1]=0;
					start = omp_get_wtime();
					MPI_Send(message,3,MPI_INT,slaveId,myid,MPI_COMM_WORLD);
					end = omp_get_wtime();
					masterZeroTime+=end-start;
				}
				else
				{
					//说明还活着
					if(flag[slaveId-1]==1)
					{
						message[2]=-2;
						start = omp_get_wtime();
						MPI_Send(message,3,MPI_INT,slaveId,myid,MPI_COMM_WORLD);
						end = omp_get_wtime();
						masterZeroTime+=end-start;
					}

				}
				
			}

		}
		cout<<"masterZeroTime="<<masterZeroTime<<endl;
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	
	//第一次通信 初始化
	
	int byteNum=firstSendSpaceNum*sizeof(SolutionSpace);
	SolutionSpace* temp_1[4];
	for(int i=0;i<numprocs - 1;i++)
	{
		temp_1[i]=(SolutionSpace*)malloc(byteNum);
		for(int j=0;j<firstSendSpaceNum;j++)
		{
			temp_1[i][j].maxWeight=-1;
		}
	}
	
	
	
	MPI_Barrier(MPI_COMM_WORLD);
	
	//第一次通信  slave--->master
	
	if (myid != 0)
	{
		double start = omp_get_wtime();
		firstSendRealNum = firstSendSpaceNum < solutionsetNum ? firstSendSpaceNum : solutionsetNum;
		MPI_Send(space, firstSendRealNum*sizeof(SolutionSpace), MPI_BYTE, 0, myid, MPI_COMM_WORLD);
		double end = omp_get_wtime();
		slaveComputingTime+=(end-start);
	}
	if(myid == 0)
	{
		int num = numprocs - 1;
		omp_set_num_threads(num);
		double start = omp_get_wtime();
		#pragma omp parallel for
		for (int i = 0; i < num;i++)
		{
			MPI_Status status;
			MPI_Recv(temp_1[i], byteNum, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			#pragma omp critical 
			{
				addToMinHeap(&count, top_k, space, temp_1[i], byteNum);
			}
		}
		double end = omp_get_wtime();
		cout<<"first communicate:"<<end-start<<endl;
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	//第二次通信  master--->slave
	
	if(myid==0)
	{
		double start = omp_get_wtime();
		if (count<top_k)
		{
			int weight = 0;
			for (int i = 1; i<numprocs; i++)
			{
				MPI_Send(&weight, 1, MPI_INT, i, myid, MPI_COMM_WORLD);
			}
		}
		else
		{
			buildMinHeap(space, top_k);
			for (int i = 1; i<numprocs; i++)
			{
				MPI_Send(&space[0].maxWeight, 1, MPI_INT, i, myid, MPI_COMM_WORLD);
			}
		}
		double end = omp_get_wtime();
		cout<<"second communicate:"<<end-start<<endl;
	}
	
	int num = 0;
	if(myid!=0)
	{
		double start = omp_get_wtime();
		int receivedWeight = 0;
		MPI_Recv(&receivedWeight, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		for (int i = firstSendRealNum; i<solutionsetNum; i++)
		{
			if (space[i].maxWeight>receivedWeight)
			{

				num++;
			}
			else
			{
				break;
			}
		}
		double end = omp_get_wtime();
		slaveComputingTime+=(end-start);
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	//为第三次通信做初始化
	
	byteNum=(top_k-firstSendSpaceNum)*sizeof(SolutionSpace);
	SolutionSpace* temp[4];
	for(int i=0;i<numprocs - 1;i++)
	{
		temp[i]=(SolutionSpace*)malloc(byteNum);
		for(int j=0;j<top_k-firstSendSpaceNum;j++)
		{
			temp[i][j].maxWeight=-1;
		}
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	
	//第三次通信  slave--->master
	
	if(myid!=0)
	{
		double start = omp_get_wtime();
		MPI_Send(&space[firstSendRealNum], sizeof(SolutionSpace)*num, MPI_BYTE, 0, myid, MPI_COMM_WORLD);
		double end = omp_get_wtime();
		slaveComputingTime+=(end-start);
	}
	if(myid==0)
	{
		double masterSummaryTime=0;
		double start,end;
		int num = numprocs - 1;
		omp_set_num_threads(num);
		start=omp_get_wtime();
		#pragma omp parallel for
		for (int i = 0; i < num;i++)
		{
			MPI_Status status;
			
			MPI_Recv(temp[i], byteNum, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			#pragma omp critical 
			{
				addToMinHeap(&count, top_k, space, temp[i], byteNum);
			}
		}
		end = omp_get_wtime();
		cout<<"third communicate:"<<end-start<<endl;
		
		maxHeapSort(space, top_k);
	
		/*主节点需要翻译*/
		CPR shirt("shirt.cp");
		/*打印结果*/
		shirt.dumpTop_k("top_k.txt", space, top_k);
		/*
		for (int i = 0; i < top_k; i++)
		{
		  cout << space[i].str << "weight: " << space[i].maxWeight<< endl;
		}
		*/
		
	}
	
	
	
	MPI_Finalize();

}







