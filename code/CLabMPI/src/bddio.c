#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include "kernel.h"
#include<stdio.h>
#include<math.h>
#include <omp.h>
#include <sys/syscall.h>
#include "hashmap.h"
#include"mpi.h"

static void bdd_printset_rec_all(FILE *, int, int *);
static void bdd_printset_rec(FILE *, int, int *,int,int);
static void bdd_fprintdot_rec(FILE*, BDD);
static int  bdd_save_rec(FILE*, int);
static int  bdd_loaddata(FILE *);
static int  loadhash_get(int);
static void loadhash_add(int, int);
static bddfilehandler filehandler;

typedef struct s_LoadHash
{
	int key;
	int data;
	int first;
	int next;
} LoadHash;

typedef struct s_SolutionSpace
{
	char str[500];
	int maxWeight;
	int minWeight;
	int count;

}SolutionSpace;


int sNum = 0;
 SolutionSpace *solutionSpace;
int decodeSpacelen = 0;
SolutionSpace *decodeSpace;
int domainSize[1000];
int domainSizeLen = 0;
int matrix[600][400];
int  top_k = 0;
int intervalLow[1000];
int intervalHigh[1000];
int MaxWeight[600];
int hashWeightAtribute[1000];
int offset[1000];



SolutionSpace *solutionSpaceLeft;
SolutionSpace *solutionSpaceRight;



const char* profilePath = "/home/linux1/shirt_profile.txt";
static LoadHash *lh_table;
static int       lh_freepos;
static int       lh_nodenum;
static int      *loadvar2level;

bddfilehandler bdd_file_hook(bddfilehandler handler)
{
	bddfilehandler old = filehandler;
	filehandler = handler;
	return old;
}

int res[2];
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
		if (str[i-1]==':'&&str[i] >= '0'&&str[i] <= '9')
		{
			int *num = countNumsHelp(i, str);
			matrix[index_row][index_col] = num[0];
			i += num[1];
			index_col++;
			count++;
		}
		i++;
	}
	index_row++;
	return count;
}
int varToLevel[1000];
void loadProfile(const char *str)
{
	if (matrix[0][0] != 0)
		return;
	int step[200];
	char line[1000];
	FILE *fp = fopen(str, "r");
	int varNums = 0;
	while (!feof(fp)&&fgets(line,1000,fp)!=NULL)
	{
	   if(line[0]!='\n'&&line[0]!='\0'&&line[0]!='\r')
       {
			domainSize[domainSizeLen] = countNums(line);
			int count = ceil(log(domainSize[domainSizeLen]) / log(2));
			matrix[domainSizeLen][0] = count;
			varNums += count;
			step[domainSizeLen] = count;
			domainSizeLen++;
	   }
	}
	fclose(fp);
	/*计算最大权重*/
	int i = 0;
	while (matrix[i][0] != 0)
	{
		int j = i;
		MaxWeight[i] = 0;
		while (matrix[j][0] != 0)
		{
			MaxWeight[i] += matrix[j][1];
			j++;
		}
		i++;
	}
	/*计算区间intervalLow  intervalHigh,*/
	i = 0;
	int val = 0;
	while (matrix[i][0] != 0)
	{
		val += matrix[i][0];
		intervalHigh[i] = val - 1;
		intervalLow[i] = val - matrix[i][0];
		i++;
	}
	/*初始化hashWeightAtribute 取上届最大值*/
	i = 0;
	int j = 0;
	while (matrix[i][0]!=0)
	{
		int k = 0;
		hashWeightAtribute[i] = matrix[i][1];
		for (k = intervalHigh[i]; k >= intervalLow[i]; k--)
		{
			varToLevel[j] = i;
			j++;
		}
		i++;
	}
	/*初始化offet数组*/
	i = 0; int offsetIndex = 0;
	while (matrix[i][0] != 0)
	{
		int j;
		for (j = matrix[i][0]-1; j >=0; j--)
		{
			offset[offsetIndex] = pow(2, j);
			offsetIndex++;
		}
		i++;
	}
}

void bdd_printall(void)
{
	bdd_fprintall(stdout);
}

void bdd_fprintall(FILE *ofile)
{
	int n;
	for (n = 0; n<bddnodesize; n++)
	{
		if (LOW(n) != -1)
		{
			fprintf(ofile, "[%5d - %2d] ", n, bddnodes[n].refcou);

			if (filehandler)
				filehandler(ofile, bddlevel2var[LEVEL(n)]);
			else
				fprintf(ofile, "%3d", bddlevel2var[LEVEL(n)]);
			fprintf(ofile, ": %3d", LOW(n));
			fprintf(ofile, " %3d", HIGH(n));
			fprintf(ofile, "\n");
		}
	}
}

void bdd_printtable(BDD r)
{
	bdd_fprinttable(stdout, r);
}

void bdd_fprinttable(FILE *ofile, BDD r)
{

	BddNode *node;
	int n;
	fprintf(ofile, "ROOT: %d\n", r);
	if (r < 2)
		return;
	bdd_mark(r);
	for (n = 0; n<bddnodesize; n++)
	{
		if (LEVEL(n) & MARKON)
		{
			node = &bddnodes[n];
			LEVELp(node) &= MARKOFF;
			fprintf(ofile, "[%5d] ", n);
			if (filehandler)
				filehandler(ofile, bddlevel2var[LEVELp(node)]);
			else
				fprintf(ofile, "%3d", bddlevel2var[LEVELp(node)]);
			fprintf(ofile, ": %3d", LOWp(node));
			fprintf(ofile, " %3d", HIGHp(node));
			fprintf(ofile, "\n");
		}
	}
}



void bdd_printset(BDD r, int threads,int TaskNumber)
{
	bdd_fprintset(stdout, r, threads, TaskNumber);
}


/*main*/
void bdd_fprintsetAll(FILE *ofile, BDD r)
{
	int *set = (int *)malloc(sizeof(int)*bddvarnum);
	bdd_printset_rec_all(ofile, r, set);
}

void bdd_fprintset(FILE *ofile, BDD r, int threads,int taskNumber)
{
	int *set;
	if (r < 2)
	{
		fprintf(ofile, "%s", r == 0 ? "F" : "T");
		return;
	}
	if ((set = (int *)malloc(sizeof(int)*bddvarnum)) == NULL)
	{

		bdd_error(BDD_MEMORY);
		return;
	}
	memset(set, 0, sizeof(int)* bddvarnum);
	bdd_printset_rec(ofile, r, set, threads, taskNumber);
	/*bdd_printset_rec_all(ofile, r, set);*/
	free(set);

}

int getMaxWeight(char *str,int weight,int father)
{
	int level = varToLevel[LEVEL(father)];
	int val = 0;
	int count = 0;
	int j;
	for (j = intervalHigh[level]; j >= intervalLow[level]; j--)
	{
		if (str[j] == '1')
		{
			val = val + pow(2, count);
		}
		count++;
	}
	/*在这进行赋值*/
	hashWeightAtribute[level] = matrix[level][(val + 1) % domainSize[level] + 1];
	weight += matrix[level][val + 1];
	weight += MaxWeight[level+1];
	return weight;
}

int getMaxWeightNaive(char *s)
{
	int result = 0;
	int val_num = 0;
	int i;
	for (i = 0; i<1000; i++)
	{
		if (matrix[i][0] != 0)
		{
			val_num += matrix[i][0];
			int val = 0;
			int count = 0;
			int j;
			for (j = val_num - 1; j >= val_num - matrix[i][0]; j--)
			{
				if (s[j] == '1')
					val = val + pow(2, count);
				count++;
			}
			result += matrix[i][val + 1];
		}
		else
		{
			break;
		}
	}
	return result;
}

int getUpperWeight(int weight, int level)
{
	return weight + MaxWeight[varToLevel[level]];
}

void exchangeElements(SolutionSpace array[], int index1, int index2)
{
	SolutionSpace temp = array[index1];
	array[index1] = array[index2];
	array[index2] = temp;
}

/*小根堆*/
void minHeap(SolutionSpace array[], int heapSize, int index)
{
	int left = index * 2 + 1;
	int right = index * 2 + 2;
	int Minimum = index;
	if (left < heapSize && array[left].maxWeight < array[index].maxWeight)
	{
		Minimum = left;
	}
	if (right < heapSize && array[right].maxWeight < array[Minimum].maxWeight)
	{
		Minimum = right;
	}
	if (index != Minimum)
	{
		exchangeElements(array, index, Minimum);
		minHeap(array, heapSize, Minimum);
	}
}
void buildMinHeap(SolutionSpace array[], int n)
{
	int half = n / 2;
	int i;
	for (i = half; i >= 0; i--) {
		minHeap(array, n, i);
	}
}

/*大根堆*/
void maxHeap(SolutionSpace array[], int heapSize, int index)
{
	int left = index * 2 + 1;
	int right = index * 2 + 2;

	int Minimum = index;

	if (left < heapSize && array[left].maxWeight < array[index].maxWeight)
	{
		Minimum = left;
	}

	if (right < heapSize && array[right].maxWeight < array[Minimum].maxWeight)
	{
		Minimum = right;
	}

	if (index != Minimum)
	{
		exchangeElements(array, index, Minimum);
		maxHeap(array, heapSize, Minimum);
	}
}
void buildMaxHeap(SolutionSpace array[], int n)
{
	int half = n / 2;
	int i;
	for (i = half; i >= 0; i--)
	{
		maxHeap(array, n, i);
	}
}

void maxHeapSort(SolutionSpace array[], int n)
{
	buildMaxHeap(array, n);
	int i;
	for (i = n - 1; i >= 1; i--)
	{
		exchangeElements(array, 0, i);
		maxHeap(array, i, 0);
	}
}


int getPresentWeight(int startSegment, int endSegment, int weight, int r, int *set)
{
	if (startSegment != endSegment && r != 0 && r != 1 && LEVEL(r) != 0)
	{
		int val = 0;
		int count = 0;
		int j;
		for (j = intervalHigh[startSegment]; j >= intervalLow[startSegment]; j--)
		{
			if (set[j] == 2)
			{
				val = val + pow(2, count);
			}
			count++;
		}
		/*给hashWeightAtribute赋值*/
		hashWeightAtribute[startSegment] = matrix[startSegment][(val + 1) % domainSize[startSegment] + 1];

		weight += matrix[startSegment][val + 1];
		/*
		在这写权重
		*/
		for (j = startSegment + 1; j < endSegment; j++)
		{
			weight += matrix[j][1];
		}
	}
	return weight;
}
int getUpperWeight2(int weight, int level)
{
	/*return weight + MaxWeight[varToLevel[level]];*/
	int UpperWeight = weight + MaxWeight[varToLevel[level] + 1] + hashWeightAtribute[varToLevel[level]];
	return UpperWeight;
}
static void bdd_printset_rec_chenbaijun_single(FILE *ofile, int r, int *set, int weight,int *position)
{
	if (r == 0)
	{
		return;
	}
	
	if (sNum >= top_k && weight < solutionSpace[0].maxWeight)
	{
		return;
	}
	if (r == 1)
	{
		char* temp = (char *)malloc(sizeof(char)* bddvarnum + 1);
		temp[bddvarnum] = '\0';
		int n; 
		for (n = bddvarnum-1; n >=0; n--)
		{
			switch(set[n])
			{
				case 2:  temp[n] = '1';  break;
				case 1:  temp[n] = '0';  break;
				default: temp[n] = '*';  break; 
			}
		}
		if (sNum < top_k)
		{
			solutionSpace[sNum].maxWeight = weight;
			strcpy(solutionSpace[sNum].str,temp);
		}
		/*达到topk建立小根堆*/
		if (sNum == top_k - 1)
		{
			buildMinHeap(solutionSpace, top_k);
		}
		if (sNum >= top_k)
		{
			if (weight > solutionSpace[0].maxWeight)
			{
				solutionSpace[0].maxWeight = weight;
				strcpy(solutionSpace[0].str,temp);
				minHeap(solutionSpace, top_k, 0);
			}
		}
		
		sNum++;
	}
	else
	{
		int  var = bddlevel2var[LEVEL(r)];
		set[var] = 1;/*0*/
		bdd_printset_rec_chenbaijun_single(ofile, LOW(r), set, weight, position);
		/*回溯，未知的路径position全都设置为0*/

		set[var] = 2;/*1*/
		/*更新权重  往后移动了offset[var]步长*/
		weight = weight - matrix[varToLevel[var]][position[varToLevel[var]] + 1] + matrix[varToLevel[var]][position[varToLevel[var]] + offset[var] + 1];
		position[varToLevel[var]] += offset[var];
		bdd_printset_rec_chenbaijun_single(ofile, HIGH(r), set, weight, position);

		set[var] = 0;/***/
		position[varToLevel[var]] -= offset[var];
	}
}
int  tempMinWeight=0;
static void bdd_printset_rec_chenbaijun_parallel_multi_machine_multi_machine(FILE *ofile, int r, int *set, int weight, int *position,SolutionSpace *space,int top_k)
{	
	if (r == 0)
	{
		return;
	}
	
	if (sNum >=top_k && weight < tempMinWeight)
	{
		return;
	}
	
	if (r == 1)
	{
		char* temp = (char *)malloc(sizeof(char)* bddvarnum + 1);
		temp[bddvarnum] = '\0';
		int n;
		for (n = bddvarnum - 1; n >= 0; n--)
		{
			switch (set[n])
			{
				case 2:  temp[n] = '1';  break;
				case 1:  temp[n] = '0';  break;
				default: temp[n] = '*';  break;
			}
		}

		if (sNum < top_k)
		{
			space[sNum].maxWeight = weight;
			strcpy(space[sNum].str,temp);
			/*space[sNum].str=temp;*/
			
		}
		if (sNum == top_k - 1)
		{
			buildMinHeap(space, top_k);
			tempMinWeight = space[0].maxWeight;
		}
		if (sNum >= top_k && weight > space[0].maxWeight)
		{	
			space[0].maxWeight = weight;
			strcpy(space[0].str,temp);
			/*space[0].str=temp;*/
			minHeap(space, top_k, 0);
			tempMinWeight = space[0].maxWeight;	
		}
		sNum++;
		
	}
	else
	{
		int  var = bddlevel2var[LEVEL(r)];
		set[var] = 1;
		
		bdd_printset_rec_chenbaijun_parallel_multi_machine_multi_machine(ofile, LOW(r), set, weight, position,space,top_k);
		
		set[var] = 2;
		weight = weight - matrix[varToLevel[var]][position[varToLevel[var]] + 1] + matrix[varToLevel[var]][position[varToLevel[var]] + offset[var] + 1];
		position[varToLevel[var]] += offset[var];
		bdd_printset_rec_chenbaijun_parallel_multi_machine_multi_machine(ofile, HIGH(r), set, weight, position,space,top_k);

		set[var] = 0;
		position[varToLevel[var]] -= offset[var];
		
	}
}

static void bdd_printset_rec_chenbaijun(FILE *ofile, int r, int *set, int weight, int threads,int taskNumber, int argc, char* argv[],int top_k)
{
	int myid,numprocs;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int nameLen;
	MPI_Status status;
	MPI_Init(&argc,&argv);
	MPI_Get_processor_name(processor_name, &nameLen);
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);
	MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
	double start = omp_get_wtime();

	/*clock_t start , finish;
	start = clock();
	double  t1 = MPI_Wtime();*/
	if(myid!=0)
	{
		int	w = MaxWeight[0];
		int position[1000];
		memset(position, 0, sizeof(position));
		int val = myid-1;
		int node = r;
		int *set = (int *)malloc(sizeof(int)*bddvarnum);
		char *key = (char *)malloc(sizeof(char)*bddvarnum);
		memset(key, 0, sizeof(key));
		/*numprocs==5*/
		int k = (log(numprocs-1) / log(2));
		int n = 0;
		while (n < k)
		{
			if (val % 2 ==0)
			{
				set[LEVEL(node)] = 1;
				key[LEVEL(node)] = '1';
				node = LOW(node);
				if(node == 1 || node == 0) break;
			}
			else
			{
				set[LEVEL(node)] = 2;
				key[LEVEL(node)] = '2';
				w = w - matrix[varToLevel[LEVEL(node)]][position[varToLevel[LEVEL(node)]] + 1] + matrix[varToLevel[LEVEL(node)]][position[varToLevel[LEVEL(node)]] + offset[LEVEL(node)] + 1];
				position[varToLevel[LEVEL(node)]] += offset[LEVEL(node)];
				node = HIGH(node);
				if (node == 1 || node == 0) break;
			}
			val = val / 2;
			n++;
		}
		SolutionSpace* space = (SolutionSpace*)malloc(sizeof(SolutionSpace)* 200000);
		bdd_printset_rec_chenbaijun_parallel_multi_machine_multi_machine(ofile, node, set, w, position,space,top_k);
		maxHeapSort(space, sNum);
		space[0].count=sNum;
		MPI_Send(space,sizeof(SolutionSpace)*sNum,MPI_BYTE,0,myid,MPI_COMM_WORLD);
	}
	if(myid==0)
	{
		/*master*/
		
		SolutionSpace *topKSpace= (SolutionSpace*)malloc(sizeof(SolutionSpace)*top_k);
		int num=numprocs;
		int count=0;
		while(num!=1)
		{
			SolutionSpace *space= (SolutionSpace*)malloc(sizeof(SolutionSpace)*200000);
			/*MPI_Recv(void* buf, int count, MPI_Datatype datatype,int source,int tag,MPI_Commc comm,MPI_Status)*/
			MPI_Recv(space,sizeof(SolutionSpace)*200000,MPI_BYTE,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
			int i=0;
			for(i=0;i<space[0].count;i++)
			{
				if(count<top_k)
				{
					topKSpace[count].maxWeight = space[i].maxWeight;
					strcpy(topKSpace[count].str,space[i].str);
				}
				if(count==top_k-1)
				{
					buildMinHeap(topKSpace, top_k);
				}
				if(count>=top_k)
				{
					if(space[i].maxWeight>topKSpace[0].maxWeight)
					{
						topKSpace[0].maxWeight =space[i].maxWeight;
						strcpy(topKSpace[0].str,space[i].str);
						minHeap(topKSpace, top_k, 0);
					}
					else
					{
						break;
					}
				}
				count++;
			}
			num--;
		}
		maxHeapSort(topKSpace, top_k);
		int i;
		for(i=0;i<top_k;i++)
		{
			printf("weight=%d || str=%s\n",topKSpace[i].maxWeight,topKSpace[i].str);
		}
		double end = omp_get_wtime();
		printf("diff = %.16g\n",end - start);
		/*double  t2 = MPI_Wtime();
		finish = clock();
		printf("%f ms\n",((double)(finish-start)/CLOCKS_PER_SEC )*1000);
		printf("MPI_Wtime: %1.2f ms\n", (t2-t1)*1000);*/
		
	}
	MPI_Finalize();
}

void bdd_decode_solution_help(int *decodeSpacelen, int *Record, const char *str)
{
	(*decodeSpacelen)++;
	int num = 0;
	int i = 0;
	for (i = 0; i < bddvarnum; i++)
	{
		if (Record[i] == 1)
		{
			num++;
		}
	}
	for (i = 1; i < pow(2, num); i++)
	{
		int val = i;
		
		strcpy(decodeSpace[*decodeSpacelen].str, str);
		int i_Record = 0;
		while (val != 0)
		{
			int Remainder = val % 2;
			while (Record[i_Record] != 1)
			{
				i_Record++;
			};
			decodeSpace[*decodeSpacelen].str[i_Record] = Remainder + 48;
			i_Record++;
			val = val / 2;
		}
		decodeSpace[*decodeSpacelen].maxWeight = getMaxWeight(decodeSpace[*decodeSpacelen].str, 0, 0);
		
		(*decodeSpacelen)++;
	}

}

void bdd_decode_solution(int top_k)
{
	int *Record = (int *)malloc(sizeof(int)* bddvarnum);
	int index = 0;
	for (index = 0; index < top_k; index++)
	{
		
		strcpy(decodeSpace[decodeSpacelen].str, solutionSpace[index].str);
		memset(Record, 0, sizeof(int)* bddvarnum);
		int flag = 1;
		int j = 0;
		for (j = 0; j < bddvarnum; j++)
		{
			if (solutionSpace[index].str[j] == '*')
			{
				decodeSpace[decodeSpacelen].str[j] = '0';
				Record[j] = 1;
				flag = 0;
			}
		}
		decodeSpace[decodeSpacelen].maxWeight = getMaxWeight(decodeSpace[decodeSpacelen].str,0,0);
		if (flag == 1)
		{
			decodeSpacelen++;
		}
		if (flag == 0)
		{
			bdd_decode_solution_help(&decodeSpacelen, Record, decodeSpace[decodeSpacelen].str);
			flag = 1;
		}

	}
}
void bdd_decode_solution_init()
{
	decodeSpacelen = 0;
	free(decodeSpace);
	free(solutionSpace);
	decodeSpace = (SolutionSpace*)malloc(sizeof(SolutionSpace)* 1000);
	solutionSpace = (SolutionSpace*)malloc(sizeof(SolutionSpace)* 1000);
}

/*
void init_first_priority(int num)
{
	top_k = num;
	solutionSpace = (SolutionSpace*)malloc(sizeof(SolutionSpace)* top_k);
	int i = 0;
	for (i = 0; i < top_k; i++)
	{
		memset(solutionSpace[i].str, '*', (sizeof(char)* bddvarnum));
		solutionSpace[i].str[bddvarnum] = '\0';
		solutionSpace[i].minWeight = 0;
		solutionSpace[i].maxWeight = 0;
	}
}*/

void bdd_printset_topk_init()
{
	sNum = 0;
	top_k = 0;
	decodeSpacelen = 0;
}


void printTop_k()
{
	int index = 0;
	for (index = 0; index <(sNum<top_k ? sNum : top_k); index++)
	{
		printf("%s maxWeight:%d\n", solutionSpace[index].str, solutionSpace[index].maxWeight);
	}	
	printf("sNum==%d\n", sNum); 
}
/* main topk*/

/*bdd topN thread taskNumber*/
void bdd_printset_topk(BDD r, int topN,int threads,int taskNumber,int argc,char* argv[])
{
	bdd_printset_topk_init();
	loadProfile(profilePath);
	/*init_first_priority(topN);*/
	int* set   = (int *)malloc(sizeof(int)*bddvarnum);
	bdd_printset_rec_chenbaijun(stdout, r, set, MaxWeight[0], threads, taskNumber,argc,argv,topN);
	/*打印top-k结果*/
	/*printTop_k();*/
}

/*左子树*/
char arr_left[100000][1000]; int count_left = 0;
static void bdd_printfset_rec_left(FILE *ofile, int r, int *set)
{
	/*printf("tid=%ld\n", syscall(SYS_gettid));*/
	if (r == 0)
	{
		return;
	}
	int first;
	if (r == 1)
	{
		/*空间开盘的要足够大*/
		char* temp = (char *)malloc(sizeof(char)* 10 * bddvarnum + 1);
		temp[bddvarnum * 10] = '\0'; int n;
		sprintf(temp, "%c", '<');
		first = 1;
		for (n = 0; n<bddvarnum; n++)
		{
			if (set[n] > 0)
			{
				if (!first)
				{
					sprintf(temp, "%s%s", temp, ", ");
				}
				first = 0;
				if (filehandler)
				{
					filehandler(ofile, bddlevel2var[n]);
				}
				else
				{
					sprintf(temp, "%s%d", temp, bddlevel2var[n]);
				}
				sprintf(temp, "%s:%d", temp, (set[n] == 2 ? 1 : 0));
			}
		}
		sprintf(temp, "%s%c", temp, '>');
		strcpy(arr_left[count_left], temp);
		count_left++;
	}
	if (r != 0 && r != 1)
	{
		set[LEVEL(r)] = 1;
		int level = LEVEL(r);
		bdd_printfset_rec_left(ofile, LOW(r), set);/*rµÄ×óº¢×Ó 0*/
		set[LEVEL(r)] = 2;
		bdd_printfset_rec_left(ofile, HIGH(r), set);/*rµÄÓÒº¢×Ó 1*/
		set[LEVEL(r)] = 0;
	}
}
char arr_right[100000][1000]; int count_right = 0;
static void bdd_printfset_rec_right(FILE *ofile, int r, int *set)
{
	/*printf("tid=%ld\n", syscall(SYS_gettid));*/
	if (r == 0)
	{
		return;
	}
	int first;
	if (r == 1)
	{
		char* temp = (char *)malloc(sizeof(char)* 10 * bddvarnum + 1);
		temp[bddvarnum * 10] = '\0'; int n;
		sprintf(temp, "%c", '<');
		first = 1;
		for (n = 0; n<bddvarnum; n++)
		{
			if (set[n] > 0)
			{
				if (!first)
				{
					sprintf(temp, "%s%s", temp, ", ");
				}
				first = 0;
				if (filehandler)
				{
					filehandler(ofile, bddlevel2var[n]);
				}
				else
				{
					sprintf(temp, "%s%d", temp, bddlevel2var[n]);
				}
				sprintf(temp, "%s:%d", temp, (set[n] == 2 ? 1 : 0));
			}
		}
		sprintf(temp, "%s%c", temp, '>');
		strcpy(arr_right[count_right], temp);
		count_right++;
	}
	if (r != 0 && r != 1)
	{
		set[LEVEL(r)] = 1;
		bdd_printfset_rec_right(ofile, LOW(r), set);/*rµÄ×óº¢×Ó 0*/
		set[LEVEL(r)] = 2;
		bdd_printfset_rec_right(ofile, HIGH(r), set);/*rµÄÓÒº¢×Ó 1*/
		set[LEVEL(r)] = 0;
	}
}
/*并行*/
char arr_parallel[200000][1000]; int count_parallel = 0;
static void bdd_printfset_rec_parallel(FILE *ofile, int r, int *set)
{
	/*printf("tid=%ld\n", syscall(SYS_gettid));*/
	if (r == 0)
	{
		return;
	}
	int first;
	if (r == 1)
	{
		char* temp = (char *)malloc(sizeof(char)* 10 * bddvarnum + 1);
		temp[bddvarnum * 10] = '\0'; int n;
		sprintf(temp, "%c", '<');
		first = 1;
		for (n = 0; n<bddvarnum; n++)
		{
			if (set[n] ==1||set[n]==2)
			{
				if (!first)
				{
					sprintf(temp, "%s%s", temp, ", ");
				}
				first = 0;
				if (filehandler)
				{
					filehandler(ofile, bddlevel2var[n]);
				}
				else
				{
					sprintf(temp, "%s%d", temp, bddlevel2var[n]);
				}
				sprintf(temp, "%s:%d", temp, (set[n] == 2 ? 1 : 0));
			}
		}
		#pragma omp critical
		{
			sprintf(temp, "%s%c", temp, '>');
			strcpy(arr_parallel[count_parallel], temp);
			count_parallel++;
		}

	}
	else
	{
		set[LEVEL(r)] = 1;
		bdd_printfset_rec_parallel(ofile, LOW(r), set);
		set[LEVEL(r)] = 2;
		bdd_printfset_rec_parallel(ofile, HIGH(r), set);
		set[LEVEL(r)] = 0;
	}
}

char arr[200000][1000]; int count = 0;
static void bdd_printset_rec_preorder(FILE *ofile, int r, int *set)
{
	if (r == 0)
	{
		return;
	}
	int first;

	if (r == 1)
	{
		char* temp = (char *)malloc(sizeof(char)* 10 * bddvarnum + 1);
		temp[10 * bddvarnum] = '\0'; int n;
		sprintf(temp, "%c", '<');
		first = 1;
		for (n = 0; n<bddvarnum; n++)
		{
			if (set[n] > 0)
			{
				if (!first)
				{
					sprintf(temp, "%s%s", temp, ", ");
				}
				first = 0;
				if (filehandler)
				{
					filehandler(ofile, bddlevel2var[n]);
				}
				else
				{
					sprintf(temp, "%s%d", temp, bddlevel2var[n]);
				}
				sprintf(temp, "%s:%d", temp, (set[n] == 2 ? 1 : 0));
			}
		}
		sprintf(temp, "%s%c", temp, '>');
		strcpy(arr[count], temp);
		count++;
	}
	if (r != 0 && r != 1)
	{
		set[LEVEL(r)] = 1;
		bdd_printset_rec_preorder(ofile, LOW(r), set);/*rµÄ×óº¢×Ó 0*/
		set[LEVEL(r)] = 2;
		bdd_printset_rec_preorder(ofile, HIGH(r), set);/*rµÄÓÒº¢×Ó 1*/
		set[LEVEL(r)] = 0;
	}
}
/*
static void bdd_printset_rec(FILE *ofile, int r, int *set)
{
	int n;
	int first;
	if (r == 0)
		return;
	else
	if (r == 1)
	{
		fprintf(ofile, "<");
		first = 1;
		for (n = 0; n<bddvarnum; n++)
		{
			if (set[n] > 0)
			{
				if (!first)
				{
					fprintf(ofile, ", ");
				}
				first = 0;
				if (filehandler)
				{
					filehandler(ofile, bddlevel2var[n]);
				}
				else
				{
					fprintf(ofile, "%d", bddlevel2var[n]);
				}
				fprintf(ofile, ":%d", (set[n] == 2 ? 1 : 0));
			}
		}
		fprintf(ofile, ">");
		fprintf(ofile, "\n");
	}
	else
	{
		set[LEVEL(r)] = 1;
		bdd_printset_rec(ofile, LOW(r), set);
		set[LEVEL(r)] = 2;
		bdd_printset_rec(ofile, HIGH(r), set);
		set[LEVEL(r)] = 0;
	}
}*/
static void bdd_printset_rec(FILE *ofile, int r, int *set, int threads, int taskNumber)
{
	hash_map  hm;
	hmap_create(&hm, 1024);
	if (threads == 1)
	{
		bdd_printset_rec_preorder(ofile, r, set);
		printf("totalCount:%d\n", count);
		int i;
		for (i = 0; i < count; i++)
		{
			printf("%s\n",arr[i]);
		}
	}
	else
	{
		int i;
		int k = (log(taskNumber) / log(2));
		omp_set_num_threads(threads);
		#pragma omp parallel for
		for (i = 0; i < taskNumber; i++)
		{
			int val = i;
			int node = r;
			int *set = (int *)malloc(sizeof(int)*bddvarnum);
			char *key = (char *)malloc(sizeof(char)*bddvarnum);
			memset(key, 0, sizeof(key));
			int n = 0;
			void* value;
			while (n < k)
			{
				if (val % 2 == 0)
				{
					set[LEVEL(node)] = 1;/*0*/
					key[LEVEL(node)] = '1';
					node = LOW(node);
					if (node == 1 || node==0) break;
				}
				else
				{
					set[LEVEL(node)] = 2;/*1*/
					key[LEVEL(node)] = '2';
					node = HIGH(node);
					if (node == 1 || node == 0) break;
				}
				val = val / 2;
				n++;
			}
			/*这最好加bloomfilter判断一下*/
			if (node == 1)
			{
				#pragma omp critical
				{
					value = hmap_search(hm, key);
					if (value == 0)
					{
						hmap_insert(hm, key, -1, (void*)1);
					}
				}
				if (value == 0)
				{
					bdd_printfset_rec_parallel(ofile, node, set);
				}
			}
			else
			{
				bdd_printfset_rec_parallel(ofile, node, set);
			}

		}

		printf("count_parallel:%d\n", count_parallel);
		for (i = 0; i < count_parallel; i++)
		{
			printf("%s\n", arr_parallel[i]);
		}
	}
}

void bdd_printdot(BDD r)
{
	bdd_fprintdot(stdout, r);
}
int bdd_fnprintdot(char *fname, BDD r)
{
	FILE *ofile = fopen(fname, "w");
	if (ofile == NULL)
		return bdd_error(BDD_FILE);
	bdd_fprintdot(ofile, r);
	fclose(ofile);
	return 0;
}

void bdd_fprintdot(FILE* ofile, BDD r)
{
	fprintf(ofile, "digraph G {\n");
	fprintf(ofile, "0 [shape=box, label=\"0\", style=filled, shape=box, height=0.3, width=0.3];\n");
	fprintf(ofile, "1 [shape=box, label=\"1\", style=filled, shape=box, height=0.3, width=0.3];\n");
	bdd_fprintdot_rec(ofile, r);
	fprintf(ofile, "}\n");
	bdd_unmark(r);
}

static void bdd_fprintdot_rec(FILE* ofile, BDD r)
{
	if (ISCONST(r) || MARKED(r))
		return;
	fprintf(ofile, "%d [label=\"", r);
	if (filehandler)
		filehandler(ofile, bddlevel2var[LEVEL(r)]);
	else
		fprintf(ofile, "%d", bddlevel2var[LEVEL(r)]);
	fprintf(ofile, "\"];\n");
	fprintf(ofile, "%d -> %d [style=dotted];\n", r, LOW(r));
	fprintf(ofile, "%d -> %d [style=filled];\n", r, HIGH(r));
	SETMARK(r);
	bdd_fprintdot_rec(ofile, LOW(r));
	bdd_fprintdot_rec(ofile, HIGH(r));

}


int bdd_fnsave(char *fname, BDD r)
{
	FILE *ofile;
	int ok;
	if ((ofile = fopen(fname, "w")) == NULL)
				return bdd_error(BDD_FILE);
		ok = bdd_save(ofile, r);
			fclose(ofile);
				return ok;

}
int bdd_save(FILE *ofile, BDD r)
{
		int err, n = 0;
		if (r < 2)
	{

		fprintf(ofile, "0 0 %d\n", r);

		return 0;

	}
		bdd_markcount(r, &n);
		bdd_unmark(r);
		fprintf(ofile, "%d %d\n", n, bddvarnum);
		for (n = 0; n<bddvarnum; n++)
				fprintf(ofile, "%d ", bddvar2level[n]);
		fprintf(ofile, "\n");
		err = bdd_save_rec(ofile, r);
		bdd_unmark(r);
		return err;

}

static int bdd_save_rec(FILE *ofile, int root)
{
		BddNode *node = &bddnodes[root];
		int err;
			if (root < 2)
				return 0;

		if (LEVELp(node) & MARKON)
				return 0;

	LEVELp(node) |= MARKON;
		
	if ((err = bdd_save_rec(ofile, LOWp(node))) < 0)
		return err;

	if ((err = bdd_save_rec(ofile, HIGHp(node))) < 0)
		return err;

	fprintf(ofile, "%d %d %d %d\n",
		root, bddlevel2var[LEVELp(node) & MARKHIDE],
		LOWp(node), HIGHp(node));
	return 0;
}

int bdd_fnload(char *fname, BDD *root)
{
	FILE *ifile;
	int ok;
	if ((ifile = fopen(fname, "r")) == NULL)
		return bdd_error(BDD_FILE);
	ok = bdd_load(ifile, root);
	fclose(ifile);
	return ok;
}

int bdd_load(FILE *ifile, BDD *root)
{
	int n, vnum, tmproot;
	if (fscanf(ifile, "%d %d", &lh_nodenum, &vnum) != 2)
		return bdd_error(BDD_FORMAT);
	/* Check for constant true / false */
	if (lh_nodenum == 0 && vnum == 0)
	{
		fscanf(ifile, "%d", root);
		return 0;
	}
	if ((loadvar2level = (int*)malloc(sizeof(int)*vnum)) == NULL)
		return bdd_error(BDD_MEMORY);
	for (n = 0; n<vnum; n++)
		fscanf(ifile, "%d", &loadvar2level[n]);
	if (vnum > bddvarnum)
		bdd_setvarnum(vnum);
	if ((lh_table = (LoadHash*)malloc(lh_nodenum*sizeof(LoadHash))) == NULL)
		return bdd_error(BDD_MEMORY);
	for (n = 0; n<lh_nodenum; n++)
	{
		lh_table[n].first = -1;
		lh_table[n].next = n + 1;
	}
	lh_table[lh_nodenum - 1].next = -1;
	lh_freepos = 0;
	tmproot = bdd_loaddata(ifile);
	for (n = 0; n<lh_nodenum; n++)
		bdd_delref(lh_table[n].data);
	free(lh_table);
	free(loadvar2level);
	*root = 0;
	if (tmproot < 0)
		return tmproot;
	else
		*root = tmproot;
	return 0;
}

static int bdd_loaddata(FILE *ifile)
{
	int key, var, low, high, root = 0, n;
	for (n = 0; n<lh_nodenum; n++)
	{
		if (fscanf(ifile, "%d %d %d %d", &key, &var, &low, &high) != 4)
			return bdd_error(BDD_FORMAT);
		if (low >= 2)
			low = loadhash_get(low);
		if (high >= 2)
			high = loadhash_get(high);
		if (low<0 || high<0 || var<0)
			return bdd_error(BDD_FORMAT);
		root = bdd_addref(bdd_ite(bdd_ithvar(var), high, low));
		loadhash_add(key, root);
	}
	return root;
}

static void loadhash_add(int key, int data)
{
	int hash = key % lh_nodenum;
	int pos = lh_freepos;
	lh_freepos = lh_table[pos].next;
	lh_table[pos].next = lh_table[hash].first;
	lh_table[hash].first = pos;
	lh_table[pos].key = key;
	lh_table[pos].data = data;

}
static int loadhash_get(int key)
{
	int hash = lh_table[key % lh_nodenum].first;
	while (hash != -1 && lh_table[hash].key != key)
		hash = lh_table[hash].next;
	if (hash == -1)
		return -1;
	return lh_table[hash].data;
}

void  bdd_printinfo()
{
	int i;
	for (i = 0; i <bddnodesize; i++)
	{
		printf("bddnodes[%2d] refcou:%5d level:%5d low:%5d high:%15d hash:%5d next:%5d", i, bddnodes[i].refcou, bddnodes[i].level, bddnodes[i].low, bddnodes[i].high, bddnodes[i].hash, bddnodes[i].next);
		printf("\n");
	}
}
static void bdd_printsetAll(FILE *ofile, int r, int *set)
{
	int n;
	if (r == 0)
		return;
	if (r == 1)
	{
		char *temp = (char *)malloc(sizeof(char)* bddvarnum + 1);
		memset(temp, '*', (sizeof(char)* bddvarnum));
		temp[bddvarnum] = '\0';
		for (n = 0; n<bddvarnum; n++)
		{
			if (set[n] == 1 || set[n] == 2)
			{
				if (filehandler)
				{
					filehandler(ofile, bddlevel2var[n]);
				}
				if (set[n] == 2)
				{
					temp[n] = 49;
				}
				if (set[n] == 1)
				{
					temp[n] = 48;
				}
			}
		}
		int maxWeight = getMaxWeight(temp,0,0);
		solutionSpace[sNum].maxWeight = maxWeight;
		strcpy(solutionSpace[sNum].str,temp);
		sNum++;
	}
	else
	{
		set[bddlevel2var[LEVEL(r)]] = 1;
		bdd_printsetAll(ofile, LOW(r), set);
		set[bddlevel2var[LEVEL(r)]] = 2;
		bdd_printsetAll(ofile, HIGH(r), set);
		set[bddlevel2var[LEVEL(r)]] = 0;
	}

}
static void bdd_printset_rec_all(FILE *ofile, int r, int *set)
{
	bdd_printset_topk_init();
	bdd_decode_solution_init();
	loadProfile(profilePath);
	bdd_printsetAll(stdout, r, set);
	bdd_decode_solution(sNum);
	int index;
	for (index = 0; index < decodeSpacelen; index++)
	{
		fprintf(ofile, "<");
		int i = 0;
		for (i = 0; i < bddvarnum - 1; i++)
		{
			fprintf(ofile, "%d:%c, ", i, decodeSpace[index].str[i]);
		}
		fprintf(ofile, "%d:%c", i, decodeSpace[index].str[i]);
		fprintf(ofile, ">");
		fprintf(ofile, "\n");
	}
}
/* EOF */