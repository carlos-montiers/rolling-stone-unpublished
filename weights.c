#include "board.h"

WEIGHTS *NewWeights() {
	WEIGHTS *r;
	r = (WEIGHTS*) My_malloc(sizeof(WEIGHTS));
	ResetWeights(r);
	r->links = 1;
	return(r);
}
void FreeWeights(WEIGHTS *w) {
	if (w == NULL) return;
	w->links--;
	if (w->links == 0) My_free(w);
}

void GlobalBackWeights(MAZE *maze, WEIGHT *w, PHYSID curr, PHYSID start, 
			     int man_dir, WEIGHT weight, int goal_man)
{
/* if goal_man is on, we are working on bg_weight, and take the goal postion
   of the man into account */
	WEIGHT *pw;
	static short     touched_from[XSIZE*YSIZE];
	static short	 dir2bit[4] = {1,2,4,8};
	int    dir, goal_mandir;
	pw = &(w[curr]);
	if (curr == start && weight==0) 
		memset(touched_from,0,sizeof(short)*XSIZE*YSIZE);
	if (!IsBitSetBS(maze->one_way,curr)) {
		if (*pw <= weight) return;
		*pw = weight;
		if (IsBitSetBS(maze->s_visited,curr)) return;
		if (IsBitSetBS(maze->dead,curr)) return;
		SetBitBS(maze->s_visited,curr);
		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->S[OppDir[dir]],
				       curr+DirToDiff[dir]))
				GlobalBackWeights(maze,w,curr+DirToDiff[dir],
						  start,dir,weight+1,goal_man);
		}
		UnsetBitBS(maze->s_visited,curr);
	} else {
		if (touched_from[curr] & dir2bit[man_dir]) return;
		touched_from[curr] |= dir2bit[man_dir];

		/* only really touched if man is now facing to goal location */
		goal_mandir=GetManDir(maze,curr,IdaInfo->IdaMaze->goal_manpos);
		if (  (*pw > weight)
		    &&(  ConnectedDir(maze,curr,man_dir,goal_mandir)
		       ||(!goal_man))) 
			*pw = weight;

		for (dir=NORTH; dir<=WEST; dir++) {
			if (  (ConnectedDir(maze,curr,man_dir,dir))
			    &&(IsBitSetBS(maze->S[OppDir[dir]],
					  curr+DirToDiff[dir])) ) {
				GlobalBackWeights(maze,w,curr+DirToDiff[dir],
						  start,dir,weight+1,goal_man);
			}
		}
		touched_from[curr] &= ~dir2bit[man_dir];
	}
}

void GlobalStoneWeights(MAZE *maze, WEIGHT *w, PHYSID curr, PHYSID start, 
				     int from_dir, WEIGHT weight) {
	WEIGHT *pw;
	static short     touched_from[XSIZE*YSIZE];
	static short	 dir2bit[4] = {1,2,4,8};
	int    dir;
	pw = &(w[curr]);
	if (curr == start && weight==0) 
		memset(touched_from,0,sizeof(short)*XSIZE*YSIZE);
	if (!IsBitSetBS(maze->one_way,curr)) {
		if (*pw <= weight) return;
		*pw = weight;
		if (IsBitSetBS(maze->s_visited,curr)) return;
		if (IsBitSetBS(maze->dead,curr)) return;
		SetBitBS(maze->s_visited,curr);
		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->S[dir],curr))
				GlobalStoneWeights(maze,w,curr+DirToDiff[dir],
						   start,OppDir[dir],weight+1);
		}
		UnsetBitBS(maze->s_visited,curr);
	} else {
		if (touched_from[curr] & dir2bit[from_dir]) return;
		if (*pw > weight) *pw = weight;
		touched_from[curr] |= dir2bit[from_dir];
		for (dir=NORTH; dir<=WEST; dir++) {
			if (  (ConnectedDir(maze,curr,from_dir,OppDir[dir]))
			    &&(IsBitSetBS(maze->S[dir],curr)) ) {
				GlobalStoneWeights(maze,w,curr+DirToDiff[dir],
						   start,OppDir[dir],weight+1);
			}
		}
		touched_from[curr] &= ~dir2bit[from_dir];
	}
}

void TradStoneWeights(MAZE *maze, PHYSID curr, PHYSID start, WEIGHT weight)
{
	int dir;

	if (IsBitSetBS(maze->dead,curr)) return;
	if (IsBitSetBS(maze->s_visited,curr)) return;
        if (weight >= StoneWeight(maze,start,curr,0)) return;
	WriteStoneWeight(weight,maze,start,curr,0);
	SetBitBS(maze->s_visited,curr);
	for (dir=NORTH; dir<=WEST; dir++) {
        	if (IsBitSetBS(maze->S[dir],curr))
                	TradStoneWeights(maze,curr+DirToDiff[dir],
					 start,weight+1);
	}
	UnsetBitBS(maze->s_visited,curr);
}

void GlobalDistWeights(MAZE *maze, PHYSID start)
/* This function sets the pseudo distance, capturing the idea of influence
 * of the squares. Use breadth first to minimize retraversal.*/
{
	#define STONEPEN 2
	#define MANPEN 1
	int    from_dir,dir,tunnel;
	WEIGHT add,scew,weight;
	PHYSID curr;
	int next_in, next_out;

	static PHYSID s_curr[ENDPATH];
	static WEIGHT s_weight[ENDPATH];
	static int    s_from_dir[ENDPATH];

	s_curr[0] = start;
	s_weight[0] = 0;
	s_from_dir[0] = NODIR;
	next_in = 1;
	next_out = 0;

	while (next_out < next_in) {
		curr = s_curr[next_out];
		weight = s_weight[next_out];
		from_dir = s_from_dir[next_out++];

		/* is this in a tunnel? */
		tunnel = maze->Phys[curr].free == 2;

		NewAddScew(maze,&add,&scew,start,curr,tunnel,from_dir);
		if (add>0 && scew==YES) add >>= 1;
		weight += add;

        	if (weight >= DistWeight(maze,start,curr)) continue;

		WriteDistWeight(weight,maze,start,curr);
		for (dir=NORTH; dir<=WEST; dir++) {
			if (from_dir == dir) continue;
			if (IsBitSetBS(maze->S[dir],curr)) {
				s_curr[next_in] = curr+DirToDiff[dir];
				s_weight[next_in] = weight+(tunnel?0:MANPEN);
				s_from_dir[next_in++] = OppDir[dir];
				if (next_in == ENDPATH) {
					My_exit(1, "Maze too large for ENDPATH, recompile with larger ENDPATH!\n");
				}
        		} else if (IsBitSetBS(maze->M[dir],curr)) {
				s_curr[next_in] = curr+DirToDiff[dir];
				s_weight[next_in] = weight+(tunnel?0:STONEPEN);
				s_from_dir[next_in++] = OppDir[dir];
				if (next_in == ENDPATH) {
					My_exit(1, "Maze too large for ENDPATH, recompile with larger ENDPATH!\n");
				}
			}
		}
	}
}

void DistHist(MAZE *maze)
/* Count Histogram for Xdists and print it out */
{
	PHYSID    from,to;
	HISTOGRAM h,hg;

	InitHist(&h);	
	InitHist(&hg);	
	for (from = 0; from < XSIZE*YSIZE; from++) {
		if (IsBitSetBS(maze->out,from)) continue;
		if (IsBitSetBS(maze->wall,from)) continue;
		if (IsBitSetBS(maze->dead,from)) continue;
		if (maze->groom_index[from]>=0) continue;
		for (to = 0; to < XSIZE*YSIZE; to++) {
			if (IsBitSetBS(maze->out,to)) continue;
			if (IsBitSetBS(maze->wall,to)) continue;
			if (IsBitSetBS(maze->dead,to)) continue;
			if (maze->groom_index[to]>=0) continue;
			IncCounter(&h,ManWeight(maze,from,to));
			if (GetScew(maze,from,to))
				IncCounter(&hg,ManWeight(maze,from,to));
		}
	}
	PrintHist2(&h,&hg);	
}

void SDistHist(MAZE *maze)
/* Count Histogram for Xdists and print it out */
{
	PHYSID    from,to;
	HISTOGRAM h,hg;

	InitHist(&h);	
	InitHist(&hg);	
	for (from = 0; from < XSIZE*YSIZE; from++) {
		if (IsBitSetBS(maze->out,from)) continue;
		if (IsBitSetBS(maze->wall,from)) continue;
		if (IsBitSetBS(maze->dead,from)) continue;
		if (maze->groom_index[from]>=0) continue;
		for (to = 0; to < XSIZE*YSIZE; to++) {
			if (IsBitSetBS(maze->out,to)) continue;
			if (IsBitSetBS(maze->wall,to)) continue;
			if (IsBitSetBS(maze->dead,to)) continue;
			if (maze->groom_index[to]>=0) continue;
			if (GetOptWeight(maze,to,from,NODIR) >= ENDPATH)
				continue;
			IncCounter(&h,GetOptWeight(maze,to,from,NODIR));
			if (GetScew(maze,from,to))
			       IncCounter(&hg,GetOptWeight(maze,to,from,NODIR));
		}
	}
	PrintHist2(&h,&hg);	
}

void XDistHist(MAZE *maze, int *all, int *scew)
/* Count Histogram for Xdists and print it out */
/* if all or scew are not NULL return the respective averages and only print 
 * histogram if either is NULL */
{
	PHYSID    from,to;
	HISTOGRAM h,hg, sqto, sqfrom;

	InitHist(&h);	
	InitHist(&hg);	
	InitHist(&sqto);
	InitHist(&sqfrom);
	for (from = 0; from < XSIZE*YSIZE; from++) {
		if (IsBitSetBS(maze->out,from)) continue;
		if (IsBitSetBS(maze->wall,from)) continue;
		if (IsBitSetBS(maze->dead,from)) continue;
		if (maze->groom_index[from]>=0) continue;
		ResetHist(&sqto);
		ResetHist(&sqfrom);
		for (to = 0; to < XSIZE*YSIZE; to++) {
			if (IsBitSetBS(maze->out,to)) continue;
			if (IsBitSetBS(maze->wall,to)) continue;
			if (IsBitSetBS(maze->dead,to)) continue;
			if (maze->groom_index[to]>=0) continue;
			IncCounter(&h,DistWeight(maze,from,to));
			IncCounter(&sqto,DistWeight(maze,from,to));
			IncCounter(&sqfrom,DistWeight(maze,to,from));
			if (GetScew(maze,from,to))
				IncCounter(&hg,DistWeight(maze,from,to));
		}
		maze->avg_influence_to[from] = GetAvgHist(&sqto);
		maze->avg_influence_from[from] = GetAvgHist(&sqfrom);
	}
	if ( all != NULL)  *all = (int) (GetAvgHist(&h)*0.5) + 1;
	if (scew != NULL) *scew = (int) GetAvgHist(&hg) + 1;
	if (scew == NULL || all == NULL) PrintHist2(&h,&hg);	
}

void GlobalManWeights(MAZE *maze, PHYSID curr, PHYSID start, WEIGHT weight) {
	int dir;

	if (IsBitSetBS(maze->m_visited,curr)) return;
        if (weight >= ManWeight(maze,start,curr)) return;
	WriteManWeight(weight,maze,start,curr);
	SetBitBS(maze->m_visited,curr);
	for (dir=NORTH; dir<=WEST; dir++) {
        	if (IsBitSetBS(maze->M[dir],curr))
                	GlobalManWeights(maze,curr+DirToDiff[dir],
					 start,weight+1);
	}
	UnsetBitBS(maze->m_visited,curr);
}

void SetGlobalWeights(MAZE *maze) {
	PHYSID start,goal;
	WEIGHT *w;
	int dir;
int i, j;

SR(Debug(2,0,"SetGlobalWeights\n"));
	Set0BS(maze->s_visited);
	Set0BS(maze->m_visited);
	for (start=0; start<XSIZE*YSIZE; start++) {
		if (IsBitSetBS(maze->out,start)) continue;
		if (IsBitSetBS(maze->wall,start)) continue;
		GlobalManWeights(maze,start,start,0);
	}

	Set0BS(maze->s_visited);
	Set0BS(maze->m_visited);
	for (start=0; start<XSIZE*YSIZE; start++) {
		if (IsBitSetBS(maze->out,start)) continue;
		if (IsBitSetBS(maze->wall,start)) continue;
		if (IsBitSetBS(maze->dead,start)) continue;
		if (Options.lb_mp==0) {
			TradStoneWeights(maze,start,start,0);
		} else if (IsBitSetBS(maze->one_way,start)) {
			for (dir=NORTH; dir<=WEST; dir++) {
			   if (IsBitSetBS(maze->M[dir],start)) {
			       w = maze->s_weights[dir][start]->w;
			       GlobalStoneWeights(maze,w,start,start,dir,0);
			       w = maze->bg_weights[dir][start]->w;
			       GlobalBackWeights(maze,w,start,start,dir,0,1);
			       w = maze->b_weights[dir][start]->w;
			       GlobalBackWeights(maze,w,start,start,dir,0,0);
			   }
			}
		} else {
			w = maze->s_weights[0][start]->w;
			GlobalStoneWeights(maze,w,start,start,0,0);
			w = maze->bg_weights[0][start]->w;
			GlobalBackWeights(maze,w,start,start,0,0,1);
			w = maze->b_weights[0][start]->w;
			GlobalBackWeights(maze,w,start,start,0,0,0);
		}
	}
	for (start=0; start<XSIZE*YSIZE; start++) {
		if (IsBitSetBS(maze->out,start)) continue;
		if (IsBitSetBS(maze->wall,start)) continue;
		if (IsBitSetBS(maze->dead,start)) continue;
		if (IsBitSetBS(maze->one_way,start)) continue;
		for (goal=0; goal<maze->number_goals; goal++) {
			if ( GetWeight(maze,maze->goals[goal].loc,start)
			    <MAXWEIGHT) break;
		}
		if (goal>=maze->number_goals) {
			SetBitBS(maze->dead,start);
			for (dir=NORTH; dir<=WEST; dir++) {
				if (IsBitSetBS(maze->M[dir],start)) 
					UnsetBitBS(maze->S[OppDir[dir]],
						   start+DirToDiff[dir]);
			}
		}
	}
	for (start=0; start<XSIZE*YSIZE; start++) {
		if (IsBitSetBS(maze->out,start)) continue;
		if (IsBitSetBS(maze->wall,start)) continue;
		GlobalDistWeights(maze,start);
	}
	
SR(Debug(2,0,"MarkTG\n"));
	MarkTG(maze);
}

void SetGoalWeights(MAZE *maze) {
/* Reworked 13.02.97: Just adding all the distances from squares NOT OUT
 * should give us a good idea what goal is most distant. Then sorting them
 * and giving them the rank should give us a good priority order */
	int    i,j,done, max;
	WEIGHT curr_prior,last_prior;
	int32_t weight[MAXGOALS], last_weight;

	for (j=0; j<maze->number_goals; j++) {
		maze->goals[j].weight = 0;
		weight[j]=0;
		for (i=0; i<XSIZE*YSIZE; i++) {
			if (maze->PHYSstone[i] >= 0) {
				weight[j] +=
				      GetWeight(maze,maze->goals[j].loc,i);
			}
		}
	}
	curr_prior = 1;
	last_prior = 1;
	last_weight = 0;
	do {
		done = TRUE;
		max = 0;
		i = -1;
		/* look for maximum weight goal and assign prior */
		for (j=0; j<maze->number_goals; j++) {
			if (weight[j] > max) {
				max = weight[j];
				i = j;
				done = FALSE;
			}
		}
		if (done == FALSE) {
			if (last_weight==weight[i]) {
				maze->goals[i].weight = last_prior;
			} else {
				maze->goals[i].weight = curr_prior;
				last_weight = weight[i];
				last_prior = curr_prior;
			}
			weight[i] = 0;
			curr_prior++;
		}
	} while (!done);
}

WEIGHT GetWeightManpos(MAZE *maze, PHYSID goal, PHYSID start, PHYSID manpos) {
	return(StoneWeight(maze,start,goal,GetManDir(maze,start,manpos)));
}

int WhereMan(MAZE *maze, PHYSID pos) {
	return(GetManDir(maze,pos,maze->manpos));
}

int GetManDir(MAZE *maze, PHYSID curr, PHYSID goal)
{
	int dir;

	if (!IsBitSetBS(maze->one_way,curr) || Options.lb_mp==0)
		dir = 0;
	else {
	    dir = ManWeight(maze,curr,goal);
	    if     (dir>ManWeight(maze,curr+1,goal)) 		dir = NORTH;
            else if(dir>ManWeight(maze,curr+YSIZE,goal)) 	dir = EAST;
            else if(dir>ManWeight(maze,curr-1,goal)) 		dir = SOUTH;
            else if(dir>ManWeight(maze,curr-YSIZE,goal)) 	dir = WEST;
	    else SR(Assert(1,"GetWeightManpos: where are you?\n"));
	}
	return(dir);
}

#define GetWeight( maze, goal, start ) \
( StoneWeight( (maze), (start), (goal), \
( IsBitSetBS( (maze)->one_way, (start) ) && Options.lb_mp !=0 ) \
? WhereMan( (maze), (start) ) : 0 ) )

/* WEIGHT GetWeight(MAZE *maze, PHYSID goal, PHYSID start) { */

/* 	if (IsBitSetBS(maze->one_way,start) && Options.lb_mp!=0) { */
/* 	  return(StoneWeight(maze,start,goal,WhereMan(maze,start))); */
/* 	} else { */
/* 	  return(StoneWeight(maze,start,goal,0)); */
/* 	} */
/* } */

WEIGHT BackGetWeight(MAZE *maze, PHYSID goal, PHYSID start) {
	if (IsBitSetBS(maze->one_way,start) && Options.lb_mp!=0) {
	  /* if this is a one way, the position of the man matters */
	  return(BackWeight(maze,start,goal,WhereMan(maze,start)));
	} else
	  return(BackWeight(maze,start,goal,0));
}

WEIGHT GetBackWeightManpos(MAZE *maze, PHYSID goal, PHYSID start,PHYSID manpos) 
{
	return(BackWeight(maze,start,goal,GetManDir(maze,start,manpos)));
}

WEIGHT BackGetGoalWeight(MAZE *maze, PHYSID goal, PHYSID start)
{
	if (!IsBitSetBS(maze->one_way,start) || Options.lb_mp==0)
	    return(BackGoalWeight(maze,start,goal,0));
	else {
		/* if this is a one way, the position of the man matters */
	    return(BackGoalWeight(maze,start,goal,WhereMan(maze,start)));
	}
}

WEIGHT GetBackGoalWeightManpos(MAZE *maze, PHYSID goal, 
					   PHYSID start, PHYSID manpos) 
{
	return(BackGoalWeight(maze,start,goal,GetManDir(maze,start,manpos)));
}

WEIGHT GetShortestWeight(MAZE *maze, PHYSID goal, PHYSID start) 
{
	WEIGHT min_w,w;
	int    dir;
	
	min_w = StoneWeightTest(maze,start,goal,0);
	if (!IsBitSetBS(maze->one_way,start) || Options.lb_mp==0)
		return(min_w);
	for (dir = EAST; dir <= WEST; dir++) {
		w = StoneWeightTest(maze,start,goal,dir);
		if (w < min_w) min_w = w;
	}
	return(min_w);
}

WEIGHT GetOptWeight(MAZE *maze, PHYSID goal, PHYSID start, int dir) 
{
	WEIGHT w;
	
	if (!IsBitSetBS(maze->one_way,start) || Options.lb_mp==0)
	    return(StoneWeight(maze,start,goal,0));
	else {
	    /* if this is a one way, the position of the man matters */
	    if (dir!=NODIR) {
	   	return(StoneWeight(maze,start,goal,dir));
	    } else {
	    	w = ENDPATH;
		if(   IsBitSetBS(maze->S[NORTH],start)
		   && w>StoneWeight(maze,start,goal,NORTH))
			w=StoneWeight(maze,start,goal,NORTH);
		if(   IsBitSetBS(maze->S[EAST],start)
		   && w>StoneWeight(maze,start,goal,EAST))
			w=StoneWeight(maze,start,goal,EAST);
		if(   IsBitSetBS(maze->S[SOUTH],start)
		   && w>StoneWeight(maze,start,goal,SOUTH))
			w=StoneWeight(maze,start,goal,SOUTH);
		if(   IsBitSetBS(maze->S[WEST],start)
		   && w>StoneWeight(maze,start,goal,WEST))
			w=StoneWeight(maze,start,goal,WEST);
	        return(w);
	    }
	} /* should never happen */
	return(0);
}

int GetScew(MAZE *maze, PHYSID from, PHYSID via)
/* Returns YES if via is on an optimal path to any of the goals */
{
	WEIGHT scew,detour;
	int    goali;

	for (goali=0; goali<maze->number_goals; goali++) {
		scew = GetOptWeight(maze,maze->goals[goali].loc,from,NODIR);
		if (scew >= ENDPATH) continue;
		
		detour = GetOptWeight(maze,via,from,NODIR)
	       		+ GetOptWeight(maze,maze->goals[goali].loc,via,NODIR);
		/* there is no path using via */
		if (detour >= ENDPATH) continue;
		if (detour - scew == 0) return( YES );
	}
	return( NO );
}

void NewAddScew(MAZE *maze, WEIGHT *add, WEIGHT *scew,
		PHYSID start, PHYSID curr, int tunnel, int from_dir)
{
	int    free,s_free;

	/* calc penalty for this square */
	free = maze->Phys[curr].free;
	s_free = maze->Phys[curr].s_free;

	/* remove entrance freedom */
	if (from_dir == NODIR) ;
	else if (IsBitSetBS(maze->S[from_dir],curr)) {
		s_free--;
		free--;
	} else free--;

	/* remove exit freedom - highest */
	if (s_free>0) {
		s_free--;
		free--;
	} else if (free>0) free--;

	*add = STONEPEN * s_free + MANPEN * (free - s_free);

	/* scew towards goal, penalizing squares off course, find min */
	*scew = GetScew(maze,start,curr);
}

WEIGHT XDistMan(MAZE *maze, PHYSID from, PHYSID to)
/* if xdist is set, uses d_weight, otherwise m_weight */
{
	if (Options.xdist == 1)
		return(DistWeight(maze,from,to));
	else
		return(ManWeight(maze,from,to));
}

WEIGHT XDistStone(MAZE *maze, PHYSID from, PHYSID to)
/* if xdist is set, uses d_weight, otherwise m_weight */
{
	if (Options.xdist == 1)
		return(DistWeight(maze,from,to));
	else
		return(GetWeight(maze,to,from));
}
