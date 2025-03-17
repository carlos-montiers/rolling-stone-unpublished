#include "board.h"

/***********************************************************************/
/*								       */
/* MOVE will now include only stone pushes and is centered on that notion.*/
/* This way from and to in MOVE and UNMOVE will have a different meaning  */
/* than in PATH							       */
/*								       */
/***********************************************************************/

#define SETMOVE(move,v,ifrom,ito)\
	(move).from = ifrom; (move).last_over = ifrom;\
	(move).macro_id = 0; (move).to = ito; (move).value = v;

static int compare_negval(const void *m1, const void *m2) {
	return(((MOVE*)m1)->value - ((MOVE*)m2)->value);
}

int IsGoalNodeBack(int g)
{
	int i;

	if (   (IdaInfo->IdaMaze->h==0)
	    && (IsBitSetBS(IdaInfo->IdaMaze->reach,
			   IdaInfo->IdaMaze->goal_manpos))) {
		if (g < IdaInfo->CurrentSolutionDepth) {
			IdaInfo->CurrentSolutionDepth=g;
			for (i=0; i<g; i++) {
				IdaInfo->IdaArray[i].solution
					= IdaInfo->IdaArray[i].currentmove;
			}
		}
		if (g > IdaInfo->Threshold) {
			Debug(0,0,"Premature Goal Found! Depth: %d\n",g);
			return(0);
		} else return(1);
	} else {
		return(0);
	}
}

int BackGenerateMoves(MAZE *maze, MOVE *moves) 
{
	PHYSID pos;
	int i,dir,dist;
	int number_moves = 0;
	BitString stones_done;

	CopyBS(stones_done,maze->stones_done);
	for (i=0; i<maze->number_stones; i++) {
		pos = maze->stones[i].loc;
		if (  (maze->PHYSstone[ pos ]<0)
		    ||(IsBitSetBS(stones_done,pos)))
			continue;
		SetBitBS(stones_done,pos);
		for (dir=NORTH; dir<=WEST; dir++) {
		  if( ( dist = IsBitSetBS(maze->reach,pos + DirToDiff[dir] ) )
			    && (IsBitSetBS(maze->S[OppDir[dir]],
					   pos+DirToDiff[dir]))
			    && (!IsBitSetBS(maze->backdead,pos+DirToDiff[dir]))
			    && (maze->PHYSstone[pos+2*DirToDiff[dir]]<0)) {
				moves[number_moves].from = pos;
				moves[number_moves].to   = pos+DirToDiff[dir];
				moves[number_moves].last_over = 
							pos+2*DirToDiff[dir];
				moves[number_moves].macro_id = 0;
				moves[number_moves].value = dist;
				number_moves++;
			}		
		}		
	}
	moves[number_moves].to = ENDPATH;
	moves[number_moves].from = ENDPATH;
	return(number_moves);
}

int BackMoveOrdering(int depth, int number_moves, MOVE bestmove) 
{
	int  i,diff;
	IDAARRAY *S = &(IdaInfo->IdaArray[depth]);
	MAZE *maze = IdaInfo->IdaMaze;
	MOVE *m,*lmove = &(IdaInfo->IdaArray[depth-1].currentmove);
	PHYSID goalpos;

	if (number_moves>1) {
		for (i=0; i<number_moves; i++) {
			m = &(S->moves[i]);
			if (ISDUMMYMOVE(*m)) {
				m->value = -ENDPATH;
				continue;
			}
			goalpos = maze->goals[
				    maze->lbtable[
				      maze->PHYSstone[m->from]].goalidx].loc;
			m->value =
			      maze->lbtable[maze->PHYSstone[m->from]].distance;
			diff = m->value 
			     - GetBackGoalWeightManpos(maze,goalpos,m->to,
					m->to+(m->to-m->from));
			if (diff>0) {
				if (lmove->to == m->from) 
					m->value -= ENDPATH;
			} else {
				m->value -= diff*100;
			}
		}
		My_qsort(&(S->moves),number_moves,sizeof(MOVE),compare_negval);
	}
	return(number_moves);
}

int BackMakeMove(MAZE *maze, MOVE *move, UNMOVE *ret)
/* this is a routine that makes a STONE move, not merely a man move like
 * DoMove. It will just put the man to the new location */
{
	ret->manfrom  = maze->manpos;
	ret->stonefrom= move->from;
	ret->stoneto  = move->to;
	ret->macro_id = move->macro_id;
	ret->move_dist= BackGetWeight(maze,move->to,move->from);
	maze->goal_sqto = -1;
	CopyBS(ret->old_no_reach,maze->old_no_reach);
	CopyBS(maze->old_no_reach,maze->no_reach);
	ret->old_GMTree = NULL;

	MANTO(maze,move->last_over);
	STONEFROMTO(maze,ret->stonefrom,ret->stoneto);
	UpdateHashKey(maze, ret);
	MarkReach(maze);
	BackUpdateLowerBound(maze, ret);

	SR(Assert(maze->manpos>0,"MakeMove: manpos < 0!\n"));
	return(1);
}

int BackUnMakeMove(MAZE *maze, UNMOVE *unmove)
{
	int      stonei;

	MANTO(maze,unmove->manfrom);
	STONEFROMTO(maze,unmove->stoneto,unmove->stonefrom);

	stonei = maze->PHYSstone[unmove->stonefrom];
		
	UpdateHashKey(maze, unmove);
	MarkReach(maze);
	CopyBS(maze->old_no_reach,unmove->old_no_reach);
	BackUpdateLowerBound2(maze, unmove);

	return(0);
}

int StartBackLowerBound(MAZE *maze)
{
	BitString remaining;
	BitString visible; /* just a dummy for the FindClosest Proc! */
	int       h;
	PHYSID    min_manpos;

	min_manpos = maze->manpos;
	Set1BS(remaining);
	Set0BS(visible);
	BitAndNotEqBS(remaining,maze->out);
	BitAndNotEqBS(remaining,maze->wall);
	BitAndNotEqBS(remaining,maze->stone);
	BitAndNotEqBS(remaining,maze->reach);

	h = maze->h;

	while (Isnt0BS(remaining)) {
		maze->manpos = FindAnySet(remaining);
		MarkReach(maze);
		BackBetterLowerBound(maze);
		if (maze->h < h) {
			h = maze->h;
			min_manpos = maze->manpos;
		}
		BitAndNotEqBS(remaining,maze->reach);
	}
	maze->manpos = min_manpos;
	MarkReach(maze);
	BackBetterLowerBound(maze);
	return(h);
}

int BackBetterLowerBound(MAZE *maze) {
	int i;

	/* initialize table with just any matching */
	maze->h = 0;
	for (i=0; i<maze->number_stones; i++) {
		maze->lbtable[i].stoneidx = i;
		maze->lbtable[i].goalidx  = i;
		maze->lbtable[i].distance = 
		     BackGetGoalWeight(maze,
				       maze->goals[i].loc,maze->stones[i].loc);
		maze->h += maze->lbtable[i].distance;
	}
	for (i=maze->number_stones; i<maze->number_goals; i++) {
		maze->lbtable[i].stoneidx = -1;
		maze->lbtable[i].goalidx  = -1;
		maze->lbtable[i].distance = -1;
	}
	BackMinMatch(maze,0,NULL);
	return(maze->h);
}

int BackMinMatch(MAZE *maze, PHYSID moveto, UNMOVE *unmove) {
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
	    disti  = BackGetGoalWeight(maze,maze->goals[goali].loc,
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
		/* There are at least that many stones as goalj */
		SR(Assert(stonei>=0,"MinMatch: stonei < 0!\n"));
		/* find out if we should swop goals or not */
		/* cost as it is right now */
		cost = (distk = BackGetGoalWeight(maze,maze->goals[goalj].loc,
			      maze->stones[stonej].loc)) + disti;
		/* cost after swap */
		distj = BackGetGoalWeight(maze,maze->goals[goalj].loc,
                              maze->stones[stonei].loc);
		cost_swap 
		     = BackGetGoalWeight(maze,maze->goals[goali].loc,
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
	    goalj = maze->lbtable[stonej].goalidx;
	    maze->lbtable[goali].stoneidx = stonej;
	    maze->lbtable[goalj].stoneidx = stonei;
	    maze->lbtable[stonei].goalidx = goalj;
	    maze->lbtable[stonej].goalidx = goali;
	    disti = maze->lbtable[stonei].distance;
	    maze->lbtable[stonei].distance = 
		BackGetGoalWeight(maze,maze->goals[goalj].loc,
			maze->stones[stonei].loc);
	    distj = maze->lbtable[stonej].distance;
	    maze->lbtable[stonej].distance = 
		BackGetGoalWeight(maze,maze->goals[goali].loc,
			maze->stones[stonej].loc);
	    stack[next_in++]=stonej;
	    stack[next_in++]=stonei;
	    if (disti>maze->lbtable[stonei].distance) levels[stonei]++;
	    if (distj>maze->lbtable[stonej].distance) levels[stonej]++;
	    SR(Assert(next_in<MAXSTONES*MAXSTONES,"MinMatch: Stack to small!\n"));
	} while (next_out < next_in && maze->h>min_h );
	SR(Assert(maze->h>=0,"MinMatch: heuristc < 0!\n"));
	return(maze->h);
}

int BackUpdateLowerBound(MAZE *maze, UNMOVE *unmove) {
/* Asumtion: Move is already made */
	BackMinMatch(maze,unmove->stoneto,unmove);
	return(maze->h);
}

int BackUpdateLowerBound2(MAZE *maze, UNMOVE *unmove) {
/* Asumtion: Move is already un-made */
	BackMinMatch(maze,unmove->stonefrom,unmove);
	return(maze->h);
}

int BackMoveSuspected(MAZE *maze, MOVE *last_move, HASHENTRY *entry)
{
/* return 1 if we suspect a deadlock search is beneficial, here should be
 * all the heuristic stuff that we hope will basically have a good guess at
 * the final outcome of the proove search - sort of like move ordering in
 * alpha-beta */

	BitString new_no_reach;

	if (entry->pensearched==1) return(0);
	if (entry->tree_size > 20) return(1);

	/* always check for deadlocks if we move onto one_way squares */
	if (IsBitSetBS(maze->one_way,last_move->to)) return(1);

	/* check if we created a new inaccessible area by subtracting the
	 * old_no_reach from the no_reach and counting the number of fields
	 * left, this number is an indicator of when we should expect to run
	 * into problems! */

	BitAndNotBS(new_no_reach,maze->no_reach,maze->old_no_reach);
	if (NumberBitsBS(new_no_reach) < 1)
		return(0);
	return(1);
}

void BackGoalsStones(MAZE *maze)
{
	int i;
	BitString tmp;
	PHYSID old_stone_loc, old_goal_loc;

	for (i = 0; i < maze->number_goals; i++) {
		old_stone_loc = maze->stones[i].loc;
		old_goal_loc  = maze->goals[i].loc;

		maze->goals[i].loc  = old_stone_loc;
		maze->goals[i].weight = 0;
		maze->stones[i].loc = old_goal_loc;
		maze->PHYSstone[old_stone_loc] = -1;
		maze->Phys[old_goal_loc].goal = -1;
		maze->Phys[old_stone_loc].goal = i;
		maze->PHYSstone[old_goal_loc] = i;
	}
	CopyBS(tmp, maze->stone);
	CopyBS(maze->stone, maze->goal);
	CopyBS(maze->goal,tmp);

	BackSetGoalWeights(maze);
	NormHashKey(maze);
	MarkReach(maze);
	BackBetterLowerBound(maze);
	StartBackLowerBound(maze);
}

int BackStartIda() {
/* Sets up all data structures and repeatedly calls ida with increasing 
   threshold to guarantee optimal solutions, returns 0 if solution found 
   otherwise the smallest heuristic value seen at any leaf node if this is
   ENDPATH there is no solution - deadlock */
/* Remember: A few things are different here.
	1) We need to mark all squares reachable at the root since we don't
	   know where the man will end up, so any square could be it.
	2) We need to set the manpos to where the lowest lower bound can 
	   be achieved (since it is a LOWER bound :( ). This is done in
	   StartBackLowerBound() which is called by BackGoalsStones(). */

	int       result=ENDPATH;
	long      last_tree_size;
	PHYSID    pos;
	MAZE	 *old_maze;

	/* initialize data structures */
	old_maze = IdaInfo->IdaMaze;
	IdaInfo->IdaMaze = CopyMaze(old_maze);
	BackGoalsStones(IdaInfo->IdaMaze);

	InitHashTables();
	total_node_count = 0;
	IdaInfo->node_count = 0;
	dl_pos_nc = dl_pos_sc = dl_neg_nc = dl_neg_sc = 0;
	pen_pos_nc = pen_pos_sc = pen_neg_nc = pen_neg_sc = 0;
	IdaInfo->CurrentHashGen = NextHashGen++;
	AvoidThisSquare = 0;
	init_stats();
	Set0BS(IdaInfo->IdaManSquares);
	Set0BS(IdaInfo->IdaStoneSquares);
	IdaInfo->CurrentSolutionDepth = ENDPATH;

	for (IdaInfo->Threshold = IdaInfo->IdaMaze->h;
	       (IdaInfo->CurrentSolutionDepth > IdaInfo->Threshold)
	     &&(!AbortSearch());
	     IdaInfo->Threshold += IdaInfo->ThresholdInc) {
		SR(Debug(2,0,"Threshold %i (%i) [%i]\n",
			IdaInfo->Threshold,
			IdaInfo->IdaMaze->h,IdaInfo->node_count));
		GTVAny(GTVOpen(IdaInfo->Threshold,
			       GTVFen(IdaInfo->IdaMaze)));
		last_tree_size = IdaInfo->r_tree_size;
		IdaInfo->r_tree_size=0;
		IdaInfo->v_tree_size=0;
		IdaInfo->IdaMaze->goal_sqto = -1;
		/* mark where man needs to end up and mark all squares 
		 * as reachable to allow for every possible "last" 
		 * move (man location) */
		for (pos=0; pos<YSIZE*XSIZE; pos++) {
			if (  IsBitSetBS(IdaInfo->IdaMaze->out,pos)
		    	    ||IsBitSetBS(IdaInfo->IdaMaze->stone,pos)
		    	    ||IsBitSetBS(IdaInfo->IdaMaze->wall,pos)) continue;
			SetBitBS(IdaInfo->IdaMaze->reach,pos);
/* 			IdaInfo->IdaMaze->PHYSreach[ pos ] = 0; */
		}
		result = BackIda(0,0); /**********************************/
		GTVAny(GTVClose());
		print_stats(2);
		if (result>=ENDPATH) 
			IdaInfo->Threshold = ENDPATH + IdaInfo->ThresholdInc;
	}
	IdaInfo->Threshold -= IdaInfo->ThresholdInc;
	if (result<ENDPATH) result = IdaInfo->Threshold - IdaInfo->IdaMaze->h;
	BackPrintSolution();
	DelCopiedMaze(IdaInfo->IdaMaze);
	IdaInfo->IdaMaze = old_maze;
	return(result);
}


void BackPrintSolution()
{
	MAZE     *maze;
	MOVE      lastmove;
	MOVE      solution[ENDPATH];
	UNMOVE    unmove;
	int       i,g;

	Debug(0,-1,"Path: ");
	i = 0; g = 0;
	lastmove = DummyMove;
	maze = CopyMaze(IdaInfo->IdaMaze);
	while (IdaInfo->IdaArray[i].solution.from != 0 && maze->h>0) {
		solution[i]=IdaInfo->IdaArray[i].solution;
		if (lastmove.to==IdaInfo->IdaArray[i].solution.from) {
			Debug(0,-1,"%s", 
				HumanMove(IdaInfo->IdaArray[i].solution)+2);
		} else {
			Debug(0,-1," %s", 
				HumanMove(IdaInfo->IdaArray[i].solution));
		}	
		lastmove = IdaInfo->IdaArray[i].solution;
		BackMakeMove(maze,&(IdaInfo->IdaArray[i].solution),&unmove);
		if (IdaInfo->IdaArray[i].unmove.move_dist>1) {
			Debug(0,-1,"*");
		}
		g += IdaInfo->IdaArray[i].unmove.move_dist;
		i++;
	}
	Debug(0,-1,"\n(moves: %i depth: %i)\n",g,i);
	solution[i] = DummyMove;
	DelCopiedMaze(maze);

	maze = CopyMaze(IdaInfo->IdaMaze);
	DelCopiedMaze(maze);
}

int BackIda(int treedepth, int g) {
/* the procedure that does the work at one node. it returns 
	X - the smallest h underneath this node */

	IDAARRAY  *S;
	HASHENTRY *entry;
	MOVE       bestmove;
	int 	   min_h,number_moves,result,i;
	long	   tree_size_save;
	long	   max_tree_size, tmp_tree_size;
	int old_h = IdaInfo->IdaMaze->h- IdaInfo->IdaMaze->pen;

        SR(Debug(4,treedepth,"starting ida (h=%i) (%s)\n",IdaInfo->IdaMaze->h,
          treedepth==0?"a1a1"
		      :PrintMove(IdaInfo->IdaArray[treedepth-1].currentmove)));
	S = &(IdaInfo->IdaArray[treedepth]);
	GTVAny(GTVNodeEnter(treedepth,g,0,GTVMove(treedepth
		?IdaInfo->IdaArray[treedepth-1].currentmove:DummyMove),0));
	if (AbortSearch()) {
		GTVAny(GTVNodeExit(treedepth,0,"Abort_Search"));
		return(IdaInfo->IdaMaze->h);
	}
	if (treedepth >=MAX_DEPTH) {
		SR(Debug(4,treedepth,"Too deep in the tree!(%i)\n",treedepth));
		GTVAny(GTVNodeExit(treedepth,0,"Too_deep_in_tree"));
		return(IdaInfo->IdaMaze->h);
	}
	tree_size_save = IdaInfo->v_tree_size;
	IncNodeCount(treedepth);

	/* check for goal state, if yes return(0) */
	if (IsGoalNodeBack(g)) {
		SR(Debug(4,treedepth,"Found goal (%i %i)******************\n",
			IdaInfo->IdaMaze->h, IdaInfo->IdaMaze->number_stones));
		GTVAny(GTVNodeExit(treedepth,0,"Goal_found"));
		return(0);
	}

	/* The following order is important, otherwise we might find a goal
	   an iteration earlier! */
	/* check for cutoff: is g+h > threshold => return(0) (fail) */
	if (g+IdaInfo->IdaMaze->h>IdaInfo->Threshold) {
		SR(Debug(4,treedepth,"Threshold cutoff (%i=%i+%i)\n", 
			g+IdaInfo->IdaMaze->h,g,IdaInfo->IdaMaze->h));
		GTVAny(GTVNodeExit(treedepth,
				   IdaInfo->IdaMaze->h,"Threshold_Cutoff"));
		return(IdaInfo->IdaMaze->h);
	}

	/* check trans table if searched already */
	entry = GetHashTable(IdaInfo->IdaMaze);
	if (entry != NULL) {
		if (entry->backward == 0) {
Mprintf( 0, "BackIda: Connected!!!!*****************\n");
			IdaInfo->CurrentSolutionDepth=IdaInfo->Threshold;
			for (i=0; i<g; i++) {
				IdaInfo->IdaArray[i].solution
					= IdaInfo->IdaArray[i].currentmove;
			}
			SR(Debug(4,treedepth,"Connect (to frnt) found ****\n"));
			GTVAny(GTVNodeExit(treedepth,0,"Connect_tofrnt_found"));
			return(0);
		} else if (IdaInfo->Threshold - g <= entry->down) {
			SR(Debug(4,treedepth, "Futil (TT) %i<=%i\n",
				IdaInfo->Threshold-g,entry->down));
			GTVAny(GTVNodeExit(treedepth,ENDPATH,"Futil (TT)"));
			IdaInfo->v_tree_size+=entry->tree_size;
			IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
			return(entry->min_h);
		}
		bestmove = entry->bestmove;
	} else {
		bestmove = DummyMove;
		StoreHashTable(IdaInfo->IdaMaze,g,IdaInfo->Threshold-g,
			ENDPATH,bestmove,0,0,0,1,1);
	}
	if (treedepth >= IdaInfo->BackDepthLimit) {
		SR(Debug(4,treedepth,"Too deep in the tree!(%i)\n",treedepth));
		GTVAny(GTVNodeExit(treedepth,0,"Too_deep_in_tree"));
		return(IdaInfo->IdaMaze->h);
	}

	min_h    = ENDPATH;
	max_tree_size = -1;

	/* create and sort (bestmove first + extension of previous stone) 
	   moves */
	number_moves = BackGenerateMoves(IdaInfo->IdaMaze,&(S->moves[0]));
	number_moves = BackMoveOrdering(treedepth,number_moves,bestmove);

	/* foreach move call ida */
	for (i=0; i<number_moves; i++) {
		if (ISDUMMYMOVE(S->moves[i])) continue;
		S->currentmove = S->moves[i];
		SR(Debug(6,treedepth,"BackMakeMove %s (%i of %i)\n", 
			PrintMove(S->currentmove),i+1,number_moves));
		/* make sure the man is put at all the positions in the
		 * first level (we don't know where it end up! */
		if (g==0) MANTO(IdaInfo->IdaMaze,S->currentmove.to);
		if (!BackMakeMove(IdaInfo->IdaMaze,&(S->currentmove),&(S->unmove))){
			continue;
		}
		tmp_tree_size = IdaInfo->v_tree_size;
		result = BackIda(treedepth+1,g+S->unmove.move_dist);
		tmp_tree_size = IdaInfo->v_tree_size - tmp_tree_size;
		BackUnMakeMove(IdaInfo->IdaMaze,&(S->unmove));
		if (old_h != IdaInfo->IdaMaze->h-IdaInfo->IdaMaze->pen)
			BackBetterLowerBound(IdaInfo->IdaMaze);
		if (result < min_h) {
			min_h = result;
			bestmove = S->currentmove;
		}
		if (tmp_tree_size>max_tree_size) {
			max_tree_size = tmp_tree_size;
		}
		/* one solution found? */
		if (result == 0) {
			S->solution = S->currentmove;
			bestmove = S->currentmove;
			SetManStoneSquares(IdaInfo->IdaMaze,bestmove);
			goto END_IDA;
		}
	}

END_IDA:
	/* write Transposition table */
	StoreHashTable(IdaInfo->IdaMaze,g,IdaInfo->Threshold-g,min_h,bestmove,
		       IdaInfo->v_tree_size-tree_size_save,0,0,1,0);
	SR(Debug(4,treedepth,"return from ida %i (%s %i)\n", 
		treedepth,
		PrintMove(bestmove),
		min_h));
	GTVAny(GTVNodeExit(treedepth,min_h,"Normal_Exit"));
	return(min_h);
}

void BackSetGoalWeights(MAZE *maze) 
{
/* Reworked 13.02.97: Just adding all the distances from squares NOT OUT
 * should give us a good idea what goal is most distant. Then sorting them
 * and giving them the rank should give us a good priority order */
	int    i,j,done, max;
	WEIGHT curr_prior,last_prior;
	long   weight[MAXGOALS], last_weight;

	for (j=0; j<maze->number_goals; j++) {
		maze->goals[j].weight = 0;
		weight[j]=0;
		for (i=0; i<XSIZE*YSIZE; i++) {
			if (maze->PHYSstone[i] >= 0) {
				weight[j] +=
				      BackGetGoalWeight(maze,
				       			maze->goals[j].loc,i);
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
