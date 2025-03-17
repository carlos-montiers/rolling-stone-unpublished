#include "board.h"

int SafeSquares(MAZE *maze, BitString safe, int groom_index,
		int use_outside, int optimal) {

/* This routine returns all the squares that are safe (will not obstruct the
 * way to ANY other square from ANY other entrance) in a certain goal room */

/* two switches are used:
   1. Use_outside switch - it determines if the man can use the outside
	of a goal area to move around. If so, during the search, it is
	possible, that certain macros are not possible, because the out-
	side of the maze is not "cooperating".
   2. Optimal     switch - it determines if reachablity is only considering
	goals that are reachable in the optimal number of moves.

RETURNS: 1 if non-reachable goals are around, else 0 */

	BitString needed, all_reachable;
	BitString before,after,tmp;
	int	  goali,entri;
	PHYSID    pos;
	PHYSID    old_manpos;

	Set0BS(safe);
	Set0BS(needed);
	Set0BS(all_reachable);
	old_manpos = maze->manpos;
	for (entri=0; entri<maze->grooms[groom_index].n; entri++) {
		/* where can we go, no restrictions no limits... */
		GoalReach(maze,before,
			  maze->grooms[groom_index].locations[entri],
			  maze->grooms[groom_index].entrances[entri],
			  use_outside?-1:groom_index,optimal);
		BitOrEqBS(all_reachable,before);
		for (goali=0; 
		     goali<maze->number_goals; 
		     goali++) {
			pos = maze->goals[goali].loc;
			if (   IsBitSetBS(needed,pos)
			    || maze->groom_index[pos] != groom_index
			    || IsBitSetBS(maze->stone,pos))
				continue;
			SetBitBS(safe,pos);
			maze->PHYSstone[pos]=0;
			GoalReach(maze,after,
				  maze->grooms[groom_index].locations[entri],
				  maze->grooms[groom_index].entrances[entri],
				  use_outside?-1:groom_index,optimal);
			maze->PHYSstone[pos]=-1;
			BitAndNotBS(tmp,before,after);
			UnsetBitBS(tmp,pos);
			if (Isnt0BS(tmp)) {
				SetBitBS(needed,pos);
			}
		}
	}
	MANTO(maze,old_manpos);
	MarkReach(maze);

	/* are there any dead goals? */
	CopyBS(tmp,safe);
	BitAndNotEqBS(tmp,all_reachable);
	BitAndNotEqBS(safe,needed);
	return(Isnt0BS(tmp));
}

int QuasiSafeSquares(MAZE *maze, BitString quasisafe, int groom_index,
		int use_outside, int optimal) {
/* This routine attampts the same as SafeSquares, except we are now
   softening the restriction that all entrances need to be able to get to
   the same squares as before. We are happy now if every square reachable
   from at least one entrance. Note that this will now muddy the water
   for solvabilty - we might not have the stones to push through the
   right entrances to solve the problem. */

	int    goali,entri;
	PHYSID pos;
	BitString tmp,reach,total_reach;

	for (goali=0; 
	     goali<maze->number_goals; 
	     goali++) {
		pos = maze->goals[goali].loc;
		if (   maze->groom_index[pos] != groom_index
		    || IsBitSetBS(maze->stone,pos))
			continue;
		Set0BS(total_reach);
		maze->PHYSstone[pos]=0;
		for (entri=0; entri<maze->grooms[groom_index].n; entri++) {
			GoalReach(maze,reach,
				  maze->grooms[groom_index].locations[entri],
				  maze->grooms[groom_index].entrances[entri],
				  use_outside?-1:groom_index,optimal);
			BitOrEqBS(total_reach,reach);
		}
		maze->PHYSstone[pos]=-1;
		CopyBS(tmp,maze->grooms[groom_index].goals);
		BitAndEqBS(tmp,maze->goal);
		BitAndNotEqBS(tmp,maze->stone);
		BitAndNotEqBS(tmp,total_reach);
		UnsetBitBS(tmp,pos);
		if (Is0BS(tmp)) SetBitBS(quasisafe,pos);
	}
	if (Is0BS(quasisafe)) return(1);
	else return(0);
}

void FixedGoals(MAZE *maze, BitString fixed, int groom_index)
/* out of all goals n a groom in the maze which ones would be "fixed"? */
/* NOTE: This routine does not detect fixed stones that are usually
 * deadlocks by interaction :########
				$$     */

{

	int    changed_fixed;
	int    goali;
	PHYSID pos;
	BitString wallstone;

	Set0BS(fixed);
	do {
		changed_fixed=NO;
		for (goali=0; goali<maze->number_goals; goali++) {
			pos = maze->goals[goali].loc;
			if (   (groom_index != maze->groom_index[pos])
			    || (IsBitSetBS(fixed,pos)))
				continue;
			BitOrBS(wallstone,maze->wall,maze->stone);
			if (   (   (   IsBitSetBS(maze->wall,pos+1)
				    || (  IsBitSetBS(fixed,pos+1)
				        &&IsBitSetBS(maze->stone,pos+1))
				    || IsBitSetBS(maze->wall,pos-1)
				    || (  IsBitSetBS(fixed,pos-1)
				        &&IsBitSetBS(maze->stone,pos-1)))
			        && (   IsBitSetBS(maze->wall,pos+YSIZE)
				    || (  IsBitSetBS(fixed,pos+YSIZE)
				        &&IsBitSetBS(maze->stone,pos+YSIZE))
				    || IsBitSetBS(maze->wall,pos-YSIZE)
				    || (  IsBitSetBS(fixed,pos-YSIZE)
				        &&IsBitSetBS(maze->stone,pos-YSIZE))))
			    || (   IsBitSetBS(wallstone,pos+YSIZE+1)
				&& IsBitSetBS(wallstone,pos+YSIZE)
				&& IsBitSetBS(wallstone,pos+1))
			    || (   IsBitSetBS(wallstone,pos+YSIZE-1)
				&& IsBitSetBS(wallstone,pos+YSIZE)
				&& IsBitSetBS(wallstone,pos-1))
			    || (   IsBitSetBS(wallstone,pos-YSIZE+1)
				&& IsBitSetBS(wallstone,pos-YSIZE)
				&& IsBitSetBS(wallstone,pos+1))
			    || (   IsBitSetBS(wallstone,pos-YSIZE-1)
				&& IsBitSetBS(wallstone,pos-YSIZE)
				&& IsBitSetBS(wallstone,pos-1)) ) {
				changed_fixed=YES;
				SetBitBS(fixed,pos);
			}
		}
	} while (changed_fixed==YES);
}

void DeadGoals (MAZE *maze, BitString dead, int groom_index)
/* This routine tries to discover if there are goals that are not reachable
 * DIRECTLY from the entrance (without pushing other stones in the goal
 * area) */
{
	int entri;
	BitString reach,alive;
	
	CopyBS(dead,maze->grooms[groom_index].goals);
	Set0BS(alive);
	for (entri=0; entri<maze->grooms[groom_index].n; entri++) {
		GoalReach(maze, reach, 
			  maze->grooms[groom_index].locations[entri],
			  maze->grooms[groom_index].entrances[entri],
			  -1,1);
		BitOrEqBS(alive,reach);
	}
	BitAndNotEqBS(dead,alive);
}

PHYSID GetBestTarget(MAZE *maze, BitString targets)
/* return the goal with the minimum weight in targets */
{
	int    min,goali;
	PHYSID pos,min_pos;
	
	min = ENDPATH;
	min_pos = 0;
	for (goali=0; goali<maze->number_goals; goali++) {
		pos = maze->goals[goali].loc;
		if (IsBitSetBS(targets,pos)) {
			if (min>maze->goals[goali].weight) {
				min_pos = pos;
				min     = maze->goals[goali].weight;
			}
		}
	}
	return(min_pos);
}


