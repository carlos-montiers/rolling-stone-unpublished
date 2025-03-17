#include "board.h"

static int compare_val(const void *m1, const void *m2) {
        return(((MOVE*)m2)->value - ((MOVE*)m1)->value);
}       


int RealSearch(int32_t effort)
{
/* This routine is searching a maze in "real"-time, meaning it commits every
 * max-nodes nodes of search to a move to make. The problem here is of course,
 * not to run into deadlock situations */

/* The basic idea is the following:
   1) We make the best effort trying to find bestmove.
   2) We check if that bestmove is reversible, commit bestmove.
   3) We check if that move is deadlock, if so goto 1).
   4) Commit to bestmove.

The only really tricky part is 1) and the multiple return to one via 3),
since we are forced into a real-time mode here. */

	MOVE Path[MAX_DEPTH];
	MOVE moves[200];
	int  curidx,midx,number_moves,bestidx;
	MAZE *Maze = IdaInfo->IdaMaze;
	UNMOVE unmove;
	int dlsearched; 
	HASHENTRY entry;

	curidx = 0;
	IdaInfo->PrintPriority = -1;
	do {/* run until solution found */
		/* Generate, evaluate moves */
		number_moves = GenerateMoves(Maze,moves);

		bestidx = 0;
		if (number_moves > 1) {
			for ( midx = 0; midx < number_moves; midx++ ) {
			      moves[midx].value = MoveValue(Maze,&(moves[midx]),
					effort/(number_moves*2));
			}
			My_qsort(moves,number_moves,sizeof(MOVE),compare_val);
			/* until we approve one move */
			for (bestidx = 0; bestidx<number_moves; bestidx++) {
				Mprintf(-1,"Revers move %s? - ",
					HumanMove(moves[bestidx]));
				MakeMove(Maze,&moves[bestidx],&unmove);
				if (ReversibleMove(Maze,moves+bestidx,0,
					effort/4, YES)) {
					UnMakeMove(Maze,&unmove);
					Mprintf(-1,"YES\n");
					break;
				}
				Mprintf(-1,"NO\n");
				dlsearched       = 0;
				entry.dlsearched = 0;
				entry.tree_size  = 50;
				Mprintf(-1,"Dead move %s? - ",
					HumanMove(moves[bestidx]));
				if (DeadMove(Maze, &entry, moves+bestidx,
					1,curidx, &dlsearched, effort/4)) {
					UnMakeMove(Maze,&unmove);
					Mprintf(-1,"YES\n");
					continue;
				} else {
					UnMakeMove(Maze,&unmove);
					Mprintf(-1,"NO\n");
					break;
				}
			}
		}
		if (bestidx >= number_moves) goto STUCK;

		/* commit to best move */
		Mprintf(-1,"Commit to move %s\n",HumanMove(moves[bestidx]));
		Path[curidx++] = moves[bestidx];
		MakeMove(Maze,moves+bestidx,&unmove);
		AddConflict(Maze->conflicts,Maze->stone,
			    Maze->no_reach,Maze->reach,2,0);
		PrintMaze(Maze);

	} while (Maze->h>0);

	Mprintf(-1,"Path: ");
	for (midx=0; midx<curidx; midx++) {
		Mprintf(-1," %s",HumanMove(Path[midx]));
	}
	Mprintf(-1,"\n");

	IdaInfo->PrintPriority = 2;
	return(1);
STUCK:
	Mprintf(-1,"No Solution found: no more moves!\n");
	IdaInfo->PrintPriority = 2;
	DelCopiedMaze(Maze);
	return(0);
}

short MoveValue(MAZE *maze, MOVE *move, int32_t effort) {

	short i;
	int   result;
	int32_t old_effort;
	MAZE *old_maze;
	UNMOVE unmove;

	if (move->from == 0 || move->from == ENDPATH ) return(-ENDPATH);
	
	Mprintf(-1,"Evaluating move %s - ",HumanMove(*move));
	init_stats();
        for (i=0; i<MAX_DEPTH; i++) {
		/* IdaInfo->IdaArray[i].solution = (MOVE){0,0,0,0,0,0}; */
                memset( &IdaInfo->IdaArray[i].solution, 0, sizeof( MOVE ) );
        }

	MakeMove(maze,move,&unmove);
	old_effort = IdaInfo->AbortNodeCount;
	IdaInfo->AbortNodeCount = effort;
	old_maze = IdaInfo->IdaMaze;
	IdaInfo->IdaMaze = maze;

	result = StartIda(NO);

	IdaInfo->IdaMaze = old_maze;
	IdaInfo->AbortNodeCount = old_effort;
	UnMakeMove(maze,&unmove);

	/* create value */
	i=0;
	while (IdaInfo->nodes_depth[i]!=0) i++;

	if (IdaInfo->IdaArray[0].solution.from!=0) {
		i = ENDPATH + MAX_DEPTH - i;
		Mprintf(-1,"solution found: %i\n",i);
	}
	else if (result >= ENDPATH) {
		i = -result;
		Mprintf(-1,"dead: %i\n",i);
	} else {
		Mprintf(-1,"depth: %i\n",i);
	}
	
	return( i );
}

