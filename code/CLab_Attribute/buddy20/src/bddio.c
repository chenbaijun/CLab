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

static void bdd_printset_rec(FILE *, int, int *);
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
	char *str;
	int weight;

}SolutionSpace;


int numSolutionSpace = 0;
SolutionSpace *solutionSpace;

int domainSize[10000];
int domainSizeLen = 0;
int matrix[10000][1000];
int top_k = 0;
int intervalLow[10000];
int intervalHigh[10000];
int MaxWeight[1000];

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
	i = 0;
	int j = 0;
	while (matrix[i][0]!=0)
	{
		int k = 0;
		for (k = intervalHigh[i]; k >= intervalLow[i]; k--)
		{
			varToLevel[j] = i;
			j++;
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



void bdd_printset(BDD r)
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
	weight += matrix[level][val + 1];
	weight += MaxWeight[level+1];
	return weight;
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
		weight += matrix[startSegment][val + 1];
		
		for (j = startSegment + 1; j < endSegment; j++)
		{
			weight += matrix[j][1];
		}
	}
	return weight;
}


int father;

/*无解码*/
static void bdd_printset_rec_chenbaijun_single(FILE *ofile, int r, int *set,int weight)
{
	if (r == 0)
	{
		return;
	}
	
	int startSegment = varToLevel[LEVEL(father)];
	int endSegment = varToLevel[LEVEL(r)];
	weight = getPresentWeight(startSegment, endSegment, weight, r, set);
	
	if (numSolutionSpace >= top_k&&getUpperWeight(weight, LEVEL(r)) < solutionSpace[0].weight)
	{
		return;
	}
	if(r == 1)
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
		int maxWeight = getMaxWeight(temp, weight, father);
		
		if (numSolutionSpace < top_k)
		{
			solutionSpace[numSolutionSpace].weight = maxWeight;
			solutionSpace[numSolutionSpace].str = temp;
		}
		/*达到topk建立小根堆*/
		if (numSolutionSpace == top_k - 1)
		{
			buildMinHeap(solutionSpace, top_k);
		}
		if (numSolutionSpace >= top_k)
		{
			if (maxWeight > solutionSpace[0].weight)
			{
				solutionSpace[0].weight = maxWeight;
				solutionSpace[0].str = temp;
				/*调整堆*/
				minHeap(solutionSpace, top_k, 0);
			}
			else
			{
				free(temp);
			}
		}
		numSolutionSpace++;
	}
	else
	{
		int  var = bddlevel2var[LEVEL(r)];
		set[var] = 1;/*0*/
		father = r;
		bdd_printset_rec_chenbaijun_single(ofile, LOW(r), set, weight);

		set[var] = 2;/*1*/
		father = r;
		bdd_printset_rec_chenbaijun_single(ofile, HIGH(r), set, weight);

		set[var] = 0;/***/
	
	}
}





/*
* 解码
*
*
*
*/
int getMaxWeight2(char *str,int weight,int father)
{
	int level = varToLevel[father];
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
	weight += matrix[level][val + 1];
	weight += MaxWeight[level+1];
	return weight;
}

int getPresentWeight2(int startSegment, int endSegment, int weight, int r, int *set)
{
	if (startSegment != endSegment)
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
		weight += matrix[startSegment][val + 1];
	}
	return weight;
}
static void bdd_printset_rec_chenbaijun_singleOpen(int i,FILE *ofile, int r, int *set,int weight)
{
	if (r == 0)
	{
		return;
	}
	
	int startSegment = varToLevel[father];
	int endSegment = varToLevel[i];
	
	weight = getPresentWeight2(startSegment, endSegment, weight, r, set);
	
	/*weight = getPresentWeight(startSegment, i, weight, r, set);*/
	
	
	if (numSolutionSpace >= top_k&&getUpperWeight(weight, i) < solutionSpace[0].weight)
	{
		return;
	}
	
	
	if(r == 1 && i==bddvarnum)
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
		/*int maxWeight = getMaxWeight2(temp, weight, father);*/
		
		if (numSolutionSpace < top_k)
		{
			solutionSpace[numSolutionSpace].weight = weight;
			solutionSpace[numSolutionSpace].str = temp;
		}
		if (numSolutionSpace == top_k - 1)
		{
			buildMinHeap(solutionSpace, top_k);
		}
		if (numSolutionSpace >= top_k)
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
		numSolutionSpace++;
	}
	else
	{
		int  var = bddlevel2var[LEVEL(r)];
		if(var>i)
		{
			set[i] = 1;/*0*/
			/*father = r;*/
			father = i;
			
			bdd_printset_rec_chenbaijun_singleOpen(i+1,ofile, r, set, weight);

			set[i] = 2;/*1*/
			/*father = r;*/
			father = i;
			bdd_printset_rec_chenbaijun_singleOpen(i+1,ofile, r , set, weight);

			set[i] = 0;/***/
		}
		else
		{
			set[var] = 1;/*0*/
			/*father = r;*/
			father = i;
			bdd_printset_rec_chenbaijun_singleOpen(i+1,ofile, LOW(r), set, weight);

			set[var] = 2;/*1*/
			/*father = r;*/
			father = i;
			bdd_printset_rec_chenbaijun_singleOpen(i+1,ofile, HIGH(r), set, weight);

			set[var] = 0;/***/
		}
	}
}

static void bdd_printset_rec_chenbaijun(FILE *ofile, int r, int *set, int weight)
{

	/*bdd_printset_rec_chenbaijun_single(ofile, r, set, weight);*/
	bdd_printset_rec_chenbaijun_singleOpen(0,ofile, r, set, weight);
	maxHeapSort(solutionSpace, top_k);
}



void init_first_priority(int num)
{
	top_k = num;
	solutionSpace = (SolutionSpace*)malloc(sizeof(SolutionSpace)* top_k);
	int i = 0;
	for (i = 0; i < top_k; i++)
	{
		solutionSpace[i].weight = 0;
	}
}


/* main topk*/
void bdd_printset_topk(BDD r, int topN)
{
	
	loadProfile(profilePath);
	init_first_priority(topN);
	int* set   = (int *)malloc(sizeof(int)*bddvarnum);
	memset(set, -1, sizeof(int)*bddvarnum);
	bdd_printset_rec_chenbaijun(stdout, r, set, 0);
	
	/* 打印topk 结果*/
	/*
	int index = 0;
	for (index = 0; index <(numSolutionSpace<top_k ? numSolutionSpace : top_k); index++)
	{
		printf("%s Weight:%d\n", solutionSpace[index].str, solutionSpace[index].weight);
	}
      
	printf("numSolutionSpace==%d\n", numSolutionSpace);
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
	bdd_printset_topk(r, top_k);

	int j = 0; 
	int num = numSolutionSpace<top_k ? numSolutionSpace : top_k;

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
