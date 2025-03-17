#include "board.h"

/**********************************************************************
 *
 *	This is a better lower bound estimator (well, routines that 
 *	implements it). It uses minimum flow, maximum matching to solve
 *	which stone goes to which goal without conflicts. If no matching
 *	exists - deadlock!
 *
 **********************************************************************/

int BetterLowerBound(MAZE *maze) {
	int i;

	/* initialize table with just any matching */
	maze->h = 0;
	maze->pen=0;
	for (i=0; i<maze->number_stones; i++) {
		maze->lbtable[i].stoneidx = i;
		maze->lbtable[i].goalidx  = i;
		maze->lbtable[i].distance = 
			GetWeight(maze,maze->goals[i].loc,maze->stones[i].loc);
		maze->h += maze->lbtable[i].distance;
	}
	for (i=maze->number_stones; i<maze->number_goals; i++) {
		maze->lbtable[i].stoneidx = -1;
		maze->lbtable[i].goalidx  = -1;
		maze->lbtable[i].distance = -1;
	}
	MinMatch(maze,0,NULL);
	return(maze->h);
}

int MinMatch(MAZE *maze, PHYSID moveto, UNMOVE *unmove) {
/*********************************************************************
   if moveto == 0 then we need to force a full lb calculation, otherwise 
   go for optimizations using the fact that only one stone was moved.
*********************************************************************/
	int i,cost,cost_swap, min_h, next_in, next_out, heur_diff, dist_diff;
	int heur_stone, dist_stone;
	int stonei,goali,stonej,goalj, disti, distj, distk;
	static short levels[MAXSTONES];
	static int stack[MAXSTONES*MAXSTONES];

	maze->h  -= maze->pen;
	maze->pen = GetPenalty(maze->conflicts,maze->stone,maze->manpos);
	maze->h  += maze->pen;
	if (moveto!=0) {
	    min_h  = maze->h-unmove->move_dist;
	    stonei = maze->PHYSstone[moveto];
	    goali  = maze->lbtable[stonei].goalidx;
	    disti  = GetWeight(maze,maze->goals[goali].loc,
		   		maze->stones[stonei].loc);
	    i      = disti - maze->lbtable[stonei].distance;
	    maze->lbtable[stonei].distance = disti;
	    maze->h += i;
	    if (maze->h==min_h) {
		return(maze->h);
	    }
	    stack[0] = stonei;
	    next_in  = 1;
	} else {
	    for (i=0; i<maze->number_stones; i++) stack[i]=i;
	    next_in  = maze->number_stones;
	    min_h    = 0;
	}
	memset(levels,0,sizeof(short)*maze->number_stones);
	next_out = 0;
	do {
	    /* only the stone that move and then that one that was traded is
	     * of interest, we don't have to to loop over all stones! */
	    stonei = stack[next_out++];
	    goali  = maze->lbtable[stonei].goalidx;
	    disti  = maze->lbtable[stonei].distance;
	    heur_diff = 0;  /* is negative, minimized!!!! */
	    heur_stone= -1;
	    dist_diff = 0;  /* is negative, closer goal found */
	    dist_stone= -1;
	    /* Loop through the goals to find the closest */
	    for (goalj=0; goalj<maze->number_goals; goalj++) {
		stonej = maze->lbtable[goalj].stoneidx;
		if (stonei == stonej) {
			continue;
		}
#ifdef NEEDED
		if (LogAndBS( maze->needed[maze->stones[stonej].loc]
			     ,maze->needed[maze->stones[stonei].loc]))
			continue;
#endif
		/* There are at least that many stones as goalj */
		SR(Assert(stonei>=0,"MinMatch: stonei < 0!\n"));
		/* find out if we should swop goals or not */
		/* cost as it is right now */
		cost = (distk = GetWeight(maze,maze->goals[goalj].loc,
			      maze->stones[stonej].loc)) + disti;
		/* cost after swap */
		distj = GetWeight(maze,maze->goals[goalj].loc,
                              maze->stones[stonei].loc);
		cost_swap 
		     = GetWeight(maze,maze->goals[goali].loc,
                              maze->stones[stonej].loc) + distj;
		if (heur_diff > (cost_swap - cost)) {
			/* lower cost matching found */
			heur_diff = (cost_swap - cost);
			heur_stone = stonej;
			if (maze->h+heur_diff==min_h) break;
		} else if (  (cost_swap==cost)
			   &&(distk*disti!=0)
			   &&(dist_diff > (distj-disti))
			   &&(levels[stonei] >= levels[stonej])) {
				/* stonei can be matched to a closer goal */
				dist_diff = distj - disti;
				dist_stone = stonej;
		}
	    }
	    if (heur_diff<0) {
		/* decrease in heuristic value found */
	        maze->h += heur_diff;
		stonej = heur_stone;
	    } else if (dist_diff<0) {
		/* closer goal found for stone that was moved */
		stonej = dist_stone;
	    } else continue;
if (PP) Mprintf( 0, "%i (%i) %i and %i (%i) %i exchange:",
	stonei, maze->lbtable[stonei].distance, maze->lbtable[stonei].goalidx,
	stonej, maze->lbtable[stonej].distance, maze->lbtable[stonej].goalidx);
	    goalj = maze->lbtable[stonej].goalidx;
	    maze->lbtable[goali].stoneidx = stonej;
	    maze->lbtable[goalj].stoneidx = stonei;
	    maze->lbtable[stonei].goalidx = goalj;
	    maze->lbtable[stonej].goalidx = goali;
	    disti = maze->lbtable[stonei].distance;
	    maze->lbtable[stonei].distance = 
		GetWeight(maze,maze->goals[goalj].loc,
			maze->stones[stonei].loc);
	    distj = maze->lbtable[stonej].distance;
	    maze->lbtable[stonej].distance = 
		GetWeight(maze,maze->goals[goali].loc,
			maze->stones[stonej].loc);
if (PP) Mprintf( 0, "%i (%i) %i and %i (%i) %i\n",
	stonei, maze->lbtable[stonei].distance, maze->lbtable[stonei].goalidx,
	stonej, maze->lbtable[stonej].distance, maze->lbtable[stonej].goalidx);
	    stack[next_in++]=stonej;
	    stack[next_in++]=stonei;
	    if (disti>maze->lbtable[stonei].distance) levels[stonei]++;
	    if (distj>maze->lbtable[stonej].distance) levels[stonej]++;
	    SR(Assert(next_in<MAXSTONES*MAXSTONES,"MinMatch: Stack to small!\n"));
	} while (next_out < next_in && maze->h>min_h );
if (PP) Mprintf( 0, "end: %i\n",maze->h);
	SR(Assert(maze->h>=0,"MinMatch: heuristc < 0!\n"));
#ifdef COPY_LB_TABLE
	/* if we undo moves without copying the lb_table, we might get a
	 * larger reduction */
	SR(Assert(maze->h>=min_h,"MinMatch: Steep drop off, overestimated %i<%i!\n",maze->h,min_h));
#endif
	return(maze->h);
}

int BetterUpdateLowerBound(MAZE *maze, UNMOVE *unmove) {
/* Asumtion: Move is already made */
#ifdef COPY_LB_TABLE
        memcpy(unmove->old_lbentries,maze->lbtable,
                sizeof(LBENTRY)*maze->number_stones);
	unmove->old_h = maze->h;
#endif
	return(MinMatch(maze,unmove->stoneto,unmove));
}

int BetterUpdateLowerBound2(MAZE *maze, UNMOVE *unmove) {
/* Asumtion: Move is already un-made */
#ifdef COPY_LB_TABLE
        memcpy(maze->lbtable,unmove->old_lbentries,
                sizeof(LBENTRY)*maze->number_stones);
	maze->h  -= maze->pen;
	maze->pen = GetPenalty(maze->conflicts,maze->stone,maze->manpos);
	maze->h  += maze->pen;
        maze->h   = unmove->old_h;
#else
	MinMatch(maze,unmove->stonefrom,unmove);
#endif
	return(maze->h);
}

int PlainLowerBound(MAZE *maze)
{
	int i;

	/* initialize table with just any matching */
	maze->h = 0;
	for (i=0; i<maze->number_stones; i++) {
		maze->lbtable[i].stoneidx = i;
		maze->lbtable[i].goalidx  = i;
		maze->lbtable[i].distance = 
			GetShortestWeight(maze,maze->goals[i].loc,maze->stones[i].loc);
		maze->h += maze->lbtable[i].distance;
	}
	for (i=maze->number_stones; i<maze->number_goals; i++) {
		maze->lbtable[i].stoneidx = -1;
		maze->lbtable[i].goalidx  = -1;
		maze->lbtable[i].distance = -1;
	}
	maze->pen=0;
	PlainMinMatch(maze,0,NULL);
	return(maze->h);
}

int PlainMinMatch(MAZE *maze, PHYSID moveto, UNMOVE *unmove)
{
/*********************************************************************
   if moveto == 0 then we need to force a full lb calculation, otherwise 
   go for optimizations using the fact that only one stone was moved.
*********************************************************************/
	int i,cost,cost_swap, min_h, next_in, next_out, heur_diff, dist_diff;
	int heur_stone, dist_stone;
	int stonei,goali,stonej,goalj, disti, distj, distk;
	static short levels[MAXSTONES];
	static int stack[MAXSTONES*MAXSTONES];

	if (moveto!=0) {
	    min_h  = maze->h-unmove->move_dist;
	    stonei = maze->PHYSstone[moveto];
	    goali  = maze->lbtable[stonei].goalidx;
	    disti  = GetShortestWeight(maze,maze->goals[goali].loc,
		   		maze->stones[stonei].loc);
	    i      = disti - maze->lbtable[stonei].distance;
	    maze->lbtable[stonei].distance = disti;
	    maze->h += i;
	    if (maze->h==min_h) {
		return(maze->h);
	    }
	    stack[0] = stonei;
	    next_in  = 1;
	} else {
	    for (i=0; i<maze->number_stones; i++) stack[i]=i;
	    next_in  = maze->number_stones;
	    min_h    = 0;
	}
	memset(levels,0,sizeof(short)*maze->number_stones);
	next_out = 0;
	do {
	    /* only the stone that move and then that one that was traded is
	     * of interest, we don't have to to loop over all stones! */
	    stonei = stack[next_out++];
	    goali  = maze->lbtable[stonei].goalidx;
	    disti  = maze->lbtable[stonei].distance;
	    heur_diff = 0;  /* is negative, minimized!!!! */
	    heur_stone= -1;
	    dist_diff = 0;  /* is negative, closer goal found */
	    dist_stone= -1;
	    /* Loop through the goals to find the closest */
	    for (goalj=0; goalj<maze->number_goals; goalj++) {
		stonej = maze->lbtable[goalj].stoneidx;
		if (stonei == stonej) {
			continue;
		}
#ifdef NEEDED
		if (LogAndBS( maze->needed[maze->stones[stonej].loc]
			     ,maze->needed[maze->stones[stonei].loc]))
			continue;
#endif
		/* There are at least that many stones as goalj */
		SR(Assert(stonei>=0,"PlainMinMatch: stonei < 0!\n"));
		/* find out if we should swop goals or not */
		/* cost as it is right now */
		cost = (distk = GetShortestWeight(maze,maze->goals[goalj].loc,
			      maze->stones[stonej].loc)) + disti;
		/* cost after swap */
		distj = GetShortestWeight(maze,maze->goals[goalj].loc,
                              maze->stones[stonei].loc);
		cost_swap 
		     = GetShortestWeight(maze,maze->goals[goali].loc,
                              maze->stones[stonej].loc) + distj;
		if (heur_diff > (cost_swap - cost)) {
			/* lower cost matching found */
			heur_diff = (cost_swap - cost);
			heur_stone = stonej;
			if (maze->h+heur_diff==min_h) break;
		} else if (  (cost_swap==cost)
			   &&(distk*disti!=0)
			   &&(dist_diff > (distj-disti))
			   &&(levels[stonei] >= levels[stonej])) {
				/* stonei can be matched to a closer goal */
				dist_diff = distj - disti;
				dist_stone = stonej;
		}
	    }
	    if (heur_diff<0) {
		/* decrease in heuristic value found */
	        maze->h += heur_diff;
		stonej = heur_stone;
	    } else if (dist_diff<0) {
		/* closer goal found for stone that was moved */
		stonej = dist_stone;
	    } else continue;
if (PP) Mprintf( 0, "%i (%i) %i and %i (%i) %i exchange:",
	stonei, maze->lbtable[stonei].distance, maze->lbtable[stonei].goalidx,
	stonej, maze->lbtable[stonej].distance, maze->lbtable[stonej].goalidx);
	    goalj = maze->lbtable[stonej].goalidx;
	    maze->lbtable[goali].stoneidx = stonej;
	    maze->lbtable[goalj].stoneidx = stonei;
	    maze->lbtable[stonei].goalidx = goalj;
	    maze->lbtable[stonej].goalidx = goali;
	    disti = maze->lbtable[stonei].distance;
	    maze->lbtable[stonei].distance = 
		GetShortestWeight(maze,maze->goals[goalj].loc,
			maze->stones[stonei].loc);
	    distj = maze->lbtable[stonej].distance;
	    maze->lbtable[stonej].distance = 
		GetShortestWeight(maze,maze->goals[goali].loc,
			maze->stones[stonej].loc);
if (PP) Mprintf( 0, "%i (%i) %i and %i (%i) %i\n",
	stonei, maze->lbtable[stonei].distance, maze->lbtable[stonei].goalidx,
	stonej, maze->lbtable[stonej].distance, maze->lbtable[stonej].goalidx);
	    stack[next_in++]=stonej;
	    stack[next_in++]=stonei;
	    if (disti>maze->lbtable[stonei].distance) levels[stonei]++;
	    if (distj>maze->lbtable[stonej].distance) levels[stonej]++;
	    SR(Assert(next_in<MAXSTONES*MAXSTONES,"PlainMinMatch: Stack to small!\n"));
	} while (next_out < next_in && maze->h>min_h );
if (PP) Mprintf( 0, "end: %i\n",maze->h);
	SR(Assert(maze->h>=0,"PlainMinMatch: heuristc < 0!\n"));
#ifdef COPY_LB_TABLE
	/* if we undo moves without copying the lb_table, we might get a
	 * larger reduction */
	SR(Assert(maze->h>=min_h,"PlainMinMatch: Steep drop off, overestimated %i<%i!\n",maze->h,min_h));
#endif
	return(maze->h);
}

