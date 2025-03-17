#include "board.h"

void MarkSidesofStone2( MAZE *maze, PHYSID stone )
{
  static PHYSID stack[ ENDPATH ];
  PHYSID pos, foo[ 4 ];
  int top, dir, count, footop, i, ti;

  Set0BS( maze->reach );

  /* find how many sides of the stone can the man potentially reach */
  count = 0;
  for( dir = 0; dir < 4; dir++ )
    if( IsBitSetBS( maze->M[ dir ], stone ) &&
	maze->PHYSstone[ stone + DirToDiff[ dir ] ] < 0 )
      count++;

  if( !count )
    return;

  stack[ 0 ] = maze->manpos;
  top = 1;
  SetBitBS( maze->reach, maze->manpos );

  /* depth first search (w/ move ordering) */
  while( top ) {
    pos = stack[ --top ];
    SetBitBS( maze->reach, pos );

    /* are we beside the stone? */
    if( stone > pos ) {
      if( stone - pos == 1 ||
	  stone - pos == YSIZE )
	if( !--count )
	  return;
    } else {
      if( stone - pos == -1 ||
	  stone - pos == -YSIZE )
	if( !--count )
	  return;
    }

    /* find possible directions */
    footop = 0;
    if( IsBitSetBS( maze->M[ 0 ], pos ) &&
	!IsBitSetBS( maze->reach, pos + 1 ) &&
	maze->PHYSstone[ pos + 1 ] < 0 ) {
      foo[ footop++ ] = pos + 1;
    }
    if( IsBitSetBS( maze->M[ 1 ], pos ) &&
	!IsBitSetBS( maze->reach, pos + YSIZE ) &&
	maze->PHYSstone[ pos + YSIZE ] < 0 ) {
      foo[ footop++ ] = pos + YSIZE;
    }
    if( IsBitSetBS( maze->M[ 2 ], pos ) &&
	!IsBitSetBS( maze->reach, pos - 1 ) &&
	maze->PHYSstone[ pos - 1 ] < 0 ) {
      foo[ footop++ ] = pos - 1;
    }
    if( IsBitSetBS( maze->M[ 3 ], pos ) &&
	!IsBitSetBS( maze->reach, pos - YSIZE ) &&
	maze->PHYSstone[ pos - YSIZE ] < 0 ) {
      foo[ footop++ ] = pos - YSIZE;
    }

    /* place directions on stack, from farthest to closest */
    switch( footop ) {
    case 4:
      if( ManWeight( maze, foo[ 0 ], stone ) <
	  ManWeight( maze, foo[ 1 ], stone ) )
	if( ManWeight( maze, foo[ 0 ], stone ) >
	    ManWeight( maze, foo[ 2 ], stone ) )
	  if( ManWeight( maze, foo[ 0 ], stone ) >
	      ManWeight( maze, foo[ 3 ], stone ) ) {
	    stack[ top++ ] = foo[ 0 ];
	    foo[ 0 ] = foo[ 3 ];
	  } else {
	    stack[ top++ ] = foo[ 3 ];
	  }
	else if( ManWeight( maze, foo[ 2 ], stone ) >
		 ManWeight( maze, foo[ 3 ], stone ) ) {
	  stack[ top++ ] = foo[ 2 ];
	  foo[ 2 ] = foo[ 3 ];
	}  else {
	  stack[ top++ ] = foo[ 3 ];
	}
      else if( ManWeight( maze, foo[ 1 ], stone ) >
	       ManWeight( maze, foo[ 2 ], stone ) )
	if( ManWeight( maze, foo[ 1 ], stone ) >
	    ManWeight( maze, foo[ 3 ], stone ) ) {
	  stack[ top++ ] = foo[ 1 ];
	  foo[ 1 ] = foo[ 3 ];
	} else {
	  stack[ top++ ] = foo[ 3 ];
	}
      else if( ManWeight( maze, foo[ 2 ], stone ) >
	       ManWeight( maze, foo[ 3 ], stone ) ) {
	stack[ top++ ] = foo[ 2 ];
	foo[ 2 ] = foo[ 3 ]; }
      else {
	stack[ top++ ] = foo[ 3 ];
      }
    case 3:
      if( ManWeight( maze, foo[ 0 ], stone ) <
	  ManWeight( maze, foo[ 1 ], stone ) )
	if( ManWeight( maze, foo[ 1 ], stone ) <
	    ManWeight( maze, foo[ 2 ], stone ) ) {
	  stack[ top++ ] = foo[ 2 ];
	  stack[ top++ ] = foo[ 1 ];
	  stack[ top++ ] = foo[ 0 ];
	} else
	  if( ManWeight( maze, foo[ 0 ], stone ) <
	      ManWeight( maze, foo[ 2 ], stone ) ) {
	    stack[ top++ ] = foo[ 1 ];
	    stack[ top++ ] = foo[ 2 ];
	    stack[ top++ ] = foo[ 0 ];
	  } else {
	    stack[ top++ ] = foo[ 1 ];
	    stack[ top++ ] = foo[ 0 ];
	    stack[ top++ ] = foo[ 2 ];
	  }
      else
	if( ManWeight( maze, foo[ 0 ], stone ) <
	    ManWeight( maze, foo[ 2 ], stone ) ) {
	  stack[ top++ ] = foo[ 2 ];
	  stack[ top++ ] = foo[ 0 ];
	  stack[ top++ ] = foo[ 1 ];
	} else
	  if( ManWeight( maze, foo[ 1 ], stone ) <
	      ManWeight( maze, foo[ 2 ], stone ) ) {
	    stack[ top++ ] = foo[ 0 ];
	    stack[ top++ ] = foo[ 2 ];
	    stack[ top++ ] = foo[ 1 ];
	  } else {
	    stack[ top++ ] = foo[ 0 ];
	    stack[ top++ ] = foo[ 1 ];
	    stack[ top++ ] = foo[ 2 ];
	  }
      break;
    case 2:
      if( ManWeight( maze, foo[ 0 ], stone ) <
	  ManWeight( maze, foo[ 1 ], stone ) ) {
	stack[ top++ ] = foo[ 1 ];
	stack[ top++ ] = foo[ 0 ];
      } else {
	stack[ top++ ] = foo[ 0 ];
	stack[ top++ ] = foo[ 1 ];
      }
      break;
    case 1:
      stack[ top++ ] = foo[ 0 ];
    }
  }
}

void MarkSidesofStone( MAZE *maze, PHYSID stone )
{
  static PHYSID stack[ENDPATH];
  PHYSID pos;
  int next_in, next_out, dir, count;

  Set0BS( maze->reach );

  stack[0] = maze->manpos;
  next_in  = 1;
  next_out = 0;
  SetBitBS( maze->reach, maze->manpos );

  /* how many sides of the stone can the man potentially reach */
  count = 0;
  for( dir = 0; dir < 4; dir++ )
    if( IsBitSetBS( maze->M[ dir ], stone ) &&
	maze->PHYSstone[ stone + DirToDiff[ dir ] ] < 0 )
      count++;

  if( !count )
    return;

  while( next_out < next_in ) {
    pos = stack[ next_out++ ];

    if( stone > pos ) {
      if( stone - pos == 1 ||
	  stone - pos == YSIZE )
	if( !--count )
	  return;
    } else {
      if( stone - pos == -1 ||
	  stone - pos == -YSIZE )
	if( !--count )
	  return;
    }

    if( IsBitSetBS( maze->M[ 0 ], pos ) &&
	!IsBitSetBS( maze->reach, pos + 1 ) &&
	maze->PHYSstone[ pos + 1 ] < 0 ) {
      stack[ next_in++ ] = pos + 1;
      SetBitBS( maze->reach, pos + 1 );
    }
    if( IsBitSetBS( maze->M[ 1 ], pos ) &&
	!IsBitSetBS( maze->reach, pos + YSIZE ) &&
	maze->PHYSstone[ pos + YSIZE ] < 0 ) {
      stack[ next_in++ ] = pos + YSIZE;
      SetBitBS( maze->reach, pos + YSIZE );
    }
    if( IsBitSetBS( maze->M[ 2 ], pos ) &&
	!IsBitSetBS( maze->reach, pos - 1 ) &&
	maze->PHYSstone[ pos - 1 ] < 0 ) {
      stack[ next_in++ ] = pos - 1;
      SetBitBS( maze->reach, pos - 1);
    }
    if( IsBitSetBS( maze->M[ 3 ], pos ) &&
	!IsBitSetBS( maze->reach, pos - YSIZE ) &&
	maze->PHYSstone[ pos - YSIZE ] < 0 ) {
      stack[ next_in++ ] = pos - YSIZE;
      SetBitBS( maze->reach, pos - YSIZE );
    }
  }
}

void MarkReachOneStone( MAZE *maze, int stone, BitString reach )
{
  static PHYSID stack[ENDPATH];
  PHYSID pos;
  int top;

  Set0BS( reach );

  stack[0] = maze->manpos;
  top = 1;
  while( top ) {
    pos = stack[ --top ];
    if( IsBitSetBS( reach, pos) ) continue;
    if( maze->PHYSstone[ pos ] == stone ) continue;

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

void FindPotStoneReach( MAZE *maze, int stone, BitString reach )
{
  BitString dirs[ 4 ], manpotreach;
  int i, top;
  char dir, foodir;
  PHYSID pos, oldstone, oldman;
  static PHYSID stack[ XSIZE * YSIZE ];
  static char manstack[ XSIZE * YSIZE ];

  Set0BS( dirs[ 0 ] );
  Set0BS( dirs[ 1 ] );
  Set0BS( dirs[ 2 ] );
  Set0BS( dirs[ 3 ] );

  MarkReachOneStone( maze, stone, manpotreach );

  oldstone = maze->stones[ stone ].loc;
  oldman = maze->manpos;
  top = 0;

  for( foodir = 0; foodir < 4; foodir++ ) {
    if( IsBitSetBS( manpotreach, oldstone - DirToDiff[ foodir ] ) &&
	!IsBitSetBS( dirs[ foodir ], oldstone ) )
      maze->manpos = oldstone - DirToDiff[ foodir ];
    else
      continue;

    MarkReachNoUnreach( maze );
  
    maze->PHYSstone[ oldstone ] = -1;
    for( i = 0; i < 4; i++ ) {
      if( IsBitSetBS( maze->reach, oldstone - DirToDiff[ i ] ) &&
	  maze->PHYSstone[ oldstone + DirToDiff[ i ] ] < 0 &&
	  IsBitSetBS( maze->S[ i ], oldstone ) ) {
	SetBitBS( dirs[ i ], oldstone );
	stack[ top ] = oldstone + DirToDiff[ i ];
	manstack[ top++ ] = i;
      }
    }

    while( top ) {
      dir = manstack[ --top ];
      pos = stack[ top ];
      SetBitBS( dirs[ dir ], pos );
      maze->PHYSstone[ pos ] = stone;
      maze->manpos = pos - DirToDiff[ dir ];
      MarkReachNoUnreach( maze );

      for( i = 0; i < 4; i++ )
	if( IsBitSetBS( maze->reach, pos - DirToDiff[ i ] ) &&
	    maze->PHYSstone[ pos + DirToDiff[ i ] ] < 0 &&
	    !IsBitSetBS( dirs[ i ], pos + DirToDiff[ i ] ) &&
	    IsBitSetBS( maze->S[ i ], pos ) ) {
	  stack[ top ] = pos + DirToDiff[ i ];
	  manstack[ top++ ] = i;
	}

      maze->PHYSstone[ pos ] = -1;
    }
  }

  BitOrBS( reach, dirs[ 0 ], dirs[ 1 ] );
  BitOrEqBS( reach, dirs[ 2 ] );
  BitOrEqBS( reach, dirs[ 3 ] );
  SetBitBS( reach, oldstone );

  maze->PHYSstone[ oldstone ] = stone;
  maze->manpos = oldman;
  MarkReachNoUnreach( maze );
}

void FindStoneReach( MAZE *maze, int stone, BitString reach )
{
  BitString dirs[ 4 ];
  int i, top;
  char dir;
  PHYSID pos, oldstone, oldman;
  static PHYSID stack[ XSIZE * YSIZE ];
  static char manstack[ XSIZE * YSIZE ];

  Set0BS( dirs[ 0 ] );
  Set0BS( dirs[ 1 ] );
  Set0BS( dirs[ 2 ] );
  Set0BS( dirs[ 3 ] );

  oldstone = maze->stones[ stone ].loc;
  oldman = maze->manpos;
  top = 0;

  MarkSidesofStone( maze, oldstone );
  
  maze->PHYSstone[ oldstone ] = -1;
  for( i = 0; i < 4; i++ ) {
    if( IsBitSetBS( maze->reach, oldstone - DirToDiff[ i ] ) &&
	maze->PHYSstone[ oldstone + DirToDiff[ i ] ] < 0 &&
	IsBitSetBS( maze->S[ i ], oldstone ) ) {
      SetBitBS( dirs[ i ], oldstone );
      stack[ top ] = oldstone + DirToDiff[ i ];
      manstack[ top++ ] = i;
      SetBitBS( dirs[ i ], oldstone + DirToDiff[ i ] );
    }
  }

  while( top ) {
    dir = manstack[ --top ];
    pos = stack[ top ];
    maze->PHYSstone[ pos ] = stone;
    maze->manpos = pos - DirToDiff[ dir ];
    MarkSidesofStone( maze, pos );

    for( i = 0; i < 4; i++ )
      if( IsBitSetBS( maze->reach, pos - DirToDiff[ i ] ) &&
	  maze->PHYSstone[ pos + DirToDiff[ i ] ] < 0 &&
	  !IsBitSetBS( dirs[ i ], pos + DirToDiff[ i ] ) &&
	  IsBitSetBS( maze->S[ i ], pos ) ) {
	stack[ top ] = pos + DirToDiff[ i ];
	manstack[ top++ ] = i;
	SetBitBS( dirs[ i ], pos + DirToDiff[ i ] );
      }

    maze->PHYSstone[ pos ] = -1;
  }

  BitOrBS( reach, dirs[ 0 ], dirs[ 1 ] );
  BitOrEqBS( reach, dirs[ 2 ] );
  BitOrEqBS( reach, dirs[ 3 ] );
  SetBitBS( reach, oldstone );

  maze->PHYSstone[ oldstone ] = stone;
  maze->manpos = oldman;
  MarkReachNoUnreach( maze );
}

/* find stonereach for all stones, using potential stone reach for stone */
void SetStoneReach( MAZE *maze ) {
  int i;

  for( i = 0; i < maze->number_stones; i++ )
    FindStoneReach( maze, i, maze->stone_reach[ i ] );
}

int PotStoneReachChanged( MAZE *maze, UNMOVE *unmove )
{
  BitString oldpot, newpot;
  PHYSID oldmanpos;
  int stone;

  oldmanpos = maze->manpos;
  maze->manpos = unmove->manfrom;
  stone = maze->PHYSstone[ unmove->stoneto ];
  maze->stones[ stone ].loc = unmove->stonefrom;
  maze->PHYSstone[ unmove->stonefrom ] = stone;
  maze->PHYSstone[ unmove->stoneto ] = -1;
  FindPotStoneReach( maze, maze->PHYSstone[ unmove->stonefrom ], oldpot );
  maze->manpos = oldmanpos;
  maze->stones[ stone ].loc = unmove->stoneto;
  maze->PHYSstone[ unmove->stoneto ] = stone;
  maze->PHYSstone[ unmove->stonefrom ] = -1;
  FindPotStoneReach( maze, maze->PHYSstone[ unmove->stoneto ], newpot );
  return StoneReachChanged( oldpot, newpot,
			    unmove->stonefrom, unmove->stoneto );
}

int StoneReachChanged( BitString old, BitString new, PHYSID from, PHYSID to )
{
  int i, f, t, r;

  /* save from/to bits */
  f = t = -1;
  if( IsBitSetBS( new, from ) ) {
    if( !IsBitSetBS( old, from ) ) {
      f = 0;
      SetBitBS( old, from );
    }
  } else {
    if( IsBitSetBS( old, from ) ) {
      f = 1;
      UnsetBitBS( old, from );
    }
  }
  if( IsBitSetBS( new, to ) ) {
    if( !IsBitSetBS( old, to ) ) {
      t = 0;
      SetBitBS( old, to );
    }
  } else {
    if( IsBitSetBS( old, to ) ) {
      t = 1;
      UnsetBitBS( old, to );
    }
  }

  r = 0;
  /* check equality */
  for( i = 0; i < NUMBERINTS; i++ ) {
    if( old[ i ] != new[ i ] ) {
      r = 1;
      break;
    }
  }

  /* restore from/to bits */
  if( f == 1 )
    SetBitBS( old, from );
  else if( f == 0 )
    UnsetBitBS( old, from );
  if( t == 1 )
    SetBitBS( old, to );
  else if( t == 0 )
    UnsetBitBS( old, from );

  return r;
}
