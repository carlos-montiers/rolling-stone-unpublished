#include "board.h"
#include <sys/file.h>

dbentry *_db;
int _dbsize = 0;
int _dbused = 0;
PENALTY _dbpatterns = { 0, 0, 0, 0 };

void PrintBit3MazeRegion( BitString wall, BitString marks, BitString mark2,
			  int xmin, int ymin, int xmax, int ymax ) {
  int x, y, pos;
  char buff[ XSIZE * 2 + 2 ];

  for( y = ymax; y >= ymin; y-- ) {
    buff[ 0 ] = '\0';
    for( x = xmin; x <= xmax; x++ ) {
      pos = XY2ID( x, y );
      if( IsBitSetBS( wall, pos) ) strcat( buff, "#" );
      else if( IsBitSetBS( marks, pos) ) strcat( buff, "*" );
      else if( IsBitSetBS( mark2, pos) ) strcat( buff, "+" );
      else strcat( buff, " " );
    }
    Mprintf( 0, "%s\n", buff );
  }
}

void PrintBit2MazeRegion( BitString wall, BitString marks,
			  int xsize, int ysize ) {
  int x, y, pos;
  char buff[ XSIZE * 2 + 2 ];

  Mprintf( 0, "." );
  for( x = 0; x < xsize; x++ )
    Mprintf( 0, "." );
  Mprintf( 0, ".\n" );
  for( y = ysize - 1; y >= 0; y-- ) {
    buff[ 0 ] = 0;
    for( x = 0; x < xsize; x++ ) {
      pos = XY2ID( x, y );
      if( IsBitSetBS( wall, pos) ) strcat( buff, "#" );
      else if( IsBitSetBS( marks, pos) ) strcat( buff, "*" );
      else strcat( buff, " " );
    }
    Mprintf( 0, ".%s.\n", buff );
  }
  Mprintf( 0, "." );
  for( x = 0; x < xsize; x++ )
    Mprintf( 0, "." );
  Mprintf( 0, ".\n" );
}

void PrintRegion( dbentry *region )
{
  int x, y, p;
  char buff[ 16 ];
  unsigned char w, s;

  Mprintf( 0, "." );
  for( x = 0; x < region->xsize; x++ )
    Mprintf( 0, "." );
  Mprintf( 0, ".\n" );
  for( y = region->ysize - 1; y >= 0; y-- ) {
    p = 0;
    w = region->walls[ y ];
    s = region->stones[ y ];
    for( x = region->xsize - 1; x >= 0; x-- ) {
      if( w & 1 )
	buff[ p ] = '#';
      else if( s & 1 )
	buff[ p ] = '*';
      else
	buff[ p ] = ' ';
      p++;
      w = w >> 1;
      s = s >> 1;
    }
    buff[ p ] = 0;
    Mprintf( 0, ".%s.\n", buff );
  }
  Mprintf( 0, "." );
  for( x = 0; x < region->xsize; x++ )
    Mprintf( 0, "." );
  Mprintf( 0, ".\n" );
}

void CullWalls( BitString wall, BitString stones )
{
  PHYSID stack[ XSIZE * YSIZE ], pos;
  int top, i;
  BitString t;

  top = 0;
  Set0BS( t );
  BitOrEqBS( wall, stones );
  for( i = 0; i < XSIZE * YSIZE; i++ )
    if( IsBitSetBS( stones, i ) )
      stack[ top++ ] = i;

  while( top ) {
    pos = stack[ --top ];
    SetBitBS( t, pos );

    pos += 1;
    if( pos > 0 && pos < XSIZE * YSIZE &&
	IsBitSetBS( wall, pos ) && !IsBitSetBS( t, pos ) )
      stack[ top++ ] = pos;
    pos += YSIZE;
    if( pos > 0 && pos < XSIZE * YSIZE &&
	IsBitSetBS( wall, pos ) && !IsBitSetBS( t, pos ) )
      stack[ top++ ] = pos;
    pos -= 1;
    if( pos > 0 && pos < XSIZE * YSIZE &&
	IsBitSetBS( wall, pos ) && !IsBitSetBS( t, pos ) )
      stack[ top++ ] = pos;
    pos -= 1;
    if( pos > 0 && pos < XSIZE * YSIZE &&
	IsBitSetBS( wall, pos ) && !IsBitSetBS( t, pos ) )
      stack[ top++ ] = pos;
    pos -= YSIZE;
    if( pos > 0 && pos < XSIZE * YSIZE &&
	IsBitSetBS( wall, pos ) && !IsBitSetBS( t, pos ) )
      stack[ top++ ] = pos;
    pos -= YSIZE;
    if( pos > 0 && pos < XSIZE * YSIZE &&
	IsBitSetBS( wall, pos ) && !IsBitSetBS( t, pos ) )
      stack[ top++ ] = pos;
    pos += 1;
    if( pos > 0 && pos < XSIZE * YSIZE &&
	IsBitSetBS( wall, pos ) && !IsBitSetBS( t, pos ) )
      stack[ top++ ] = pos;
    pos += 1;
    if( pos > 0 && pos < XSIZE * YSIZE &&
	IsBitSetBS( wall, pos ) && !IsBitSetBS( t, pos ) )
      stack[ top++ ] = pos;
  }

  CopyBS( wall, t );
  BitAndNotEqBS( wall, stones );
}

static unsigned char _bitmask[ 8 ] = { 1, 2, 4, 8, 16, 32, 64, 128 };
static unsigned char _allmask[ 8 ] = { 1, 3, 7, 15, 31, 63, 127, 255 };

void RotateRegion( dbentry *region )
{
  int x, y;
  unsigned char wt[ 6 ], st[ 6 ];

  for( y = 0; y < region->ysize; y++ ) {
    wt[ y ] = region->walls[ y ];
    st[ y ] = region->stones[ y ];
  }
  for( x = 0; x < region->xsize; x++ ) {
    region->walls[ x ] = 0;
    region->stones[ x ] = 0;
    for( y = 0; y < region->ysize; y++ ) {
      if( wt[ y ] & _bitmask[ x ] )
	region->walls[ x ] |= _bitmask[ region->ysize - y - 1 ];
      if( st[ y ] & _bitmask[ x ] )
	region->stones[ x ] |= _bitmask[ region->ysize - y - 1 ];
    }
  }
  x = region->xsize;
  region->xsize = region->ysize;
  region->ysize = x;
}

void FlipRegion( dbentry *region )
{
  int y, t;

  for( y = ( region->ysize - 1 ) >> 1; y >= 0; y-- ) {
    t = region->walls[ y ];
    region->walls[ y ] = region->walls[ region->ysize - y - 1 ];
    region->walls[ region->ysize - y - 1 ] = t;
    t = region->stones[ y ];
    region->stones[ y ] = region->stones[ region->ysize - y - 1 ];
    region->stones[ region->ysize - y - 1 ] = t;
  }
}

void EncodeRegion( BitString wall, BitString stones,
		   int xmin, int ymin, int xmax, int ymax,
		   dbentry *region )
{
  int x, xp, y, yp;

  yp = 0;
  for( y = ymin; y <= ymax; y++ ) {
    region->walls[ yp ] = 0;
    region->stones[ yp ] = 0;
    xp = xmax * YSIZE;
    for( x = xmax - xmin; x >= 0; x-- ) {
      if( IsBitSetBS( wall, xp + y ) )
	region->walls[ yp ] = ( region->walls[ yp ] << 1 ) + 1;
      else
	region->walls[ yp ] = region->walls[ yp ] << 1;
      if( IsBitSetBS( stones, xp + y ) )
	region->stones[ yp ] = ( region->stones[ yp ] << 1 ) + 1;
      else
	region->stones[ yp ] = region->stones[ yp ] << 1;
      xp -= YSIZE;
    }
    yp++;
  }

  region->xsize = xmax - xmin + 1;
  region->ysize = ymax - ymin + 1;
}

void DecodeRegion( dbentry *region, BitString wall, BitString stones )
{
  int x, y, xp;
  unsigned char wt, st;

  Set0BS( wall );
  Set0BS( stones );
  for( y = 0; y < region->ysize; y++ ) {
    wt = region->walls[ y ];
    st = region->stones[ y ];
    xp = 0;
    for( x = 0; x < region->xsize; x++ ) {
      if( wt & 1 )
	SetBitBS( wall, xp + y );
      wt = wt >> 1;
      if( st & 1 )
	SetBitBS( stones, xp + y );
      st = st >> 1;
      xp += YSIZE;
    }
  }
}

int RegionEqual( dbentry *a, dbentry *b )
{
  int j;

  if( a->xsize != b->xsize ||
      a->ysize != b->ysize )
    return 0;
  for( j = 0; j < a->ysize; j++ ) {
    if( ( a->walls[ j ] ^ b->walls[ j ] ) & _allmask[ a->xsize ] )
      return 0;
    if( ( a->stones[ j ] ^ b->stones[ j ] ) & _allmask[ a->xsize ] )
      return 0;
  }
  return 1;
}

void SaveRegion( FILE *file, dbentry *region )
{
  int i, j;

/* PrintRegion( region ); */
  /* check for duplicates */
  for( i = 0; i < _dbused; i++ ) {
    for( j = 0; j < 4; j++ ) {
      if( RegionEqual( &_db[ i ], region ) ) {
/* printf( "duplicate of %i (rotation %i)\n", i, j ); */
	return;
      }
      RotateRegion( region );
    }
    FlipRegion( region );
    for( j = 0; j < 4; j++ ) {
      if( RegionEqual( &_db[ i ], region ) ) {
/* printf( "flip duplicate of %i (rotation %i)\n", i, j ); */
	return;
      }
      RotateRegion( region );
    }
    FlipRegion( region );
  }

  if( _dbused == _dbsize ) {
    _dbsize += 100;
    if( !( _db = realloc( _db, sizeof( dbentry ) * _dbsize ) ) ) {
      perror( "could not resize database" );
      exit( -1 );
    }
  }
/* printf( "new pattern: %i used\n", _dbused ); */
  memcpy( &_db[ _dbused++ ], region, sizeof( dbentry ) );
}

/* region must have at least 2 walls, and fit in 6x6 */
int CheckRegion( MAZE *maze, CFLT *c, int xmin, int ymin, int xmax, int ymax )
{
  int x, xp, y, ns = 2;

  if( xmax - xmin > 5 || ymax - ymin > 5 )
    return 0;

  if( !c->hits )
    return 0;

  if( c->fromdb )
    return 0;

  xp = xmin * YSIZE;
  for( x = xmin; x <= xmax; x++ ) {
    for( y = ymin; y <= ymax; y++)
      if( IsBitSetBS( maze->wall, xp + y ) )
	if( !--ns )
	  return 1;
    xp += YSIZE;
  }
  return 0;
}

int GetRegion( MAZE *maze, CFLT *c, dbentry *region )
{
  int x, xp, y;
  int xmin, xmax, ymin, ymax;
  BitString wall;

  xmin = XSIZE - 1;
  ymin = YSIZE - 1;
  xmax = ymax = 0;
  xp = 0;
  for( x = 0; x < XSIZE; x++ ) {
    for( y = 0; y < YSIZE; y++ ) {
      if( IsBitSetBS( c->conflict, xp + y  ) ) {
	if( x < xmin )
	  xmin = x;
	if( x > xmax )
	  xmax = x;
	if( y < ymin )
	  ymin = y;
	if( y > ymax )
	  ymax = y;
      }
    }
    xp += YSIZE;
  }

  xmin--;
  ymin--;
  xmax++;
  ymax++;

  /* suitable pattern to encode? */
  if( !CheckRegion( maze, c, xmin, ymin, xmax, ymax ) )
    return 0;

  CopyBS( wall, maze->wall );
  /* remove excess walls */
  CullWalls( wall, c->conflict );

  EncodeRegion( wall, c->conflict, xmin, ymin, xmax, ymax, region );
  return 1;
}

void DumpConflicts( MAZE *maze, CONFLICTS *c )
{
  int pen, cfl;
  dbentry region;
  FILE *file;

  if( !( file = fopen( DBFILE, "r+" ) ) ) {
    perror( "could not open db file" );
    return;
  }

  flock( fileno( file ), LOCK_EX );

  fseek( file, 0, SEEK_END );
  if( _dbused = ftell( file ) ) {
    if( !( _db = malloc( _dbused ) ) ) {
      perror( "could not allocate database" );
      exit( -1 );
    }
    fseek( file, 0, SEEK_SET );
    if( fread( _db, _dbused, 1, file ) != 1 ) {
      perror( "could not read database" );
      exit( -1 );
    }
    _dbused /= sizeof( dbentry );
  }
  _dbsize = _dbused;
/* printf( "USED: %i\n", _dbused ); */

  for( pen = 0; pen < c->number_penalties; pen++ )
    for( cfl = 0; cfl < c->pen[ pen ].number_conflicts; cfl++ )
      if( NumberBitsBS( c->pen[ pen ].cflts[ cfl ].conflict ) <= 4 ) {
/* 	Mprintf( 0, "Pattern %d, pen: %d, hits: %d \n", cfl, */
/* 		 c->pen[ pen ].penalty, */
/* 		 c->pen[ pen ].cflts[ cfl ].hits ); */
/* 	PrintBit3Maze( maze, c->pen[ pen ].cflts[ cfl ].conflict, */
/* 		       c->pen[ pen ].cflts[ cfl ].no_reach, 0 ); */
	if( GetRegion( maze, &c->pen[ pen ].cflts[ cfl ], &region ) ) {
	  SaveRegion( file, &region );
	}
      }
  fseek( file, 0, SEEK_SET );
  fwrite( _db, _dbused * sizeof( dbentry ), 1, file );
  free( _db );
  _dbused = _dbsize = 0;

  flock( fileno( file ), LOCK_UN );

  fclose( file );
}

int MatchRegionToMaze( MAZE *maze, dbentry *region, int xpos, int ypos )
{
  int x, y, xp;
  unsigned char w, s;

  for( y = 0; y < region->ysize; y++ ) {
    w = region->walls[ y ];
    s = region->stones[ y ];
    xp = xpos * YSIZE;
    for( x = 0; x < region->xsize; x++ ) {
      if( w & 1 ) {
	if( !IsBitSetBS( maze->wall, xp + ypos + y ) )
	  return 0;
      } else if( s & 1 && ( IsBitSetBS( maze->out, xp + ypos + y ) ||
			    IsBitSetBS( maze->goal, xp + ypos + y ) ||
			    IsBitSetBS( maze->dead, xp + ypos + y ) ) )
	return 0;
      w = w >> 1;
      s = s >> 1;
      xp += YSIZE;
    }
  }
/* printf( "Match at %i,%i!\n", xpos, ypos ); */
  return 1;
}

#define QUICK_NUM_NODES 100
int QuickRunPattern( CONFLICTS *cflts, BitString conflict )
{
  IDA *old_idainfo, idainfo;
  int result, c, mp;
  BitString reach, no_reach, all;

  Set1BS( all );
  BitAndNotEqBS( all, IdaInfo->IdaMaze->out );
  BitAndNotEqBS( all, conflict );

  InitIDA( &idainfo );
  old_idainfo = IdaInfo;
  IdaInfo = &idainfo;
  IdaInfo->IdaMaze = CopyMaze( old_idainfo->IdaMaze );

  while( ( mp = FindFirstSet( all ) ) > 0 ) {
    IdaInfo->IdaMaze->manpos = mp;
    PenDeactivateStones( IdaInfo->IdaMaze, conflict );
    IdaInfo->node_count = 0;
    IdaInfo->AbortNodeCount = QUICK_NUM_NODES;
    IdaInfo->goal_last_to = 0;
    IdaInfo->PrintPriority = 0;
    IdaInfo->HashTable = HashTableElse;
    MarkReach( IdaInfo->IdaMaze );
    CopyBS( reach, IdaInfo->IdaMaze->reach );
    CopyBS( no_reach, IdaInfo->IdaMaze->no_reach );
    IdaInfo->ThresholdInc = 1;
    result = PenStartIda();

/* printf( "IdaInfo->node_count: %i\n", IdaInfo->node_count ); */
    c = IdaInfo->node_count;

    if( c >= QUICK_NUM_NODES )
      break;
    if( result ) {
/* printf( "adding!\n\n" ); */
      AddConflict( cflts, conflict, no_reach, reach, result, 1 );
    }

    BitAndNotEqBS( all, reach );
  }

  DelCopiedMaze( IdaInfo->IdaMaze );
  IdaInfo = old_idainfo;
  if( c >= QUICK_NUM_NODES )
    return 0;
  else
    return 1;
}

void AddRegionToPatterns( CONFLICTS *c, dbentry *region, int xpos, int ypos )
{
  int x, y, xp;
  unsigned char s;
  BitString conflict;

  /* need to resize? */
  if( _dbpatterns.number_conflicts == _dbpatterns.array_size ) {
    _dbpatterns.array_size *= 2;
    if( !( _dbpatterns.cflts = realloc( _dbpatterns.cflts, sizeof( CFLT ) *
					_dbpatterns.array_size ) ) ) {
      perror( "could not grow database pattern array" );
      exit( -1 );
    }
  }

  /* create the pattern from the region */
  Set0BS( conflict );
  for( y = 0; y < region->ysize; y++ ) {
    s = region->stones[ y ];
    xp = xpos * YSIZE;
    for( x = 0; x < region->xsize; x++ ) {
      if( s & 1 )
	SetBitBS( conflict, xp + ypos + y );
      s = s >> 1;
      xp += YSIZE;
    }
  }

  /* check for duplicates */
  for( x = 0; x < _dbpatterns.number_conflicts; x++ )
    if( EqualBS( _dbpatterns.cflts[ x ].conflict, conflict ) )
      return;

  /* do a quick run to keep _dbpatterns array size down */
  if( QuickRunPattern( c, conflict ) )
    /* nothing left to test */
    return;

  /* store the pattern */
  CopyBS( &_dbpatterns.cflts[ _dbpatterns.number_conflicts ].conflict,
	  conflict );
  _dbpatterns.cflts[ _dbpatterns.number_conflicts ].onestone =
    FindFirstSet( conflict );
  _dbpatterns.number_conflicts++;
}

void IntegrateRegion( MAZE *maze, CONFLICTS *c, dbentry *region )
{
  int x, y, i;

  for( x = 0; x <= XSIZE - region->xsize; x++ ) {
    for( y = 0; y <= YSIZE - region->ysize; y++ ) {
      for( i = 0; i < 4; i++ ) {
	if( MatchRegionToMaze( maze, region, x, y ) ) {
/* printf( "adding %i,%i\n", x, y ); */
	  AddRegionToPatterns( c, region, x, y );
	}
	RotateRegion( region );
      }
      FlipRegion( region );
      for( i = 0; i < 4; i++ ) {
	if( MatchRegionToMaze( maze, region, x, y ) ) {
/* printf( "adding %i,%i\n", x, y ); */
	  AddRegionToPatterns( c, region, x, y );
	}
	RotateRegion( region );
      }
      FlipRegion( region );
    }
  }
}

int CmpPattern( const void *ca, const void *cb )
{
  CFLT *a = (CFLT *)ca, *b = (CFLT *)cb;
  int na, nb;

  if( ( na = NumberBitsBS( a->conflict ) ) <
      ( nb = NumberBitsBS( b->conflict ) ) )
    return -1;
  else
    return na != nb;
}

void LoadConflicts( MAZE *maze, CONFLICTS *c )
{
  int i;
  FILE *file;

  Mprintf( 0, "loading database\n" );

  /* read file in */
  if( !( file = fopen( DBFILE, "r+" ) ) ) {
    perror( "could not open db file" );
    return;
  }
  flock( fileno( file ), LOCK_SH );

  fseek( file, 0, SEEK_END );
  if( _dbused = ftell( file ) ) {
    if( !( _db = malloc( _dbused ) ) ) {
      perror( "could not allocate database" );
      exit( -1 );
    }
    fseek( file, 0, SEEK_SET );
    if( fread( _db, _dbused, 1, file ) != 1 ) {
      perror( "could not read database" );
      exit( -1 );
    }
    _dbused /= sizeof( dbentry );
  }
  _dbsize = _dbused;

  flock( fileno( file ), LOCK_UN );
  fclose( file );

  if( _dbpatterns.cflts )
    free( _dbpatterns.cflts );
  if( !( _dbpatterns.cflts = malloc( sizeof( CFLT ) * 100 ) ) ) {
    perror( "could not get database pattern array" );
    exit( -1 );
  }
  _dbpatterns.array_size = 100;
  _dbpatterns.number_conflicts = 0;

  /* integrate database entries */
  for( i = 0; i < _dbused; i++ ) {
if( _dbused > 5 && !( i % ( _dbused / 5 ) ) )
printf( "%i of %i\n", i, _dbused );
    IntegrateRegion( maze, c, &_db[ i ] );
  }

  /* sort with patterns with fewest stones first */
  qsort( (void *)_dbpatterns.cflts, _dbpatterns.number_conflicts,
	 sizeof( CFLT ), CmpPattern );

  /* clean up */
  free( _db );
  _dbused = _dbsize = 0;

  Mprintf( 0, "finished (%i patterns created)\n",
	   _dbpatterns.number_conflicts );
}
