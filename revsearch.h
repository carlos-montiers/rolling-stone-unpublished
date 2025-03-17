int  IsGoalNodeRev();
int  ReversibleMove(MAZE *maze, MOVE *last_move, int depth,
	int32_t effort, int force);
void MoveGoalsToStones(MAZE *maze, MOVE *last_move, BitString visible);
int  NonReversibleSuspected(MAZE *maze, MOVE *next_move);
