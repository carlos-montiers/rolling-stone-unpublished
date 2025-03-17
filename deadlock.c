#include "board.h"

int DeadLock(MAZE *maze, MOVE move) {
/* Does not catch this pattern:
    $#
    #
*/

	PHYSID frontdiff,sidediff;

	if (Options.dl_mg==0) return(0);
	if (maze->Phys[move.to].goal >= 0) return(0);
	frontdiff = move.to - move.from;
	if (abs(frontdiff)==1) sidediff=YSIZE;
	else sidediff=1;

	/* check two-wall block */
	if (  (IsBitSetBS(maze->wall,move.to+frontdiff))
	    &&(  (IsBitSetBS(maze->wall,move.to+sidediff))
	       ||(IsBitSetBS(maze->wall,move.to-sidediff))))
		return(1);

	/* check the four-block */
	if (  (IsBitSetBS(maze->wall,move.to+frontdiff))
	    ||(maze->PHYSstone[move.to+frontdiff]>=0)) {
		/* check one side */
		if (   (  (IsBitSetBS(maze->wall,move.to+sidediff))
			||(maze->PHYSstone[move.to+sidediff]>=0))
		    && (  (IsBitSetBS(maze->wall,move.to+sidediff+frontdiff))
			||(maze->PHYSstone[move.to+sidediff+frontdiff]>=0))) 
			return(1);
		sidediff = -sidediff;
		if (   (  (IsBitSetBS(maze->wall,move.to+sidediff))
			||(maze->PHYSstone[move.to+sidediff]>=0))
		    && (  (IsBitSetBS(maze->wall,move.to+sidediff+frontdiff))
			||(maze->PHYSstone[move.to+sidediff+frontdiff]>=0))) 
			return(1);
	}
	return(0);
}


int DeadLock2(MAZE *maze, MOVE *move) {
	int     dir,dirloop;

	if (Options.dl2_mg==0) return(0);
	if (AvoidThisSquare!=0) return(0);
	if (ISDUMMYMOVE(*move)) return(0);
	if (maze->Phys[move->to].goal >= 0) return(0);
	if (IsBitSetBS(maze->one_way,move->to)) {
		/* the stone lands on a one way square, check if it is
		 * permanently dead */
		/* check for each direction it could be pushed to (using the
		 * for OneWayWeight pointers to determine where the man can
		 * go), 1) if SX is set, 2) if no stone is there 3) no
		 * deadlock is created */
		dir = DiffToDir(move->last_over - move->to);
                if (dir == NODIR) return(0);
		/* If one move can be generated, return NO deadlock */
		for (dirloop=NORTH; dirloop<=WEST; dirloop++) {
			if (   (ConnectedDir(maze,move->to,dir,dirloop))
		            && (IsBitSetBS(maze->S[OppDir[dirloop]],move->to))
		            && ( (move->last_over==move->to-DirToDiff[dirloop])
		                ||(  (maze->PHYSstone[move->to
						     -DirToDiff[dirloop]]<0)
		                   &&(!DeadTree(maze,
						move->to-DirToDiff[dirloop],
						OppDir[dirloop]))))) {
				/* move is possible */
				return(0);
			}
		}
		/* deadlock */
		return(1);
	} 
	return(0);
}

