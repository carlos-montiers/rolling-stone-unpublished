extern int PosNr;

void ReadMaze(FILE *fp, MAZE *maze );
void PrintMaze(MAZE *maze);
void PrintSquare(MAZE *maze, PHYSID pos);
char *PrintMove(MOVE move);
void PrintTable(MAZE *maze);
char *HumanMove(MOVE move);
void PrintBit2Maze(MAZE *maze,BitString marks);
void PrintBit3Maze(MAZE *maze,BitString marks,BitString mark2, PHYSID manpos);
