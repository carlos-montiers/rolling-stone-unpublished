typedef struct {
	WINDOW *fwindow;	/* frame for window */
	WINDOW *window;		/* real window */
	char    title[100];
	int     xsize, ysize;
} FWINDOW;			/* framed window */



typedef struct {

	int	 maze_ysize;	/* Max_y + 2, to save space */
	int      show_pen,
		 show_index;    /* current penalty coordinates */

	FWINDOW *fdefault;	/* window current Mprintf will print
				 * to */
	FWINDOW *start;		/* start   fwindow */
	FWINDOW *pattern;	/* pattern fwindow */
	FWINDOW *ps_start;	/* patter search start fwindow */
	FWINDOW *general;	/* general fwindow */
	FWINDOW *depth;		/* depth count fwindow */
	FWINDOW *deepest;	/* deepest maze fwindow */

	FWINDOW *nav_menu;	/* navigator menu fwindow - dynamic */

	/* Options section, break points, print out */

	short	 enter_menu;	/* set by SIGINT handler to start menu */

	short	 on_deepest;	/* What to do on updating deepest */
	short	 on_ps_start;	/* What to do on updating ps_start */

	short	 pr_nummoves;	/* Print number_moves */
	short    pr_upd_at;	/* update node counts each XX nodes */
	
} MYNAVIGATOR;

#ifdef NAVIGATOR

FWINDOW *newfwin(int ysize, int xsize, int ystart, int xstart, char *title);
void     Fwdelwin( FWINDOW *fw );
void     Fwclear( FWINDOW *fw );
void     FWActiveFrame( FWINDOW *fw );
void     FWDeActiveFrame( FWINDOW *fw );
void     FwSetTitle(FWINDOW *fw, char *title);
void     Fwresize(FWINDOW *fw, int xsize, int ysize);
void     Fwmove(FWINDOW *fw, int xoffs, int yoffs);
void     Fwaddstr(FWINDOW *fw, char *msg);
void     Fwrefresh(FWINDOW *fw);
void     FwPrintMaze(FWINDOW *fw, MAZE *maze);
void     FwPrintBitMaze(FWINDOW *fw, BitString bits);
void     FwPrintBit2Maze(FWINDOW *fw, BitString bits1,
				BitString bits2, PHYSID manpos);
void     FwSetStatus1(FWINDOW *fw, char *msg);
void     FwSetStatus2(FWINDOW *fw, char *msg);

WINDOW *GetDefaultWindow(FWINDOW *fw);

extern MYNAVIGATOR MyNavigator;

void NavigatorInit();
void NavigatorRefresh();
void NavigatorSetMaxSize(int max_size);
void NavigatorSetStartMaze(MAZE *maze, char *title);
void NavigatorSetPattern(MAZE *maze, BitString bs1, BitString bs2);
void NavigatorSetPSMaze(MAZE *maze, BitString bs1, BitString bs2);
void NavigatorUnsetPSMaze();
int  NavigatorPSConflict(MAZE *maze, int penalty, int index);
void NavigatorPSNEWS(MAZE *maze, int dir);
void NavigatorSetDeepest(MAZE *maze, int depth);
void NavigatorSetNodeCount(int32_t total_node_count, int32_t node_count);
void NavigatorSetDepth(char *msg);
void NavigatorIncNodeCount(int dth);
void NavigatorEndwin();
void NavigatorStartNavMenu();
void NavigatorEndNavMenu();
void NavigatorMenu();
void NavigatorPrintOptions();
void NavigatorHandleBreakPoint(short todo);

	/***************************************************************/
#else

#define newfwin(ysize, xsize, ystart, xstart, title)
#define Fwdelwin( fw );
#define Fwclear( fw )
#define FWActiveFrame( fw )
#define FWDeActiveFrame( fw )
#define FwSetTitle(fw, title)
#define Fwresize(fw, xsize, ysize)
#define Fwmove(fw, xoffs, yoffs)
#define Fwaddstr(fw, msg)
#define Fwrefresh(fw)
#define FwPrintMaze(fw, maze)
#define FwPrintBitMaze(fw, bits)
#define FwPrintBit2Maze(fw, bits1, bits2, manpos)

#define GetDefaultWindow(fw)

#define NavigatorInit()
#define NavigatorRefresh()
#define NavigatorSetMaxSize(max_size)
#define NavigatorSetStartMaze(maze, title)	PrintMaze(maze)
#define NavigatorSetPattern(maze, bs1, bs2)
#define NavigatorSetPSMaze(maze, bs1, bs2)
#define NavigatorUnsetPSMaze()
#define NavigatorPSConflict(maze, pen, index)
#define NavigatorPSNEWS(maze, dir)
#define NavigatorSetDeepest(maze, depth)
#define NavigatorSetNodeCount(total_node_count, node_count)
#define NavigatorSetDepth(msg)
#define NavigatorIncNodeCount(dth)
#define NavigatorEndwin()
#define NavigatorStartNavMenu()
#define NavigatorEndNavMenu()
#define NavigatorMenu()
#define NavigatorPrintOptions()
#define NavigatorHandleBreakPoint(todo)
#define FwSetStatus1(fw, msg)
#define FwSetStatus2(fw, msg)

#endif
