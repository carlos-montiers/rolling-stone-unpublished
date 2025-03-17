int MinMatch(MAZE *maze, PHYSID moveto, UNMOVE *unmove);
int BetterLowerBound(MAZE *maze);
int BetterUpdateLowerBound(MAZE *maze, UNMOVE *unmove);
int BetterUpdateLowerBound2(MAZE *maze, UNMOVE *unmove);

int PlainLowerBound(MAZE *maze);
int PlainMinMatch(MAZE *maze, PHYSID moveto, UNMOVE *unmove);
