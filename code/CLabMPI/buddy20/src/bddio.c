#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include "kernel.h"
#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <sys/syscall.h>
#include "hashmap.h"
#include "mpi.h"

static void bdd_printset_rec_all(FILE *, int, int *);
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

static int sNum = 0;

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

static int tempMinWeight = 0;

void setBound(int temp)
{
	tempMinWeight=tempMinWeight>temp?tempMinWeight:temp;
}

char *path;

static void bdd_printset_rec_chenbaijun_parallel1(SolutionSpace* space, int r, int top_k, int weight, int *position, int *set, int* solutionsize,int* topkth_weight)
{	
	
	if (r == 0)
	{
		return;
	}
	
	if (sNum >= top_k && ((weight < tempMinWeight)||weight<(*topkth_weight)))
	{	
		return;
	}
	
	if (r == 1)
	{
		
		path[bddvarnum] = '\0';
		int n;
		for (n = bddvarnum - 1; n >= 0; n--)
		{
			switch (set[n])
			{
				case 2:  path[n] = '1';  break;
				case 1:  path[n] = '0';  break;
				default: path[n] = '*';  break;
			}
		}
		#pragma omp critical
		{
			if (sNum < top_k)
			{
				space[sNum].maxWeight = weight;
				strcpy(space[sNum].str, path);
			}
			if (sNum == top_k - 1)
			{
				buildMinHeap(space, top_k);
				(*topkth_weight) = space[0].maxWeight;

			}
			if (sNum >= top_k)
			{
				if(weight > space[0].maxWeight)
				{
					space[0].maxWeight = weight;
					strcpy(space[0].str, path);
					minHeap(space, top_k, 0);
					(*topkth_weight) = space[0].maxWeight;
				}
				
			}
			sNum++;
			
			
			if ((*solutionsize)<top_k)(*solutionsize)++;
		}
		
	}
	else
	{
		int  var = bddlevel2var[LEVEL(r)];
		
		set[var] = 1;
		bdd_printset_rec_chenbaijun_parallel1(space, LOW(r), top_k, weight, position, set, solutionsize,topkth_weight);

		set[var] = 2;
		weight = weight - Matrix[var2Level[var]][position[var2Level[var]] + 1] + Matrix[var2Level[var]][position[var2Level[var]] + varoffset[var] + 1];
		position[var2Level[var]] += varoffset[var];
		bdd_printset_rec_chenbaijun_parallel1(space, HIGH(r), top_k, weight, position, set, solutionsize,topkth_weight);

		set[var] = 0;
		position[var2Level[var]] -= varoffset[var];
		
	}
}
/*
*
* bdd_printset_rec_chenbaijun_parallel2 解码
*/
static void bdd_printset_rec_chenbaijun_parallel2(int i,SolutionSpace* space, int r, int top_k, int weight, int *position, int *set, int* solutionsize,int* topkth_weight)
{	
	
	if (r == 0)
	{
		return;
	}
	
	if (sNum >= top_k && ((weight < tempMinWeight)||weight<(*topkth_weight)))
	{	
		return;
	}
	
	if (r == 1 && i==bddvarnum)
	{
		
		path[bddvarnum] = '\0';
		int n;
		for (n = bddvarnum - 1; n >= 0; n--)
		{
			switch (set[n])
			{
				case 2:  path[n] = '1';  break;
				case 1:  path[n] = '0';  break;
				default: path[n] = '*';  break;
			}
		}
		#pragma omp critical
		{
			if (sNum < top_k)
			{
				space[sNum].maxWeight = weight;
				strcpy(space[sNum].str, path);
			}
			if (sNum == top_k - 1)
			{
				buildMinHeap(space, top_k);
				(*topkth_weight) = space[0].maxWeight;

			}
			if (sNum >= top_k)
			{
				if(weight > space[0].maxWeight)
				{
					space[0].maxWeight = weight;
					strcpy(space[0].str, path);
					minHeap(space, top_k, 0);
					(*topkth_weight) = space[0].maxWeight;
				}
				
			}
			sNum++;
			
			
			if ((*solutionsize)<top_k)(*solutionsize)++;
		}
		
	}
	else
	{
		int  var = bddlevel2var[LEVEL(r)];
		if(var>i)
		{
			set[i] = 1;
			bdd_printset_rec_chenbaijun_parallel2(i+1,space, r, top_k, weight, position, set, solutionsize,topkth_weight);

			set[i] = 2;
			weight = weight - Matrix[var2Level[i]][position[var2Level[i]] + 1] + Matrix[var2Level[i]][position[var2Level[i]] + varoffset[i] + 1];
			position[var2Level[i]] += varoffset[i];
			bdd_printset_rec_chenbaijun_parallel2(i+1,space, r, top_k, weight, position, set, solutionsize,topkth_weight);

			set[i] = 0;
			position[var2Level[i]] -= varoffset[i];
			
			
		}
		else
		{
			
			set[var] = 1;
			bdd_printset_rec_chenbaijun_parallel2(i+1,space, LOW(r), top_k, weight, position, set, solutionsize,topkth_weight);

			set[var] = 2;
			weight = weight - Matrix[var2Level[var]][position[var2Level[var]] + 1] + Matrix[var2Level[var]][position[var2Level[var]] + varoffset[var] + 1];
			position[var2Level[var]] += varoffset[var];
			bdd_printset_rec_chenbaijun_parallel2(i+1,space, HIGH(r), top_k, weight, position, set, solutionsize,topkth_weight);

			set[var] = 0;
			position[var2Level[var]] -= varoffset[var];
			
			
		}

		
	}
}

void addToMinHeap(int* count,int top_k,SolutionSpace* topk_space,SolutionSpace* space,int byteNum)
{
		int i=0;
		for(i=0;i<byteNum/sizeof(SolutionSpace);i++)
		{
			if((*count)<top_k)
			{
				topk_space[(*count)].maxWeight = space[i].maxWeight;
				strcpy(topk_space[(*count)].str,space[i].str);
			}
			if((*count)==top_k-1)
			{
				buildMinHeap(topk_space, top_k);
			}
			if((*count)>=top_k)
			{
				if(space[i].maxWeight>topk_space[0].maxWeight)
				{
					topk_space[0].maxWeight =space[i].maxWeight;
					strcpy(topk_space[0].str,space[i].str);
					minHeap(topk_space, top_k, 0);
				}
				else
				{
					break;
				}
			}
			(*count)++;
		}
}


int splitNum=0;
int rootTable[10000];
int rootWeight[10000];
int* rootSet[10000];
int* rootPosition[10000];
static void split1(int root,int* position,int* set,int weight,int hop)
{
	
	if(root==0)
	{
		return;
	}
	
	if(hop==0)
	{
		
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
		
		set[var] = 1;
		split1(LOW(root),position,set,weight,hop-1);
	
		set[var] = 2;
		weight = weight - Matrix[var2Level[var]][position[var2Level[var]] + 1] + Matrix[var2Level[var]][position[var2Level[var]] + varoffset[var] + 1];
		position[var2Level[var]] += varoffset[var];
		split1(HIGH(root),position,set,weight,hop-1);

		set[var] = 0;
		position[var2Level[var]] -= varoffset[var];	
	}
}

static void split2(int i,int root,int* position,int* set,int weight,int hop)
{
	if(root==0)
	{
		return;
	}
	if(hop==0&&i==bddlevel2var[LEVEL(root)])
	{
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
		set[i] = 1;
		split2(i+1,root,position,set,weight,hop);
	
		set[i] = 2;
		weight = weight - Matrix[var2Level[i]][position[var2Level[i]] + 1] + Matrix[var2Level[i]][position[var2Level[i]] + varoffset[i] + 1];
		position[var2Level[i]] += varoffset[i];
		split2(i+1,root,position,set,weight,hop);

		set[i] = 0;
		position[var2Level[i]] -= varoffset[i];
		
	}
	else
	{
		
		set[var] = 1;
		split2(i+1,LOW(root),position,set,weight,hop-1);
	
		set[var] = 2;
		weight = weight - Matrix[var2Level[var]][position[var2Level[var]] + 1] + Matrix[var2Level[var]][position[var2Level[var]] + varoffset[var] + 1];
		position[var2Level[var]] += varoffset[var];
		split2(i+1,HIGH(root),position,set,weight,hop-1);

		set[var] = 0;
		position[var2Level[var]] -= varoffset[var];
	}	
}

static void bdd_printset_rec_chenbaijun(BDD r, int top_k, int init_weight, int *position, int *set, SolutionSpace* space, int* solutionsize,int* topkth_weight)
{
	path=(char*) malloc(sizeof(char)*(bddvarnum+1));
	/*
	bdd_printset_rec_chenbaijun_parallel(space, r, top_k, init_weight, position, set, solutionsize, topkth_weight);
	free(path);
	printf("end!\n");
	*/
	
	int hop=0;
	splitNum=0;
	/*
	split1(r,position,set,init_weight,hop);
	*/
	
	split2(0,r,position,set,init_weight,hop);
	
	/*
	printf("Partition-Num=%d\n",splitNum);
	*/
	int i;
	
	omp_set_num_threads(1);
	#pragma omp parallel for
	for(i=0;i<splitNum;i++)
	{
		/*
		bdd_printset_rec_chenbaijun_parallel1(space, rootTable[i], top_k, rootWeight[i], rootPosition[i], rootSet[i], solutionsize, topkth_weight);
		*/
		int level=bddlevel2var[LEVEL(rootTable[i])];
		bdd_printset_rec_chenbaijun_parallel2(level,space, rootTable[i], top_k, rootWeight[i], rootPosition[i], rootSet[i], solutionsize, topkth_weight);	
	}
	
	
	for(i=0; i<splitNum; i++)  
	{
		 free(rootSet[i]); 
		 free(rootPosition[i]); 
	}
	free(path);
	
	
	
}

/* main topk*/
void bdd_printset_topk(BDD r, int top_k, int init_weight, int *position, int *set, SolutionSpace* space, int* solutionsize,int* topkth_weight)
{
	
	bdd_printset_rec_chenbaijun(r, top_k, init_weight, position, set, space, solutionsize, topkth_weight);
	
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

int weight_topk[10000000];
int* printDumpSolutionspace(FILE *ofile, SolutionSpace* space, int top_k)
{
	int j = 0;
	int i = 0;
	for (i = 0; i<top_k; i++)
	{
		char *str = space[i].str;
		weight_topk[i] = space[i].maxWeight;
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