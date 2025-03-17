#include "board.h"
#include <curses.h>

typedef struct {
	WINDOW *current;
	WINDOW *conlfict;
	WINDOW *nodecount;
	WINDOW *treepos;
	
	WINDOW *general;
	WINDOW *command;

	IDA *idainfo;
} My_screen;

My_screen ThisScreen;

DisplayRegisterIdaInfo(IDA *idainfo)
{
	ThisScreen.idainfo = idainfo;
}

DisplayInit()
{
	initscr();
	cbreak();
	noecho();
	nonl();
	scrollok(stdscr,FALSE);
	intrflush(stdscr,FALSE);
	keypad(stdscr,TRUE);

	ThisScreen.current   = newwin(YSIZE,XSIZE,0,0);
	ThisScreen.conflict  = newwin(YSIZE,XSIZE,0,XSIZE);
	ThisScreen.nodecount = newwin(1,XSIZE,0,0);
	ThisScreen.treepos   = newwin(1,XSIZE,0,XSIZE);

	ThisScreen.general   = newwin(LINES-YSIZE-1-4,COLS,YSIZE+1,0);
	ThisScreen.command   = newwin(4,COLS,COLS-4,0);
}

DisplayUpdate()
{
	DisplayCurrent();
	DisplayConflict();
	DisplayNodeCount();
	DisplayTreePos();

	wrefresh(stdscr);
}

DisplayEnd()
{
	endwin();
}


/*
	wmove(stdscr,i,0);
	waddstr(stdscr,buff);
	if (i<LINES) wclrtobot(stdscr);
	wmove(stdscr,LineOnScreen,0);
	wrefresh(stdscr);
*/
