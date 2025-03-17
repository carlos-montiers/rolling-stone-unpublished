int PenMoveOrdering(int depth, int number_moves, MOVE bestmove);
int  NoMoreMoves(MAZE *maze, MOVE *last_move);
int  PenMoveSuspected(MAZE *maze, MOVE *last_move, HASHENTRY *entry);
void PenMiniConflict(BitString visible, int penalty);

int  PenIsGoalNode(int g);
int  PenMove(MAZE *maze, HASHENTRY *entry, MOVE *last_move, int treedepth,
	int g, int targetpen, int *pensearched, int32_t effort);
void PenDeactivateStones(MAZE *maze, BitString visible);
int  PenStartIda();
int  PenIda(int treedepth, int g);
int  PenMakeMove(MAZE *maze, MOVE *move, UNMOVE *ret);
int  PenUnMakeMove(MAZE *maze, UNMOVE *unmove);
int  PenLowerBound(MAZE *maze);
int  PenMinMatch(MAZE *maze, PHYSID moveto, UNMOVE *unmove);
int  PenUpdateLowerBound(MAZE *maze, PHYSID pos, UNMOVE *unmove);
void ConflictStartIda(int Threshold, int immediate);
void ConflictIda(int treedepth, int g);
void FindFringeStones( MAZE *maze, BitString fs );
PHYSID FindClosestPosFringe( MAZE *maze, BitString squares,
			     BitString already_visible, BitString fringe );
