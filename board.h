#include <signal.h>
#include <time.h>
#include <malloc.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <inttypes.h>

#define PATTERNSEARCH 10000
#define PATTERNSTONES 12
extern int PATTERNRATIO;
#define RANDOM_MAX 2147483647

#define PENPATTERNSEARCHPP 0
#define DEADPATTERNSEARCHPP 0

#ifndef TRUE
  #define TRUE 		1
  #define FALSE 	0
#endif
#define YES  		1
#define NO    		0
#define MAX(x,y)	((x)>(y)?(x):(y))
#define MIN(x,y)	((x)<(y)?(x):(y))
#define INI		0x7f
#define MAXWEIGHT	0x7f7f
#define MAXGOALS	100
#define MAXSTONES	100
#define MAX_MOVES 	120
#define MAX_DEPTH 	700
#define MAX_PENHIST	41
#define MAX_HASHENTRIES 4*8*32768
#define HASHMASK        (MAX_HASHENTRIES -1)
#define GMMAX_HASHENTRIES 8192
#define GMHASHMASK      (GMMAX_HASHENTRIES -1)
#define MAXDIFFICULTY	0x1fff /* This is sort of a sane upper bound */
#define DONEDIFFICULTY	0x7fff /* This is to indicate that we tried before */
#define MAX_SQUARES     25
#define MAX_GROOMS      10
#define MAX_LOCATIONS   30
#define XY2ID(x,y)	(x*YSIZE+y)
#define ENDPATH		4095
#ifdef SMALL
/* XSIZE and YSIZE have to be different!!!! */
   #define XSIZE	23
   #define YSIZE	17
#else
   #define XSIZE 	25
   #define YSIZE 	20
#endif
#define BASETYPE	uint32_t
#define PRINTBASETYPE(a) Mprintf( 0, "%08" PRIx32,a);

#define NUMBERBITS 	XSIZE*YSIZE
#define BYTEPERINT 	sizeof(BASETYPE)
#define NUMBERINTS 	(NUMBERBITS/(BYTEPERINT*8)+\
				((NUMBERBITS%(BYTEPERINT*8))?1:0))

#ifdef PC
   #define DL1PATHFILE "screens/DL.1.pc"
   #define DL2PATHFILE "screens/DL.2.pc"
#else
   #define DL1PATHFILE "screens/DL.1"
   #define DL2PATHFILE "screens/DL.2"
#endif


#define min(a,b)	(((a)<(b))?a:b)
#define max(a,b)	(((a)>(b))?a:b)

#define EMPTY 	0	/* Nothing on square */
#define NONE 	0	/*  */
#define NODIR 	-1	/* No direction */
#define NORTH	0	/* Possible to go there? */
#define EAST	1	/* Possible to go there? */
#define SOUTH	2	/* Possible to go there? */
#define WEST	3	/* Possible to go there? */

#define CONFLICT_INC 10

typedef uint16_t USHORT;
typedef  int16_t PHYSID;
typedef uint16_t WEIGHT;
typedef uint64_t HASHKEY;

typedef struct {
	int stoneidx;	/* referenced by goalidx */
	int distance;	/* referenced by stoneidx */
	int goalidx;	/* referenced by stoneidx */
} LBENTRY;

typedef BASETYPE BitString[NUMBERINTS];

typedef struct {		/* defines one low level square */
	USHORT  tunnel:6;
	USHORT  min_dim:6;
	USHORT  free:3;
	USHORT  s_tunnel:6;
	USHORT  s_min_dim:6;
	USHORT  s_free:3;
	signed short  lstruct;
	signed char   goal;	/* stone idx into stones table */
} PHYS;

typedef struct {
	PHYSID loc;
} STN;

typedef struct {
	PHYSID from;
	PHYSID to;
	PHYSID last_over;
	short  macro_id;
	WEIGHT move_dist;
	short  value;
} MOVE;

extern MOVE DummyMove;
#define EQMOVE(m1,m2)	((m1).from==(m2).from&&(m1).to==(m2).to)
#define ISDUMMYMOVE(m)	((m).from==ENDPATH&&(m).to==ENDPATH)

typedef struct {
	PHYSID manfrom;
	PHYSID stonefrom;
	PHYSID stoneto;
	short  macro_id;
	WEIGHT move_dist;
	PHYSID old_closest_confl;
	int    old_stoneid;
	BitString old_no_reach;
  BitString save_no_reach;
  BitString save_reach;
#ifdef STONEREACH
  BitString stone_reach[ MAXSTONES ];
#endif
#ifdef COPY_LB_TABLE
        LBENTRY    old_lbentries[MAXSTONES];
	int	   old_h;
#endif
	struct UGLY_GMNODE  *old_GMTree;
} UNMOVE;

typedef struct {
	HASHKEY lock;
	struct UGLY_GMNODE *gmnode;
} GMHASHENTRY;

typedef struct {
	HASHKEY lock;
	PHYSID  man;
	PHYSID  goal_sqto;
	MOVE  bestmove;
	short g;        /* how deep int the tree */
	short down;	/* how deeply was it searched */
	short min_h;	/* minimum h seen in this subtree */
	short gen;	/* This is the generation -> meaning the search
			   something was searched in. Meaning, every
			   search might be searching a different maze,
			   to not get wrong entries (especially with
			   the Reversable searches) */
	unsigned short pensearched:1;	/* penalty search done already */
	unsigned short dlsearched:1;	/* deadlock search done already */
	unsigned short backward:1;	/* this is a backward search entry */
	unsigned short pathflag:1;	/* this is a flag indicating part
					   of current path (== cycle)*/
	int32_t tree_size;
/* TT GHI fix */
/* int lastnonlmove; */
} HASHENTRY;

typedef struct {
	PHYSID loc;
	WEIGHT weight;
	int    tried;
} GOL;

typedef struct {
	PHYSID from;
	PHYSID to;
	PHYSID last_over;
} MACRO;

typedef struct {
	int    type;
	short  number_macros;
	MACRO  *macros;
} MACROS;

typedef struct {
	PHYSID fw;
	PHYSID bw;
} TOGOAL;

typedef struct {
	short  n;
	short  maninout;
	short  index;
	short  stone_inside;
	short  number_goals;
	short  number_squares;
	short  deadentrances;
	HASHKEY hashkey;
	PHYSID locations[MAX_LOCATIONS];
	PHYSID entrances[MAX_LOCATIONS];
	BitString goals;
	BitString squares;
} GROOM;

typedef struct UGLY_GMNODE {
	short    references;
	short    number_entries;
	short	 min_dist;
	HASHKEY  hashkey;
	struct UGLY_GMENTRY *entries;
} GMNODE;

typedef struct UGLY_GMENTRY {
	short    goal;
	short    entrance;
	WEIGHT   distance;
	PHYSID   last_over;
	PHYSID   goal_loc;
	PHYSID   entrance_loc;
	GMNODE  *next;
} GMENTRY;

typedef struct {
	USHORT   room:1;
	USHORT   hallway:1;
	USHORT   intersection:1;
	USHORT   manonly:1;
	USHORT   deadend:1;
	USHORT   exit:1;

	short    number_stones;
	short    max_number_stones;
	short    number_goals;
	short    number_squares;
	short    number_s_squares;
	short    number_doors;
	short    number_s_doors;
} STRUCT;

typedef struct {
	BitString conflict;
	BitString no_reach;
	int       hits;
  PHYSID onestone;
  char fromdb;
} CFLT;

typedef struct {
	int   number_conflicts;
	int   array_size;
	int   penalty;
	CFLT *cflts;
} PENALTY;

typedef struct {
	PHYSID manpos;
	PHYSID stonepos;
	BitString relevant;
	BitString stones;
} TESTED;

typedef struct {
	int	  number_patterns;
	int	  number_penalties;
	int 	  array_size_pen;
	PENALTY  *pen;

	int	  number_deadtested;
	int	  array_size_deadtested;
	TESTED   *deadtested;

	int	  number_pentested;
	int	  array_size_pentested;
	TESTED   *pentested;

	int32_t   penalty_hist[MAX_PENHIST];
	int32_t   penalty_depth[MAX_DEPTH];
} CONFLICTS;

typedef struct {
	int links;
	WEIGHT w[XSIZE*YSIZE];
} WEIGHTS;

typedef struct {
	BitString    wall;
	BitString    dead;
	BitString    backdead;
	BitString    M[4];		/* Can the man go there? */
	BitString    S[4];		/* Can the stone move there? */
	BitString    out;
	BitString    m_visited;
	BitString    s_visited;
	BitString    one_way;	
	BitString    stone;	
	BitString    goal;	
	BitString    reach;	
	BitString    no_reach;	
	BitString    old_no_reach;	
	BitString    stones_done;	/* stones in goal area for good */
	BitString    eqsq;
	PHYS 	     Phys[XSIZE*YSIZE];
	WEIGHTS     *s_weights[4][XSIZE*YSIZE];
	WEIGHTS     *bg_weights[4][XSIZE*YSIZE];
	WEIGHTS     *b_weights[4][XSIZE*YSIZE];
	WEIGHTS     *m_weights[XSIZE*YSIZE];
	WEIGHTS     *d_weights[XSIZE*YSIZE];
	LBENTRY     *lbtable;
	GMNODE     **gmtrees;
	short        groom_index[XSIZE*YSIZE];
	MACROS       macros[XSIZE*YSIZE];
	float	     avg_influence_to[XSIZE*YSIZE];
	float	     avg_influence_from[XSIZE*YSIZE];
#ifdef NEEDED
	BitString    needed[XSIZE*YSIZE];
#endif
	short        number_grooms;
	short        number_structs;
	short        number_goals;
	short	     number_stones;
	GROOM       *grooms;
	STRUCT      *structs;
	STN         *stones;
	GOL         *goals;
	PHYSID       manpos;
	PHYSID       goal_manpos;
	int          h;
	int          pen;
	int	     g;
	HASHKEY      hashkey;
	CONFLICTS   *conflicts;
	/* For the goal-push cutoff, if we could push a stone to a goal
	   using a macro, we cut all alternatives to pushing that stone */
	PHYSID	     goal_sqto;
	int	     currentmovenumber;	/* basically index into IDAARRAY */

	/* stone idx into stones table */
	signed char PHYSstone[ XSIZE * YSIZE ];
BitString stone_reach[ MAXSTONES ]; /* only number_stones used */
  int influence[ XSIZE * YSIZE ][ XSIZE * YSIZE ];
} MAZE;

typedef struct {
	MOVE   moves[MAX_MOVES];
	MOVE   solution;
	MOVE   currentmove;
	UNMOVE unmove;
	int    number_moves;
	int    currentindex;
	short  distant;
} IDAARRAY;

typedef struct {
	MAZE      *IdaMaze;
	BitString  IdaManSquares;
	BitString  IdaStoneSquares;
	int        Threshold;
	int        ThresholdInc;
	IDAARRAY   IdaArray[MAX_DEPTH];
	int32_t    AbortNodeCount;
	int        ForwDepthLimit;	/* primarily used for   */
	int	   BackDepthLimit;	/* bidirectional search */
	short      CurrentHashGen;
	int32_t    last_tree_size;
	int        base_indent;
	int	   MiniFlag;		/* set to YES in PenMiniConflict */

	int        CurrentSolutionDepth;

	/* functions and datastructures to call/use accoring to what-search */
	HASHENTRY *HashTable;
	int 	  (*MoveOrdering) ();

	/* RevSearch: To make sure man is at right location after putting
	   all stones back we need this variable */
	PHYSID     reach_at_goal;

	/* DeadSearch: "last_move".to location and shortest conflict location */
	PHYSID       goal_last_to;
	PHYSID       closest_confl;
	BitString    shadow_stones;
	BitString    no_reach;

	/* stats stuff */
	int32_t    r_tree_size; /* real number nodes searched this iteration */
	int32_t    v_tree_size; /* virtual (including nodes saved by trnspsns */
	int32_t    node_count;	/* number nodes search during this ida */
				/* total_node_count is only reset when a
				 * normal search is setup, therefore
				 * contains ALL nodes searched (real) */
	int32_t    nodes_depth[MAX_DEPTH];
	int32_t    no_lcut_nodes[MAX_DEPTH];
	int32_t    no_lcut_moves[MAX_DEPTH];
	int32_t    no_lcut_h[MAX_DEPTH];
	int32_t    no_lcut_g[MAX_DEPTH];
	int32_t    lcut_nodes[MAX_DEPTH];
	int32_t    lcut_moves[MAX_DEPTH];
	int32_t    lcut_allmoves[MAX_DEPTH];
	int32_t    lcut_h[MAX_DEPTH];
	int32_t    lcut_g[MAX_DEPTH];
	int32_t    both_nodes[MAX_DEPTH];
	int32_t    both_moves[MAX_DEPTH];
	int32_t    both_h[MAX_DEPTH];
	int32_t    both_g[MAX_DEPTH];
	time_t     start_time;
#ifdef STONEREACH
  int32_t reach_cuts;
#endif
	
	int32_t    tt_hits;
	int32_t    tt_cols;
	int32_t    tt_reqs;
	int32_t    gmtt_hits;
	int32_t    gmtt_cols;
	int32_t    gmtt_reqs;

	int 	   PrintPriority;
#ifdef STONEREACH
  int neilflag;
  int depth;
#endif
} IDA;

/************ exports ******************/


#include "deadlock.h"
#include "debug.h"
#include "hashtable.h"
#include "gmhashtable.h"
#include "ida.h"
#include "init.h"
#include "io.h"
#include "lowerbound.h"
#include "mark.h"
#include "moves.h"
#include "mymem.h"
#include "stats.h"
#include "weights.h"
#include "dl.h"
#include "tree.h"
#include "menu.h"
#include "macro.h"
#include "gtv.h"
#include "bitstring.h"
#include "conflicts.h"
#include "revsearch.h"
#include "deadsearch.h"
#include "pensearch.h"
#include "safesquare.h"
#include "backsearch.h"
#include "bisearch.h"
#include "realtime.h"
#include "histogram.h"
#include "navigator.h"
#include "confdb.h"
#include "stonereach.h"
#include "priority.h"

int StartRandom();

extern int  PP;
extern int32_t dl_pos_nc, dl_neg_nc;	/* node counts for pos/neg searches */
extern int  dl_pos_sc, dl_neg_sc;	/* search count for pos/neg */
extern int32_t pen_pos_nc, pen_neg_nc;	/* node counts for pos/neg searches */
extern int  pen_pos_sc, pen_neg_sc;	/* search count for pos/neg */
