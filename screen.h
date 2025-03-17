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
	
} MYNAVIGATOR;

#ifdef NAVIGATOR

FWINDOW *newfwin(int ysize, int xsize, int ystart, int xstart, char *title);
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

WINDOW *GetDefaultWindow(FWINDOW *fw);

extern MYNAVIGATOR MyNavigator;

void NavigatorInit();
void NavigatorRefresh();
void NavigatorSetMaxSize(int max_size);
void NavigatorSetStartMaze(MAZE *maze, char *title);
void NavigatorSetPattern(MAZE *maze, BitString bs1, BitString bs2);
void NavigatorSetPSMaze(MAZE *maze, BitString bs1, BitString bs2);
void NavigatorUnsetPSMaze();
int  NavigatorPSConflict(MAZE *maze);
void NavigatorPSNEWS(MAZE *maze, int dir);
void NavigatorSetDeepest(MAZE *maze, int depth);
void NavigatorSetNodeCount(int32_t total_node_count, int32_t node_count);
void NavigatorSetDepth(char *msg);

	/***************************************************************/
#else

#define newfwin(ysize, xsize, ystart, xstart, title)
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
#define NavigatorPSConflict(maze)
#define NavigatorPSNEWS(maze, dir)
#define NavigatorSetDeepest(maze, depth)
#define NavigatorSetNodeCount(total_node_count, node_count)
#define NavigatorSetDepth(msg)

#endif
