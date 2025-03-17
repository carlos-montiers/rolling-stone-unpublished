#define MANTO(maze,to) \
	maze->manpos = to

#define STONEFROMTO(maze,from,to) {\
	int stonei               = maze->PHYSstone[from]; \
	maze->PHYSstone[from]   = -1; \
	maze->PHYSstone[to]     = stonei; \
	maze->stones[stonei].loc = to; \
	UnsetBitBS(maze->stone,from); \
	SetBitBS(maze->stone,to); \
	maze->structs[maze->Phys[from].lstruct].number_stones--;\
	maze->structs[maze->Phys[to].lstruct].number_stones++;\
	}
	  
extern int DirToDiff[4];
extern int  OppDir[4];
extern int NextDir[4];
extern int PrevDir[4];

int  GenerateMoves(MAZE *maze, MOVE *moves);
int  MakeMove(MAZE *maze, MOVE *move, UNMOVE *unmove);
int  UnMakeMove(MAZE *maze, UNMOVE *unmove);
int  DistToGoal(MAZE *maze, PHYSID start, PHYSID goal, PHYSID *last_over);
void Moves(MAZE *maze, PHYSID *from, signed char *reach);
void GenAllSquares( PHYSID pos, PHYSID *from, BitString all_squares );
void PushesMoves(MAZE *maze, PHYSID start, PHYSID goal, 
		 int *pushes, int *moves, 
		 BitString stone_squares, BitString man_squares);
void PushesMoves2(MAZE *maze, PHYSID start, PHYSID goal, 
		 int *pushes, int *moves, 
		 BitString stone_squares, BitString man_squares);
int  ValidSolution(MAZE *maze, MOVE *solution);
int DiffToDir(int diff);

/* all the move ordering functions: */
int NoMoveOrdering(int depth, int number_moves, MOVE bestmove);
int BestMoveOrdering(int depth, int number_moves, MOVE bestmove);
int InertiaMoveOrdering(int depth, int number_moves, MOVE bestmove);
int NewMoveOrdering(int depth, int number_moves, MOVE bestmove);
int ManDistMoveOrdering(int depth, int number_moves, MOVE bestmove);

