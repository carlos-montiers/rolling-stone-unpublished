extern IDA *IdaInfo;

int  StartIda(int nomacro);
void PrintSolution();
int  IsGoalNodeNorm(int g);
/* TT GHI fix */
/* int  Ida(int depth, int g, int lastnonlmove); */
int  Ida(int depth, int g, int prev_pen);
void SetManStoneSquares(MAZE *maze, MOVE bestmove);
int  AbortSearch();
void InitIDA(IDA *ida);

int  DistantSquares(PHYSID s1, PHYSID s2, int growding);
int  DistantMove(MAZE *maze, MOVE *last_move, MOVE *test_move);
void SetLocalCut(int k, int m, int d);
int  RegisterMove(MOVE *move, int depth);
int  Growding(MAZE *maze, PHYSID sq);
