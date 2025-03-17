int SafeSquares(MAZE *maze, BitString safe, int groom_index,
		int use_outside, int optimal);
int QuasiSafeSquares(MAZE *maze, BitString quasisafe, int groom_index,
		int use_outside, int optimal);
void FixedGoals(MAZE *maze, BitString fixed, int groom_index);
void DeadGoals (MAZE *maze, BitString dead, int groom_index);
PHYSID GetBestTarget(MAZE *maze, BitString targets);
