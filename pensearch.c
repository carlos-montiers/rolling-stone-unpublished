#include "board.h"

/* This stuff does not work yet since shadow stones are influencing the
 * decision about somehting being a deadlock or not, the result of the
 * PenMove search depends on the position of the shadow stones, which it
 * should not. */

int PenIsGoalNode(int g)
{
	if (IdaInfo->IdaMaze->number_stones == 0 || IdaInfo->IdaMaze->h == 0) {
		IdaInfo->CurrentSolutionDepth=g;
		SR(Debug(3,0,"PenIsGoalNode: no stones left\n"));
		return(1);
	}
	return(0);
}

PHYSID FindClosestPosFringe( MAZE *maze,
			     BitString squares,
			     BitString already_visible,
			     BitString fringe )
/* Find the square (position) the in squares that has a stone on it
 * that is not already visible */
{
  int stonei;
  int dist = ENDPATH;
  PHYSID pos = 0, p;
	
  for( stonei = 0; stonei < maze->number_stones; stonei++ ) {
    p = maze->stones[ stonei ].loc;
    if( IsBitSetBS( squares, p ) &&
	IsBitSetBS( fringe, p ) &&
	!IsBitSetBS( already_visible, p ) ) {
      if( dist > XDistStone( maze, maze->manpos, p ) ) {
	dist = XDistStone( maze, maze->manpos, p );
	pos = p;
      }
    }
  }
  return(pos);
}

void FindFringeStones( MAZE *maze, BitString fs )
{
  int x, y;
  BASETYPE a, b, c;

  Set0BS( fs );
  for( x = YSIZE; x < ( XSIZE - 1 ) * YSIZE; x += YSIZE )
    for( y = 1; y < YSIZE - 1; y++ )
      if( IsBitSetBS( maze->stone, x + y ) ) {
	if( !( ( a = IsBitSetBS( maze->reach, x + y + 1 ) ) ||
	       IsBitSetBS( maze->wall, x + y + 1 ) ||
	       IsBitSetBS( maze->stone, x + y + 1 ) ) ) {
	  if( IsBitSetBS( maze->reach, x + y + YSIZE ) ||
	      IsBitSetBS( maze->reach, x + y - 1 ) ||
	      IsBitSetBS( maze->reach, x + y - YSIZE ) ) {
	    SetBitBS( fs, x + y );
	    continue;
	  }
	}
	if( !( ( b = IsBitSetBS( maze->reach, x + y + YSIZE ) ) ||
	       IsBitSetBS( maze->wall, x + y + YSIZE ) ||
	       IsBitSetBS( maze->stone, x + y + YSIZE ) ) ) {
	  if( a || IsBitSetBS( maze->reach, x + y - 1 ) ||
	      IsBitSetBS( maze->reach, x + y - YSIZE ) ) {
	    SetBitBS( fs, x + y );
	    continue;
	  }
	}
	if( !( ( c = IsBitSetBS( maze->reach, x + y - 1 ) ) ||
	       IsBitSetBS( maze->wall, x + y - 1 ) ||
	       IsBitSetBS( maze->stone, x + y - 1 ) ) ) {
	  if( a || b || IsBitSetBS( maze->reach, x + y - YSIZE ) ) {
	    SetBitBS( fs, x + y );
	    continue;
	  }
	}
	if( !( IsBitSetBS( maze->reach, x + y - YSIZE ) ||
	       IsBitSetBS( maze->wall, x + y - YSIZE ) ||
	       IsBitSetBS( maze->stone, x + y - YSIZE ) ) ) {
	  if( a || b || c ) {
	    SetBitBS( fs, x + y );
	    continue;
	  }
	}
      }
}

/* Try to find out if we can move this stone still to a goal,
   include those that might be creating a deadlock,
   after finding that this is a deadlock, find the minimal set of
   stones belonging to a deadlock */
int  PenMove(MAZE *maze, HASHENTRY *entry, MOVE *last_move, int treedepth,
	int g, int targetpen, int *pensearched, int32_t effort)
{
	BitString	visible,relevant;
	PHYSID		pos;
	IDA		idainfo,*old_idainfo;
	int32_t 	node_count;
	int		result;
	int		number_stones;
	int		old_gm;
	int		max_pen;
	SR(int here_nodes = total_node_count;)

	/* don't bother if one of the following is true */
	if (   Options.pen_srch == 0
	    || last_move->macro_id != 0
	    || treedepth == 0) {
		SR(Debug(3,0,"PenMove ## End search - Not even considered\n"));
		return(0);
	}
	/* don't search, but try finding Penalties */
	if (!PenMoveSuspected(maze, last_move, entry)) {
		SR(Debug(3,0,"PenMove #### End search - Not suspected\n"));
		return(0);
	}

	*pensearched = 1;
	old_idainfo = IdaInfo;
	InitIDA(&idainfo);
	IdaInfo                = &idainfo;
	IdaInfo->IdaMaze        = CopyMaze(maze);
	IdaInfo->AbortNodeCount = effort;
	IdaInfo->goal_last_to   = last_move->to;
	IdaInfo->base_indent    = old_idainfo->base_indent + treedepth;
	IdaInfo->PrintPriority  = PENPATTERNSEARCHPP;
	IdaInfo->HashTable      = HashTableElse;
	IdaInfo->ThresholdInc   = 1;
	old_gm = Options.mc_gm;
	Options.mc_gm=0;
	Set1BS(IdaInfo->no_reach);
	Set0BS(visible); Set0BS(relevant);
	SetBitBS(visible,last_move->to);
	node_count    = 0;
	number_stones = 1;
	max_pen	      = 0;
#ifdef SMARTMANPATH
CopyBS( IdaInfo->shadow_stones, maze->stone );
UnsetBitBS( IdaInfo->shadow_stones, last_move->to );
#endif

	SR(Debug(3,0,"PenMove #### Start search\n"));
	for (;;) {
		SR(Debug(3,0,"PenMove Iteration, stones: %i\n", number_stones));
		PenDeactivateStones(IdaInfo->IdaMaze,visible);
#ifdef SMARTMANPATH
BitAndNotEqBS( IdaInfo->shadow_stones, visible );
#endif
		IdaInfo->node_count    = 0;
NavigatorSetPSMaze(IdaInfo->IdaMaze,visible,maze->stone);
		result                 = PenStartIda();
		BitOrEqBS(relevant,IdaInfo->IdaManSquares);
		if (result > 0) {
			if (max_pen<result) max_pen = result;
			if (IdaInfo->IdaMaze->h+result >= ENDPATH)
				DConflictStartIda(0);
			else
				ConflictStartIda(IdaInfo->IdaMaze->h+result,0);
			node_count += IdaInfo->node_count;
			IdaInfo->node_count = 0;
		}
		if (AbortSearch()) {
			SR(Debug(3,0,"PenMove: too many nodes: %i\n",
			      IdaInfo->node_count));
			break;
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
		/* If we have to many stones in the works, abort */
		number_stones = NumberBitsBS(visible);
		if (number_stones>min(PATTERNSTONES,(maze->number_stones-2))) {
			SR(Debug(3,0,"PenMove: too many stones: %i\n",
			      number_stones));
			break;
		}
	}
	AddTestedPen(maze->conflicts,relevant,
			   old_idainfo->IdaMaze->stone,
			   IdaInfo->IdaMaze->manpos,last_move->to);
	node_count += IdaInfo->node_count;
/*if( max_pen == 0 ) {
printf("last move was %i,%i to %i,%i\n", last_move->from / YSIZE, last_move->from % YSIZE, last_move->to / YSIZE, last_move->to % YSIZE );
PrintMaze( maze );
}*/
	IdaInfo->node_count = 0;
	DelCopiedMaze(IdaInfo->IdaMaze);
	if (max_pen >= targetpen) {
		pen_pos_nc += node_count;
		pen_pos_sc ++;
	} else {
		pen_neg_nc += node_count;
		pen_neg_sc ++;
	}
	IdaInfo = old_idainfo;
	SR(Debug(3,0,"PenMove ## End search - ALIVE (nodes: %" PRIi32 " stones: %i)\n",
		node_count,number_stones));
	Options.mc_gm=old_gm;
NavigatorUnsetPSMaze();
        maze->h  -= maze->pen;
        max_pen   = maze->pen;
        maze->pen = GetPenalty(maze->conflicts,maze->stone,maze->manpos);
        maze->h  += maze->pen;
        return(maze->pen - max_pen);
}

void PenDeactivateStones(MAZE *maze, BitString visible)
{
	PHYSID    pos;

	maze->number_stones=0;
	CopyBS(maze->stone,visible);
	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (IsBitSetBS(visible,pos)) {
			maze->PHYSstone[pos] = maze->number_stones;
			maze->stones[maze->number_stones++].loc = pos;
		} else {
			maze->PHYSstone[pos] = -1;
		}
	}
	NormHashKey(maze);
	MarkReach(maze);
	PenLowerBound(maze);
}

int NoMoreMoves(MAZE *maze, MOVE *last_move)
{
#define SETMOVE(move,v,ifrom,ito)\
	(move).from = ifrom; (move).last_over = ifrom; (move).move_dist = 0;\
	(move).macro_id = 0; (move).to = ito; (move).value = v;

	PHYSID pos = last_move->to;
	MOVE   move;
	int dir,dist,number_moves=0;

	for (dir=NORTH; dir<=WEST; dir++) {
		if ( (dist=IsBitSetBS(maze->reach, pos+DirToDiff[dir]))
		    && (IsBitSetBS(maze->S[OppDir[dir]],pos))
		    && (maze->PHYSstone[pos-DirToDiff[dir]]<0)) {
			SETMOVE(move,dist,pos,pos-DirToDiff[dir]);
			if ((!DeadLock(maze,move))&&(!DeadLock2(maze,&move)))
				number_moves++;
		}		
	}
	return(number_moves==0);
}

#ifdef RANDOM
extern double percent;
#endif

int PenMoveSuspected(MAZE *maze, MOVE *last_move, HASHENTRY *entry)
{
/* return 1 if we suspect a deadlock search is beneficial, here should be
 * all the heuristic stuff that we hope will basically have a good guess at
 * the final outcome of the proove search - sort of like move ordering in
 * alpha-beta */

	BitString new_no_reach;

	if (entry->pensearched==1) return(0);
	if (entry->tree_size > 20) goto CHECK_TESTED;
	if (NoMoreMoves(maze,last_move)) goto CHECK_TESTED;

	/* always check for deadlocks if we move onto one_way squares */
/* 	if (IsBitSetBS(maze->one_way,last_move->to)) goto CHECK_TESTED; */

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
	if(WasTestedPen(maze->conflicts,maze->stone,maze->manpos,last_move->to))
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

void PenMiniConflict(BitString visible, int penalty)
/* stones marked in visible are a deadlock set, try and minimize the set by
 * removing one stone at a time, finding its necessity in the deadlock */
{
	int    result;
	PHYSID pos;
	int32_t node_count;
	BitString already;
	int	  old_gm;
	IDA	  idainfo,*old_idainfo;

	SR(Debug(3,0,"PenMiniConflict #### Start\n"));
	old_idainfo = IdaInfo;
	InitIDA(&idainfo);
	IdaInfo                 = &idainfo;
	IdaInfo->IdaMaze        = old_idainfo->IdaMaze;
	CopyBS(IdaInfo->no_reach,old_idainfo->no_reach);
	IdaInfo->ThresholdInc   = 1;
	IdaInfo->AbortNodeCount = PATTERNSEARCH;
	IdaInfo->goal_last_to   = old_idainfo->goal_last_to;
	IdaInfo->closest_confl  = 0;
	IdaInfo->base_indent   += 2;
	IdaInfo->PrintPriority  = PENPATTERNSEARCHPP;
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
		SR(Debug(3,0,"PenMiniConflict #### Start iteration\n"));
		UnsetBitBS(visible,pos);
		PenDeactivateStones(IdaInfo->IdaMaze,visible);
		node_count += IdaInfo->node_count;
		IdaInfo->node_count = 0;
		result = PenStartIda();
		if (result<penalty) {
if( result > 0 ) {
AddDelayedConflict(MainIdaInfo.IdaMaze->conflicts, visible,
	    IdaInfo->IdaMaze->no_reach, IdaInfo->IdaMaze->reach, result, 0 );
/*
printf("result: %i, penalty: %i\n",result, penalty);
PosNr = pos;
PrintBit3Maze(IdaInfo->IdaMaze, visible, IdaInfo->IdaMaze->no_reach,
	IdaInfo->IdaMaze->manpos);
PosNr = 0;
*/
}
			SetBitBS(visible,pos);
		} else {
			SR(Debug(3,0,"PenMiniConflict: Extra Stone\n"));
		}
	}
	PenDeactivateStones(IdaInfo->IdaMaze,visible);
	Options.mc_gm=old_gm;
	penmcount += node_count;
	IdaInfo = old_idainfo;
	IdaInfo->node_count += node_count;
	SR(Debug(3,0,"PenMiniConflict #### End\n"));
}

PHYSID FindFarthestPosStone(MAZE *maze, BitString squares, 
		      		  BitString already_visible)
/* Find the square (position) the in squares that has a stone on it
 * that is not already visible */
{
  int stonei;
  int dist = -1;
  PHYSID pos, p;
	
  pos = 0;
  for( stonei = 0; stonei < maze->number_stones; stonei++ ) {
    p = maze->stones[ stonei ].loc;
    if( !IsBitSetBS( already_visible, p ) &&
	IsBitSetBS( squares, p ) ) {
      if( dist < XDistStone( maze, maze->manpos, p ) ) {
	dist = XDistStone( maze, maze->manpos, p );
	pos = p;
      }
    }
  }
  return pos;
}

void PenMiniConflict2(BitString visible, int penalty)
/* stones marked in visible are a deadlock set, try and minimize the set by
 * removing one stone at a time, finding its necessity in the deadlock */
{
	int    result;
	PHYSID pos;
	int32_t node_count;
	BitString already;
	int	  old_gm;
	IDA	  idainfo,*old_idainfo;

	SR(Debug(3,0,"PenMiniConflict #### Start\n"));
	old_idainfo = IdaInfo;
	InitIDA(&idainfo);
	IdaInfo                 = &idainfo;
	IdaInfo->IdaMaze        = old_idainfo->IdaMaze;
	CopyBS(IdaInfo->no_reach,old_idainfo->no_reach);
	IdaInfo->ThresholdInc   = 1;
	IdaInfo->AbortNodeCount = PATTERNSEARCH;
	IdaInfo->goal_last_to   = old_idainfo->goal_last_to;
	IdaInfo->closest_confl  = 0;
	IdaInfo->base_indent   += 2;
	IdaInfo->PrintPriority  = PENPATTERNSEARCHPP;
	IdaInfo->HashTable      = HashTableElse;
	IdaInfo->MiniFlag	= YES;
	old_gm = Options.mc_gm; 
	Options.mc_gm=0;
	node_count = 0;
	Set0BS(already);
	for (;;) {
		pos = FindFarthestPosStone(IdaInfo->IdaMaze,visible,already);
		if (pos==0) break;
		SetBitBS(already,pos);
		SR(Debug(3,0,"PenMiniConflict #### Start iteration\n"));
		UnsetBitBS(visible,pos);
		PenDeactivateStones(IdaInfo->IdaMaze,visible);
		node_count += IdaInfo->node_count;
		IdaInfo->node_count = 0;
		result = PenStartIda();
		if (result<penalty) {
if( result > 0 ) {
AddDelayedConflict(MainIdaInfo.IdaMaze->conflicts, visible,
	    IdaInfo->IdaMaze->no_reach, IdaInfo->IdaMaze->reach, result, 0 );
}
			SetBitBS(visible,pos);
		} else {
			SR(Debug(3,0,"PenMiniConflict: Extra Stone\n"));
		}
	}
	PenDeactivateStones(IdaInfo->IdaMaze,visible);
	Options.mc_gm=old_gm;
	IdaInfo = old_idainfo;
	IdaInfo->node_count += node_count;
	SR(Debug(3,0,"PenMiniConflict #### End\n"));
}

int PenStartIda() {
/* Sets up all data structures and repeatedly calls ida with increasing 
   threshold to guarantee optimal solutions, returns 0 if solution found 
   otherwise the smallest heuristic value seen at any leaf node if this is
   ENDPATH there is no solution - deadlock */

	int       result=ENDPATH;
	int32_t   last_tree_size;

	if (AbortSearch()) return(0);
	/* initialize data structures */
	AvoidThisSquare = 0;
	init_stats();
	Set0BS(IdaInfo->IdaManSquares);
	Set0BS(IdaInfo->IdaStoneSquares);
	IdaInfo->Threshold = IdaInfo->IdaMaze->h;
	IdaInfo->CurrentHashGen = 0;
	
	IdaInfo->CurrentSolutionDepth = ENDPATH;
	BitNotAndNotBS(IdaInfo->no_reach,IdaInfo->IdaMaze->reach,
					 IdaInfo->IdaMaze->out);
	result = ENDPATH;
	for (IdaInfo->Threshold = IdaInfo->IdaMaze->h;
	        (IdaInfo->CurrentSolutionDepth > IdaInfo->Threshold)
	     && (result !=0)
	     && (!AbortSearch());
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
		result = PenIda(0,0); /**********************************/
		GTVAny(GTVClose());
		print_stats(4);
		if (result>=ENDPATH) IdaInfo->Threshold = ENDPATH;
	}
	if (   (IdaInfo->CurrentSolutionDepth<ENDPATH)
	    || (result==0)
	    || (AbortSearch()))
		IdaInfo->Threshold -= IdaInfo->ThresholdInc;
	if (result<ENDPATH) result = IdaInfo->Threshold - IdaInfo->IdaMaze->h;
penscount += IdaInfo->node_count;
	return(result);
}

static int PenCompare(const void *m1, const void *m2) {
        return(((MOVE*)m1)->value - ((MOVE*)m2)->value);
}

int PenMoveOrdering(int depth, int number_moves, MOVE bestmove)
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
                My_qsort(&(S->moves),number_moves,sizeof(MOVE),PenCompare);
        }
        return(number_moves);
}


int PenIda(int treedepth, int g) {
/* the procedure that does the work at one node. it returns 
	X - the smallest h underneath this node */

	IDAARRAY  *S;
	HASHENTRY *entry;
	MOVE       bestmove;
	int 	   min_h,number_moves,result,i;
	int32_t    tree_size_save;
	int32_t    max_tree_size, tmp_tree_size;
	int        dir; 
	SR(int here_nodes = total_node_count);
	SR(int old_h = IdaInfo->IdaMaze->h- IdaInfo->IdaMaze->pen);

        SR(Debug(4,treedepth,"starting ida (h=%i) (%s) %" PRIi32 " %" PRIx64 "\n",
		IdaInfo->IdaMaze->h,
          	treedepth==0?"a1a1"
		      :PrintMove(IdaInfo->IdaArray[treedepth-1].currentmove),
		total_node_count,IdaInfo->IdaMaze->hashkey));
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
	if (PenIsGoalNode(g)) {
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
	number_moves = PenMoveOrdering(treedepth,number_moves,bestmove);

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
		SR(Debug(6,treedepth,"PenMakeMove %s (%i of %i)\n", 
			PrintMove(S->currentmove),i+1,number_moves));
		if (!PenMakeMove(IdaInfo->IdaMaze,&(S->currentmove),&(S->unmove))){
			continue;
		}
		tmp_tree_size = IdaInfo->v_tree_size;
		result = PenIda(treedepth+1,g+S->unmove.move_dist);
		tmp_tree_size = IdaInfo->v_tree_size - tmp_tree_size;
		PenUnMakeMove(IdaInfo->IdaMaze,&(S->unmove));
		SR(Assert(old_h==IdaInfo->IdaMaze->h-IdaInfo->IdaMaze->pen,
			"PenUnMakeMove: old_h!=h!\n"));
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
#ifdef SMARTMANPATH
			SetManStoneSquares2(IdaInfo->IdaMaze,bestmove);
#else
			SetManStoneSquares(IdaInfo->IdaMaze,bestmove);
#endif
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

int PenMakeMove(MAZE *maze, MOVE *move, UNMOVE *ret)
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

	/* touching a goal removes stone */
	if (maze->Phys[move->to].goal>=0) {
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
/* MarkReach( maze ); */
MarkReachQuick( maze, move->from );
		/* make sure we deepen the tree to cut off bad moves later */
		old_h = maze->h;
		PenLowerBound(maze);
		ret->move_dist = old_h - maze->h;
	} else {
		UpdateHashKey(maze, ret);
if( move->macro_id )
  MarkReach( maze );
else
  UpdateReach( maze, ret->stoneto );
		PenUpdateLowerBound(maze, ret->stoneto, ret);
		ret->old_stoneid = -1;
	}

	SR(Assert(maze->manpos>0,"PenMakeMove: manpos < 0!\n"));
	return(1);
}

int PenUnMakeMove(MAZE *maze, UNMOVE *unmove)
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
		PenUpdateLowerBound(maze, unmove->stonefrom, unmove);
	} else {
		NormHashKey(maze);
		PenLowerBound(maze);
	}
	
	if (  ((unmove->old_stoneid != -1)||(maze->goal_sqto==unmove->stoneto))
	    &&(maze->h - unmove->move_dist == new_h    /* optimal move */)) {
		/* This is either the start or a continuation of a goal move */
		maze->goal_sqto = unmove->stonefrom;
	} else maze->goal_sqto = -1;

	return(0);
}

int PenLowerBound(MAZE *maze) {
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

int PenUpdateLowerBound(MAZE *maze, PHYSID stonepos, UNMOVE *unmove) {
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


void ConflictStartIda(int Threshold, int immediate)
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
	IdaInfo->PrintPriority  = PENPATTERNSEARCHPP;
	IdaInfo->HashTable      = HashTableElse;

	AvoidThisSquare = 0;
	init_stats();
	IdaInfo->Threshold = Threshold;
	IdaInfo->CurrentSolutionDepth = ENDPATH;
	IdaInfo->IdaMaze->goal_sqto = -1;
	InitDelayedConflict();
	ConflictIda(0,0);
	InsertDelayedConflict();
	print_stats(4);

	DelCopiedMaze(IdaInfo->IdaMaze);
	IdaInfo = old_idainfo;
	return;
}

void ConflictIda(int treedepth, int g)
{
/* We don't need TT, since we insert penalties at the nodes and if we come
 * back to the same node, we should find the penalty which makes the
 * h==Threshold and then gets "cut-off" automatically 
   1) don't check for goals 2) don't check for maxdepth */

	IDAARRAY  *S;
	HASHENTRY *entry;
	MOVE       bestmove;
	int 	   number_moves,i;
	int        dir, old_Threshold,old_h;
	static int add_count;
	BitString  o_stones; 
/* needed to search both directions
BitString old_stones;
int old_thresh_2; */
	SR(int here_nodes = total_node_count);

	if (g==0) add_count = 0;
	S = &(IdaInfo->IdaArray[treedepth]);
	IncNodeCount(treedepth);

	/* if g+h is larger or equal to Threshold return, not adding
	 * conflicts */
	if (g+IdaInfo->IdaMaze->h>=IdaInfo->Threshold) {
		return;
	}
	i = IdaInfo->Threshold - (g+IdaInfo->IdaMaze->h);
	CopyBS(o_stones,IdaInfo->IdaMaze->stone);
	old_h = IdaInfo->IdaMaze->h;
        if (   treedepth==0
	       || NumberBitsBS(IdaInfo->IdaMaze->stone)<PATTERNSTONES/2) {
/* search both nearest to farthest, and farthest to nearest
CopyBS( old_stones, IdaInfo->IdaMaze->stone );
PenMiniConflict2(IdaInfo->IdaMaze->stone,i);
old_thresh_2 = IdaInfo->Threshold;
IdaInfo->Threshold = IdaInfo->IdaMaze->h + g + i;
AddConflict(IdaInfo->IdaMaze->conflicts,IdaInfo->IdaMaze->stone,
	    IdaInfo->IdaMaze->no_reach, IdaInfo->IdaMaze->reach, i, 0);
IdaInfo->Threshold = old_thresh_2;
CopyBS( IdaInfo->IdaMaze->stone, old_stones ); */

		if (i>=ENDPATH)
			DeadMiniConflict(IdaInfo->IdaMaze->stone);
		else
			PenMiniConflict(IdaInfo->IdaMaze->stone,i);
	}
	/* Update Threshold, since stones could be removed */
	old_Threshold = IdaInfo->Threshold;
	IdaInfo->Threshold -=    old_h - IdaInfo->IdaMaze->h;
	AddConflict(IdaInfo->IdaMaze->conflicts,IdaInfo->IdaMaze->stone,
		IdaInfo->IdaMaze->no_reach, IdaInfo->IdaMaze->reach, i, 0);
	add_count++;

	if (   (treedepth>=2 && add_count>3)
	    || (NumberBitsBS(IdaInfo->IdaMaze->stone) > PATTERNSTONES/2)
	    || Options.multiins==0) goto END_POINT;

	/* just for best move and move ordering, Is this needed???? */
	entry = PSGetHashTable(IdaInfo->IdaMaze);
	if (entry != NULL) bestmove = entry->bestmove;
	else bestmove = DummyMove;
	number_moves = GenerateMoves(IdaInfo->IdaMaze,&(S->moves[0]));
	number_moves = PenMoveOrdering(treedepth,number_moves,bestmove);

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
		if (!PenMakeMove(IdaInfo->IdaMaze,
				 &(S->currentmove),&(S->unmove))) {
			continue;
		}
		ConflictIda(treedepth+1,g+S->unmove.move_dist);
		PenUnMakeMove(IdaInfo->IdaMaze,&(S->unmove));
	}
END_POINT:
	PenDeactivateStones(IdaInfo->IdaMaze,o_stones);
	/* Update Threshold, since stones are added again */
	IdaInfo->Threshold = old_Threshold;
	return;
}

