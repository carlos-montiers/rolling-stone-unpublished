WEIGHTS *NewWeights();
void     FreeWeights(WEIGHTS *w);
/*
#define  ResetWeights(wei) memset((wei)->w,0xff,sizeof(WEIGHT)*XSIZE*YSIZE)
*/
#define ResetWeights(wei) {\
		int i;\
		for (i=0; i<XSIZE*YSIZE; i++) (wei)->w[i] = ENDPATH; }

#define  DubWeights(maze,pos,dir,dir_dub) \
		if (dir!=dir_dub) { \
		    maze->s_weights[dir][pos]->links++; \
		    maze->s_weights[dir_dub][pos] = maze->s_weights[dir][pos]; \
		    maze->bg_weights[dir][pos]->links++; \
		    maze->bg_weights[dir_dub][pos] = maze->bg_weights[dir][pos]; \
		    maze->b_weights[dir][pos]->links++; \
		    maze->b_weights[dir_dub][pos] = maze->b_weights[dir][pos]; \
		}

#define  BackGoalWeight(maze,pos,goal,dir) \
		(((maze)->bg_weights[dir][pos] == NULL)\
		?ENDPATH\
		:((maze)->bg_weights[dir][pos])->w[goal])

#define  BackWeight(maze,pos,goal,dir) \
		(((maze)->b_weights[dir][pos] == NULL)\
		?ENDPATH\
		:((maze)->b_weights[dir][pos])->w[goal])

#define  StoneWeightTest(maze,pos,goal,dir) \
		((maze)->s_weights[dir][pos]\
		?((maze)->s_weights[dir][pos])->w[goal]\
		:ENDPATH)
#define  StoneWeight(maze,pos,goal,dir) \
		(((maze)->s_weights[dir][pos])->w[goal])

#define  ConnectedDir(maze,pos,dir1,dir2) \
		((dir1==NODIR||dir2==NODIR)?(0): \
		((maze)->s_weights[dir1][pos]==(maze)->s_weights[dir2][pos]))

#define  ManWeight(maze,pos,goal) (((maze)->m_weights[pos])->w[goal])
#define  ReadManWeight(weight,maze,pos,goal) \
		if ((maze)->m_weights[pos] == NULL) weight = ENDPATH;\
		else weight = ((maze)->m_weights[pos])->w[goal];
#define  WriteManWeight(weight,maze,pos,goal) \
		if ((maze)->m_weights[pos] == NULL) \
			(maze)->m_weights[pos] = NewWeights();\
		((maze)->m_weights[pos])->w[goal] = weight;

#define  DistWeight(maze,pos,goal) (((maze)->d_weights[pos])->w[goal])
/* use pattern influences as well */
/* #define  DistWeight(maze,pos,goal) ((maze)->influence[pos][goal]\ */
/* 	?(int)((1.0-1.0/(maze)->influence[pos][goal])*\ */
/* 	       (maze)->d_weights[pos]->w[goal])\ */
/* 	:(maze)->d_weights[pos]->w[goal]) */
#define  ReadDistWeight(weight,maze,pos,goal) \
		if ((maze)->d_weights[pos] == NULL) weight = ENDPATH;\
		else weight = ((maze)->d_weights[pos])->w[goal];
#define  WriteDistWeight(weight,maze,pos,goal) \
		if ((maze)->d_weights[pos] == NULL) \
			(maze)->d_weights[pos] = NewWeights();\
		((maze)->d_weights[pos])->w[goal] = weight;

#define  WriteStoneWeight(weight,maze,pos,goal,dir) \
		if ((maze)->s_weights[dir][pos] == NULL) \
			(maze)->s_weights[dir][pos] = NewWeights();\
		((maze)->s_weights[dir][pos])->w[goal] = weight;


void GlobalStoneWeights(MAZE *maze, WEIGHT *weights, PHYSID curr, PHYSID start, 
				     int from_dir, WEIGHT weight);
void GlobalBackWeights(MAZE *maze, WEIGHT *w, PHYSID curr, PHYSID start, 
			     int man_dir, WEIGHT weight, int goal_man);
void TradStoneWeights(MAZE *maze, PHYSID curr, PHYSID goal, WEIGHT weight);
void GlobalDistWeights(MAZE *maze, PHYSID start);
void GlobalManWeights(MAZE *maze, PHYSID curr, PHYSID goal, WEIGHT weight);
void SetGlobalWeights(MAZE *maze) ;
void SetGoalWeights(MAZE *maze);

WEIGHT GetWeightManpos(MAZE *maze, PHYSID goal, PHYSID start, PHYSID manpos);

#define GetWeight( maze, goal, start ) \
( StoneWeight( (maze), (start), (goal), \
( IsBitSetBS( (maze)->one_way, (start) ) && Options.lb_mp !=0 ) \
? WhereMan( (maze), (start) ) : 0 ) )
/* WEIGHT GetWeight(MAZE *maze, PHYSID goal, PHYSID start); */
WEIGHT GetOptWeight(MAZE *maze, PHYSID goal, PHYSID start, int dir);
WEIGHT GetShortestWeight(MAZE *maze, PHYSID goal, PHYSID start);

WEIGHT GetBackWeightManpos(MAZE *maze,PHYSID goal,PHYSID start,PHYSID manpos);
WEIGHT BackGetWeight(MAZE *maze, PHYSID goal, PHYSID start);
WEIGHT GetBackGoalWeightManpos(MAZE *maze, PHYSID goal, 
					   PHYSID start, PHYSID manpos);
WEIGHT BackGetGoalWeight(MAZE *maze, PHYSID goal, PHYSID start);

int    WhereMan(MAZE *maze, PHYSID pos);
int    GetManDir(MAZE *maze, PHYSID curr, PHYSID goal);
int    GetScew(MAZE *maze, PHYSID from, PHYSID via);
void   NewAddScew(MAZE *maze, WEIGHT *add, WEIGHT *scew,
		PHYSID start, PHYSID curr, int tunnel, int from_dir);

WEIGHT XDistMan(MAZE *maze, PHYSID from, PHYSID to);
WEIGHT XDistStone(MAZE *maze, PHYSID from, PHYSID to);

void  DistHist(MAZE *maze);
void SDistHist(MAZE *maze);
void XDistHist(MAZE *maze, int *all, int *scew);
