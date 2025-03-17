#include "board.h"

typedef struct {
  PHYSID from, to, manfrom;
} SMOVE;

int GenMoves( MAZE *maze, SMOVE *moves ) 
{
  PHYSID pos;
  int i, dir;
  int num;
  MOVE move;

  num = 0;
  for( i = 0; i < maze->number_stones; i++ ) {
    pos = maze->stones[ i ].loc;
    for( dir = 0; dir < 4; dir++ ) {
      if( IsBitSetBS( maze->reach, pos - DirToDiff[ dir ] ) &&
	  IsBitSetBS( maze->S[ dir ], pos ) &&
	  maze->PHYSstone[ pos + DirToDiff[ dir ] ] < 0 ) {
	move.from = pos;
	move.to = pos + DirToDiff[ dir ];
	move.last_over = maze->manpos;
	if( !DeadLock( maze, move ) &&
	    !DeadLock2( maze, &move ) ) {
	  moves[ num ].from = pos;
	  moves[ num ].to = pos + DirToDiff[ dir ];
	  moves[ num ].manfrom = maze->manpos;
	  num++;
	}
      }
    }
  }

  return num;
}

int IsGoalNodeRandom( MAZE *maze )
{
  int i;

  for( i = 0; i < maze->number_stones; i++ )
    if( !IsBitSetBS( maze->goal, maze->stones[ i ].loc ) )
      return 0;
  return 1;
}

#define NNODE 1000
void RunDead()
{
  IDA *old_idainfo, idainfo;
  int result;

  InitIDA( &idainfo );
  old_idainfo = IdaInfo;
  IdaInfo = &idainfo;
  IdaInfo->IdaMaze = CopyMaze( old_idainfo->IdaMaze );

  IdaInfo->node_count = 0;
  IdaInfo->AbortNodeCount = NNODE;
  IdaInfo->goal_last_to = 0;
  IdaInfo->PrintPriority = 0;
  IdaInfo->HashTable = HashTableElse;
  IdaInfo->ThresholdInc = 1;
  result = DeadStartIda();

/* printf( "IdaInfo->node_count: %i\n", IdaInfo->node_count ); */

  if( IdaInfo->node_count >= NNODE && result >= ENDPATH ) {
/* printf( "adding!\n\n" ); */
    AddConflict( old_idainfo->IdaMaze->conflicts, old_idainfo->IdaMaze->stone,
		 old_idainfo->IdaMaze->no_reach, old_idainfo->IdaMaze->reach,
		 result, 1 );
  }

  DelCopiedMaze( IdaInfo->IdaMaze );
  IdaInfo = old_idainfo;
}

#define STACKSIZE 100000
#define REUSEGOOD 1	/* seems like this can't be > 1 or else the stack
			   explodes as stones are pushed on and off goals */

int StartRandom() {
  int i, nmoves, stacktop, goodtop;
  int goodstack[ 4096 ];
  int gooduses[ 4096 ];
  SMOVE stack[ STACKSIZE ];
  SMOVE moves[ 256 ];

  PHYSID manpos;
  BitString stone;	
  STN stones[ MAXSTONES ];
  signed char PHYSstone[ XSIZE * YSIZE ];
  STRUCT structs[ XSIZE * YSIZE ];

  /* initialize data structures */
  total_node_count = 0;
  IdaInfo->node_count = 0;

  init_stats();
  memset( &IdaInfo->IdaArray[ 0 ].solution, 0, sizeof( MOVE ) );
  dl_pos_nc = dl_pos_sc = dl_neg_nc = dl_neg_sc = 0;
  pen_pos_nc = pen_pos_sc = pen_neg_nc = pen_neg_sc = 0;
  AvoidThisSquare = 0;
  Set0BS(IdaInfo->IdaManSquares);
  Set0BS(IdaInfo->IdaStoneSquares);

  /* save initial state */
  manpos = IdaInfo->IdaMaze->manpos;
  CopyBS( stone, IdaInfo->IdaMaze->stone );
  memcpy( stones, IdaInfo->IdaMaze->stones,
	  sizeof( STN ) * IdaInfo->IdaMaze->number_stones );
  memcpy( PHYSstone, IdaInfo->IdaMaze->PHYSstone, XSIZE * YSIZE );
  memcpy( structs, IdaInfo->IdaMaze->stones,
	  sizeof( STRUCT ) * IdaInfo->IdaMaze->number_structs );

  stacktop = 0;
  goodtop = 0;
  while( 1 ) {
    /* generate all moves */
    nmoves = GenMoves( IdaInfo->IdaMaze, moves );

    /* discard deadlocks */
    while( nmoves ) {
      i = random() % nmoves;
      MANTO( IdaInfo->IdaMaze, moves[ i ].from );
      STONEFROMTO( IdaInfo->IdaMaze, moves[ i ].from, moves[ i ].to );
      MarkReach( IdaInfo->IdaMaze );
      if( GetPenalty( IdaInfo->IdaMaze->conflicts, IdaInfo->IdaMaze->stone,
		      IdaInfo->IdaMaze->manpos ) < ENDPATH )
	break;
      MANTO( IdaInfo->IdaMaze, moves[ i ].manfrom );
      STONEFROMTO( IdaInfo->IdaMaze, moves[ i ].to, moves[ i ].from );
      memcpy( &moves[ i ], &moves[ --nmoves ], sizeof( SMOVE ) );
    }

    /* time to backtrack? */
    if( !nmoves || stacktop == STACKSIZE ) {
      if( goodtop ) {
	goodtop--;
	while( stacktop > goodstack[ goodtop ] ) {
	  stacktop--;
	  MANTO( IdaInfo->IdaMaze, stack[ stacktop ].manfrom );
	  STONEFROMTO( IdaInfo->IdaMaze,
		       stack[ stacktop ].to, stack[ stacktop ].from );
	}
	if( ++gooduses[ goodtop ] < REUSEGOOD ) {
printf( "WAURCHIOESNITH!\n" );
	  goodtop++;
	}
      } else if( stacktop > 1000 ) {
	IdaInfo->IdaMaze->manpos = manpos;
	CopyBS( IdaInfo->IdaMaze->stone, stone );
	memcpy( IdaInfo->IdaMaze->stones, stones,
		sizeof( STN ) * IdaInfo->IdaMaze->number_stones );
	memcpy( IdaInfo->IdaMaze->PHYSstone, PHYSstone, XSIZE * YSIZE );
	memcpy( IdaInfo->IdaMaze->stones, structs,
		sizeof( STRUCT ) * IdaInfo->IdaMaze->number_structs );
	stacktop = 0;
      } else {
	while( stacktop ) {
	  stacktop--;
	  MANTO( IdaInfo->IdaMaze, stack[ stacktop ].manfrom );
	  STONEFROMTO( IdaInfo->IdaMaze,
		       stack[ stacktop ].to, stack[ stacktop ].from );
	}
      }
      MarkReach( IdaInfo->IdaMaze );
/* printf( "<<<<<<<<<\n" );
PrintMaze( IdaInfo->IdaMaze );
printf( ">>>>>>>>>\n" ); */
      continue;
    }

    /* did we solve the maze? */
    if( IsGoalNodeRandom( IdaInfo->IdaMaze ) ) {
      printf( "Woohoo!\n" );
      exit( -1 );
    }

    /* save chosen move on stack */
    memcpy( &stack[ stacktop ], &moves[ i ], sizeof( SMOVE ) );
    stacktop++;

    /* save this state? */
    if( IsBitSetBS( IdaInfo->IdaMaze->goal, moves[ i ].to ) &&
	!IsBitSetBS( IdaInfo->IdaMaze->goal, moves[ i ].from ) ) {
      if( goodtop == 4096 ) {
	printf( "goodstack overflow!\n" );
	exit( -1 );
      }
      gooduses[ goodtop ] = 0;
      goodstack[ goodtop++ ] = stacktop;
    }

    /* check for deadlocks */
    if( !( random() % 10 ) )
      RunDead();

    /* update node count */
    total_node_count++;
    if( !(total_node_count % 100000 ) ) {
      PrintMaze( IdaInfo->IdaMaze );
    }
  }
}
