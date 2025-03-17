#include "board.h"

#define STARTP 27
#define ENDP   60

void sigint () {

#ifdef NAVIGATOR
	MyNavigator.enter_menu = YES;
#else
	Mprintf( 0, "\n");
	PrintMaze(IdaInfo->IdaMaze);
#endif
	signal(SIGINT,sigint);
}

int main() {

	char   *ev;
	int    i;

	NavigatorInit();
	InitRandom();
	init_opts();
	InitBS();
	InitIDA(&MainIdaInfo);
	IdaInfo = &MainIdaInfo;

	ev = getenv("PP");
	if (ev!=NULL) IdaInfo->PrintPriority=atoi(ev);
	else IdaInfo->PrintPriority=2;
	Mprintf(2, "PrintPriority: %i\n", IdaInfo->PrintPriority);

	i = InitTree(DlSup1);
	LoadTree(i,DL1PATHFILE);
	i = InitTree(DlSup2);
	LoadTree(i,DL2PATHFILE);
/*
*/
	signal(SIGINT,sigint);

	MainMenu();

	My_exit(0,"");
	return(0);
}

void TestX(MAZE *maze, PHYSID from, PHYSID to)
{
	MOVE move;
	HASHENTRY entry;
	UNMOVE unmove;
	int  r;
	int dlsearched;

	move.from = from;
	move.to   = to;
	move.last_over = from;
	move.macro_id  = 0;
	move.move_dist = 1;

	entry.dlsearched=0;
	entry.tree_size=100000;

	MakeMove(maze,&move,&unmove);

	r = DeadMove(maze,&entry,&move,1,0,&dlsearched,PATTERNSEARCH);
	
	Mprintf( 0, "Return value from DeadMove = %i\n", r);
}
