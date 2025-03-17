extern PHYSID AvoidThisSquare;

void MarkTG(MAZE *maze);
void MarkAll(MAZE *maze);
void MarkOut(MAZE *maze, PHYSID pos);
void CleanReach(MAZE *maze);
void UpdateReach( MAZE *maze, PHYSID stonepos );
void MarkReach(MAZE *maze);
void MarkReachNoUnreach(MAZE *maze);
void MarkReachQuick( MAZE *maze, PHYSID from );
void MarkDead(MAZE *maze);
void MarkBackDead(MAZE *maze);
void MarkEqual(MAZE *maze);

void MarkOneHelp(MAZE *maze, PHYSID curr, PHYSID avoid);
void MarkOneTest(MAZE *maze, int dir, PHYSID curr);
void MarkOne(MAZE *maze);
void MarkTun(MAZE *maze);
void MarkStruct(MAZE *maze);
#ifdef NEEDED
void MarkNeeded(MAZE *maze);
int  GetNeeded(MAZE *maze, BitString needed, PHYSID from, PHYSID to);
#endif
int  Hinch(MAZE *maze, PHYSID pos, PHYSID prev);
int  GetYDim(MAZE *maze);
