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

void Fwdelwin( FWINDOW *fw )
/* delete windows */
{
	if (fw == NULL) return;
	delwin(fw->window);
	delwin(fw->fwindow);
	My_free(fw);
}

void Fwclear( FWINDOW *fw )
{
	if (fw == NULL) return;
	werase(fw->window);
	wmove(fw->window,0,0);
}

void FWActiveFrame( FWINDOW *fw )
{
	if (fw == NULL) return;
	wstandout( fw->fwindow );
	FwSetTitle( fw, NULL );
	wstandend( fw->fwindow );
}

void FWDeActiveFrame( FWINDOW *fw )
{
	if (fw == NULL) return;
	FwSetTitle( fw, NULL );
}

void FwSetTitle(FWINDOW *fw, char *title)
/* Set title in the frame of the FWINDOW */
{
	if (fw == NULL) return;
	if (title != NULL) strcpy(fw->title,title);
	box(fw->fwindow,0,0);
	mvwaddnstr(fw->fwindow, 0, 1, fw->title, fw->xsize-2);
}

void FwSetStatus1(FWINDOW *fw, char *msg)
/* Set status line 1 with message, clear toendofline */
{
	mvwaddnstr(fw->window, fw->ysize-3, 1, msg, fw->xsize-3);
	wclrtoeol(fw->window);
	wrefresh(fw->window);
}

void FwSetStatus2(FWINDOW *fw, char *msg)
/* Set status line 1 with message, clear toendofline */
{
	mvwaddnstr(fw->window, fw->ysize-2, 1, msg, fw->xsize-3);
	wclrtoeol(fw->window);
	wrefresh(fw->window);
}

void     Fwresize(FWINDOW *fw, int xsize, int ysize)
/* Resize window */
{
	if (fw == NULL) return;
	if (xsize != -1) fw->xsize = xsize;
	if (ysize != -1) fw->ysize = ysize;
	wresize(fw->fwindow, fw->ysize, fw->xsize);
	wresize(fw->window, fw->ysize-2, fw->xsize-2);
	box(fw->fwindow,0,0);
	mvwaddnstr(fw->fwindow, 0, 1, fw->title, fw->xsize-2);
}

void     Fwmove(FWINDOW *fw, int xstart, int ystart)
{
	if (fw == NULL) return;
	mvwin(fw->fwindow,ystart,xstart);
	mvwin(fw->window,ystart+1,xstart+1);
}

void Fwaddstr(FWINDOW *fw, char *msg)
{
	if (fw == NULL) return;
	waddstr(fw->window,msg);
	wrefresh(fw->window);
}

void FwrefreshAll(FWINDOW *fw)
{
	if (fw == NULL) return;
	touchwin(fw->fwindow);
	wrefresh(fw->fwindow);
	touchwin(fw->window);
	wrefresh(fw->window);
}

void Fwrefresh(FWINDOW *fw)
{
	if (fw == NULL) return;
	wrefresh(fw->window);
}

void FwPrintBitMaze(FWINDOW *fw, BitString bits)
/* print Maze into fw */
{

	FWINDOW *oldfw;
	WINDOW *w;

	if (fw == NULL) return;
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

	if (fw == NULL) return;
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

	if (fw == NULL) return;
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
	if (fw == NULL) return(NULL);
	return(fw->window);
}

/****************************   NAVIGATOR ADT   **********************/

MYNAVIGATOR MyNavigator;

CMDMENU cmdNavMenu[] = { 
   { "E", CmdNavEX,		"E          Examine all settings" },
   { "P", CmdNavPatt,		"P [lrud]*  Show diff pattern"},
   { "D", CmdNavOnDeep,		"D sec      Set delay on deepest"},
   { "S", CmdNavOnPS,		"S sec      Set delay on ps_start"},
   { "N", CmdNavPrNM,		"N [on/off] Print move number info"},
   { "U", CmdNavPrUA,		"U #nodes   Update each XX nodes"},
   { NULL } };

void NavigatorMenu()
/* Allows control over all navigator options */
{
   char   *param;
   char    cmdstr[SZ_CMDSTR+1];
   int     dir;
   COMMAND cmd;

   NavigatorStartNavMenu();
   NavigatorPrintOptions();
   for ( ;; ) {
      cmd=GetCommand("Command",cmdNavMenu,cmdstr);
      switch ( cmd ) {
	case CmdNavEX:
		NavigatorPrintOptions();
		break;
   	case CmdNavPatt :
		param = CmdParam(cmdstr,1);
		while (   param != NULL
		       && param[0] != '\0'
		       && param[0] != ' ') {
			switch (param[0]) {
		  	case 'i': case 'u': 
				dir = NORTH;
				break;
		  	case 'j': case 'l': 
				dir = WEST;
				break;
		  	case 'k': case 'r': 
				dir = EAST;
				break;
		  	case 'd': case 'm': 
				dir = SOUTH;
				break;
		  	default:
				dir = NODIR;
			}
			if (dir != NODIR) {
				NavigatorPSNEWS(IdaInfo->IdaMaze,dir);
			}
			param++;
		}
		break;
	case CmdNavOnDeep:
		MyNavigator.on_deepest = atoi(CmdParam(cmdstr,1));
		break;
	case CmdNavOnPS:
		MyNavigator.on_ps_start = atoi(CmdParam(cmdstr,1));
		break;
	case CmdNavPrNM:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				MyNavigator.pr_nummoves = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				MyNavigator.pr_nummoves = NO;
		}
		else MyNavigator.pr_nummoves = !MyNavigator.pr_nummoves;	
		break;
	case CmdNavPrUA:
		MyNavigator.pr_upd_at = atoi(CmdParam(cmdstr,1));
		if (MyNavigator.pr_upd_at == 0) MyNavigator.pr_upd_at = 10;
		break;
	case CmdQuit:
   		NavigatorEndNavMenu();
   		MyNavigator.enter_menu = NO;
		return;
	default:
		break;
      }
   }
}

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
	MyNavigator.show_pen    = 2;
	MyNavigator.show_index  = 0;

	MyNavigator.enter_menu  = NO;
	MyNavigator.on_deepest  = 0;
	MyNavigator.on_ps_start = 0;
	MyNavigator.pr_nummoves = 0;
	MyNavigator.pr_upd_at   = 100;

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
	NavigatorPSConflict(maze,-1,-1);
	Fwclear(MyNavigator.ps_start);
	Fwclear(MyNavigator.depth);
	Fwclear(MyNavigator.deepest);
	Fwclear(MyNavigator.nav_menu);
	
	NavigatorRefresh();
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

	if (maze == NULL) return;
	p = FindPenalty( maze->conflicts, MyNavigator.show_pen );
	GetPriorPostPen(maze, p->penalty, &prior_pen, &post_pen);
	switch (dir) {
	case NORTH:	if (MyNavigator.show_index > 0) {
				MyNavigator.show_index--;
				NavigatorPSConflict(maze,-1,-1);
			}
			break;
	case SOUTH:	if (MyNavigator.show_index < p->number_conflicts-1) {
				MyNavigator.show_index++;
				NavigatorPSConflict(maze,-1,-1);
			}
			break;
	case  WEST:	if (prior_pen != -1) {
				MyNavigator.show_pen = prior_pen;
				NavigatorPSConflict(maze,-1,-1);
			}
			break;
	case  EAST:	if (post_pen != -1) {
				MyNavigator.show_pen = post_pen;
				NavigatorPSConflict(maze,-1,-1);
			}
			break;
	   default:
	}
}

int  NavigatorPSConflict(MAZE *maze, int penalty, int index)
/* print given conflict into ps_pattern window, if exists return 1,
 * else, return 0 and do not print, but clear */
{
	PENALTY *p;
	int     prior_pen, post_pen;

	if (penalty == -1) penalty = MyNavigator.show_pen;
	else MyNavigator.show_pen = penalty;
	if (index == -1)   index   = MyNavigator.show_index;
	else MyNavigator.show_index = index;
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
		wprintw(MyNavigator.pattern->window,"\n  index:     %4d/%d-%d",
			index+1, p->number_conflicts, p->cflts[index].hits);
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
	NavigatorHandleBreakPoint(MyNavigator.on_ps_start);
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
	NavigatorHandleBreakPoint(MyNavigator.on_deepest);
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

void NavigatorIncNodeCount(int dth)
/* stub called in IncNodeCount */
{
	if (total_node_count%MyNavigator.pr_upd_at == 0)
		NavigatorSetNodeCount(total_node_count, IdaInfo->node_count);
	if (   IdaInfo->nodes_depth[dth] == 1
	    && IdaInfo == &MainIdaInfo) {
		NavigatorSetNodeCount(total_node_count, IdaInfo->node_count);
		NavigatorSetDeepest(IdaInfo->IdaMaze, dth);
		NavigatorSetDepth(CreateStringDepth(MyNavigator.pr_nummoves));
	}
	if (MyNavigator.enter_menu == YES) NavigatorMenu();
}

void NavigatorEndwin()
/* called to end NAVIGATOR seesion */
{
	endwin();
}

void NavigatorStartNavMenu()
/* create and startup the NavMenu */
{
	MyNavigator.nav_menu =
		newfwin(LINES/3, 4*COLS/5,
			LINES/2, COLS/10, "Navigator Control");
	MyNavigator.fdefault = MyNavigator.nav_menu;
	FwrefreshAll(MyNavigator.nav_menu);
}

void NavigatorEndNavMenu()
/* Remove NavMenu */
{
	Fwdelwin(MyNavigator.nav_menu);
	MyNavigator.nav_menu = NULL;
	MyNavigator.fdefault = MyNavigator.general;
	NavigatorRefresh();
}

void NavigatorPrintOptions()
/* print out all Navigator option settings */
{
	Mprintf( 0, "on_Deepest:%d,", MyNavigator.on_deepest );
	Mprintf( 0, " on_pS_start:%d,", MyNavigator.on_ps_start );
	Mprintf( 0, " pr_Nummoves:%c,", MyNavigator.pr_nummoves?'Y':'N' );
	Mprintf( 0, " pr_Upd_at:%d\n", MyNavigator.pr_upd_at );
}

void NavigatorHandleBreakPoint(short todo)
/* todo contains the code for what to do: if -1, break (open Menu), else
 * sleep todo secs */
{
	if (todo < 0) NavigatorMenu();
	else sleep(todo);
}

#endif /* NAVIGATOR */

