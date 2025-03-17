#include "board.h"

/****************************   FWINDOW ADT   **********************/

#ifdef NAVIGATOR

FWINDOW *newfwin(int ysize, int xsize, int ystart, int xstart, char *title)
/* create a framed window */
{

	FWINDOW *fw;

	fw = My_malloc(sizeof(FWINDOW));
	fw->fwindow = newwin(ysize,xsize,ystart,xstart);
	fw->window  = newwin(ysize-2,xsize-2,ystart+1,xstart+1);
	scrollok(fw->window,TRUE);
	Fwresize(fw,xsize,ysize);
	FwSetTitle(fw,title);

	return(fw);
}

void Fwclear( FWINDOW *fw )
{
	werase(fw->window);
	wmove(fw->window,0,0);
}

void FWActiveFrame( FWINDOW *fw )
{
	wstandout( fw->fwindow );
	FwSetTitle( fw, NULL );
	wstandend( fw->fwindow );
}

void FWDeActiveFrame( FWINDOW *fw )
{
	FwSetTitle( fw, NULL );
}

void FwSetTitle(FWINDOW *fw, char *title)
/* Set title in the frame of the FWINDOW */
{
	if (title != NULL) strcpy(fw->title,title);
	box(fw->fwindow,0,0);
	mvwaddnstr(fw->fwindow, 0, 1, fw->title, fw->xsize-2);
}

void     Fwresize(FWINDOW *fw, int xsize, int ysize)
/* Resize window */
{
	if (xsize != -1) fw->xsize = xsize;
	if (ysize != -1) fw->ysize = ysize;
	wresize(fw->fwindow, fw->ysize, fw->xsize);
	wresize(fw->window, fw->ysize-2, fw->xsize-2);
	box(fw->fwindow,0,0);
	mvwaddnstr(fw->fwindow, 0, 1, fw->title, fw->xsize-2);
}

void     Fwmove(FWINDOW *fw, int xstart, int ystart)
{
	mvwin(fw->fwindow,ystart,xstart);
	mvwin(fw->window,ystart+1,xstart+1);
}

void Fwaddstr(FWINDOW *fw, char *msg)
{
	waddstr(fw->window,msg);
	wrefresh(fw->window);
}

void FwrefreshAll(FWINDOW *fw)
{
	touchwin(fw->fwindow);
	wrefresh(fw->fwindow);
	touchwin(fw->window);
	wrefresh(fw->window);
}

void Fwrefresh(FWINDOW *fw)
{
	wrefresh(fw->window);
}

void FwPrintBitMaze(FWINDOW *fw, BitString bits)
/* print Maze into fw */
{

	FWINDOW *oldfw;
	WINDOW *w;

	w = GetDefaultWindow(fw);
	Fwclear(fw);
	oldfw = MyNavigator.fdefault;
	MyNavigator.fdefault = fw;
	PrintBit2Maze(IdaInfo->IdaMaze,bits);
	MyNavigator.fdefault = oldfw;
}

void FwPrintBit2Maze(FWINDOW *fw, BitString bits1,
			BitString bits2, PHYSID manpos)
/* print Maze into fw */
{

	FWINDOW *oldfw;
	WINDOW *w;

	w = GetDefaultWindow(fw);
	Fwclear(fw);
	oldfw = MyNavigator.fdefault;
	MyNavigator.fdefault = fw;
	PrintBit3Maze(IdaInfo->IdaMaze,bits1,bits2,manpos);
	MyNavigator.fdefault = oldfw;
}

void FwPrintMaze(FWINDOW *fw, MAZE *maze)
/* print Maze into fw */
{

	FWINDOW *oldfw;
	WINDOW *w;

	w = GetDefaultWindow(fw);
	Fwclear(fw);
	oldfw = MyNavigator.fdefault;
	MyNavigator.fdefault = fw;
	PrintMaze(maze);
	MyNavigator.fdefault = oldfw;
}

WINDOW *GetDefaultWindow(FWINDOW *fw)
/* return the window where the output should go by fdefault */
{
	return(fw->window);
}

/****************************   NAVIGATOR ADT   **********************/

MYNAVIGATOR MyNavigator;

void SwitchToInteractive()
/* switch to character at a time mode */
{
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
}

void SwitchToStandard()
/* switch to standard unix behaviour */
{
	nocbreak();
	noecho();
	nl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
}

void NavigatorInit() 
/* called at very beginning of program to setup terminal and Navigator data
 * structure */
{
	initscr();

	if (COLS < XSIZE*3 + 6) {
		My_exit(1,"Make Terminal larger, number columns not sufficient!!!\n%i columns are needed!\n", XSIZE*3+6);
	}

	SwitchToStandard();
	clear();
	refresh();
	MyNavigator.show_pen   = 2;
	MyNavigator.show_index = 0;

	MyNavigator.start   = newfwin(YSIZE+2,XSIZE+2,0,0,"start");
	MyNavigator.pattern = newfwin(YSIZE+2,XSIZE+2,0,XSIZE+2,"pattern");
	MyNavigator.ps_start= newfwin(YSIZE+2,XSIZE+2,0,(XSIZE+2)*2,"ps_start");
	MyNavigator.depth   = newfwin(YSIZE+2,COLS-(XSIZE+2),
					YSIZE+2,0,"depth");
	MyNavigator.deepest = newfwin(YSIZE+2,XSIZE+2,
					YSIZE+2,(XSIZE+2)*2,"deepest");
	MyNavigator.general = newfwin(LINES-((YSIZE+2)*2),COLS,
					(YSIZE+2)*2,0,"general");
	
	MyNavigator.fdefault = MyNavigator.general;
	NavigatorSetMaxSize(YSIZE+2);
}

void NavigatorRefresh()
/* This redraws and refreshes all sub-windows of MyNavigator.
 * It should be called at startup, after all structures are initialized or
 * after window-size changes (New Maze events) */
{
	FwrefreshAll(MyNavigator.start   );
	FwrefreshAll(MyNavigator.pattern );
	FwrefreshAll(MyNavigator.ps_start);
	FwrefreshAll(MyNavigator.depth   );
	FwrefreshAll(MyNavigator.deepest );
	FwrefreshAll(MyNavigator.general );
}

void NavigatorSetMaxSize(int maxYSize)
/* Set the size for the maze fwindows and adjust the rest */
{
	int old_maxysize;

	old_maxysize = MyNavigator.maze_ysize;
	MyNavigator.maze_ysize      = maxYSize;
	Fwresize(MyNavigator.start,-1,maxYSize);
	Fwresize(MyNavigator.pattern,-1,maxYSize);
	Fwresize(MyNavigator.ps_start,-1,maxYSize);
	if (old_maxysize > maxYSize) {
		Fwmove(MyNavigator.depth,0,maxYSize);
		Fwresize(MyNavigator.depth,-1,maxYSize);
		Fwmove(MyNavigator.deepest,COLS-(XSIZE+2),maxYSize);
		Fwresize(MyNavigator.deepest,-1,maxYSize);
		Fwmove(MyNavigator.general,0,2*maxYSize);
		Fwresize(MyNavigator.general,-1,(LINES - 2*maxYSize));
	} else {
		Fwresize(MyNavigator.depth,-1,maxYSize);
		Fwmove(MyNavigator.depth,0,maxYSize);
		Fwresize(MyNavigator.deepest,-1,maxYSize);
		Fwmove(MyNavigator.deepest,COLS-(XSIZE+2),maxYSize);
		Fwresize(MyNavigator.general,-1,(LINES - 2*maxYSize));
		Fwmove(MyNavigator.general,0,2*maxYSize);
	}
	NavigatorRefresh();
}

void NavigatorSetStartMaze(MAZE *maze, char *title)
/* used to set the start maze and print it on the screen */
{
	MyNavigator.show_pen   = 2;
	MyNavigator.show_index = 0;
	NavigatorSetMaxSize(GetYDim(maze)+6);
	FwSetTitle(MyNavigator.start,title);
	FwPrintMaze(MyNavigator.start,maze);
	NavigatorPSConflict(maze);
}

void NavigatorSetPattern(MAZE *maze, BitString bs1, BitString bs2)
{
	FwPrintBit2Maze(MyNavigator.pattern,bs1,bs2,0);
	Fwrefresh(MyNavigator.pattern);
}

void NavigatorPSNEWS(MAZE *maze, int dir)
/* change pattern displayed according to dir: NS for index, EW for penalty */
{
	PENALTY *p;
	int      prior_pen, post_pen;

	p = FindPenalty( maze->conflicts, MyNavigator.show_pen );
	GetPriorPostPen(maze, p->penalty, &prior_pen, &post_pen);
	switch (dir) {
	case NORTH:	if (MyNavigator.show_index > 0) {
				MyNavigator.show_index--;
				NavigatorPSConflict(maze);
			}
			break;
	case SOUTH:	if (MyNavigator.show_index < p->number_conflicts-1) {
				MyNavigator.show_index++;
				NavigatorPSConflict(maze);
			}
			break;
	case  WEST:	if (prior_pen != -1) {
				MyNavigator.show_pen = prior_pen;
				NavigatorPSConflict(maze);
			}
			break;
	case  EAST:	if (post_pen != -1) {
				MyNavigator.show_pen = post_pen;
				NavigatorPSConflict(maze);
			}
			break;
	   default:
	}
}

int  NavigatorPSConflict(MAZE *maze)
/* print given conflict into ps_pattern window, if exists return 1,
 * else, return 0 and do not print, but clear */
{
	PENALTY *p;
	int     prior_pen, post_pen;
	int	penalty, index;

	penalty = MyNavigator.show_pen;
	index   = MyNavigator.show_index;
	p = FindPenalty(maze->conflicts, penalty);
	if (p != NULL && p->number_conflicts > index) {
		NavigatorSetPattern(maze,p->cflts[index].conflict,
					p->cflts[index].no_reach);
		GetPriorPostPen(maze, penalty, &prior_pen, &post_pen);
		Fwaddstr(MyNavigator.pattern,"penalty:");
		if (prior_pen != -1) {
			wprintw(MyNavigator.pattern->window, "%4d", prior_pen);
		} else {
			wprintw(MyNavigator.pattern->window, "    ");
		}

		wstandout(MyNavigator.pattern->window);
		wprintw(MyNavigator.pattern->window, " %4d", penalty);
		wstandend(MyNavigator.pattern->window);

		if (post_pen != -1) {
			wprintw(MyNavigator.pattern->window, " %4d", post_pen);
		}
		wprintw(MyNavigator.pattern->window,"\n  index:     %4d/%d",
			index+1,
			p->number_conflicts);
		wrefresh(MyNavigator.pattern->window);
		return(1);
	}
	Fwclear(MyNavigator.pattern);
	mvwaddnstr(MyNavigator.pattern->window,2,2,"No patterns yet",YSIZE);
	wrefresh(MyNavigator.pattern->window);
	return(0);
}

void NavigatorSetPSMaze(MAZE *maze, BitString bs1, BitString bs2)
{
	FwPrintBit2Maze(MyNavigator.ps_start,bs1,bs2,maze->manpos);
	FWActiveFrame( MyNavigator.ps_start );
	FwrefreshAll(MyNavigator.ps_start);
}

void NavigatorUnsetPSMaze()
{
	FWDeActiveFrame( MyNavigator.ps_start );
	FwrefreshAll(MyNavigator.ps_start);
}

void NavigatorSetDeepest(MAZE *maze, int depth)
/* used to set the deepest maze */
{
	FwPrintMaze(MyNavigator.deepest,maze);

	wprintw(MyNavigator.deepest->window, "at: %3d", depth);
	Fwrefresh(MyNavigator.deepest);
}

void NavigatorSetNodeCount(long total_node_count, long node_count)
{
	mvwprintw(MyNavigator.depth->window,0,0,
		" total nodes: %10li, curr nodes: %10ld",
		total_node_count, node_count);
	Fwrefresh(MyNavigator.depth);
}

void NavigatorSetDepth(char *msg)
{
	wmove(MyNavigator.depth->window,1,0);
	wclrtobot(MyNavigator.depth->window);
	waddstr(MyNavigator.depth->window,msg);
	Fwrefresh(MyNavigator.depth);
}

#endif /* NAVIGATOR */

