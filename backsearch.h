int  IsGoalNodeBack(int g);
int  BackGenerateMoves(MAZE *maze, MOVE *moves);
int  BackMoveOrdering(int depth, int number_moves, MOVE bestmove);
int  BackMakeMove(MAZE *maze, MOVE *move, UNMOVE *ret);
int  BackUnMakeMove(MAZE *maze, UNMOVE *unmove);
int  StartBackLowerBound(MAZE *maze);
int  BackBetterLowerBound(MAZE *maze);
int  BackMinMatch(MAZE *maze, PHYSID moveto, UNMOVE *unmove);
int  BackUpdateLowerBound(MAZE *maze, UNMOVE *unmove);
int  BackUpdateLowerBound2(MAZE *maze, UNMOVE *unmove);
int  BackMoveSuspected(MAZE *maze, MOVE *last_move, HASHENTRY *entry);
void BackGoalsStones(MAZE *maze);
int  BackStartIda();
int  BackIda(int treedepth, int g);
void BackSetGoalWeights(MAZE *maze);
void BackPrintSolution();

