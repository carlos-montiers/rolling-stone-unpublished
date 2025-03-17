#include "board.h"

static BitString paths[ MAXSTONES ];

/* find man-reachable squares considering only one stone at stoneloc,
   ignoring all but structure of maze */
void MarkReachOffMaze( MAZE *maze, PHYSID stoneloc, PHYSID manpos,
		       BitString reach )
{
  static PHYSID stack[ENDPATH];
  PHYSID pos;
  int top;

  Set0BS( reach );

  stack[0] = manpos;
  top = 1;
  while( top ) {
    pos = stack[ --top ];

    if( IsBitSetBS( reach, pos) ) continue;
    if( pos == stoneloc ) continue;

    SetBitBS( reach,pos );

    if( IsBitSetBS( maze->M[ 0 ], pos ) )
      stack[ top++ ] = pos + 1;
    if( IsBitSetBS( maze->M[ 1 ], pos ) )
      stack[ top++ ] = pos + YSIZE;
    if( IsBitSetBS( maze->M[ 2 ], pos ) )
      stack[ top++ ] = pos - 1;
    if( IsBitSetBS( maze->M[ 3 ], pos ) )
      stack[ top++ ] = pos - YSIZE;
  }
}

/* move the man optimally from from to to, avoiding stone
   NOTE: assumes this is possible */
void FindOptMan( MAZE *maze, PHYSID from, PHYSID to, PHYSID stone,
		 BitString path )
{
  static PHYSID queue[ ENDPATH ], parent[ XSIZE * YSIZE ];
  int top, bottom, pos, i;
  BitString visit;

  bottom = 0;
  top = 0;
  Set0BS( visit );
  SetBitBS( visit, from );
  parent[ from ] = -1;

  pos = from;
  while( pos != to ) {
    for( i = 0; i < 4; i++ )
      if( pos + DirToDiff[ i ] != stone && 
	  !IsBitSetBS( visit, pos + DirToDiff[ i ] ) &&
	  !IsBitSetBS( maze->wall, pos + DirToDiff[ i ] ) ) {
	SetBitBS( visit, pos + DirToDiff[ i ] );
	parent[ queue[ top++ ] = pos + DirToDiff[ i ] ] = pos;
      }
    pos = queue[ bottom++ ];
  }

  Set0BS( path );
  while( parent[ pos ] != -1 ) {
    SetBitBS( path, pos );
    pos = parent[ pos ];
  }
}

typedef struct {
  PHYSID manfrom;
  PHYSID stonefrom;
  PHYSID stoneto;
  short dist;
} pathmove;

void FindSquarestoGoal( MAZE *maze, int stone, int goal, BitString squares )
{
  static pathmove queue[ ENDPATH ];
  int top, bottom, i, mindist;
  BitString visit[ 4 ], reach;
  PHYSID gpos;

  top = 0;
  bottom = 0;
  for( i = 0; i < 4; i++ )
    Set0BS( visit[ i ] );
  mindist = ENDPATH;
  Set0BS( squares );
  gpos = maze->goals[ goal ].loc;

  MarkReachOffMaze( maze, maze->stones[ stone ].loc, maze->manpos, reach );
  for( i = 0; i < 4; i++ ) {
    if( IsBitSetBS( reach, maze->stones[ stone ].loc - DirToDiff[ i ] ) ) {
      SetBitBS( visit[ i ], maze->stones[ stone ].loc );
      if( IsBitSetBS( maze->S[ i ], maze->stones[ stone ].loc ) ) {
	queue[ top ].manfrom = maze->manpos;
	queue[ top ].stonefrom = maze->stones[ stone ].loc;
	queue[ top ].stoneto = maze->stones[ stone ].loc + DirToDiff[ i ];
	SetBitBS( visit[ i ], maze->stones[ stone ].loc + DirToDiff[ i ] );
	queue[ top++ ].dist = 1;
      }
    }
  }

/* printf( "searching for %i,%i\n", gpos / YSIZE, gpos % YSIZE ); */

  while( bottom < top ) {
/* printf( "%i,%i -> %i,%i (%i)\n",
	queue[ bottom ].stonefrom / YSIZE, queue[ bottom ].stonefrom % YSIZE,
	queue[ bottom ].stoneto / YSIZE, queue[ bottom ].stoneto % YSIZE,
	queue[ bottom ].dist ); */
    if( queue[ bottom ].stoneto == gpos ) {
/* printf( "hit goal\n" ); */
      mindist = queue[ bottom ].dist;
      SetBitBS( squares, gpos );
    }

    MarkReachOffMaze( maze, queue[ bottom ].stoneto,
		      queue[ bottom ].stonefrom, reach );
    for( i = 0; i < 4; i++ ) {
      if( IsBitSetBS( reach, queue[ bottom ].stoneto - DirToDiff[ i ] ) ) {
	SetBitBS( visit[ i ], queue[ bottom ].stoneto );
	if( queue[ bottom ].dist < mindist &&
	    IsBitSetBS( maze->S[ i ], queue[ bottom ].stoneto ) &&
	    !IsBitSetBS( visit[ i ],
			 queue[ bottom ].stoneto + DirToDiff[ i ] ) ) {
	  queue[ top ].manfrom = queue[ bottom ].stonefrom;
	  queue[ top ].stonefrom = queue[ bottom ].stoneto;
	  queue[ top ].stoneto = queue[ bottom ].stoneto + DirToDiff[ i ];
	  SetBitBS( visit[ i ], queue[ bottom ].stoneto + DirToDiff[ i ] );
	  queue[ top++ ].dist = queue[ bottom ].dist + 1;
	}
      }
    }
    bottom++;
  }

  Set0BS( visit[ 0 ] );
  while( --top >= 0 ) {
    if( IsBitSetBS( squares, queue[ top ].stoneto ) ) {
      SetBitBS( squares, queue[ top ].stonefrom );
      FindOptMan( maze, queue[ top ].manfrom,
		  queue[ top ].stonefrom * 2 - queue[ top ].stoneto,
		  queue[ top ].stonefrom, reach );
      BitOrEqBS( visit[ 0 ], reach );
    }
  }
  BitOrEqBS( squares, visit[ 0 ] );
}

void FindSquarestoGoals( MAZE *maze, int stone, BitString squares )
{
  int goal;
  BitString one;

  Set0BS( squares );
  for( goal = 0; goal < maze->number_goals; goal++ ) {
    FindSquarestoGoal( maze, stone, goal, one );
    BitOrEqBS( squares, one );
  }
}

void IgnoreStones( MAZE *maze, BitString ignore )
{
  int s, i;
  char f;

  /* set paths */
  for( s = 0; s < maze->number_stones; s++ )
    FindSquarestoGoals( maze, s, paths[ s ] );

  Set0BS( ignore );
  for( s = 0; s < maze->number_stones; s++ ) {
    f = 1;
    for( i = 0; i < maze->number_stones; i++ )
      if( i != s ) {
	if( IsBitSetBS( paths[ i ], maze->stones[ s ].loc ) ) {
	  f = 0;
	  break;
	}
      }
    if( f )
      SetBitBS( ignore, maze->stones[ s ].loc );
  }
}
