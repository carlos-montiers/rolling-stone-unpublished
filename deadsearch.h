int DeadMoveOrdering(int depth, int number_moves, MOVE bestmove);
void DeadDeactivateStones(MAZE *maze, BitString visible);
int  DeadMoveSuspected(MAZE *maze, MOVE *last_move, HASHENTRY *entry);
PHYSID FindClosestPosMan(MAZE *maze, BitString squares, 
		      BitString already_visible);
PHYSID FindClosestPosStone(MAZE *maze, BitString squares, 
		      BitString already_visible);
void DeadMiniConflict(BitString visible);

int  DeadIsGoalNode();
int  DeadMove(MAZE *maze, HASHENTRY *entry, MOVE *last_move, 
		int treedepth, int g, int *dlsearched, int32_t effort);
int  DeadStartIda();
int  DeadIda(int treedepth, int g);
int  DeadMakeMove(MAZE *maze, MOVE *move, UNMOVE *ret);
int  DeadUnMakeMove(MAZE *maze, UNMOVE *unmove);
int  DeadLowerBound(MAZE *maze);
int  DeadMinMatch(MAZE *maze, PHYSID moveto, UNMOVE *unmove);
int  DeadUpdateLowerBound(MAZE *maze, PHYSID pos, UNMOVE *unmove);
void DConflictStartIda(int immediate);
void DConflictIda(int treedepth, int g);
