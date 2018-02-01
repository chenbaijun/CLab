#include<string.h>
#include<stdlib.h>
#include<time.h>
#include<fcntl.h>
#include<assert.h>
#include<sys/stat.h>
#include"kernel.h"
#include<stdio.h>
#include<math.h>
#include<omp.h>
#include<sys/syscall.h>
#include<pthread.h>


static void bdd_printset_rec(FILE *ofile, int r, int *set);
static void bdd_fprintdot_rec(FILE*, BDD);
static int  bdd_save_rec(FILE*, int);
static int  bdd_loaddata(FILE *);
static int  loadhash_get(int);
static void loadhash_add(int, int);

/*
void bdd_decode_solution_init();
void bdd_printset_solutionspace_init();
void loadProfile(const char *str);
void init_first_priority(int num);
void bdd_decode_solution(int top_k);
static void bdd_printset_rec_chenbaijun(FILE *ofile, int r, int *set);
*/
typedef struct CPU_PACKED         
{  
char name[20];            /*定义一个char类型的数组名name有20个元素 */ 
unsigned int user;        /*定义一个无符号的int类型的user  */
unsigned int nice;        /*定义一个无符号的int类型的nice  */
unsigned int system;      /*定义一个无符号的int类型的system  */
unsigned int idle;        /*定义一个无符号的int类型的idle  */
unsigned int iowait;  
unsigned int irq;  
unsigned int softirq;  
}CPU_OCCUPY;  
double cal_cpuoccupy (CPU_OCCUPY *o, CPU_OCCUPY *n)  
{  
    double od, nd;  
    double id, sd;  
    double cpu_use ;  
  
    od = (double) (o->user + o->nice + o->system +o->idle+o->softirq+o->iowait+o->irq);
    nd = (double) (n->user + n->nice + n->system +n->idle+n->softirq+n->iowait+n->irq);
  
    id = (double) (n->idle);    
    sd = (double) (o->idle) ; 
	
    if((nd-od) != 0)  
    cpu_use =100.0- ((id-sd))/(nd-od)*100.00; 
	
    else cpu_use = 0;  
    return cpu_use;  
}  
  
void get_cpuoccupy (CPU_OCCUPY *cpust)  
{  
    FILE *fd;  
    int n;  
    char buff[256];  
    CPU_OCCUPY *cpu_occupy;  
    cpu_occupy=cpust;  
  
    fd = fopen ("/proc/stat", "r");  
    fgets (buff, sizeof(buff), fd);  
  
    sscanf (buff, "%s %u %u %u %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice,&cpu_occupy->system, &cpu_occupy->idle ,&cpu_occupy->iowait,&cpu_occupy->irq,&cpu_occupy->softirq);  
  
    fclose(fd);  
}  
  
double getCpuRate()  
{  
    CPU_OCCUPY cpu_stat1;  
    CPU_OCCUPY cpu_stat2;  
    double cpu;  
    get_cpuoccupy((CPU_OCCUPY *)&cpu_stat1);  
    sleep(1);  
  
    get_cpuoccupy((CPU_OCCUPY *)&cpu_stat2);  
    cpu = cal_cpuoccupy ((CPU_OCCUPY *)&cpu_stat1, (CPU_OCCUPY *)&cpu_stat2);  
    return cpu;  
}  
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
	char *str;
	int weight;
}SolutionSpace;


int numSpaceSolution = 0;
int top_k = 0;
int domainSizeLen = 0;
SolutionSpace *solutionSpace;

int domainSize[10000];
int matrix[10000][100];
int intervalLow[10000];
int intervalHigh[10000];
int MaxWeight[10000];
int offset[10000];
const char* profilePath;/* = "/home/fengtao/chenbaijun/chenbaijun/shirt_profile.txt";*/
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
		/*hashWeightAtribute[i] = matrix[i][1];*/
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
	bdd_fprintset(stdout, r);
}


void bdd_fprintset(FILE *ofile, BDD r)
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
	bdd_printset_rec(ofile, r, set);
	/*bdd_printset_rec_all(ofile, r, set);*/
	free(set);

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
	if (left < heapSize && array[left].weight < array[index].weight)
	{
		Minimum = left;
	}
	if (right < heapSize && array[right].weight < array[Minimum].weight)
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

	if (left < heapSize && array[left].weight < array[index].weight)
	{
		Minimum = left;
	}

	if (right < heapSize && array[right].weight < array[Minimum].weight)
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



/*无解码*/
int tempWeight=0;
static void bdd_printset_rec_chenbaijun_single1_threads(int i,FILE *ofile, int r, int *set, int weight,int *position)
{
	if (r == 0)
	{
		return;
	}
	if (numSpaceSolution >= top_k && weight < tempWeight)
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
		#pragma omp critical
		{
			if (numSpaceSolution < top_k)
			{
				solutionSpace[numSpaceSolution].weight = weight;
				solutionSpace[numSpaceSolution].str = temp;
			}
			if (numSpaceSolution == top_k - 1)
			{
				buildMinHeap(solutionSpace, top_k);
				tempWeight=solutionSpace[0].weight;
			}
			if (numSpaceSolution >= top_k)
			{
				if (weight > solutionSpace[0].weight)
				{
					/*无须删除内存*/
					solutionSpace[0].weight = weight;
					solutionSpace[0].str = temp;
					minHeap(solutionSpace, top_k, 0);
					tempWeight=solutionSpace[0].weight;
				}
				else
				{
					free(temp);
				}
			}
			numSpaceSolution++;
		}
	}
	else
	{
		int  var = bddlevel2var[LEVEL(r)];
		
		/*该节点分裂*/
		if(1)
		{
			#pragma omp parallel sections
			{
				#pragma omp section
				{
					
					printf("ID: %d, Max threads: %d, Num threads: %d \n",omp_get_thread_num(), omp_get_max_threads(), omp_get_num_threads());  
					int position2[1000];
					int* set2= (int *)malloc(sizeof(int)*bddvarnum);
					memcpy(set2, set, sizeof(int)*bddvarnum);
					memcpy(position2, position, sizeof(position2));
					set2[var]=1;
					int weight2=weight;
					bdd_printset_rec_chenbaijun_single1_threads(0,ofile, LOW(r), set2, weight2, position2);
				}
				#pragma omp section
				{	
					printf("ID: %d, Max threads: %d, Num threads: %d \n",omp_get_thread_num(), omp_get_max_threads(), omp_get_num_threads());  
					int position2[1000];
					int* set2= (int *)malloc(sizeof(int)*bddvarnum);
					memcpy(set2, set, sizeof(int)*bddvarnum);
					memcpy(position2, position, sizeof(position2));
					set2[var] = 2;
					int weight2=weight;
					
					weight2 = weight2 - matrix[varToLevel[var]][position2[varToLevel[var]] + 1] + matrix[varToLevel[var]][position2[varToLevel[var]] + offset[var] + 1];
					position2[varToLevel[var]] += offset[var];
					bdd_printset_rec_chenbaijun_single1_threads(0,ofile, HIGH(r), set2, weight2, position2);
				}
			}
			set[var] = 0;
			
		}

		else
		{
			
			set[var] = 1;/*0*/
			bdd_printset_rec_chenbaijun_single1_threads(0,ofile, LOW(r), set, weight, position);
				
			set[var] = 2;/*1*/
			weight = weight - matrix[varToLevel[var]][position[varToLevel[var]] + 1] + matrix[varToLevel[var]][position[varToLevel[var]] + offset[var] + 1];
			position[varToLevel[var]] += offset[var];
			bdd_printset_rec_chenbaijun_single1_threads(0,ofile, HIGH(r), set, weight, position);
				
			set[var] = 0;
			position[varToLevel[var]] -= offset[var];
		}
		
		
	}
}


/*
*	
*  bdd_printset_rec_chenbaijun_single2_threads 新方法解码
*
*/
/*无解码*/

double CPU_RATE=0;
static void bdd_printset_rec_chenbaijun_single2_threads(int i,FILE *ofile, int r, int *set, int weight,int *position)
{
	if (r == 0)
	{
		return;
	}
	if (numSpaceSolution >= top_k && weight < tempWeight)
	{
		return;
	}
	if (r == 1 && i==bddvarnum)
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
		#pragma omp critical
		{
			if (numSpaceSolution < top_k)
			{
				solutionSpace[numSpaceSolution].weight = weight;
				solutionSpace[numSpaceSolution].str = temp;
			}
			if (numSpaceSolution == top_k - 1)
			{
				buildMinHeap(solutionSpace, top_k);
				tempWeight=solutionSpace[0].weight;
			}
			if (numSpaceSolution >= top_k)
			{
				if (weight > solutionSpace[0].weight)
				{
					/*无须删除内存*/
					solutionSpace[0].weight = weight;
					solutionSpace[0].str = temp;
					minHeap(solutionSpace, top_k, 0);
					tempWeight=solutionSpace[0].weight;
				}
				else
				{
					free(temp);
				}
			}
			numSpaceSolution++;
		}
	}
	else
	{
		
		int  var = bddlevel2var[LEVEL(r)];
		/*var > i 路径中没有走过的路 */
		if(var>i)
		{
			/*节点分裂条件  */
			/*if(CPU_RATE<0 && CPU_RATE!=0)*/
			if(0)
			{
					#pragma omp parallel sections
					{
						#pragma omp section
						{
							/*printf("ID: %d, Max threads: %d, Num threads: %d \n",omp_get_thread_num(), omp_get_max_threads(), omp_get_num_threads());  */
							int position2[1000];
							int* set2= (int *)malloc(sizeof(int)*bddvarnum);
							memcpy(set2, set, sizeof(int)*bddvarnum);
							memcpy(position2, position, sizeof(position2));
							set2[i]=1;
							int weight2=weight;
							int index=i;
							bdd_printset_rec_chenbaijun_single2_threads(index+1,ofile, r, set2, weight2, position2);
						}
						#pragma omp section
						{	
							/*printf("ID: %d, Max threads: %d, Num threads: %d \n",omp_get_thread_num(), omp_get_max_threads(), omp_get_num_threads());  */
							int position2[1000];
							int* set2= (int *)malloc(sizeof(int)*bddvarnum);
							memcpy(set2, set, sizeof(int)*bddvarnum);
							memcpy(position2, position, sizeof(position2));
							set2[i] = 2;
							int weight2=weight;
							
							weight2 = weight2 - matrix[varToLevel[i]][position2[varToLevel[i]] + 1] + matrix[varToLevel[i]][position2[varToLevel[i]] + offset[i] + 1];
							position2[varToLevel[i]] += offset[i];
							int index=i;
							bdd_printset_rec_chenbaijun_single2_threads(index+1,ofile, r, set2, weight2, position2);
						}
					}
					set[i] = 0;
					
			}
			else
			{
					
					set[i] = 1;/*0*/
					bdd_printset_rec_chenbaijun_single2_threads(i+1,ofile, r, set, weight, position);
						
					set[i] = 2;/*1*/
					weight = weight - matrix[varToLevel[i]][position[varToLevel[i]] + 1] + matrix[varToLevel[i]][position[varToLevel[i]] + offset[i] + 1];
					position[varToLevel[i]] += offset[i];
					bdd_printset_rec_chenbaijun_single2_threads(i+1,ofile, r, set, weight, position);
						
					set[i] = 0;
					position[varToLevel[i]] -= offset[i];
			}
		}
		else
		{ 		
				/*节点分裂*/
				/*if(CPU_RATE<90 && rand()% bddvarnum==1)*/
				if(rand()% bddvarnum==1)
				{
					#pragma omp parallel sections
					{
						#pragma omp section
						{
							/*printf("ID: %d, Max threads: %d, Num threads: %d \n",omp_get_thread_num(), omp_get_max_threads(), omp_get_num_threads());  */
							int position2[1000];
							int* set2= (int *)malloc(sizeof(int)*bddvarnum);
							memcpy(set2, set, sizeof(int)*bddvarnum);
							memcpy(position2, position, sizeof(position2));
							set2[var]=1;
							int weight2=weight;
							int index=i;
							
							bdd_printset_rec_chenbaijun_single2_threads(index+1,ofile, LOW(r), set2, weight2, position2);
						}
						#pragma omp section
						{	
							/*printf("ID: %d, Max threads: %d, Num threads: %d \n",omp_get_thread_num(), omp_get_max_threads(), omp_get_num_threads());  */
							int position2[1000];
							int* set2= (int *)malloc(sizeof(int)*bddvarnum);
							memcpy(set2, set, sizeof(int)*bddvarnum);
							memcpy(position2, position, sizeof(position2));
							set2[var] = 2;
							int weight2=weight;	
							weight2 = weight2 - matrix[varToLevel[var]][position2[varToLevel[var]] + 1] + matrix[varToLevel[var]][position2[varToLevel[var]] + offset[var] + 1];
							position2[varToLevel[var]] += offset[var];
							int index=i;
							
							bdd_printset_rec_chenbaijun_single2_threads(index+1,ofile, HIGH(r), set2, weight2, position2);
						}
					}
					set[var] = 0;
				}
				else
				{
					
					set[var] = 1;/*0*/
					bdd_printset_rec_chenbaijun_single2_threads(i+1,ofile, LOW(r), set, weight, position);
						
					set[var] = 2;/*1*/
					weight = weight - matrix[varToLevel[var]][position[varToLevel[var]] + 1] + matrix[varToLevel[var]][position[varToLevel[var]] + offset[var] + 1];
					position[varToLevel[var]] += offset[var];
					bdd_printset_rec_chenbaijun_single2_threads(i+1,ofile, HIGH(r), set, weight, position);
						
					set[var] = 0;
					position[varToLevel[var]] -= offset[var];
				}
		}
	}
}

/*
*  bdd_printset_rec_chenbaijun_single1 没有对结果进行展开
*
*/
static void bdd_printset_rec_chenbaijun_single1(int i,FILE *ofile, int r, int *set, int weight,int *position)
{
	if (r == 0)
	{
		return;
	}
		
	if (numSpaceSolution >= top_k && weight < solutionSpace[0].weight)
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
		if (numSpaceSolution < top_k)
		{
			solutionSpace[numSpaceSolution].weight = weight;
			solutionSpace[numSpaceSolution].str = temp;
		}
		/*达到topk建立小根堆*/
		if (numSpaceSolution == top_k - 1)
		{
			buildMinHeap(solutionSpace, top_k);
		}
		if (numSpaceSolution >= top_k)
		{
			if (weight > solutionSpace[0].weight)
			{
				solutionSpace[0].weight = weight;
				solutionSpace[0].str = temp;
				minHeap(solutionSpace, top_k, 0);
			}
			else
			{
				free(temp);
			}
		}
		numSpaceSolution++;
	}
	else
	{
		int  var = bddlevel2var[LEVEL(r)];
	
		/*int  var = bddlevel2var[LEVEL(r)];*/
		set[var] = 1;/*0*/
		bdd_printset_rec_chenbaijun_single1(0,ofile, LOW(r), set, weight, position);
		/*回溯，未知的路径position全都设置为0*/
				
		set[var] = 2;/*1*/
		/*更新权重  往后移动了offset[var]步长*/
		weight = weight - matrix[varToLevel[var]][position[varToLevel[var]] + 1] + matrix[varToLevel[var]][position[varToLevel[var]] + offset[var] + 1];
		position[varToLevel[var]] += offset[var];
		bdd_printset_rec_chenbaijun_single1(0,ofile, HIGH(r), set, weight, position);
			
		set[var] = 0;/***/
		position[varToLevel[var]] -= offset[var];
	}
}

/*可以解码*/
static void bdd_printset_rec_chenbaijun_single2(int i,FILE *ofile, int r, int *set, int weight,int *position)
{
	if (r == 0)
	{
		return;
	}
	
	if (numSpaceSolution >= top_k && weight < solutionSpace[0].weight)
	{
		return;
	}
	
	if (r == 1 && i==bddvarnum)
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
		if (numSpaceSolution < top_k)
		{
			solutionSpace[numSpaceSolution].weight = weight;
			solutionSpace[numSpaceSolution].str = temp;
		}
		
		if (numSpaceSolution == top_k - 1)
		{
			buildMinHeap(solutionSpace, top_k);
		}
		if (numSpaceSolution >= top_k)
		{
			if (weight > solutionSpace[0].weight)
			{
				solutionSpace[0].weight = weight;
				solutionSpace[0].str = temp;
				minHeap(solutionSpace, top_k, 0);
			}
			else
			{
				free(temp);
			}
		}
		numSpaceSolution++;
	}
	else
	{
		int  var = bddlevel2var[LEVEL(r)];
		/*不相关*/
		if(var>i)
		{
			set[i] = 1;/*0*/
			bdd_printset_rec_chenbaijun_single2(i+1,ofile, r, set, weight, position);
			
			
			set[i] = 2;/*1*/
			/*更新权重  往后移动了offset[var]步长*/
			weight = weight - matrix[varToLevel[i]][position[varToLevel[i]] + 1] + matrix[varToLevel[i]][position[varToLevel[i]] + offset[i] + 1];
			/*
			if (numSpaceSolution >= top_k && weight < solutionSpace[0].weight)
			{
				return;
			}
			*/
			position[varToLevel[i]] += offset[i];
			
			bdd_printset_rec_chenbaijun_single2(i+1,ofile, r, set, weight, position);
			
			set[i] = 0;
			position[varToLevel[i]] -= offset[i];
			
		}
		else
		{
			/*int  var = bddlevel2var[LEVEL(r)];*/
			set[var] = 1;/*0*/
			bdd_printset_rec_chenbaijun_single2(i+1,ofile, LOW(r), set, weight, position);
			/*回溯，未知的路径position全都设置为0*/

			set[var] = 2;/*1*/
			/*更新权重  往后移动了offset[var]步长*/
			weight = weight - matrix[varToLevel[var]][position[varToLevel[var]] + 1] + matrix[varToLevel[var]][position[varToLevel[var]] + offset[var] + 1];
			/*
			if (numSpaceSolution >= top_k && weight < solutionSpace[0].weight)
			{
				return;
			}
			*/
			position[varToLevel[var]] += offset[var];
			bdd_printset_rec_chenbaijun_single2(i+1,ofile, HIGH(r), set, weight, position);
		
			set[var] = 0;/***/
			position[varToLevel[var]] -= offset[var];
		}
		
	}
}

/*并行top-k*/

int  tempMinWeight = 0;
static void bdd_printset_rec_chenbaijun_parallel1(int i, FILE *ofile, int r, int *set, int weight, int *position)
{
	if (r == 0)
	{
		return;
	}
	
	if (numSpaceSolution >=top_k && weight < tempMinWeight)
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
		#pragma omp critical
		{
			if (numSpaceSolution < top_k)
			{
				solutionSpace[numSpaceSolution].weight = weight;
				solutionSpace[numSpaceSolution].str = temp;
			}
			if (numSpaceSolution == top_k - 1)
			{
				
				buildMinHeap(solutionSpace, top_k);
				tempMinWeight = solutionSpace[0].weight;
				
			}
			if (numSpaceSolution >= top_k)
			{
				if (weight > solutionSpace[0].weight)
				{
					solutionSpace[0].weight = weight;
					solutionSpace[0].str = temp;
					minHeap(solutionSpace, top_k, 0);
					tempMinWeight = solutionSpace[0].weight;
				}
				else
				{
					free(temp);
				}
			}
			numSpaceSolution++;
		}
	}
	else
	{
		int  var = bddlevel2var[LEVEL(r)];
		
		set[var] = 1;/*0*/
		bdd_printset_rec_chenbaijun_parallel1(0,ofile, LOW(r), set, weight, position);

		set[var] = 2;/*1*/
		weight = weight - matrix[varToLevel[var]][position[varToLevel[var]] + 1] + matrix[varToLevel[var]][position[varToLevel[var]] + offset[var] + 1];
		position[varToLevel[var]] += offset[var];
		bdd_printset_rec_chenbaijun_parallel1(0,ofile, HIGH(r), set, weight, position);

		set[var] = 0;/***/
		position[varToLevel[var]] -= offset[var];
			
	}
}

/*解码*/
static void bdd_printset_rec_chenbaijun_parallel2(int i, FILE *ofile, int r, int *set, int weight, int *position)
{
	if (r == 0)
	{
		return;
	}
	
	if (numSpaceSolution >=top_k && weight < tempMinWeight)
	{
		return;
	}

	if (r == 1 && i==bddvarnum)
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
		#pragma omp critical
		{
			if (numSpaceSolution < top_k)
			{
				solutionSpace[numSpaceSolution].weight = weight;
				solutionSpace[numSpaceSolution].str = temp;
			}
			if (numSpaceSolution == top_k - 1)
			{
				
				buildMinHeap(solutionSpace, top_k);
				tempMinWeight = solutionSpace[0].weight;
				
			}
			if (numSpaceSolution >= top_k)
			{
				if (weight > solutionSpace[0].weight)
				{
					solutionSpace[0].weight = weight;
					solutionSpace[0].str = temp;
					minHeap(solutionSpace, top_k, 0);
					tempMinWeight = solutionSpace[0].weight;
				}
				else
				{
					free(temp);
				}
			}
			numSpaceSolution++;
		}
	}
	else
	{
		int  var = bddlevel2var[LEVEL(r)];
		
		if(var>i)
		{
			set[i] = 1;/*0*/
			bdd_printset_rec_chenbaijun_parallel2(i+1,ofile, r, set, weight, position);
			
			set[i] = 2;/*1*/
			/*更新权重  往后移动了offset[var]步长*/
			weight = weight - matrix[varToLevel[i]][position[varToLevel[i]] + 1] + matrix[varToLevel[i]][position[varToLevel[i]] + offset[i] + 1];
			position[varToLevel[i]] += offset[i];
			bdd_printset_rec_chenbaijun_parallel2(i+1,ofile, r, set, weight, position);
			
			set[i] = 0;
			position[varToLevel[i]] -= offset[i];
			
		}
		else
		{
			set[var] = 1;/*0*/
			bdd_printset_rec_chenbaijun_parallel2(i+1,ofile, LOW(r), set, weight, position);

			set[var] = 2;/*1*/
			weight = weight - matrix[varToLevel[var]][position[varToLevel[var]] + 1] + matrix[varToLevel[var]][position[varToLevel[var]] + offset[var] + 1];
			position[varToLevel[var]] += offset[var];
			bdd_printset_rec_chenbaijun_parallel2(i+1,ofile, HIGH(r), set, weight, position);

			set[var] = 0;/***/
			position[varToLevel[var]] -= offset[var];
			
			
		}

	}
}


/*
* 
* split1对跳数过多的情况进行了优化
*
*/

int splitNum=0;
int rootTable[100000];
int rootWeight[100000];
int **rootSet;
int **rootPosition;
static void split1(int i,int root,int* position,int* set,int weight,int jump)
{
	if(root==0)
	{
		return;
	}
	/*
	if(root==1)
	{
	}
	*/
	if(jump==0)
	{
		/*需要做一些工作*/
		rootWeight[splitNum]=weight;
		rootTable[splitNum]=root;
		rootSet[splitNum]=(int *)malloc(bddvarnum*sizeof(int)); 
		rootPosition[splitNum]=(int *)malloc(1000*sizeof(int)); 
		memcpy(rootSet[splitNum],set,bddvarnum*sizeof(int));
		memcpy(rootPosition[splitNum],position,1000*sizeof(int));
		splitNum++;
		return;
	}
	
	
	int  var = bddlevel2var[LEVEL(root)];
	
	if(var<bddvarnum)
	{
		/*printf("%d\n",var);*/
		set[var] = 1;/*0*/
		split1(0,LOW(root),position,set,weight,jump-1);
	
		set[var] = 2;/*1*/
		weight = weight - matrix[varToLevel[var]][position[varToLevel[var]] + 1] + matrix[varToLevel[var]][position[varToLevel[var]] + offset[var] + 1];
		position[varToLevel[var]] += offset[var];
		split1(0,HIGH(root),position,set,weight,jump-1);

		set[var] = 0;/***/
		position[varToLevel[var]] -= offset[var];
	}
	else
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

		if (numSpaceSolution < top_k)
		{
			solutionSpace[numSpaceSolution].weight = weight;
			solutionSpace[numSpaceSolution].str = temp;
		}
		if (numSpaceSolution == top_k - 1)
		{
				
			buildMinHeap(solutionSpace, top_k);
			tempMinWeight = solutionSpace[0].weight;
				
		}
		if (numSpaceSolution >= top_k)
		{
			if (weight > solutionSpace[0].weight)
			{
				solutionSpace[0].weight = weight;
				solutionSpace[0].str = temp;
				minHeap(solutionSpace, top_k, 0);
				tempMinWeight = solutionSpace[0].weight;
			}
			else
			{
				free(temp);
			}
		}
		numSpaceSolution++;
		
		
		
	}
}
static void split2(int i,int root,int* position,int* set,int weight,int jump)
{
	if(root==0)
	{
		return;
	}
	if(i==bddlevel2var[LEVEL(root)]&&i==bddvarnum)
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

		if (numSpaceSolution < top_k)
		{
			solutionSpace[numSpaceSolution].weight = weight;
			solutionSpace[numSpaceSolution].str = temp;
		}
		if (numSpaceSolution == top_k - 1)
		{
				
			buildMinHeap(solutionSpace, top_k);
			tempMinWeight = solutionSpace[0].weight;
				
		}
		if (numSpaceSolution >= top_k)
		{
			if (weight > solutionSpace[0].weight)
			{
				solutionSpace[0].weight = weight;
				solutionSpace[0].str = temp;
				minHeap(solutionSpace, top_k, 0);
				tempMinWeight = solutionSpace[0].weight;
			}
			else
			{
				free(temp);
			}
		}
		numSpaceSolution++;
		return;
		
	}
	else if(jump==0&&i==bddlevel2var[LEVEL(root)])
	{
		/*需要做一些工作*/
		rootWeight[splitNum]=weight;
		rootTable[splitNum]=root;
		rootSet[splitNum]=(int *)malloc(bddvarnum*sizeof(int)); 
		rootPosition[splitNum]=(int *)malloc(1000*sizeof(int)); 
		memcpy(rootSet[splitNum],set,bddvarnum*sizeof(int));
		memcpy(rootPosition[splitNum],position,1000*sizeof(int));
		splitNum++;
		return;
	}
	
	
	
	
	int  var = bddlevel2var[LEVEL(root)];
	if(var>i)
	{
		set[i] = 1;/*0*/
		split2(i+1,root,position,set,weight,jump);
	
		set[i] = 2;/*1*/
		weight = weight - matrix[varToLevel[i]][position[varToLevel[i]] + 1] + matrix[varToLevel[i]][position[varToLevel[i]] + offset[i] + 1];
		position[varToLevel[i]] += offset[i];
		split2(i+1,root,position,set,weight,jump);

		set[i] = 0;/***/
		position[varToLevel[i]] -= offset[i];
		
	}
	else
	{
		
		set[var] = 1;/*0*/
		split2(i+1,LOW(root),position,set,weight,jump-1);
	
		set[var] = 2;/*1*/
		weight = weight - matrix[varToLevel[var]][position[varToLevel[var]] + 1] + matrix[varToLevel[var]][position[varToLevel[var]] + offset[var] + 1];
		position[varToLevel[var]] += offset[var];
		split2(i+1,HIGH(root),position,set,weight,jump-1);

		set[var] = 0;/***/
		position[varToLevel[var]] -= offset[var];
	}

	
}


int off_thead=0;
/*double CPU_RATE;*/
void *listening_cpu_rate(void)
{
    int i;
    while(1)
    {
        CPU_RATE=getCpuRate();
		/*sleep(1);*/
		usleep(10000);
		
		if(off_thead==1)
		{
			break;
		}
    }
	 pthread_exit(0);  
}

static void bdd_printset_rec_chenbaijun(FILE *ofile, int r, int *set, int weight, int threads,int hop)
{
	if (threads == 1)
	{
		
		int position[1000];
		set= (int *)malloc(sizeof(int)*bddvarnum);
		/*初始化路径*/
		memset(position, 0, sizeof(position));
		memset(set, -1, sizeof(int)*bddvarnum);
		/*
		* 	single1 未展开
		*   single2 将结果进行展开
		*
		*/
		/*bdd_printset_rec_chenbaijun_single1(0,ofile, r, set, weight,position);*/
		bdd_printset_rec_chenbaijun_single2(0,ofile, r, set, weight,position);
		
		
		maxHeapSort(solutionSpace, top_k);
		
		
		
	}
	else if (threads >= 2)
	{
		
		
		
		int position[1000];
		set= (int *)malloc(sizeof(int)*bddvarnum);
		memset(position, 0, sizeof(position));
		memset(set, -1, sizeof(int)*bddvarnum);
		int weight=MaxWeight[0];
		rootSet=(int**)malloc(sizeof(int*) * 100000);
		rootPosition=(int**)malloc(sizeof(int*) * 100000);
		split2(0,r,position,set,weight,hop);
		
		/*printf("Partition Num=%d\n",splitNum);*/
		int i;
		omp_set_num_threads(splitNum);
		#pragma omp parallel for
		for(i=0;i<splitNum;i++)
		{
			int level=bddlevel2var[LEVEL(rootTable[i])];
			bdd_printset_rec_chenbaijun_parallel2(level,ofile, rootTable[i], rootSet[i], rootWeight[i], rootPosition[i]);
		}
		maxHeapSort(solutionSpace, top_k);
		
		
		
		
	}
	else
	{
		/*未实现好！！有待进一步研究*/
		printf("多线程线程！ 新版本\n");
		off_thead=0;
		int position[1000];
		set= (int *)malloc(sizeof(int)*bddvarnum);
		/*设置递归并行*/
		omp_set_nested(1);
		/*
		printf("nested:%d\n",omp_get_nested());
		
		omp_set_num_threads(10);
		*/
		/*omp_set_dynamic(1);*/
		memset(position, 0, sizeof(position));
		memset(set, -1, sizeof(int)*bddvarnum);
		/*bdd_printset_rec_chenbaijun_single1_threads(0,ofile, r, set, weight,position);*/
		
		/*
		* 监听系统资源的线程
		*
		*/
		int ret=0;
		pthread_t id;
		ret = pthread_create(&id, NULL, (void*)listening_cpu_rate, NULL);
		
		bdd_printset_rec_chenbaijun_single2_threads(0,ofile, r, set, weight,position);
		off_thead=1;
		maxHeapSort(solutionSpace, top_k);
		
	}
}



void initsolutionSpace(int num)
{
	top_k = num;
	solutionSpace = (SolutionSpace*)malloc(sizeof(SolutionSpace)* top_k);
	int i = 0;
	for (i = 0; i < top_k; i++)
	{
		solutionSpace[i].weight = 0;
	}
}

/*打印top-k结果*/
void print_top_k()
{
	int index = 0;
	int total_num=(numSpaceSolution<top_k ? numSpaceSolution : top_k);
	for (index = 0; index <total_num; index++)
	{
		printf("%s weight:%d\n", solutionSpace[index].str, solutionSpace[index].weight);
	}   
	printf("numSpaceSolution=%d\n", numSpaceSolution); 
}
/*


*/
void bdd_printset_topk(BDD r, int topN, int threadsnum, int hops)
{
	loadProfile(profilePath);
	initsolutionSpace(topN);
	/*weight取最大值*/
	int* path;
	bdd_printset_rec_chenbaijun(stdout, r, path, MaxWeight[0], threadsnum, hops);
	/*
		打印
		print_top_k();
	*/
	
}

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
void bdd_set_weightfile_path(char* path)
{
	profilePath=path;
}
int weight_topk[10000000];
int* bdd_fprintset_topk(FILE *ofile, BDD r, int top_k)
{
	/*bdd_printset_topk( BDD r, int top_k,int threadnums,int hops )*/
	bdd_printset_topk(r, top_k,1,1);
	/*bdd_printset_topk(r, top_k, 3, 3);*/

	int j = 0;
	int num = numSpaceSolution<top_k ? numSpaceSolution : top_k;

	int i = 0;
	for (i = 0; i<num; i++)
	{
		char *str = solutionSpace[i].str;
		weight_topk[i] = solutionSpace[i].weight;
		fprintf(ofile, "<");
		int first = 1;
		for (j = 0; j<bddvarnum; j++)
		{
			if (str[j] != '*')
			{
				if (!first)
				{
					fprintf(ofile, ", ");
				}
				first = 0;
				fprintf(ofile, "%d", j);
				fprintf(ofile, ":");
				fprintf(ofile, "%c", str[j]);
			}
		}
		fprintf(ofile, ">");
	}
	return weight_topk;
}


int* bdd_fprintset_topk_multicore(FILE *ofile, BDD r, int top_k, int threadnums, int hops)
{
	/*bdd_printset_topk( BDD r, int top_k,int threadnums,int hops )*/
	bdd_printset_topk(r, top_k, threadnums, hops);
	int j = 0;
	int num = numSpaceSolution<top_k ? numSpaceSolution : top_k;

	int i = 0;
	for (i = 0; i<num; i++)
	{
		char *str = solutionSpace[i].str;
		weight_topk[i] = solutionSpace[i].weight;
		fprintf(ofile, "<");
		int first = 1;
		for (j = 0; j<bddvarnum; j++)
		{
			if (str[j] != '*')
			{
				if (!first)
				{
					fprintf(ofile, ", ");
				}
				first = 0;
				fprintf(ofile, "%d", j);
				fprintf(ofile, ":");
				fprintf(ofile, "%c", str[j]);
			}
		}
		fprintf(ofile, ">");
	}
	return weight_topk;
}
/* EOF */

