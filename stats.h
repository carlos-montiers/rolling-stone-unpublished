typedef struct {
	unsigned short tt:1;		/* HashTable */	
	unsigned short dl_mg:1;		/* use DeadLock det. in moveGen */
	unsigned short dl2_mg:1;	/* use DeadLock2 in moveGen */
	unsigned short dl_srch:1;	/* use DeadMove search */
	unsigned short pen_srch:1;	/* use PenMove search */
	unsigned short multiins:1;	/* multiple pattern insertion */
	unsigned short st_testd:1;	/* store tested patterns */
	unsigned short lb_mp:1;		/* LB using manpos (backward-forward) */
	unsigned short lb_cf:1;		/* LB using conflicts */
	unsigned short mc_tu:1;		/* Macro using tunnels */
	unsigned short mc_gm:1;		/* General Goal Macros */
	unsigned short cut_goal:1;	/* goal_push cut */
	unsigned short xdist:1;		/* use the extended distance measure */
	unsigned short local:1;  	/* local cut */
	unsigned short autolocal:1;  	/* auto set local cut */
	short          local_k;		/* k parameter of local cut */
	short          local_m;		/* m parameter of local cut */
	short          local_d;		/* m parameter of local cut */
	int            dl_db;		/* Deadlock using Pat. DBs */
} OPTIONS;

extern OPTIONS Options;
extern int32_t total_node_count;
extern int32_t penscount, penmcount, deadscount, deadmcount;

void IncNodeCount(int depth);

void init_stats();
void init_opts();
void print_stats(int pri);

unsigned char GetPatternIndex(MAZE *maze, int pos);
void RecordPatterns(MAZE *maze);
void CountPatterns();
void InitPatterns();
char *CreateStringDepth(short numberinfo);

