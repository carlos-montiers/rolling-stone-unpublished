#include "board.h"

/*******************************************************************************/
/* 									       */
/* 	MOVE will now include only stone pushes and is centered on that notion.*/
/*	This way from and to in MOVE and UNMOVE will have a different meaning  */
/* 	than in PATH							       */
/* 									       */
/*******************************************************************************/

MOVE BestMove;
MOVE LastMove;

#define SETMOVE(move,v,ifrom,ito)\
	(move).move_dist = 1; (move).from = ifrom; (move).last_over = ifrom;\
	(move).macro_id = 0; (move).to = ito; (move).value = v;

int DirToDiff[4] = {1,YSIZE,-1,-YSIZE};
int  OppDir[4] = {2,3,0,1};
int NextDir[4] = {1,2,3,0};
int PrevDir[4] = {3,0,1,2};

int GenerateMoves(MAZE *maze, MOVE *moves) 
{
	PHYSID pos;
	int i,dir,dist;
	int number_moves = 0;
	BitString stones_done;

	CopyBS(stones_done,maze->stones_done);
	for (i=0; i<maze->number_stones; i++) {
		pos = maze->stones[i].loc;
		if (  (maze->PHYSstone[pos]<0)
		    ||(IsBitSetBS(stones_done,pos)))
			continue;
		SetBitBS(stones_done,pos);
		for (dir=NORTH; dir<=WEST; dir++) {
			if( ( dist=IsBitSetBS( maze->reach,
					       pos+DirToDiff[dir]) )
			    && (IsBitSetBS(maze->S[OppDir[dir]],pos))
			    && (maze->PHYSstone[pos-DirToDiff[dir]]<0)) {
				SETMOVE(moves[number_moves],dist,
				        pos,pos-DirToDiff[dir]);
				if (!DeadLock(maze,moves[number_moves])) {
					if (SubMacro(maze,moves,&number_moves))
						goto END;
					if (!DeadLock2(maze,
						      &moves[number_moves]))
						number_moves++;
				}
			}		
		}		
	}
END:
	moves[number_moves].to = ENDPATH;
	moves[number_moves].from = ENDPATH;
	return(number_moves);
}

/*void SetMoveValue(MAZE *o_maze, MOVE *move)
{
	static MOVE moves[MAX_MOVES];
	static UNMOVE unmove;
	MAZE *maze;

	maze = CopyMaze(o_maze);
	MakeMove(maze,move,&unmove);
	move->value = GenerateMoves(maze,moves);
	DelCopiedMaze(maze);
}*/

static int compare_val(const void *m1, const void *m2) {
	return(((MOVE*)m2)->value - ((MOVE*)m1)->value);
}

static int compare_negval(const void *m1, const void *m2) {
/* secondary ordering by man move distance */
/* if( ( (MOVE *)m1 )->value == ( (MOVE *)m2 )->value && */
/*     !ISDUMMYMOVE( *(MOVE *)m1 ) && !ISDUMMYMOVE( *( MOVE *)m2 ) ) */
/*   return IdaInfo->IdaMaze->m_weights[ ( (MOVE *)m1 )->last_over ]->w */
/*     [ IdaInfo->IdaMaze->manpos ] - */
/*     IdaInfo->IdaMaze->m_weights[ ( (MOVE *)m2 )->last_over ]->w */
/*     [ IdaInfo->IdaMaze->manpos ]; */
	return(((MOVE*)m1)->value - ((MOVE*)m2)->value);
}

static int compare(const void *m1, const void *m2) {
#define INERTIA -50
#define BESTFIRST -100
	int v1,v2;
	v1=IdaInfo->IdaMaze->lbtable[IdaInfo->IdaMaze->PHYSstone[((MOVE*)m1)->from]].distance;
	v2=IdaInfo->IdaMaze->lbtable[IdaInfo->IdaMaze->PHYSstone[((MOVE*)m2)->from]].distance;
	if (EQMOVE( *((MOVE*)m1), BestMove)) v1=BESTFIRST;
	else if (((MOVE*)m1)->from==LastMove.to) v1=INERTIA;
	if (EQMOVE( *((MOVE*)m2), BestMove)) v2=BESTFIRST;
	else if (((MOVE*)m2)->from==LastMove.to) v2=INERTIA;
	return(v1-v2);
}

int NoMoveOrdering(int depth, int number_moves, MOVE bestmove) 
{
	return(number_moves);
}

int NewMoveOrdering(int depth, int number_moves, MOVE bestmove) 
{
	int  i,diff;
	IDAARRAY *S;
	MAZE *maze;
	MOVE *m,*lmove;
	PHYSID goalpos;
	GROOM *groom;
	int e, d, dist;

	if (number_moves>1) {
 		lmove = depth?&(IdaInfo->IdaArray[depth-1].currentmove)
				:&DummyMove;
		S = &(IdaInfo->IdaArray[depth]);
		maze = IdaInfo->IdaMaze;
		for (i=0; i<number_moves; i++) {
			m = &(S->moves[i]);
			if (ISDUMMYMOVE(*m)) {
				m->value = -ENDPATH;
				continue;
			}
			/* Use the closest entrance to the goalll area the
			 * stone is in (if) as goalpos */
			goalpos = maze->goals[
				    maze->lbtable[
				      maze->PHYSstone[m->from]].goalidx].loc;
			if (maze->groom_index[m->from] >= 0 ) {
			    dist  = GetWeight(maze,goalpos,m->from);
			} else if (maze->groom_index[goalpos] >= 0 ) {
			    groom = maze->grooms + maze->groom_index[goalpos];
			    dist  = GetWeight(maze,goalpos,m->from);
			    for (e = 0; e < groom->n; e++) {
				d = GetWeight(maze,groom->locations[e],m->from);
				if (dist > d) {
					dist = d;
					goalpos = groom->locations[e];
				}
			    }
			} else {
			    dist = ENDPATH;
			    for (e = 0; e < maze->number_goals; e++) {
				d = GetWeight(maze,maze->goals[e].loc,m->from);
				if (dist > d) {
					dist = d;
					goalpos = maze->goals[e].loc;
				}
			    }
			}
			m->value = dist;

			if (m->macro_id != 4) diff = m->value - 
			      GetWeightManpos(maze,goalpos,m->to,m->from);
			else diff = m->value;
			if (diff>0) {
				if (lmove->to == m->from) 
					m->value -= ENDPATH;
			} else {
				m->value -= diff*100;
			}
		}
		My_qsort(&(S->moves),number_moves,sizeof(MOVE),compare_negval);
/*
		if (IdaInfo == &MainIdaInfo) {
			printf("============================================\n");
			PrintMaze(IdaInfo->IdaMaze);
			for (i=0; i<number_moves; i++) {
				printf("%s\n",PrintMove(S->moves[i]));
				PosNr = S->moves[i].from;
				PrintMaze(IdaInfo->IdaMaze);
				PosNr = 0;
			}
		}
*/
	}
	return(number_moves);
}

int OldNewMoveOrdering(int depth, int number_moves, MOVE bestmove) 
{
	int  i,diff;
	IDAARRAY *S;
	MAZE *maze;
	MOVE *m,*lmove;
	PHYSID goalpos;

	if (number_moves>1) {
 		lmove = depth?&(IdaInfo->IdaArray[depth-1].currentmove)
				:&DummyMove;
		S = &(IdaInfo->IdaArray[depth]);
		maze = IdaInfo->IdaMaze;
		for (i=0; i<number_moves; i++) {
			m = &(S->moves[i]);
			if (ISDUMMYMOVE(*m)) {
				m->value = -ENDPATH;
				continue;
			}
			goalpos = maze->goals[
				    maze->lbtable[
				      maze->PHYSstone[m->from]].goalidx].loc;
			m->value =
			      maze->lbtable[maze->PHYSstone[m->from]].distance;
			if (m->macro_id != 4) diff = m->value - 
			      GetWeightManpos(maze,goalpos,m->to,m->from);
			else diff = m->value;
			if (diff>0) {
				if (lmove->to == m->from) 
					m->value -= ENDPATH;
			} else {
				m->value -= diff*100;
			}
		}
		My_qsort(&(S->moves),number_moves,sizeof(MOVE),compare_negval);
	}
	return(number_moves);
}
int BestMoveOrdering(int depth, int number_moves, MOVE bestmove) 
{
	IDAARRAY *S = &(IdaInfo->IdaArray[depth]);
	MOVE tmove,lmove;
	int  placed = 0;
	int  i;
	if(!ISDUMMYMOVE(bestmove)) for(i=placed+1;i<number_moves;i++) {
		if (EQMOVE(bestmove,S->moves[i])) {
			S->moves[i] = S->moves[placed];
			S->moves[placed] = bestmove;
			placed++;
			break;
		}
	}
	if (depth == 0) return(number_moves);
	lmove = IdaInfo->IdaArray[depth-1].currentmove;
	for (i=placed; i<number_moves; i++) {
		if (lmove.to == S->moves[i].from) {
			/* could be continuation CAN'T exclude reverse 
		 	 * move from last move since manpos is 
			 * different then before (side of the stone!!*/
			if (lmove.from != S->moves[i].to) {
				/* a real continuation */
				tmove = S->moves[i];
				S->moves[i] = S->moves[placed];
				S->moves[placed] = tmove;
				placed++;
			}
		}
	}
	return(number_moves);
}
int InertiaMoveOrdering(int depth, int number_moves, MOVE bestmove) 
{
	if (number_moves>1) {
		BestMove = bestmove;
		LastMove = depth==0?DummyMove
				   :IdaInfo->IdaArray[depth].currentmove;
		My_qsort(&(IdaInfo->IdaArray[depth].moves),
		      number_moves,sizeof(MOVE),compare);
	}
	return(number_moves);
}
int ManDistMoveOrdering(int depth, int number_moves, MOVE bestmove) 
{
	if (number_moves>1) {
		My_qsort(&(IdaInfo->IdaArray[depth].moves),
			number_moves,sizeof(MOVE),compare_val);
	}
	return(number_moves);
}

int MakeMove(MAZE *maze, MOVE *move, UNMOVE *ret)
/* this is a routine that makes a STONE move, not merely a man move like
 * DoMove. It will just put the man to the new location */
{
	int goali, i;
	GMNODE **gmtree=NULL;

	/* if stone is moved into a new structure and 
	   if structure can't carry more stones abort */
	if (   (maze->Phys[move->to].lstruct != maze->Phys[move->from].lstruct)
	    &&((maze->structs[maze->Phys[move->to].lstruct].number_stones >=
	       (maze->structs[maze->Phys[move->to].lstruct].max_number_stones))))
		return(0);

	ret->manfrom  = maze->manpos;
	ret->stonefrom= move->from;
	ret->stoneto  = move->to;
	ret->macro_id = move->macro_id;
	ret->move_dist= move->move_dist;
	CopyBS(ret->old_no_reach,maze->old_no_reach);
	ret->old_GMTree = NULL;
	maze->goal_sqto = -1;

	if (maze->groom_index[move->to]>=0 && Options.mc_gm==1) {
		gmtree = &(maze->gmtrees[maze->groom_index[move->to]]);
		if (move->macro_id == 4) {
			/* this is the case when the goal macro worked */
			ret->old_GMTree = *gmtree;
	
			SetBitBS(maze->stones_done,move->to);
			/* set GMTree to next level down */
			goali = maze->Phys[move->to].goal;
			SR(Assert(goali>=0,
				"MakeMove: Goal Macro to non-goal!\n"));
			i = 0;
			while (   (i<(*gmtree)->number_entries)
		       	&& ((*gmtree)->entries[i].goal!=goali)) i++;
			if (i<(*gmtree)->number_entries) {
				(*gmtree) = (*gmtree)->entries[i].next;
			} else {
				(*gmtree) = NULL;
			}
		} else  {
			/* we made a non-scheduled move in the goal area, 
			  stop using goal macros! Turn off maze->gmtrees[x] 
			  to signal that! */
			if (   (*gmtree) != NULL
			    && (*gmtree)->number_entries>0) return(0);
			Options.mc_gm = 0;
			ret->old_GMTree = *gmtree;
			(*gmtree) = NULL;
		}
	}
	MANTO(maze,move->last_over);
	STONEFROMTO(maze,ret->stonefrom,ret->stoneto);
	UpdateHashKey(maze, ret);
	CopyBS( ret->save_reach, maze->reach );
	CopyBS( ret->save_no_reach, maze->no_reach );

#ifdef STONEREACH
	if( IdaInfo->depth <= 30 )
	for( i = 0; i < maze->number_stones; i++ )
	  CopyBS( ret->stone_reach[ i ], maze->stone_reach[ i ] );
	SetStoneReach( maze );
#endif

/* if( !move->macro_id )
  UpdateReach( maze, ret->stoneto );
else */
  MarkReach( maze );
	BetterUpdateLowerBound(maze, ret);

	SR(Assert(maze->manpos>0,"MakeMove: manpos < 0!\n"));
	maze->currentmovenumber++;
	maze->g += ret->move_dist;
	return(1);
}

int UnMakeMove(MAZE *maze, UNMOVE *unmove)
{
	GMNODE **gmtree;
	int      stonei;
#ifdef STONEREACH
	int      i;
#endif
	int	 new_h;

	new_h = maze->h;
	MANTO(maze,unmove->manfrom);
	STONEFROMTO(maze,unmove->stoneto,unmove->stonefrom);

	if (unmove->old_GMTree != NULL) {
		Options.mc_gm = 1;
		gmtree = &(maze->gmtrees[maze->groom_index[unmove->stoneto]]);
		(*gmtree) = unmove->old_GMTree;
	}
	stonei = maze->PHYSstone[unmove->stonefrom];

	UnsetBitBS(maze->stones_done,unmove->stoneto);
	UnsetBitBS(maze->stones_done,unmove->stonefrom);

	UpdateHashKey(maze, unmove);
	/* MarkReach(maze); */
	CopyBS( maze->reach, unmove->save_reach );
	CopyBS( maze->no_reach, unmove->save_no_reach );
	CopyBS(maze->old_no_reach,unmove->old_no_reach);
#ifdef STONEREACH
	for( i = 0; i < maze->number_stones; i++ )
	  CopyBS( maze->stone_reach[ i ], unmove->stone_reach[ i ] );
#endif
	BetterUpdateLowerBound2(maze, unmove);

	if (  ((unmove->macro_id==4) || (maze->goal_sqto==unmove->stoneto))
	    &&(maze->h - unmove->move_dist == new_h    /* optimal move */)) {
		/* This is either the start or a continuation of a goal move */
		maze->goal_sqto = unmove->stonefrom;
	} else maze->goal_sqto = -1;

	maze->currentmovenumber--;
	maze->g -= unmove->move_dist;
	return(0);
}

int DistToGoal(MAZE *maze, PHYSID start, PHYSID goal, PHYSID *last_over) {
/* pseudo-recursive function to see how many pushes are needed to get to
 * goal */

        static PHYSID stack[XSIZE*YSIZE];
        static PHYSID from[XSIZE*YSIZE];
        static PHYSID dist[XSIZE*YSIZE];
	static int s_visited[4][XSIZE*YSIZE];
        PHYSID pos,fro,old_stone,old_man;
        int goal_dist, next_in, next_out, dir, dir1, dis;

	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		for (dir=NORTH; dir<=WEST; dir++) {
			s_visited[dir][pos]=-1;
		}
	}

	/* set initial from to a position immediate to "start" on the right side */
	for (dir=NORTH; dir<=WEST; dir++) {
		if (IsBitSetBS(maze->reach,start+DirToDiff[dir])) {
			from[0]  = start+DirToDiff[dir];
			break;
		}
	}
	old_man  = maze->manpos;
        stack[0] = start;
	dist[0]  = 0;
	old_stone= maze->PHYSstone[start];
	maze->PHYSstone[start]=-1;
        next_in  = 1;
        next_out = 0;
	goal_dist = -1;
        while (next_out < next_in) {
		fro = from[next_out];
                pos = stack[next_out];
                dis = dist[next_out++];
                if (maze->PHYSstone[pos] >= 0) 
			continue;
		if (pos==goal) {
			*last_over = fro;
			goal_dist = dis;
			break;
		}
		dir = DiffToDir(fro-pos);
		if (s_visited[dir][pos] > 0) continue;

		/* set maze up to make the reach analysis for the man */
		MANTO(maze,fro);
		AvoidThisSquare = pos;
		MarkReach(maze);

		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->reach,pos+DirToDiff[dir]))
				s_visited[dir][pos] = dis;
		}
		for (dir=NORTH; dir<=WEST; dir++) {
                	if (   IsBitSetBS(maze->S[dir],pos)
			    && IsBitSetBS(maze->reach, pos-DirToDiff[dir])) {
				from[next_in] = pos;
				dist[next_in] = dis + 1;
				stack[next_in++] = pos+DirToDiff[dir];
			}
		}
		dis = -1;
        }
	MANTO(maze,old_man);
	maze->PHYSstone[start]=old_stone;
	AvoidThisSquare = 0;
	MarkReach(maze);
	return(goal_dist);
}

void Moves(MAZE *maze, PHYSID *from, signed char *reach )
{
  static PHYSID stack[ ENDPATH ];
  static PHYSID f[ ENDPATH ];
  PHYSID pos;
  int next_in, next_out, dir;

  memset( reach, -1, XSIZE * YSIZE );
  stack[ 0 ] = maze->manpos;
  f[ 0 ] = 0;
  from[ 0 ] = -1;
  next_in  = 1;
  next_out = -1;
  while( ++next_out < next_in ) {
    pos = stack[ next_out ];
    if( reach[ pos ] >= 0 ) continue;
    if( maze->PHYSstone[ pos ] >= 0 ) continue;
    if( AvoidThisSquare == pos ) continue;
    reach[ pos ] = reach[ from[ pos ] = f[ next_out ] ] + 1;
    for( dir = NORTH; dir <= WEST; dir++ )
      if( IsBitSetBS( maze->M[ dir ], pos ) ) {
	f[ next_in ] = pos;
	stack[ next_in++ ] = pos + DirToDiff[ dir ];
      }
  }
}

/* generate a path that does not touch any of the squares in shadows */
void Moves2(MAZE *maze, PHYSID *from, signed char *reach, BitString shadows )
{
  static PHYSID stack[ ENDPATH ];
  static PHYSID f[ ENDPATH ];
  PHYSID pos;
  int next_in, next_out, dir;

  memset( reach, -1, XSIZE * YSIZE );
  stack[ 0 ] = maze->manpos;
  f[ 0 ] = 0;
  from[ 0 ] = -1;
  next_in  = 1;
  next_out = -1;
  while( ++next_out < next_in ) {
    pos = stack[ next_out ];
    if( reach[ pos ] >= 0 ) continue;
    if( maze->PHYSstone[ pos ] >= 0 ) continue;
    if( AvoidThisSquare == pos ) continue;
    if( IsBitSetBS( shadows, pos ) ) continue;
    reach[ pos ] = reach[ from[ pos ] = f[ next_out ] ] + 1;
    for( dir = NORTH; dir <= WEST; dir++ )
      if( IsBitSetBS( maze->M[ dir ], pos ) ) {
	f[ next_in ] = pos;
	stack[ next_in++ ] = pos + DirToDiff[ dir ];
      }
  }
}

void GenAllSquares( PHYSID pos, PHYSID *from, BitString all_squares )
{
  Set0BS( all_squares );
  SetBitBS( all_squares, pos );
  while( from[ pos ] > 0 ) {
    pos = from[ pos ];
    SetBitBS( all_squares, pos );
  }
}

void PushesMoves(MAZE *maze, PHYSID start, PHYSID goal, 
		 int *pushes, int *moves, 
		 BitString stone_squares, BitString man_squares)
{

  static PHYSID stack[XSIZE*YSIZE];
  static PHYSID from[XSIZE*YSIZE];
  static PHYSID mand[XSIZE*YSIZE];
  static PHYSID n_pu[XSIZE*YSIZE];
  static PHYSID n_mo[XSIZE*YSIZE];
  static PHYSID movefrom[ XSIZE * YSIZE ];
  static BitString s_squares[XSIZE*YSIZE];
  static BitString m_squares[XSIZE*YSIZE];
  static BitString all_square;
  static int s_visited[XSIZE*YSIZE];
  static signed char reach[ XSIZE * YSIZE ];
  PHYSID pos,fro,old_stone,old_man;
  int next_in, next_out, dir;

  memset( s_visited, 0, sizeof( int ) * XSIZE * YSIZE );
  memset( n_pu, 0, sizeof( PHYSID ) * XSIZE * YSIZE );
  memset( n_mo, 0, sizeof( PHYSID ) * XSIZE * YSIZE );

  old_man  = maze->manpos;
  from[0]  = maze->manpos;
  n_pu[maze->manpos]  = -1;
  n_mo[maze->manpos]  = -1;
  stack[0] = start;
  mand[0]  = 0;
  Set0BS(s_squares[0]);
  old_stone= maze->PHYSstone[start];
  maze->PHYSstone[start]=-1;
  next_in  = 1;
  next_out = 0;
  AvoidThisSquare = start;
  while (next_out < next_in) {
    fro = from[next_out];
    pos = stack[next_out++];
    if (maze->PHYSstone[pos] >= 0) 
      continue;
    if (s_visited[pos] ) continue;
    s_visited[pos] = 1;
    n_pu[pos]      = n_pu[fro]+1;
    n_mo[pos]      = n_mo[fro]+ mand[next_out-1] +1;
    if (pos==goal) break;

    /* set maze up to make the reach analysis for the man */
    MANTO(maze,fro);
    AvoidThisSquare = pos;
    Moves(maze,movefrom,reach);

    for (dir=NORTH; dir<=WEST; dir++) {
      if (   IsBitSetBS(maze->S[dir],pos)
	     && reach[pos-DirToDiff[dir]]>=0) {
	mand[next_in] 
	  = reach[pos-DirToDiff[dir]];
	CopyBS(m_squares[next_in],
	       m_squares[next_out-1]);
	GenAllSquares( pos - DirToDiff[ dir ], movefrom, all_square );
	BitOrEqBS( m_squares[next_in], all_square );
	from[next_in] = pos;
	CopyBS(s_squares[next_in],
	       s_squares[next_out-1]);
	SetBitBS(s_squares[next_in],pos);
	stack[next_in++] = pos +DirToDiff[dir];
      }
    }
  }
  MANTO(maze,old_man);
  maze->PHYSstone[start]=old_stone;
  AvoidThisSquare = 0;
  MarkReach(maze);
  *pushes = n_pu[goal];
  *moves  = n_mo[goal];
  CopyBS(stone_squares,s_squares[next_out-1]);
  SetBitBS(stone_squares,pos);
  CopyBS(man_squares,m_squares[next_out-1]);
  SetBitBS(man_squares,fro);
}

void PushesMoves2(MAZE *maze, PHYSID start, PHYSID goal, 
		 int *pushes, int *moves, 
		 BitString stone_squares, BitString man_squares)
{
  static PHYSID stack[XSIZE*YSIZE];
  static PHYSID from[XSIZE*YSIZE];
  static PHYSID mand[XSIZE*YSIZE];
  static PHYSID n_pu[XSIZE*YSIZE];
  static PHYSID n_mo[XSIZE*YSIZE];
  static PHYSID movefrom[ XSIZE * YSIZE ];
  static BitString s_squares[XSIZE*YSIZE];
  static BitString m_squares[XSIZE*YSIZE];
  static BitString all_square;
  static int s_visited[XSIZE*YSIZE];
  static signed char reach[ XSIZE * YSIZE ];
  PHYSID pos,fro,old_stone,old_man;
  int next_in, next_out, dir;

  memset( s_visited, 0, sizeof( int ) * XSIZE * YSIZE );
  memset( n_pu, 0, sizeof( PHYSID ) * XSIZE * YSIZE );
  memset( n_mo, 0, sizeof( PHYSID ) * XSIZE * YSIZE );

  old_man = maze->manpos;
  from[ 0 ] = maze->manpos;
  n_pu[ maze->manpos ] = -1;
  n_mo[ maze->manpos ] = -1;
  stack[ 0 ] = start;
  mand[ 0 ] = 0;
  Set0BS( s_squares[ 0 ] );
  old_stone = maze->PHYSstone[ start ];
  maze->PHYSstone[ start ] = -1;
  next_in = 1;
  next_out = 0;
  AvoidThisSquare = start;
  while( next_out < next_in ) {
    fro = from[ next_out ];
    pos = stack[ next_out++ ];
    if( maze->PHYSstone[ pos ] >= 0 )
      continue;
/*     if( s_visited[ pos ] ) continue; */
/*     s_visited[ pos ] = 1; */
    n_pu[ pos ] = n_pu[ fro ] + 1;
    n_mo[ pos ] = n_mo[ fro ] + mand[ next_out - 1 ] + 1;
    if( pos == goal )
      break;

    /* set maze up to make the reach analysis for the man */
    MANTO( maze, fro );
    AvoidThisSquare = pos;
    Moves2( maze, movefrom, reach, IdaInfo->shadow_stones );

    /* try to avoid shadow stones */
    for( dir = NORTH; dir <= WEST; dir++ ) {
      if( IsBitSetBS( maze->S[ dir ], pos ) &&
	  reach[ pos - DirToDiff[ dir ] ] >=0 &&
	  !s_visited[ pos + DirToDiff[ dir ] ] ) {
	mand[ next_in ] = reach[ pos - DirToDiff[ dir ] ];
	CopyBS( m_squares[ next_in ], m_squares[ next_out - 1 ] );
	GenAllSquares( pos - DirToDiff[ dir ], movefrom, all_square );
	BitOrEqBS( m_squares[ next_in ], all_square );
	from[ next_in ] = pos;
	CopyBS( s_squares[ next_in ], s_squares[ next_out - 1 ] );
	SetBitBS( s_squares[ next_in ], pos );
	stack[ next_in++ ] = pos + DirToDiff[ dir ];
	s_visited[ pos + DirToDiff[ dir ] ] = 1;
      }
    }

    /* now try without shadow stones */
    Moves( maze, movefrom, reach );

    for( dir = NORTH; dir <= WEST; dir++ ) {
      if( IsBitSetBS( maze->S[ dir ], pos ) &&
	  reach[ pos - DirToDiff[ dir ] ] >=0 &&
	  !s_visited[ pos ] ) {
	mand[ next_in ] = reach[ pos - DirToDiff[ dir ] ];
	CopyBS( m_squares[next_in], m_squares[ next_out - 1 ] );
	GenAllSquares( pos - DirToDiff[ dir ], movefrom, all_square );
	BitOrEqBS( m_squares[next_in], all_square );
	from[next_in] = pos;
	CopyBS( s_squares[ next_in ], s_squares[ next_out - 1 ] );
	SetBitBS( s_squares[ next_in ], pos );
	stack[ next_in++ ] = pos + DirToDiff[ dir ];
	s_visited[ pos + DirToDiff[ dir ] ] = 1;
      }
    }
  }
  MANTO( maze, old_man );
  maze->PHYSstone[ start ] = old_stone;
  AvoidThisSquare = 0;
  MarkReach( maze );
  *pushes = n_pu[ goal ];
  *moves = n_mo[ goal ];
  CopyBS( stone_squares, s_squares[ next_out - 1 ] );
  SetBitBS( stone_squares, pos );
  CopyBS( man_squares, m_squares[ next_out - 1 ] );
  SetBitBS( man_squares, fro );
}

int ValidSolution(MAZE *maze, MOVE *solution) {

	int number_pushes, number_moves, i;
	int p,m;
	UNMOVE unmove;
	BitString stone,man;

	number_pushes = number_moves = 0;
	i = 0;
	while (!ISDUMMYMOVE(solution[i])) {
		p = m = 0;
		PushesMoves(maze,solution[i].from,solution[i].to,
			    &p,&m,stone,man);
		number_pushes += p;
		number_moves  += m;
		MakeMove(maze,&(solution[i]),&unmove);
		i++;
	}
	Mprintf(0,"Moves: %i, Pushes: %i, Treedepth: %i\n",
		number_moves,number_pushes,i);
	if (maze->h!=0) 
		return(0);
	else return(number_pushes);

}


int DiffToDir(int diff)
{
	switch (diff) {
	case -1:
		return(SOUTH);
	case  1:
		return(NORTH);
	case -YSIZE:
		return(WEST);
	case YSIZE:
		return(EAST);
	default: 
		return(NODIR);
	}
}


