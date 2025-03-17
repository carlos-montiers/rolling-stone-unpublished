#include "board.h"

#define CFLTS_PPLEVEL 5

int VerifyConflict(CONFLICTS *c, BitString conflict, BitString reach, int pen)
{
	IDA      *old_idainfo, idainfo;
	int	  peni;
	int       coni;
	BitString stones_left;
	int 	  penalty;
	int	  count,result;

	Mprintf( 0, "Start VerifyConflict\n");
	CopyBS(stones_left,conflict);
	penalty = 0;
	count = 0;
	PrintMaze(IdaInfo->IdaMaze);
	PrintBit3Maze(IdaInfo->IdaMaze,conflict,reach,0);
	Mprintf( 0, "Prior Penalties for this Conflict:\n");
	for (peni=0; peni<c->number_penalties; peni++) {
		for (coni=0; coni<c->pen[peni].number_conflicts; coni++) {
		    if (  (AllBitsSetBS(stones_left,
					c->pen[peni].cflts[coni].conflict))
		        &&!LogAndBS(reach,c->pen[peni].cflts[coni].no_reach)) {
		    	count++;
			PrintBit3Maze(IdaInfo->IdaMaze,
				c->pen[peni].cflts[coni].conflict,
				c->pen[peni].cflts[coni].no_reach,0);
			penalty += c->pen[peni].penalty;
			if (penalty>=ENDPATH) goto END_GET;
			UnsetBS(stones_left,c->pen[peni].cflts[coni].conflict);
		    }
		}
	}
END_GET:
/*
if (pen>=ENDPATH) return(0);
	penalty = GetPenalty(c,conflict,IdaInfo->IdaMaze->manpos);
*/
	Mprintf( 0, "Total Penalty found: %d in %d conflicts\n",penalty,count);

	InitIDA(&idainfo);
	old_idainfo = IdaInfo;
	IdaInfo                = &idainfo;
	IdaInfo->IdaMaze = CopyMaze(old_idainfo->IdaMaze);
	PenDeactivateStones(IdaInfo->IdaMaze,conflict);
	IdaInfo->node_count     = 0;
	IdaInfo->AbortNodeCount = 1000000;
	IdaInfo->goal_last_to   = old_idainfo->goal_last_to;
	IdaInfo->PrintPriority  = 3;
	IdaInfo->HashTable      = HashTableElse;
	if (pen<ENDPATH) {
		IdaInfo->ThresholdInc   = 1;
		result = PenStartIda();
	} else {
		IdaInfo->ThresholdInc   = 2;
		result = DeadStartIda();
	}
	Mprintf( 0, "Researching the Conflict gave: %d, asked was %d (n: %" PRId32 "), prior: %d\n",
		result,pen,IdaInfo->node_count,penalty);
	if (result<pen) Mprintf( 0, "SHIT+SHIT+SHIT+SHIT+SHIT+SHIT+SHIT+SHIT\n");
	DelCopiedMaze(IdaInfo->IdaMaze);
	IdaInfo = old_idainfo;
	return(result);
}

void InitConflicts(CONFLICTS *c)
{
	SR(Debug(CFLTS_PPLEVEL,0,"InitConflicts\n"));
	c->number_patterns  = 0;
	c->number_penalties = 0;
	c->array_size_pen   = 0;
	c->pen              = NULL;

	c->number_deadtested 	 = 0;
	c->array_size_deadtested = 0;
	c->deadtested		 = NULL;

	c->number_pentested	 = 0;
	c->array_size_pentested  = 0;
	c->pentested		 = NULL;

	memset(c->penalty_hist,0,sizeof(int32_t)*MAX_PENHIST);
	memset(c->penalty_depth,0,sizeof(int32_t)*MAX_DEPTH);
}

void DelConflicts(CONFLICTS *c)
{
	int i;
	SR(Debug(CFLTS_PPLEVEL,0,"DelConflicts\n"));
	for (i=0; i<c->number_penalties; i++) My_free(c->pen[i].cflts);
	My_free(c->pen);
	My_free(c->deadtested);
	My_free(c->pentested);
	InitConflicts(c);
}

void AddPenalties(CONFLICTS *c)
/* just increase the size of the penalty array by CONFLICT_INC */
{
	SR(Debug(CFLTS_PPLEVEL,0,"AddPenalties: (%i to %i)\n",
		c->array_size_pen,c->array_size_pen+CONFLICT_INC));
	c->pen=My_realloc(c->pen,
			  sizeof(PENALTY)*(c->array_size_pen+CONFLICT_INC));
	c->array_size_pen += CONFLICT_INC;
}

void InsertPenalty(CONFLICTS *c, int i, int p)
/* Insert a new penalty struct at position i for penalty p */
{
	SR(Debug(CFLTS_PPLEVEL,0,"InsertPenalty: at %i for pen: %i\n",i,p));
	if (c->array_size_pen<=c->number_penalties) AddPenalties(c);
	memmove(c->pen+i+1,c->pen+i,
		sizeof(PENALTY)*(c->number_penalties-i));
	InitPenalty(c->pen+i,p);
	c->number_penalties++;
}

void InitPenalty(PENALTY *p, int penalty)
/* init penalty structure */
{
	SR(Debug(CFLTS_PPLEVEL,0,"InitPenalty: pen: %i\n",penalty));
	p->penalty = penalty;
	p->array_size = 0;
	p->number_conflicts = 0;
	p->cflts = NULL;
}

PENALTY *FindPenalty(CONFLICTS *c, int penalty)
/* Find a CFLTS in conflicts.pen do nothing if there otherwise insert one */
{
	int i;

	SR(Debug(CFLTS_PPLEVEL,0,"FindPenalty: pen: %i\n",penalty));
	SR(Assert((((penalty&1)==0) && penalty>=2) || penalty <=ENDPATH,
		"FindPenalty: Penalty wrong: %i", penalty));
	i=0;
	while (  (i < c->number_penalties)
	       &&(penalty < c->pen[i].penalty)) {
		i++;
	}
	if (i>=c->number_penalties || penalty != c->pen[i].penalty) {
		/* We have to create this entry by inserting it */
		InsertPenalty(c,i,penalty);
	}
	return(c->pen+i);
}

void AddConflicts(PENALTY *p)
{
	SR(Debug(CFLTS_PPLEVEL,0,"AddConflicts: (%i to %i)\n",
		p->array_size,p->array_size+CONFLICT_INC));
	p->cflts=My_realloc(p->cflts,sizeof(CFLT)*(p->array_size+CONFLICT_INC));
	p->array_size += CONFLICT_INC;
}

/* 3 if a == b, 1 if a is a subset of b, 2 if b is a subset of a,
   0 otherwise */
int SubsetBS( BitString a, BitString b )
{
  int ab = 1, ba = 2, i;

  ab = ba = 0;
  for( i = 0; i < NUMBERINTS; i++ ) {
    if( a[ i ] & ~b[ i ] )
      ab = 0;
    if( b[ i ] & ~a[ i ] )
      ba = 0;
    if( !ab && !ba )
      return 0;
  }
  return ab | ba;
}

int RemoveDuplicates( CONFLICTS *c, int penalty,
		      BitString pattern, BitString no_reach )
{
  int peni, coni;

  for( peni = 0; peni < c->number_penalties; peni++ ) {
    for( coni = 0; coni < c->pen[ peni ].number_conflicts; coni++ ) {
      if( LogAndBS( c->pen[ peni ].cflts[ coni ].no_reach, no_reach ) ) {
	if( c->pen[ peni ].penalty < penalty ) {
	  if( SubsetBS( c->pen[ peni ].cflts[ coni ].conflict, pattern ) > 1 )
	    memcpy( &c->pen[ peni ].cflts[ coni ],
		    &c->pen[ peni ].cflts[ --c->pen[ peni ].number_conflicts ],
		    sizeof( CFLT ) );
	} else if( c->pen[ peni ].penalty == penalty ) {
	  switch( SubsetBS( c->pen[ peni ].cflts[ coni ].conflict,
			    pattern ) ) {
	  case 1:
	    return 0;
	  case 3:
	    return 0;
	  case 2:
	    memcpy( &c->pen[ peni ].cflts[ coni ],
		    &c->pen[ peni ].cflts[ --c->pen[ peni ].number_conflicts ],
		    sizeof( CFLT ) );
	  }
	} else {
	  switch( SubsetBS( c->pen[ peni ].cflts[ coni ].conflict,
			    pattern ) ) {
	  case 1:
	    return 0;
	  case 3:
	    return 0;
	  }
	}
      }
    }
  }
  return 1;
}

void InsertConflict(PENALTY *p, BitString c, BitString no_reach)
/* Insert c and no_reach into the PEANLTY structure */
{
	SR(Debug(CFLTS_PPLEVEL,0,"InsertConflict\n"));

/*
printf("InsertConflict: penalty: %d\n",p->penalty);
PrintBit3Maze(IdaInfo->IdaMaze,c,no_reach,0);
*/

	if (p->array_size<=p->number_conflicts) AddConflicts(p);
	CopyBS(p->cflts[p->number_conflicts].conflict,c);
	CopyBS(p->cflts[p->number_conflicts].no_reach,no_reach);
	p->cflts[p->number_conflicts].hits=0;
p->cflts[p->number_conflicts].onestone = FindFirstSet( c );
	p->number_conflicts++;
	NavigatorPSConflict(IdaInfo->IdaMaze,p->penalty,p->number_conflicts-1);
}

void UpdateInfluence( BitString conflict, int penalty )
{
  int i, j, p;

/*   p = ( NumberBitsBS( conflict ) - 1 ); */
/*   p = penalty / ( p * p ); */
  if( penalty == ENDPATH )
    p = NumberBitsBS( conflict ) - 1;
  else
    p = NumberBitsBS( conflict );
  for( i = 0; i < XSIZE * YSIZE; i++ )
    if( IsBitSetBS( conflict, i ) )
      for( j = i + 1; j < XSIZE * YSIZE; j++ )
	if( IsBitSetBS( conflict, j ) )
	  if( p > MainIdaInfo.IdaMaze->influence[ i ][ j ] ) {
	    MainIdaInfo.IdaMaze->influence[ i ][ j ] = p;
	    MainIdaInfo.IdaMaze->influence[ j ][ i ] = p;
	  }
}

int FindInfluence( BitString stones )
{
  int i, j, t;

  t = 0;
  for( i = 0; i < XSIZE * YSIZE; i++ )
    if( IsBitSetBS( stones, i ) )
      for( j = i + 1; j < XSIZE * YSIZE; j++ )
	if( IsBitSetBS( stones, j ) )
	  t += MainIdaInfo.IdaMaze->influence[ i ][ j ];
  return t;
}

#define MAX_DELAYED 100
int	   number_delayed;
CONFLICTS *c_delayed[MAX_DELAYED];
BitString  conflict_delayed[MAX_DELAYED];
BitString  no_reach_delayed[MAX_DELAYED];
BitString  reach_delayed[MAX_DELAYED];
int	   penalty_delayed[MAX_DELAYED];
char	   fromdb_delayed[MAX_DELAYED];

void InitDelayedConflict()
{
	number_delayed = 0;
}

void AddDelayedConflict(CONFLICTS *c, BitString conflict,
	BitString no_reach, BitString reach, int penalty, char fromdb )
{
	c_delayed[number_delayed] = c;
	CopyBS(conflict_delayed[number_delayed],conflict);
	CopyBS(no_reach_delayed[number_delayed],no_reach);
	CopyBS(reach_delayed[number_delayed],reach);
	penalty_delayed[number_delayed] = penalty;
	fromdb_delayed[number_delayed] = fromdb;
	number_delayed++;
}

void InsertDelayedConflict()
{
	while (number_delayed > 0) {
		number_delayed--;
		AddConflict(c_delayed[number_delayed],
			    conflict_delayed[number_delayed],
			    no_reach_delayed[number_delayed],
			    reach_delayed[number_delayed],
			    penalty_delayed[number_delayed],
			    fromdb_delayed[number_delayed]);
	}
}

void AddConflict(CONFLICTS *c, BitString conflict,
	BitString no_reach, BitString reach, int penalty, char fromdb )
{
  int prior_pen;
  PENALTY *pen;

UpdateInfluence( conflict, penalty );
  SR(Debug(CFLTS_PPLEVEL,0,"AddConflict\n"));
  /* Check if we have a conflict already that does at least as good
     for the given stones and insert if not */
  if ((prior_pen =
       GetPenalty(c,conflict,IdaInfo->IdaMaze->manpos)) < penalty) {

    if (NumberBitsBS(conflict)<2) {
      Mprintf( 0, "ALARM: Pattern wrong!\n");
      PrintBit3Maze(IdaInfo->IdaMaze,conflict,no_reach,0);
    }
/*     if( RemoveDuplicates( c, penalty, conflict, no_reach ) ) { */
      c->number_patterns++;
      penalty = min(ENDPATH,penalty+prior_pen);
      InsertConflict(pen = FindPenalty(c,penalty),conflict,no_reach);
      pen->cflts[ pen->number_conflicts - 1 ].fromdb = fromdb;
/*     } */
  } SR(else Debug(CFLTS_PPLEVEL,0,"AddConflict failed\n");)
}

int  GetPriorPostPen(MAZE *maze, int penalty, int *prior, int *post)
{
	int i;
	
	for (i=0;
	        i<maze->conflicts->number_penalties
	     && maze->conflicts->pen[i].penalty != penalty;
	     i++ ) ;

	if ( i >= maze->conflicts->number_penalties ) return(0);

	if ( i == 0 ) *prior = -1;
	else *prior = maze->conflicts->pen[ i - 1 ].penalty;

	if ( i == maze->conflicts->number_penalties - 1 ) *post = -1;
	else *post = maze->conflicts->pen[ i + 1 ].penalty;
	
	return( 1 );
}

#ifdef DYNAMICDB
void CheckDBPattern( CONFLICTS *cflts, CFLT *c )
{
  IDA *old_idainfo, idainfo;
  int result;
BitString reach, no_reach;

  InitIDA( &idainfo );
  old_idainfo = IdaInfo;
  IdaInfo = &idainfo;
  IdaInfo->IdaMaze = CopyMaze( old_idainfo->IdaMaze );
  PenDeactivateStones( IdaInfo->IdaMaze, c->conflict );
  IdaInfo->node_count = 0;
  IdaInfo->AbortNodeCount = 1000000;
  IdaInfo->goal_last_to = 0;
  IdaInfo->PrintPriority = 0;
  IdaInfo->HashTable = HashTableElse;
MarkReach( IdaInfo->IdaMaze );
CopyBS( reach, IdaInfo->IdaMaze->reach );
CopyBS( no_reach, IdaInfo->IdaMaze->no_reach );
  if( 1 ) {
    IdaInfo->ThresholdInc = 1;
    result = PenStartIda();
  } else {
    IdaInfo->ThresholdInc = 2;
    result = DeadStartIda();
  }
/* printf( "IdaInfo->node_count: %i\n", IdaInfo->node_count ); */
  DelCopiedMaze( IdaInfo->IdaMaze );
  IdaInfo = old_idainfo;

/* printf( "penalty: %i\n", result ); */
/* PrintBit3Maze( IdaInfo->IdaMaze, c->conflict, no_reach, IdaInfo->IdaMaze->manpos ); */
/* printf( "\n" ); */

  if( result ) {
/* printf( "adding!\n\n" ); */
    AddConflict( cflts, c->conflict, no_reach, reach, result, 0 );
  }
}
#endif

void PrintConflicts(MAZE *maze, CONFLICTS *c)
{
	int	  peni;
	int       coni;
	PHYSID    old_manpos = maze->manpos;

	Mprintf( 0, "Number of patterns: %d\n", c->number_patterns);
	for (peni=0; peni<c->number_penalties; peni++) {
		Mprintf( 0, "Penalty: %i #confl: %d\n", c->pen[peni].penalty,
			c->pen[peni].number_conflicts);
		for (coni=0; coni<c->pen[peni].number_conflicts; coni++) {
			Mprintf( 0, "Pattern %d, pen: %d, hits: %d \n",coni,
				c->pen[peni].penalty,
				c->pen[peni].cflts[coni].hits);
			PrintBit3Maze(maze,c->pen[peni].cflts[coni].conflict,
				c->pen[peni].cflts[coni].no_reach,0);
		}
	}
	Mprintf( 0, "Number of deadtested: %d\n", c->number_deadtested);
	for (peni=0; peni<c->number_deadtested; peni++) {
		maze->manpos = c->deadtested[peni].manpos;
		PrintBit3Maze(maze,c->deadtested[peni].stones,
				   c->deadtested[peni].relevant,0);
	}
	Mprintf( 0, "Number of pentested: %d\n", c->number_pentested);
	for (peni=0; peni<c->number_pentested; peni++) {
		maze->manpos = c->pentested[peni].manpos;
		PrintBit3Maze(maze,c->pentested[peni].stones,
				   c->pentested[peni].relevant,0);
	}
	Mprintf( 0, "penalty: count; ");
	for (peni=0; peni<MAX_PENHIST; peni+=2) {
		if (c->penalty_hist[peni]>0) 
			Mprintf( 0, "%d:%" PRId32 ", ", peni, c->penalty_hist[peni]);
	}
	Mprintf( 0, "\ndepth: count; ");
	for (peni=0; peni<MAX_DEPTH; peni++) {
		if (c->penalty_depth[peni]>0) 
			Mprintf( 0, "%d:%" PRId32 ", ", peni, c->penalty_depth[peni]);
	}
	Mprintf( 0, "\n");
	maze->manpos = old_manpos;
}

void AddTestedPen(CONFLICTS *c, BitString relevant, BitString stones, 
				PHYSID manpos, PHYSID stonepos)
{
	TESTED *pen;
	if (Options.st_testd==0 || NumberBitsBS(relevant) == 0) return;
	if (c->array_size_pentested <= c->number_pentested) {
		c->pentested=My_realloc(c->pentested,
			sizeof(CFLT)*(c->array_size_pentested+CONFLICT_INC));
		c->array_size_pentested += CONFLICT_INC;
	}
	pen = &(c->pentested[c->number_pentested]);
	pen->manpos = manpos; pen->stonepos = stonepos;
	CopyBS(pen->relevant,relevant);
	CopyBS(pen->stones,stones);
	c->number_pentested++;
}

void AddTestedDead(CONFLICTS *c, BitString relevant, BitString stones,
				PHYSID manpos, PHYSID stonepos)
{
	TESTED *dead;
	if (Options.st_testd==0 || NumberBitsBS(relevant) == 0) return;
	if (c->array_size_deadtested <= c->number_deadtested) {
		c->deadtested=My_realloc(c->deadtested,
			sizeof(CFLT)*(c->array_size_deadtested+CONFLICT_INC));
		c->array_size_deadtested += CONFLICT_INC;
	}
	dead = &(c->deadtested[c->number_deadtested]);
	dead->manpos = manpos; dead->stonepos = stonepos;
	CopyBS(dead->relevant,relevant);
	CopyBS(dead->stones,stones);
	c->number_deadtested++;
}

int WasTestedPen(CONFLICTS *c, BitString stones, PHYSID manpos, PHYSID stonepos)
{
/* was this or a similar position tested before? Yes if
	1) man square on same square AND
	2) stone on same square AND
	3) no stone is on relevant squares that was not before.
Play with what are relevant squares (touched for a solution? close to the
stones in the fianlpattern???) */	

	int i;
	BitString tmp;

	if (Options.st_testd==0) return(0);
	for (i=0; i<c->number_pentested; i++) {
		if (  (manpos==c->pentested[i].manpos)
		    &&(stonepos==c->pentested[i].stonepos)) {
			BitAndBS(tmp,stones,c->pentested[i].relevant);
			BitAndNotEqBS(tmp,c->pentested[i].stones);
			if (NumberBitsBS(tmp)==0) {
				return(1);
			}
		}
	}
	return(0);
}

int WasTestedDead(CONFLICTS *c, BitString stones, PHYSID manpos,PHYSID stonepos)
{
	int i;
	BitString tmp;

	if (Options.st_testd==0) return(0);
	for (i=0; i<c->number_deadtested; i++) {
		if (  (manpos==c->deadtested[i].manpos)
		    &&(stonepos==c->deadtested[i].stonepos)) {
			BitAndBS(tmp,stones,c->deadtested[i].relevant);
			BitAndNotEqBS(tmp,c->deadtested[i].stones);
			if (NumberBitsBS(tmp)==0) {
				return(1);
			}
		}
	}
	return(0);
}


int  GetPenaltyOld(CONFLICTS *c, BitString stones, PHYSID manpos )
    /* This is the way we did GetPenalty first. Quick and dirty and we hope,
     * not very efficient, since we are trying to do it "better" */
{
	int	  peni;
	int       coni;
	BitString stones_left;
	int 	  penalty;
	int	  count;
#ifdef DYNAMICDB
static char t = 0;
#endif

	if (Options.lb_cf==0) return(0);

#ifdef DYNAMICDB
if( !t )
for( coni = 0; coni < _dbpatterns.number_conflicts; coni++ )
  if( IsBitSetBS( stones, _dbpatterns.cflts[ coni ].onestone ) &&
      AllBitsSetBS( stones, _dbpatterns.cflts[ coni ].conflict ) ) {
t = 1;
    CheckDBPattern( c, &_dbpatterns.cflts[ coni ] );
t = 0;
    memcpy( &_dbpatterns.cflts[ coni ],
	    &_dbpatterns.cflts[ --_dbpatterns.number_conflicts ],
	    sizeof( CFLT ) );
    break;
  }
#endif

	SR(Debug(CFLTS_PPLEVEL,0,"GetPenalty\n"));
	CopyBS(stones_left,stones);
	penalty = 0;
	count = 0;
	for (peni=0; peni<c->number_penalties; peni++) {
		for (coni=0; coni<c->pen[peni].number_conflicts; coni++) {
		    count++;
		    if ( IsBitSetBS( stones_left,
				     c->pen[peni].cflts[coni].onestone ) &&
			 !IsBitSetBS( c->pen[ peni ].cflts[ coni ].no_reach,
				      manpos ) &&
			 AllBitsSetBS(stones_left,
				      c->pen[peni].cflts[coni].conflict) ) {
		      penalty += c->pen[peni].penalty;
		      c->pen[peni].cflts[coni].hits++;
SR(if (PP>4) {
	Mprintf( 0, "GetPenalty hit: peni: %d, coni: %d, penalty: %d\n",
		peni,coni, c->pen[peni].penalty);
	PrintBit3Maze(IdaInfo->IdaMaze,
		c->pen[peni].cflts[coni].conflict,
		c->pen[peni].cflts[coni].no_reach,0);
})
			if (penalty>=ENDPATH) {
				SR(Debug(CFLTS_PPLEVEL,0,
					"GetPenalty: Deadlock found\n"));
				goto END_GETPEN;
			}
			UnsetBS(stones_left,c->pen[peni].cflts[coni].conflict);
		    }
		}
	}
	SR(Debug(CFLTS_PPLEVEL,0,"GetPenalty: pen found: %i (s:%i)\n",
		penalty,count));
END_GETPEN:
	if (penalty&1 && penalty!=ENDPATH) penalty++;
	if (penalty>=MAX_PENHIST) c->penalty_hist[MAX_PENHIST-1]++;
	else c->penalty_hist[penalty]++;
	peni = max(0,IdaInfo->Threshold - IdaInfo->IdaMaze->h);
	peni = min(MAX_DEPTH-1,peni);
	if (penalty>0) c->penalty_depth[ peni ]++;
/*
peni = GetPenaltyMaximize(c, stones, manpos);
if (peni != penalty) {
    printf("Problem\n");
}
*/
	return(penalty);
}

#define BitType uint32_t
#define BitSize (sizeof(BitType)*8)
#define BitSet(bs,bitnumber)   		((bs)  |=  (((BitType)1)<<bitnumber))
#define BitSet0(bs)	      		((bs)   =  0)
#define BitUnSet(bs,bitnumber) 		((bs)  &= ~(((BitType)1)<<bitnumber))
#define BitsNot0(bs)	      		((bs)  !=  0)
#define BitCopy(bs1, bs2)		((bs1)  =  (bs2))
#define BitOrEq(bs1, bs2)		((bs1) |=  (bs2))
#define BitAndNot(bs1,bs2,bs3)		((bs1)  =  (bs2) &~ (bs3))
#define BitAndNotEq(bs1,bs2)		((bs1) &= ~(bs2))

int BitNext(BitType bs)
    /* returns -1 for no bits left */
    /* does not find "smallest" bitfirst!!! */
{
/* 
    int i;
    char *a;
    a = (char*) &bs;

    for (i=0; i<sizeof(BitType); i++) {
	if ( BitFirst[(int)a[i]] > -1 )
	    return(BitFirst[(int)a[i]]+8*i);
    }
    return(-1);
*/
    uint32_t x;

    if( (x = (bs & 0x000000ff)) )
	    return( BitFirst[ x >> 0 ] + 0 );
    if( (x = (bs & 0x0000ff00)) )
	    return( BitFirst[ x >> 8 ] + 8 );
    if( (x = (bs & 0x00ff0000)) )
	    return( BitFirst[ x >> 16 ] + 16);
    if( (x = (bs & 0xff000000)) )
	    return( BitFirst[ x >> 24 ] + 24);
    return( -1 );
}

void FindTransitiveClosure(int pattern, BitType *patterns,
	BitType *conflict_table)
{
    BitType	tested, to_test;

    BitSet0(*patterns);
    BitSet0(tested);
    BitSet0(to_test);
    BitSet(to_test,pattern);
    do {
	BitSet(tested,pattern);
	BitOrEq(*patterns,conflict_table[pattern]);
	BitAndNot(to_test,*patterns,tested);
	pattern = BitNext(to_test);
    } while (pattern != -1);
}

int Maximize(BitType open, BitType *conflict_table, int *penalties)
    /* This routine tries to maximize the set covering problem of the patterns
     * entered and returns the maximal penalty we can obtain
     * open: the patterns that can still be included without conflict */
{
    /* works like a tree search in which every node represents certain
     * patterns included in the penalty set */
    int max_penalty, penalty;
    int pattern;
    BitType new_open;

    max_penalty = 0;
    pattern = BitNext(open);
    while (pattern != -1) {
	    /* for each pattern left */
	    BitAndNot(new_open,open,conflict_table[pattern]);
	    penalty = Maximize(new_open,conflict_table,penalties)
		    + penalties[pattern];
	    if (penalty > max_penalty) max_penalty = penalty;
	    
	    BitUnSet(open,pattern);
	    pattern = BitNext(open);
    }
    return( max_penalty );
}

/* XXX */
int  GetPenaltyMaximize(CONFLICTS *c, BitString stones, PHYSID manpos )
{
	int	  peni;
	int       coni;
	int 	  total_penalty;
	int	  pattern;		/* pattern index */
	BitType   patterns_all;		/* all the patterns in a bitstring */
	BitType   patterns_left;	/* all the patterns in a bitstring */
	BitType   bpatterns;		/* tmp variable */

	int	  match_count;		/* number of patterns matched */
	BitString *patterns[BitSize];	/* all the matching patterns */
	int	  penalties[BitSize];	/* penalties to corresponding pattern */

	char	  conflict_count[BitSize];
					/* How many patterns conflict with
					 * this one */
	BitType	  conflict_table[BitSize];
					/* Bit pattern representing with which
					 * this pattern conflicts. */
BitString patternunion;
int sum_penalty;
#ifdef DYNAMICDB
static char t = 0;
#endif

	if (Options.lb_cf==0) return(0);

#ifdef DYNAMICDB
if( !t )
for( coni = 0; coni < _dbpatterns.number_conflicts; coni++ )
  if( IsBitSetBS( stones, _dbpatterns.cflts[ coni ].onestone ) &&
      AllBitsSetBS( stones, _dbpatterns.cflts[ coni ].conflict ) ) {
t = 1;
    CheckDBPattern( c, &_dbpatterns.cflts[ coni ] );
t = 0;
    memcpy( &_dbpatterns.cflts[ coni ],
	    &_dbpatterns.cflts[ --_dbpatterns.number_conflicts ],
	    sizeof( CFLT ) );
    break;
  }
#endif

	/* find all the patterns, save the stones bitmap in the patterns and the
	 * penalties in the penalties array. */
	match_count = 0;
	BitSet0(patterns_all);
	for (peni=0; peni<c->number_penalties; peni++) {
		for (coni=0; coni<c->pen[peni].number_conflicts; coni++) {
		    if ( IsBitSetBS( stones,
				     c->pen[peni].cflts[coni].onestone ) &&
			 !IsBitSetBS( c->pen[ peni ].cflts[ coni ].no_reach,
				      manpos ) &&
			 AllBitsSetBS(stones,
				      c->pen[peni].cflts[coni].conflict) ) {
			c->pen[peni].cflts[coni].hits++;
			if (c->pen[peni].penalty>=ENDPATH) {
				SR(Debug(CFLTS_PPLEVEL,0,
					"GetPenalty: Deadlock found\n"));
				return(ENDPATH);
			}
			penalties[match_count] = c->pen[peni].penalty;
			patterns[match_count]  = 
					&(c->pen[peni].cflts[coni].conflict);
			BitSet(patterns_all,match_count);
			match_count++;
if (match_count >= BitSize) goto ENDLOOP;
/*			Assert(match_count <= BitSize, "GetPenalty: Too many matching patterns! Increase BitSize!\n");
*/
		    }
		}
	}
ENDLOOP:
	/* Handle the trivia */
	if (match_count == 0) return(0);
	else if (match_count == 1) return(penalties[0]);
	else if (match_count == 2) {
	    if (LogAndBS(*(patterns[0]),*(patterns[1])))
		return(max(penalties[0],penalties[1]));
	    else return(penalties[0]+penalties[1]);
	}

	/* Prepare the conflict arrays, Zero only what is needed */
	memset(conflict_count,0,sizeof(char)*match_count);
	memset(conflict_table,0,sizeof(BitType)*match_count);
	for (peni=0; peni<match_count; peni++) {
		/* Set the pattern conflicting with itself in bit pattern */
		BitSet(conflict_table[peni],peni);
		for (coni=peni+1; coni<match_count; coni++) {
			/* See if those two conflict */
			if (LogAndBS(*(patterns[peni]),*(patterns[coni]))) {
			    /* They conflict */
			    BitSet(conflict_table[peni],coni);
			    BitSet(conflict_table[coni],peni);
			    conflict_count[peni]++;
			    conflict_count[coni]++;
			}
		}
	}

	/* Repeatedly, find transitive closure of stone packs, remember those
	 * already included to avoid circular references, start from
	 * beginning, pass those on to the maximizer */

	total_penalty = 0;
	BitCopy(patterns_left,patterns_all);
	while (BitsNot0(patterns_left)) {
		/* find next conflict */
		pattern = BitNext(patterns_left);
		if (conflict_count[pattern] == 0) {
		    /* simple penalty */
		    total_penalty += penalties[pattern];
		    BitUnSet(patterns_left,pattern);
		} else {
		    /* conflict pack */
		    FindTransitiveClosure(pattern,&bpatterns,conflict_table);
		    total_penalty += Maximize(bpatterns,
					      conflict_table,
					      penalties);
		    BitAndNotEq(patterns_left,bpatterns);
		}
	}
	SR(Debug(CFLTS_PPLEVEL,0,"GetPenalty: pen found: %i (s:%i)\n",
		total_penalty,match_count));
	if (total_penalty&1 && total_penalty!=ENDPATH) total_penalty++;
/*
peni = GetPenaltyOld(c, stones, manpos);
if ( IdaInfo == &MainIdaInfo && peni != total_penalty && total_penalty < 2000 && peni < 2000 ) {
        printf("Inc %d to %d at %" PRId32 "\n", peni, total_penalty, total_node_count);
        PrintBit2Maze(IdaInfo->IdaMaze,stones);
        for (i = 0; i<match_count; i++) {
            printf("%i gives %i\n", i, penalties[i]);
            PrintBit2Maze(IdaInfo->IdaMaze,*(patterns[i]));
        }
        Assert(( peni < total_penalty) || (total_penalty > 2000), "GetPEnalty: new GetP is smaller!\n");
}
   if( IdaInfo == &MainIdaInfo ) { 
     sum_penalty = 0; 
     for( peni = 0; peni < match_count; peni++ ) { 
       sum_penalty += penalties[ peni ]; 
     } 
     BitAndBS( patternunion, IdaInfo->IdaMaze->goal, IdaInfo->IdaMaze->stone );
     sum_penalty = sum_penalty - total_penalty - 
       ( NumberBitsBS( patternunion ) + 1 ) / 2;
     if( sum_penalty < 0 )
       sum_penalty = 0;
     return total_penalty + sum_penalty;
   } else 
*/
	return total_penalty;
}

