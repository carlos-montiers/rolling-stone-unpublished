#include "board.h"

/* This stuff does not work yet since shadow stones are influencing the
 * decision about somehting being a deadlock or not, the result of the
 * DeadMove search depends on the position of the shadow stones, which it
 * should not. */

int DeadIsGoalNode(int g)
{
	if (IdaInfo->IdaMaze->number_stones == 0 || IdaInfo->IdaMaze->h==0) {
		IdaInfo->CurrentSolutionDepth=g;
		SR(Debug(3,0,"DeadIsGoalNode: no stones left\n"));
		return(1);
	}
	if (  (NumberBitsBS(IdaInfo->IdaMaze->no_reach)==0)
	    &&(IdaInfo->closest_confl != 0)) {
		IdaInfo->CurrentSolutionDepth=g;
		SR(Debug(3,0,"DeadIsGoalNode: all reachable\n"));
		return(1);
	}
	return(0);
}

/* Try to find out if we can move this stone still to a goal,
   include those that might be creating a deadlock,
   after finding that this is a deadlock, find the minimal set of
   stones belonging to a deadlock */
int  DeadMove(MAZE *maze, HASHENTRY *entry, MOVE *last_move, 
		int treedepth, int g, int *dlsearched, long effort)
{
	BitString	visible,relevant;
	PHYSID		pos;
	IDA		idainfo,*old_idainfo;
	long		node_count;
	int		result;
	int		number_stones;
	int		old_gm;
	SR(int here_nodes = total_node_count);

	/* don't bother if one of the following is true */
	if (   Options.dl_srch == 0
	    || last_move->macro_id != 0
	    || treedepth == 0) {
		SR(Debug(3,0,"DeadMove ## End search - Not even considered\n"));
		return(0);
	}
	/* don't search, but try finding Penalties */
	if (!DeadMoveSuspected(maze, last_move, entry)) {
		SR(Debug(3,0,"DeadMove #### End search - Not suspected\n"));
		return(0);
	}

	*dlsearched = 1;
	old_idainfo = IdaInfo;
	InitIDA(&idainfo);
	IdaInfo                 = &idainfo;
	IdaInfo->IdaMaze        = CopyMaze(maze);
	IdaInfo->ThresholdInc   = 2;
	IdaInfo->AbortNodeCount = effort;
	IdaInfo->goal_last_to   = last_move->to;
	IdaInfo->closest_confl  = 0;
	IdaInfo->base_indent    = old_idainfo->base_indent + treedepth;
	IdaInfo->PrintPriority  = DEADPATTERNSEARCHPP;
	IdaInfo->HashTable      = HashTableElse;
	old_gm = Options.mc_gm;
	Options.mc_gm=0;
	CopyBS(IdaInfo->shadow_stones,maze->stone);
	BitNotAndNotBS(IdaInfo->no_reach,maze->reach,maze->out);
	Set0BS(visible); Set0BS(relevant);
	SetBitBS(visible,last_move->to);
	node_count    = 0;
	number_stones = 1;

	SR(Debug(3,0,"DeadMove #### Start search\n"));
	for (;;) {
		SR(Debug(3,0,"DeadMove Iteration, stones: %i\n", 
			number_stones));
		DeadDeactivateStones(IdaInfo->IdaMaze,visible);
		IdaInfo->node_count    = 0;
NavigatorSetPSMaze(IdaInfo->IdaMaze,visible,maze->stone);
		result                 = DeadStartIda();
		node_count            += IdaInfo->node_count;
		if (AbortSearch()) {
			SR(Debug(3,0,"DeadMove: too many nodes: %i\n",
			      IdaInfo->node_count));
			break;
		}
		BitOrEqBS(relevant,IdaInfo->IdaManSquares);
		if (result>=ENDPATH) {
			DConflictStartIda(0);
			dl_pos_nc += IdaInfo->node_count;
			dl_pos_sc ++;
			DelCopiedMaze(IdaInfo->IdaMaze);
			IdaInfo = old_idainfo;
			SR(Debug(3,0,"DeadMove ## End search - DEAD (nodes:"));
			SR(Debug(3,0," %li, stones: %i, result: %i)\n", 
				node_count, number_stones, result));
			Options.mc_gm=old_gm;
NavigatorUnsetPSMaze();
			return(1);
		}
		/* Turn off all goal stones */
		BitAndNotEqBS(IdaInfo->IdaStoneSquares,IdaInfo->IdaMaze->goal);
		/* if a stone is blocked, try that block, else man blocks */
		pos = FindClosestPosStone(maze,
			IdaInfo->IdaStoneSquares, visible);
		if (pos > 0) SetBitBS(visible,pos);
		else {
			BitAndNotEqBS(IdaInfo->IdaManSquares,
				IdaInfo->IdaMaze->goal);
			pos = FindClosestPosMan(maze, 
				IdaInfo->IdaManSquares, visible);
			if (pos > 0) SetBitBS(visible,pos);
			else break;
		}
		/* If we have too many stones in the works, abort */
		number_stones = NumberBitsBS(visible);
		if (number_stones>min(PATTERNSTONES,(maze->number_stones-2))) {
			SR(Debug(3,0,"DeadMove: too many stones: %i\n",
			      number_stones));
			break;
		}
	}
	AddTestedDead(maze->conflicts,relevant,
			   old_idainfo->IdaMaze->stone,
			   IdaInfo->IdaMaze->manpos,last_move->to);
	DelCopiedMaze(IdaInfo->IdaMaze);
	dl_neg_nc += IdaInfo->node_count;
	dl_neg_sc ++;
	IdaInfo = old_idainfo;
	SR(Debug(3,0,"DeadMove ## End search - ALIVE (nodes: %li stones: %i)\n",
		node_count,number_stones));
	Options.mc_gm=old_gm;
NavigatorUnsetPSMaze();
	return(0);      
}

void DeadDeactivateStones(MAZE *maze, BitString visible)
{
	PHYSID    pos;

	maze->number_stones=0;
	CopyBS(maze->stone,visible);
	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (IsBitSetBS(visible,pos)) {
			maze->PHYSstone[ pos ] = maze->number_stones;
			maze->stones[maze->number_stones++].loc = pos;
		} else {
			maze->PHYSstone[ pos ] = -1;
		}
	}
	NormHashKey(maze);
	MarkReach(maze);
	DeadLowerBound(maze);
}

#ifdef RANDOM
extern double percent;
#endif

int DeadMoveSuspected(MAZE *maze, MOVE *last_move, HASHENTRY *entry)
{
/* return 1 if we suspect a deadlock search is beneficial, here should be
 * all the heuristic stuff that we hope will basically have a good guess at
 * the final outcome of the proove search - sort of like move ordering in
 * alpha-beta */

	BitString new_no_reach;

	if (entry->dlsearched==1) return(0);
	if (entry->tree_size > 20) goto CHECK_TESTED;
	if (NoMoreMoves(maze,last_move)) goto CHECK_TESTED;

	/* always check for deadlocks if we move onto one_way squares */
	if (IsBitSetBS(maze->one_way,last_move->to)) goto CHECK_TESTED;

	/* int c_old,c_new;
	c_old = NumberBitsBS(maze->old_no_reach);
	c_new = NumberBitsBS(maze->no_reach);
	if (c_old == c_new + 1) goto CHECK_TESTED; */

	/* check if we created a new inaccessible area by subtracting the
	 * old_no_reach from the no_reach and counting the number of fields
	 * left, this number is an indicator of when we should expect to run
	 * into problems! */

	BitAndNotBS(new_no_reach,maze->no_reach,maze->old_no_reach);
	if (NumberBitsBS(new_no_reach) == 0) return(0);

	/* a new inaccessible area was created, check now if we tested it
	 * already. */
CHECK_TESTED:
	if(WasTestedDead(maze->conflicts,maze->stone,maze->manpos,last_move->to))
	{
		return(0);
	}
#ifdef RANDOM
if( ( (double)random() / RANDOM_MAX ) < percent )
  return 1;
else
  return 0;
#else
	return(1);
#endif
}

PHYSID FindClosestPosStone(MAZE *maze, BitString squares, 
		      		  BitString already_visible)
/* Find the square (position) the in squares that has a stone on it
 * that is not already visible */
{
	int    stonei;
	int    dist = ENDPATH;
	PHYSID pos  = 0,p;
	
	for (stonei=0; stonei<maze->number_stones; stonei++) {
		p = maze->stones[stonei].loc;
		if (  !IsBitSetBS(already_visible,p)
		    && IsBitSetBS(squares,p)) {
                        if (  dist > XDistStone(maze,maze->manpos,p)) {
                                dist = XDistStone(maze,maze->manpos,p);
				pos  = p;
			}
		}
	}
	return(pos);
}

PHYSID FindClosestPosMan(MAZE *maze, BitString squares, 
		      		  BitString already_visible)
/* Find the square (position) the in squares that has a stone on it
 * that is not already visible */
{
	int    stonei;
	int    dist = ENDPATH;
	PHYSID pos  = 0,p;
	
	for (stonei=0; stonei<maze->number_stones; stonei++) {
		p = maze->stones[stonei].loc;
		if (  !IsBitSetBS(already_visible,p)
		    && IsBitSetBS(squares,p)) {
			if (  dist > XDistMan(maze,maze->manpos,p)) {
				dist = XDistMan(maze,maze->manpos,p);
				pos  = p;
			}
		}
	}
	return(pos);
}

void DeadMiniConflict(BitString visible)
/* stones marked in visible are a deadlock set, try and minimize the set by
 * removing one stone at a time, finding its necessity in the deadlock */
/* Make sure all stones that were initially on are when we get out */
{
	int    result;
	PHYSID pos;
	long   node_count;
	BitString already;
	int	  old_gm;
	IDA	  idainfo,*old_idainfo;

	SR(Debug(3,0,"DeadMiniConflict #### Start\n"));
	old_idainfo = IdaInfo;
	InitIDA(&idainfo);
	IdaInfo                 = &idainfo;
	CopyBS(IdaInfo->no_reach,old_idainfo->no_reach);
	IdaInfo->IdaMaze        = old_idainfo->IdaMaze;
	IdaInfo->ThresholdInc   = 2;
	IdaInfo->AbortNodeCount = PATTERNSEARCH;
	IdaInfo->goal_last_to   = old_idainfo->goal_last_to;
	IdaInfo->closest_confl  = 0;
	IdaInfo->base_indent   += 2;
	IdaInfo->PrintPriority  = DEADPATTERNSEARCHPP;
	IdaInfo->HashTable      = HashTableElse;
	IdaInfo->MiniFlag	= YES;
	old_gm = Options.mc_gm;
	Options.mc_gm=0;
	node_count = 0;
	Set0BS(already);
	for (;;) {
		pos = FindClosestPosStone(IdaInfo->IdaMaze,visible,already);
		if (pos==0) break;
		SetBitBS(already,pos);
		SR(Debug(3,0,"DeadMiniConflict ### Start iteration\n"));
		UnsetBitBS(visible,pos);
		DeadDeactivateStones(IdaInfo->IdaMaze,visible);
		node_count += IdaInfo->node_count;
		IdaInfo->node_count = 0;
		result = DeadStartIda();
		if (result<ENDPATH) {
			SetBitBS(visible,pos);
		} else {
			SR(Debug(3,0,"DeadMiniConflict Extra Stone\n"));
		}
	}
	DeadDeactivateStones(IdaInfo->IdaMaze,visible);
	Options.mc_gm=old_gm;
	deadmcount += node_count;
	IdaInfo = old_idainfo;
	IdaInfo->node_count += node_count;
	SR(Debug(3,0,"DeadMiniConflict #### End\n"));
}

/* void DeadMiniConflictFull( BitString visible ) */
/* { */
/*   DeadMiniConflictFull2( visible, 0 ); */
/* } */

int DeadMiniConflictFull( BitString visible/* , int depth */ )
{
  BitString unvisited;
  int i, minimal, old_gm, result;
  IDA idainfo,*old_idainfo;

/* printf( "minimizing (%i)\n", depth ); */
/* PrintBit2Maze( IdaInfo->IdaMaze, visible ); */

  /* test if visible is still a deadlock */
  if( NumberBitsBS( visible ) == 1 ) {
/* printf( "only one stone: no deadlock\n" ); */
    return 0; /* one stone - no deadlock */
  }

  /* run the search */
  old_idainfo = IdaInfo;
  InitIDA( &idainfo );
  IdaInfo = &idainfo;
  IdaInfo->IdaMaze = old_idainfo->IdaMaze;
  IdaInfo->ThresholdInc = 2;
  IdaInfo->AbortNodeCount = PATTERNSEARCH;
  IdaInfo->goal_last_to = old_idainfo->goal_last_to;
  IdaInfo->closest_confl = 0;
  IdaInfo->base_indent += 2;
  IdaInfo->PrintPriority = 0;
  IdaInfo->HashTable = HashTableElse;
  old_gm = Options.mc_gm;
  Options.mc_gm = 0;
  DeadDeactivateStones( IdaInfo->IdaMaze, visible );
  CopyBS( IdaInfo->no_reach, IdaInfo->IdaMaze->no_reach );
  IdaInfo->node_count = 0;
  result = DeadStartIda();
  old_idainfo->node_count += IdaInfo->node_count;
  Options.mc_gm = old_gm;
  IdaInfo = old_idainfo;
  if( result < ENDPATH ) {
/* printf( "no deadlock (result %i)\n", result ); */
    return 0; /* no deadlock */
  }

  minimal = 1;
  CopyBS( unvisited, visible );
  while( ( i = FindFirstSet( unvisited ) ) >= 0 ) {
    UnsetBitBS( unvisited, i );
    UnsetBitBS( visible, i );
    if( DeadMiniConflictFull( visible/* , depth + 1 */ ) )
      minimal = 0;
    SetBitBS( visible, i );
  }

  DeadDeactivateStones( IdaInfo->IdaMaze, visible );
  if( minimal ) {
/* printf( "adding (%i)!\n", depth ); */
/* PrintBit2Maze( IdaInfo->IdaMaze, visible ); */
    /* this is locally minimal - add it as a pattern */
    MarkReach( IdaInfo->IdaMaze );
    AddConflict( MainIdaInfo.IdaMaze->conflicts, visible,
		 IdaInfo->IdaMaze->no_reach, IdaInfo->IdaMaze->reach,
		 result, 0 );
  }

  /* this is still a deadlock */
  return 1;
}

int DeadStartIda() {
/* Sets up all data structures and repeatedly calls ida with increasing 
   threshold to guarantee optimal solutions, returns 0 if solution found 
   otherwise the increase of maze->h by Threshold, if this is
   ENDPATH there is no solution - deadlock */

	int       result=ENDPATH;
	long      last_tree_size;

	if (AbortSearch()) return(0);
	/* initialize data structures */
	AvoidThisSquare = 0;
	init_stats();
	Set0BS(IdaInfo->IdaManSquares);
	Set0BS(IdaInfo->IdaStoneSquares);
	IdaInfo->Threshold = IdaInfo->IdaMaze->h;
	IdaInfo->CurrentHashGen = 1;
	IdaInfo->CurrentSolutionDepth = ENDPATH;
	BitNotAndNotBS(IdaInfo->no_reach,IdaInfo->IdaMaze->reach,
					 IdaInfo->IdaMaze->out);
	for (IdaInfo->Threshold = IdaInfo->IdaMaze->h;
	       (IdaInfo->CurrentSolutionDepth > IdaInfo->Threshold)
	     &&(result != 0)
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
		result = DeadIda(0,0); /**********************************/
		GTVAny(GTVClose());
		print_stats(4);
		if (result>=ENDPATH) IdaInfo->Threshold = ENDPATH;
	}
	if (  (IdaInfo->CurrentSolutionDepth<ENDPATH)
	    ||(result==0)
	    ||(AbortSearch()))
		IdaInfo->Threshold -= IdaInfo->ThresholdInc;
	if (result<ENDPATH) result = IdaInfo->Threshold - IdaInfo->IdaMaze->h;
	deadscount += IdaInfo->node_count;
	return(result);
}

static int DeadCompare(const void *m1, const void *m2) {
        return(((MOVE*)m1)->value - ((MOVE*)m2)->value);
}

int DeadMoveOrdering(int depth, int number_moves, MOVE bestmove)
{
        int  i,diff;
        IDAARRAY *S = &(IdaInfo->IdaArray[depth]);
        MAZE *maze = IdaInfo->IdaMaze;
        MOVE *m,*lmove;
        PHYSID goalpos;

        if (number_moves>1) {
                lmove = depth?&(IdaInfo->IdaArray[depth-1].currentmove)
                                :&DummyMove;
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
                        if (m->macro_id != 4) diff = m->value -
                              GetWeightManpos(maze,goalpos,m->to,m->from);
                        else diff = m->value;
                        if (diff>0) {
                                if (lmove->to == m->from)
                                        m->value -= ENDPATH;
                        } else {
                                m->value -= diff*100;
                        }
                }
                My_qsort(&(S->moves),number_moves,sizeof(MOVE),DeadCompare);
        }
        return(number_moves);
}


int DeadIda(int treedepth, int g) {
/* the procedure that does the work at one node. it returns 
	X - the smallest h underneath this node */

	IDAARRAY  *S;
	HASHENTRY *entry;
	MOVE       bestmove;
	int 	   min_h,number_moves,result,i;
	long	   tree_size_save;
	long	   max_tree_size, tmp_tree_size;
	int        dir;


	SR(GMNODE **old_gmnode = IdaInfo->IdaMaze->gmtrees);
	SR(int here_nodes = total_node_count);
	SR(int old_h = IdaInfo->IdaMaze->h- IdaInfo->IdaMaze->pen);

        SR(Debug(4,treedepth,"starting ida (h=%i) (%s) %li %llx\n",
		IdaInfo->IdaMaze->h,
          	treedepth==0?"a1a1"
		      :PrintMove(IdaInfo->IdaArray[treedepth-1].currentmove),
		total_node_count,IdaInfo->IdaMaze->hashkey));
	GTVAny(GTVNodeEnter(treedepth,g,0,GTVMove(treedepth
		?IdaInfo->IdaArray[treedepth-1].currentmove:DummyMove),0));
	if (AbortSearch()) {
		GTVAny(GTVNodeExit(treedepth,0,"Abort_Search"));
		return(IdaInfo->IdaMaze->h);
	}
	SR(if (treedepth >=MAX_DEPTH) {
		SR(Debug(4,treedepth,"Too deep in the tree!(%i)\n",treedepth));
		GTVAny(GTVNodeExit(treedepth,0,"Too_deep_in_tree"));
		return(IdaInfo->IdaMaze->h);
	})

	S = &(IdaInfo->IdaArray[treedepth]);
	tree_size_save = IdaInfo->v_tree_size;
	IncNodeCount(treedepth);

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
	/* check for goal state, if yes return(1) */
	if (DeadIsGoalNode(g)) {
		SR(Debug(4,treedepth,"Found goal (%i %i)******************\n",
			IdaInfo->IdaMaze->h, IdaInfo->IdaMaze->number_stones));
		GTVAny(GTVNodeExit(treedepth,0,"Goal_found"));
		return(0);
	}

	/* check trans table if searched already */
	entry = PSGetHashTable(IdaInfo->IdaMaze);
	if (entry != NULL) {
		if (entry->pathflag == 1) {
			SR(Debug(4,treedepth, "Cycle (TT)\n"));
			GTVAny(GTVNodeExit(treedepth, ENDPATH,"Cycle (TT)"));
			IdaInfo->v_tree_size+=entry->tree_size;
			IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
			return(IdaInfo->IdaMaze->h);
		}
		bestmove = entry->bestmove;
		if (IdaInfo->Threshold - g < entry->down) {
			if (entry->min_h != 0) {
				SR(Debug(4,treedepth, "Futil (TT) %i < %i\n",
					IdaInfo->Threshold-g,entry->down));
				GTVAny(GTVNodeExit(treedepth,
					ENDPATH,"Futil (TT)"));
				IdaInfo->v_tree_size+=entry->tree_size;
				IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
				return(entry->min_h);
			}
		} else if (IdaInfo->Threshold - g == entry->down) {
			if (entry->min_h != 0 || IdaInfo->MiniFlag==YES) {
				SR(Debug(4,treedepth, "Futil (TT) %i == %i\n",
					IdaInfo->Threshold-g,entry->down));
				GTVAny(GTVNodeExit(treedepth,
					ENDPATH,"Futil (TT)"));
				IdaInfo->v_tree_size+=entry->tree_size;
				IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
				return(entry->min_h);
			}
		} else {
			if (entry->min_h==0 && IdaInfo->MiniFlag==YES) {
				SR(Debug(4,treedepth,
					"Futil (TT) %i > %i (0!!)\n",
					IdaInfo->Threshold-g,entry->down));
				GTVAny(GTVNodeExit(treedepth,
					ENDPATH,"Futil (TT)"));
				IdaInfo->v_tree_size+=entry->tree_size;
				IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
				return(0);
			}
		}
		SetPathFlag(IdaInfo->IdaMaze);
	} else {
		bestmove = DummyMove;
		PSStoreHashTable(IdaInfo->IdaMaze,g,IdaInfo->Threshold-g,
			IdaInfo->IdaMaze->h,bestmove,0,0,0,0,1);
	}
	min_h    = ENDPATH;
	max_tree_size = -1;

	/* create and sort (bestmove first + extension of previous stone) 
	   moves */
	number_moves = GenerateMoves(IdaInfo->IdaMaze,&(S->moves[0]));
if (number_moves==0) DConflictStartIda(1);
	number_moves = DeadMoveOrdering(treedepth,number_moves,bestmove);

	/* foreach move call ida */
	for (i=0; i<number_moves; i++) {
		if (ISDUMMYMOVE(S->moves[i])) continue;
		if (IdaInfo->IdaMaze->goal_sqto!=-1) {
			SR(Debug(4,treedepth,"Goal Cut move\n")); 
			continue;
		}
		S->currentmove = S->moves[i];
		/* check for deadlock */
		dir = DiffToDir(S->currentmove.to-S->currentmove.last_over);
		if (dir!=NODIR) {
			if (DeadTree(IdaInfo->IdaMaze,S->currentmove.to,dir)) {
				SR(Debug(6,treedepth,
					"DeadTree fd deadlock (%i-%i)\n",
					S->currentmove.from,
					S->currentmove.to)); 
				continue;
			}
		}
		SR(Debug(6,treedepth,"DeadMakeMove %s (%i of %i)\n", 
			PrintMove(S->currentmove),i+1,number_moves));
		if (!DeadMakeMove(IdaInfo->IdaMaze,&(S->currentmove),&(S->unmove))){
			continue;
		}
		tmp_tree_size = IdaInfo->v_tree_size;
		result = DeadIda(treedepth+1,g+S->unmove.move_dist);
		tmp_tree_size = IdaInfo->v_tree_size - tmp_tree_size;
		DeadUnMakeMove(IdaInfo->IdaMaze,&(S->unmove));
		SR(Assert(old_h==IdaInfo->IdaMaze->h-IdaInfo->IdaMaze->pen,
			"DeadUnMakeMove: old_h!=h!\n"));
		SR(Assert(old_gmnode==IdaInfo->IdaMaze->gmtrees,
			"DeadIda: old_gmtrees[0] changed!!\n"));
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
	if (!AbortSearch())
		PSStoreHashTable(IdaInfo->IdaMaze,g,IdaInfo->Threshold-g,min_h,
			bestmove,IdaInfo->v_tree_size-tree_size_save,0,0,0,0);
	else
		UnSetPathFlag(IdaInfo->IdaMaze);
	SR(Debug(4,treedepth,"return from ida %i (%s %i)\n", 
		treedepth,
		PrintMove(bestmove),
		min_h));
	GTVAny(GTVNodeExit(treedepth,min_h,"Normal_Exit"));
	return(min_h);
}

int DeadMakeMove(MAZE *maze, MOVE *move, UNMOVE *ret)
/* this is a routine that makes a STONE move, not merely a man move like
 * DoMove. It will just put the man to the new location */
{
	int old_h;

	ret->manfrom  = maze->manpos;
	ret->stonefrom= move->from;
	ret->stoneto  = move->to;
	ret->macro_id = move->macro_id;
	ret->move_dist= GetWeight(maze,move->to,move->from);
	maze->goal_sqto = -1;

	CopyBS( ret->save_reach, maze->reach );
	CopyBS( ret->save_no_reach, maze->no_reach );

	MANTO(maze,move->last_over);
	STONEFROMTO(maze,ret->stonefrom,ret->stoneto);

	ret->old_closest_confl = IdaInfo->closest_confl;
	if (IsBitSetBS(IdaInfo->shadow_stones,move->to)) {
		if ( ManWeight(maze,IdaInfo->goal_last_to,move->to)
		    <ManWeight(maze,IdaInfo->goal_last_to,
				    IdaInfo->closest_confl))
			IdaInfo->closest_confl = move->to;
	}
	/* either touching a goal or an area unreachable before, removes
	 * stone */
	if (  (maze->Phys[move->to].goal>=0)
	    ||(  !IsBitSetBS(IdaInfo->no_reach,ret->stoneto)
	       && IdaInfo->closest_confl != 0)) {
		/* remove stone */
		ret->old_stoneid = maze->PHYSstone[ret->stoneto];
		maze->PHYSstone[ret->stoneto] = -1;
		maze->number_stones--;
		if (maze->number_stones>ret->old_stoneid) {
			/* relocate stone if necessary */
			maze->stones[ret->old_stoneid] 
				= maze->stones[maze->number_stones];
			maze->PHYSstone[maze->stones[ret->old_stoneid].loc]
				= ret->old_stoneid;
		}
		UnsetBitBS(maze->stone,ret->stoneto);
		NormHashKey(maze);
		MarkReachQuick( maze, move->from );
		CopyBS(maze->old_no_reach,maze->no_reach);
		BitNotAndNotAndNotBS(maze->no_reach,maze->reach,maze->out,maze->stone);

		/* make sure we deepen the tree to cut off bad moves later */
		old_h = maze->h;
		DeadLowerBound(maze);
		ret->move_dist = old_h - maze->h;
	} else {
		UpdateHashKey(maze, ret);
		MarkReach(maze);
		DeadUpdateLowerBound(maze, ret->stoneto, ret);
		ret->old_stoneid = -1;
	}

	SR(Assert(maze->manpos>0,"DeadMakeMove: manpos < 0!\n"));
	return(1);
}

int DeadUnMakeMove(MAZE *maze, UNMOVE *unmove)
{
	int      stonei;
	int	 new_h;

	new_h = maze->h;
	stonei = maze->PHYSstone[unmove->stoneto];
	if (unmove->old_stoneid != -1) {
		/* recreate stone */
		if (maze->number_stones>unmove->old_stoneid) {
			/* relocate stone, if was before */
			maze->stones[maze->number_stones] 
				= maze->stones[unmove->old_stoneid];
			maze->PHYSstone[maze->stones[unmove->old_stoneid].loc]
				= maze->number_stones;
		}
		maze->number_stones++;
		maze->stones[unmove->old_stoneid].loc = unmove->stoneto;
		maze->PHYSstone[unmove->stoneto] = unmove->old_stoneid;
		SetBitBS(maze->stone,unmove->stoneto);
	}
	MANTO(maze,unmove->manfrom);
	STONEFROMTO(maze,unmove->stoneto,unmove->stonefrom);

	UnsetBitBS(maze->stones_done,unmove->stoneto);
	UnsetBitBS(maze->stones_done,unmove->stonefrom);

	CopyBS( maze->reach, unmove->save_reach );
	CopyBS( maze->no_reach, unmove->save_no_reach );
	/* MarkReach(maze); */
	if (unmove->old_stoneid == -1) {
		UpdateHashKey(maze, unmove);
		DeadUpdateLowerBound(maze, unmove->stonefrom, unmove);
	} else {
		NormHashKey(maze);
		DeadLowerBound(maze);
	}
	IdaInfo->closest_confl = unmove->old_closest_confl;

	if (  ((unmove->old_stoneid != -1)||(maze->goal_sqto==unmove->stoneto))
	    &&(maze->h - unmove->move_dist == new_h    /* optimal move */)) {
		/* This is either the start or a continuation of a goal move */
		maze->goal_sqto = unmove->stonefrom;
	} else maze->goal_sqto = -1;

	return(0);
}

int DeadLowerBound(MAZE *maze) {
/* This is a fast and bad lower bound - just the closest goal - since we
remove stones when we hit goals */
	int    stonei,goali;
	PHYSID stonepos, goalpos;
	int    dist;

	/* initialize table with just any matching */
	maze->h = 0;
	maze->pen = GetPenalty(maze->conflicts,maze->stone,maze->manpos);
	maze->h  += maze->pen;
	for (stonei=0; stonei<maze->number_stones; stonei++) {
		stonepos = maze->stones[stonei].loc;
		maze->lbtable[stonei].distance = ENDPATH;
		for (goali=0; goali<maze->number_goals; goali++) {
			goalpos = maze->goals[goali].loc;
			dist = GetWeight(maze,goalpos,stonepos);
			if (dist < maze->lbtable[stonei].distance) {
				maze->lbtable[stonei].distance=dist;
				maze->lbtable[stonei].goalidx=goali;
			}
		}
		maze->h += maze->lbtable[stonei].distance;
	}
	return(maze->h);
}

int DeadUpdateLowerBound(MAZE *maze, PHYSID stonepos, UNMOVE *unmove) {
/* Update lowerbound after move to square pos */
	int    stonei,goali;
	PHYSID goalpos;
	int    dist;

	maze->h  -= maze->pen;
	maze->pen = GetPenalty(maze->conflicts,maze->stone,maze->manpos);
	maze->h  += maze->pen;
	stonei   = maze->PHYSstone[stonepos];
	maze->h -= maze->lbtable[stonei].distance;
	maze->lbtable[stonei].distance = ENDPATH;
	for (goali=0; goali<maze->number_goals; goali++) {
		goalpos = maze->goals[goali].loc;
		dist = GetWeight(maze,goalpos,stonepos);
		if (dist < maze->lbtable[stonei].distance) {
			maze->lbtable[stonei].distance=dist;
			maze->lbtable[stonei].goalidx=goali;
		}
	}
	maze->h += maze->lbtable[stonei].distance;
	return(maze->h);
}


void DConflictStartIda(int immediate)
{
/* Enter all non-optimally judged positions into the conflict base */
	IDA		idainfo,*old_idainfo;

	if (immediate==1) {
		AddConflict(IdaInfo->IdaMaze->conflicts,IdaInfo->IdaMaze->stone,
		IdaInfo->IdaMaze->no_reach, IdaInfo->IdaMaze->reach, ENDPATH, 0);
		return;
	}
	old_idainfo = IdaInfo;
	InitIDA(&idainfo);
	IdaInfo                = &idainfo;
	IdaInfo->IdaMaze        = CopyMaze(old_idainfo->IdaMaze);
	IdaInfo->AbortNodeCount = PATTERNSEARCH;
	IdaInfo->base_indent    = old_idainfo->base_indent + 2;
	IdaInfo->goal_last_to   = old_idainfo->goal_last_to;
	IdaInfo->PrintPriority  = DEADPATTERNSEARCHPP;
	IdaInfo->HashTable      = HashTableElse;
	BitNotAndNotBS(IdaInfo->no_reach,IdaInfo->IdaMaze->reach,
					 IdaInfo->IdaMaze->out);

	AvoidThisSquare = 0;
	init_stats();
	IdaInfo->Threshold = ENDPATH;
	IdaInfo->CurrentSolutionDepth = ENDPATH;
	IdaInfo->IdaMaze->goal_sqto = -1;
	DConflictIda(0,0);
	print_stats(4);

	DelCopiedMaze(IdaInfo->IdaMaze);
	IdaInfo = old_idainfo;
	return;
}

void DConflictIda(int treedepth, int g)
{
/* We don't need TT, since we insert penalties at the nodes and if we come
 * back to the same node, we should find the penalty which makes the
 * h==Threshold and then gets "cut-off" automatically 
   1) don't check for goals 2) don't check for maxdepth */

	IDAARRAY  *S;
	HASHENTRY *entry;
	MOVE       bestmove;
	int 	   number_moves,i;
	int        dir;
	static int add_count;
	BitString  o_stones;
	SR(int here_nodes = total_node_count);

	if (g==0) add_count=0;
	S = &(IdaInfo->IdaArray[treedepth]);
	IncNodeCount(treedepth);

	CopyBS(o_stones,IdaInfo->IdaMaze->stone);
        if (   treedepth==0
            || NumberBitsBS(IdaInfo->IdaMaze->stone)<PATTERNSTONES/2)
		DeadMiniConflict(IdaInfo->IdaMaze->stone);
	AddConflict(IdaInfo->IdaMaze->conflicts,IdaInfo->IdaMaze->stone,
		IdaInfo->IdaMaze->no_reach, IdaInfo->IdaMaze->reach, ENDPATH, 0);
	add_count++;

	if (   (treedepth>=2&&add_count>3)
	    || (NumberBitsBS(IdaInfo->IdaMaze->stone) > PATTERNSTONES/2)
	    || Options.multiins==0) goto END_POINT;

	/* just for best move and move ordering, Is this needed???? */
	entry = PSGetHashTable(IdaInfo->IdaMaze);
	if (entry != NULL) bestmove = entry->bestmove;
	else bestmove = DummyMove;
	number_moves = GenerateMoves(IdaInfo->IdaMaze,&(S->moves[0]));
	number_moves = DeadMoveOrdering(treedepth,number_moves,bestmove);

	/* foreach move call ida */
	for (i=0; i<number_moves; i++) {
		if (ISDUMMYMOVE(S->moves[i])) continue;
		S->currentmove = S->moves[i];
		/* check for deadlock */
		dir = DiffToDir(S->currentmove.to-S->currentmove.last_over);
		if (dir!=NODIR) {
			if (DeadTree(IdaInfo->IdaMaze,S->currentmove.to,dir)) {
				continue;
			}
		}
		if (!DeadMakeMove(IdaInfo->IdaMaze,
				 &(S->currentmove),&(S->unmove))) {
			continue;
		}
		DConflictIda(treedepth+1,g+S->unmove.move_dist);
		DeadUnMakeMove(IdaInfo->IdaMaze,&(S->unmove));
	}
END_POINT:

	DeadDeactivateStones(IdaInfo->IdaMaze,o_stones);
	return;
}

