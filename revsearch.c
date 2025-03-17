#include "board.h"

int IsGoalNodeRev(int g)
{
	if (   (IdaInfo->IdaMaze->number_stones==0)
	    && (IsBitSetBS(IdaInfo->IdaMaze->reach,IdaInfo->reach_at_goal))) 
		return(1);
	return(0);
}

/* find out if the last move introduced a deadlock */
/* return 1 if deadlock, 0 if not */
/* Idea here is that if a move is reversible it does not change the
deadlockness of a state, meaning if it is a reversible move, then a state
that was a deadlock or not will remain a deadlock or not. */
/* A non-reversible move does not necessarilly lead into deadlock, 
since some non-reversible moves push stones towards goals. */

int ReversibleMove(MAZE *maze, MOVE *last_move, int depth,
		long effort, int force) 
{
	BitString  old_visible,visible,diff;
	IDA	   idainfo,*old_idainfo;
	long       node_count;
	int	   result;
	OPTIONS    old_opt;

	/* first find out if we should suspect a deadlock */
	if (   force == NO
	    && !NonReversibleSuspected(maze, last_move)) 
		return(0);

	/* if so start the real deadlock search */
	Debug(4,0,"Started ReversibleMove search\n");
	old_opt = Options;
	Options.mc_gm = 0;
	Options.dl_srch = 0;
	Options.pen_srch = 0;
	
	old_idainfo = IdaInfo;
	InitIDA(&idainfo);
	IdaInfo = &idainfo;
	IdaInfo->IdaMaze = CopyMaze(maze);
	IdaInfo->reach_at_goal = last_move->from
				- (last_move->to-last_move->from);
	IdaInfo->base_indent   = old_idainfo->base_indent + depth;
	IdaInfo->AbortNodeCount = effort;
	Set0BS(visible);
	SetBitBS(visible,last_move->to);
	node_count = 0;
	do {
		MoveGoalsToStones(IdaInfo->IdaMaze,last_move,visible);
		result = StartIda(NO);
		node_count+=IdaInfo->node_count;
		/* no solution? */
		if (Is0BS(IdaInfo->IdaManSquares) || result>=ENDPATH) {
			DelCopiedMaze(IdaInfo->IdaMaze);
			IdaInfo = old_idainfo;
			Debug(4,0,"NOT ReversibleMove (n: %li, res: %i)\n",
				node_count, result);
			Options = old_opt;
			return(0);
		}
		/* if a stone is blocked, try that block, else man blocks */
		CopyBS(old_visible,visible);
		BitAndBS(visible,maze->stone, IdaInfo->IdaStoneSquares);
		BitOrEqBS(visible,old_visible);
		BitAndNotBS(diff,visible,old_visible);
		if (Is0BS(diff)) {
			BitAndBS(visible,maze->stone, IdaInfo->IdaManSquares);
			BitOrEqBS(visible,old_visible);
		}
		/* if stones included in patter are sitting on goals
		 * then we can't say anything */
		if (LogAndBS(visible,maze->goal)) {
			Mprintf(2,"Goals touched in ReversibleMove search!\n");
			break;
		}
		/* while we have stones to obstruct the way */
	} while (!EqualBS(old_visible,visible));
	DelCopiedMaze(IdaInfo->IdaMaze);
	IdaInfo = old_idainfo;
	Debug(4,0,"YES ReversibleMove (n: %li)\n", node_count);
	Options = old_opt;
	return(1);
}

/* This function should return 1 if it suspects heuristically that a
 * deadlock might be present after making last_move */
int NonReversibleSuspected(MAZE *maze, MOVE *last_move)
{
	/* if macro, return 0, since no deadlock */
	if (last_move->macro_id != 0) return(0);
	/* if move is physically not reversible, return 0 */
	if (GetOptWeight(maze,last_move->from,last_move->to,NODIR)>1) 
		return(0);
	if (IsBitSetBS(maze->stone,
			last_move->to+(last_move->to-last_move->from)))
		return(0);
	return(1);
}

void MoveGoalsToStones(MAZE *maze, MOVE *last_move, BitString visible)
{
/* Asumtions: there is enough space in goals and stones to hold all */
	PHYSID pos;
	int next_index;

	next_index = 0;
	for (pos = 0; pos<XSIZE*YSIZE; pos++ ) {
		if (IsBitSetBS(visible,pos)) {
			/* Set stone and goal */
			if (last_move->to == pos) {
				/* Make the goal for this stone where it
				 * came from */
				maze->goals[next_index].loc  = last_move->from;
				maze->Phys[last_move->from].goal = next_index;
			} else {
				maze->goals[next_index].loc  = pos;
				maze->Phys[pos].goal         = next_index;
			}
			maze->stones[next_index].loc = pos;
			maze->PHYSstone[pos]        = next_index;
			next_index++;
		} else {
			/* remove stone and goal if there */
			if (last_move->from != pos) {
				maze->Phys[pos].goal  = -1;
			}
			maze->PHYSstone[pos] = -1;
		}
	}
	maze->number_stones = next_index;
	maze->number_goals  = next_index;
	MarkReach(maze);
	BetterLowerBound(maze);
	NormHashKey(maze);
}

