#include "board.h"

void MarkAll(MAZE *maze) {

SR(Debug(2,0,"MarkOut\n"));
	MarkOut(maze,maze->manpos);
SR(Debug(2,0,"MarkReach\n"));
	MarkReach(maze);
SR(Debug(2,0,"MarkDead\n"));
	MarkDead(maze);
SR(Debug(2,0,"MarkOne\n"));
	MarkOne(maze);
SR(Debug(2,0,"MarkTun\n"));
	MarkTun(maze);
SR(Debug(2,0,"MarkStruct\n"));
	MarkStruct(maze);
}

PHYSID   AvoidThisSquare=0;

void MarkReachNeeded(MAZE *maze, PHYSID manpos, PHYSID c_pos, PHYSID test) {
/* pseudo-recursive function to mark the fields that are reachable assuming
 * the two stones on posibions c_pos and test */
/* since we need only the reachablity around c_pos, stop after determining
 * */
	
	static PHYSID stack[ENDPATH];
	PHYSID pos;
	int next_in, next_out,dir;
	CleanReach(maze);

	stack[0] = manpos;
	next_in  = 1;
	next_out = 0;
	while (next_out < next_in) {
		pos = stack[next_out++];
		if (IsBitSetBS(maze->reach, pos )) continue;
		if (c_pos==pos || test ==pos) continue;
		SetBitBS(maze->reach, pos);
/* 		maze->PHYSreach[ pos ] = 1; */
		SetBitBS(maze->reach,pos);
		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->M[dir],pos)) 
				stack[next_in++] = pos +DirToDiff[dir];
		}
	}
}

#ifdef NEEDED
int GetNeeded(MAZE *maze, BitString needed, PHYSID from, PHYSID to) {
/* Determines which squares the stone must pass over to get to "to" */
	
	static PHYSID stack[ENDPATH];
	static PHYSID sfrom[ENDPATH];
	static char   visited[XSIZE*YSIZE];
	PHYSID pos,test_square,prev;
	int    from_dir,diff,dir;
	int    next_in, next_out;

	if (GetOptWeight(maze,to,from,NODIR)>=ENDPATH) {
		Set1BS(needed);
		return(1);
	}
	Set0BS(needed);
	SetBitBS(needed,from);
	SetBitBS(needed,to);
	for (test_square=1; test_square<YSIZE*XSIZE; test_square++) {
		if (IsBitSetBS(maze->out,test_square)) continue;
		if (IsBitSetBS(maze->wall,test_square)) continue;
		if (IsBitSetBS(maze->dead,test_square)) continue;
		if (test_square==from) continue;
		if (test_square==to) continue;


		/* the stack contains reachable stone locations */
		next_out = 0;
		next_in = 0;
		memset(visited,0,sizeof(char)*YSIZE*XSIZE);
		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->S[dir],from)) {
				sfrom[next_in]   = from;
				stack[next_in++] = from+DirToDiff[dir];
			}
		}
		pos = prev = from_dir = 0;
		while (next_out < next_in) {
			prev = sfrom[next_out];
			pos = stack[next_out++];
			/* goal ("to") reached? */
			if (pos == to) break;
			/* avoid test_square */
			if (pos == test_square) continue;
			if (IsBitSetBS(maze->dead,pos)) continue;
			/* if this was visited already skip */
			diff = pos - prev;
			if (diff==1)           from_dir = SOUTH;
			else if (diff==YSIZE)  from_dir = WEST;
			else if (diff==-1)     from_dir = NORTH;
			else if (diff==-YSIZE) from_dir = EAST;
			if (visited[pos]&(1<<from_dir)) continue;

			MarkReachNeeded(maze, prev, pos, test_square);
			/* else propagate stone to neighbores */
			/* and mark as visited */
			for (dir=NORTH; dir<=WEST; dir++) {
				if (   (   prev!=pos-DirToDiff[dir] 
					|| IsBitSetBS(maze->one_way,prev))
			    	    && (  ConnectedDir(maze,pos,from_dir,dir)
					||!IsBitSetBS(maze->one_way,pos))
			            && IsBitSetBS(maze->reach,
						  pos+DirToDiff[dir])) {
					visited[pos] |= 1<<dir;
					if (IsBitSetBS(maze->S[OppDir[dir]],
						       pos)) {
						sfrom[next_in]   = pos;
						stack[next_in++] = 
							pos-DirToDiff[dir];
					}
				}
			}
		}
		/* if goal ("to") was NOT found this was needed */
		if (pos != to) SetBitBS(needed,test_square);
	}
	return(0);
}

void MarkNeeded(MAZE *maze) {
	BitString needed;
	int i,j;

	SR(Debug(2,0,"MarkNeeded\n"));
	/* loop on every square IN maze */
for (i=1; i<YSIZE*XSIZE; i++) {
	Set0BS(maze->needed[i]);
}
Mprintf( 0, "Skip MarkNeeded\n"); return;
	for (i=1; i<YSIZE*XSIZE; i++) {
		Set0BS(maze->needed[i]);
		if (IsBitSetBS(maze->out,i))  continue;
		if (IsBitSetBS(maze->wall,i)) continue;
		if (IsBitSetBS(maze->dead,i)) continue;
		if (maze->Phys[i].goal>=0)   continue;

		Set1BS(maze->needed[i]);
		for (j=0; j<maze->number_goals; j++) {
			GetNeeded(maze,needed,i,maze->goals[j].loc);
			BitAndEqBS(maze->needed[i],needed);
			if (Is0BS(maze->needed[i])) break;
		}
	}
}
#endif

void MarkTG(MAZE *maze) {
/* mark to goal, put the square to which all the goals are */
	int    i,j,k,count,dir1,dir2,dir3;
	TOGOAL to_goals[XSIZE*YSIZE];
	PHYSID goalloc,togoal;
	WEIGHT w;
	BitString zero, ones, stones;

	for (i=0; i<XSIZE*YSIZE; i++) {
		/* test for each goal, if it is in the same direction */
		to_goals[i].fw      = 0;
		to_goals[i].bw      = 0;
		if (!IsBitSetBS(maze->out,i)) {
			for (j=0; j<maze->number_goals; j++) {
				goalloc = maze->goals[j].loc;
				w = GetOptWeight(maze,goalloc,i,NODIR);
				count = 0;
				togoal = 0;
				for (dir1=NORTH; dir1<=WEST; dir1++) {
					if (   IsBitSetBS(maze->S[dir1],i)
				            &&(w-1==GetOptWeight(maze,goalloc,
							i+DirToDiff[dir1],
							OppDir[dir1]))) {
				    		count++;
				    		togoal = i+DirToDiff[dir1];;
					}
				}
				if (count!=1||(j>0&&to_goals[i].fw!=togoal)) {
				    to_goals[i].fw = -ENDPATH;
				    break;
				} else {
				    to_goals[i].fw = togoal;
				}
			}
		} else {
			to_goals[i].fw = -ENDPATH;
		}
	}
	for (i=0; i<XSIZE*YSIZE; i++) {
		/* remove those that are against walls that would be
		 * deadlocks anyways */
		if (IsBitSetBS(maze->out ,i)
		    ||IsBitSetBS(maze->wall, i)) {
			continue;
		}
		j = to_goals[i].fw;
		if (j != -ENDPATH) {
			if (j<0) j=-j;
			dir1 = j-i;
			k = to_goals[j].fw;
			if (k != -ENDPATH) {
				if (k<0) k=-k;
				dir2 = k - j;
				if (dir1 != dir2)
				     to_goals[i].fw=-to_goals[i].fw;
			} else {
				to_goals[i].fw = -to_goals[i].fw;
			}
                        if (abs(dir1) == YSIZE) dir2=1;
                        else dir2=YSIZE;
                        if( (  (IsBitSetBS(maze->wall,i+dir2))
                             ||(IsBitSetBS(maze->wall,i-dir2)))
                           &&( (IsBitSetBS(maze->wall,j+dir2))
                             ||(IsBitSetBS(maze->wall,j-dir2)))) {
                                to_goals[i].fw = -to_goals[i].fw;
                        }
		}
		to_goals[i].bw = -ENDPATH;
	}
	for (i=0; i<XSIZE*YSIZE; i++) {
		/* chain th fw and bw of adjasent squares */
		if (IsBitSetBS(maze->out,i)
		    ||IsBitSetBS(maze->wall,i)) {
			continue;
		}
		j = to_goals[i].fw;
		if (j>0) {
			to_goals[j].bw = (PHYSID)i;
		}
		j = to_goals[i].bw;
		if (j>0) {
			to_goals[j].fw = (PHYSID)i;
		}
	}
	for (i=0; i<XSIZE*YSIZE; i++) {
		/* turn off those articulation squares like #2 low left that
		 * are already penalized by backout heuristic */
		if (IsBitSetBS(maze->out,i) 
		    ||IsBitSetBS(maze->wall,i)) {
			continue;
		}
		if (IsBitSetBS(maze->one_way,i)) {
			/* which direction is the chain */
			if (maze->Phys[i].free<4) continue;
			if ((j=to_goals[i].fw)>0) {
				dir1 = DiffToDir(j-i);
				/* all other dirs must be connected */
				for (dir2=NORTH;dir2<=WEST;dir2++) {
					dir3 = NextDir[dir2];
					if (  dir2!=dir1
					    &&dir3!=dir1
					    &&!ConnectedDir(maze,i,dir2,dir3)) {
						to_goals[i].fw=-ENDPATH;
						to_goals[j].bw=-ENDPATH;
					}
				}
			} else if ((j=to_goals[i].bw)>0) {
				dir1 = DiffToDir(j-i);
				/* all other dirs must be connected */
				for (dir2=NORTH;dir2<=WEST;dir2++) {
					dir3 = NextDir[dir2];
					if (  dir2!=dir1
					    &&dir3!=dir1
					    &&!ConnectedDir(maze,i,dir2,dir3)) {
						to_goals[i].bw=-ENDPATH;
						to_goals[j].fw=-ENDPATH;
					}
				}
			}
		}
	}
	
	Set0BS(zero);
	Set1BS(ones);
	for (i=0; i<XSIZE*YSIZE; i++) {
		/* finally, seed the conflict table */
		if (IsBitSetBS(maze->out,i) 
		    ||IsBitSetBS(maze->wall,i)) {
			continue;
		}
		/* find the beginning of a chain */
		if (  (to_goals[i].fw>0)
		    &&(to_goals[i].bw<=0)) {
			j=i;
			do {
				Set0BS(stones);
				SetBitBS(stones,j);
				SetBitBS(stones,to_goals[j].fw);
				AddConflict(maze->conflicts,
					stones,zero,ones,2,0);
				j = to_goals[j].fw;
			} while (to_goals[j].fw>0);
		}
	}
}

void MarkOut(MAZE *maze, PHYSID pos) {
/* recursive function to unmark the inner fields from the out-flag */
	int dir;
	
	if (!IsBitSetBS(maze->out,pos)) return;
	UnsetBitBS(maze->out,pos);
	for (dir=NORTH; dir<=WEST; dir++) {
		if (IsBitSetBS(maze->M[dir],pos)) 
			MarkOut(maze, pos+DirToDiff[dir]);
	}
}

void CleanReach(MAZE *maze) {
	Set0BS(maze->reach);
}

void MarkReach(MAZE *maze) {
  /* recursive function to mark the fields that are reachable */
	
  static PHYSID stack[ENDPATH];
  PHYSID pos;
  int top;

  Set0BS(maze->reach);

  stack[0] = maze->manpos;
  top = 1;
  while( top ) {
    pos = stack[ --top ];
    if( IsBitSetBS( maze->reach, pos) ) continue;
    if( maze->PHYSstone[ pos ] >= 0 ) continue;
    if( AvoidThisSquare == pos ) continue;

    SetBitBS( maze->reach,pos );

    if( IsBitSetBS( maze->M[ 0 ], pos ) )
      stack[ top++ ] = pos + 1;
    if( IsBitSetBS( maze->M[ 1 ], pos ) )
      stack[ top++ ] = pos + YSIZE;
    if( IsBitSetBS( maze->M[ 2 ], pos ) )
      stack[ top++ ] = pos - 1;
    if( IsBitSetBS( maze->M[ 3 ], pos ) )
      stack[ top++ ] = pos - YSIZE;
  }
  CopyBS(maze->old_no_reach,maze->no_reach);
  BitNotAndNotAndNotBS(maze->no_reach,maze->reach,maze->out,maze->stone);
}

void MarkReachNoUnreach(MAZE *maze) {
  /* recursive function to mark the fields that are reachable */
	
  static PHYSID stack[ENDPATH];
  PHYSID pos;
  int top;

  Set0BS(maze->reach);

  stack[0] = maze->manpos;
  top = 1;
  while( top ) {
    pos = stack[ --top ];
    if( IsBitSetBS( maze->reach, pos) ) continue;
    if( maze->PHYSstone[ pos ] >= 0 ) continue;

    SetBitBS( maze->reach,pos );

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

void MarkReachQuick( MAZE *maze, PHYSID from ) {
  /* recursive function to mark the fields that are reachable */
	
  static PHYSID stack[ENDPATH];
  PHYSID pos;
  int top;

  UnsetBitBS( maze->reach, from );

  stack[0] = from;
  top = 1;
  while( top ) {
    pos = stack[ --top ];
    if( IsBitSetBS( maze->reach, pos) ) continue;
    if( maze->PHYSstone[ pos ] >= 0 ) continue;
    if( AvoidThisSquare == pos ) continue;

    SetBitBS( maze->reach,pos );

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

int UnReach( MAZE *maze, PHYSID start, BitString treach ) {
  static PHYSID queue[ ENDPATH ];
  PHYSID pos;
  int top, bottom;

  queue[ 0 ] = start;
  top = 1;
  bottom = -1;
  while( ++bottom < top ) {
    pos = queue[ bottom ];

    if( pos == maze->manpos )
      return 1;

    if( !IsBitSetBS( maze->reach, pos ) ||
	IsBitSetBS( treach, pos ) ||
	maze->PHYSstone[ pos ] >= 0 ||
	AvoidThisSquare == pos )
      continue;

    SetBitBS( treach, pos );

    if( IsBitSetBS( maze->M[ 0 ], pos ) )
      queue[ top++ ] = pos + 1;
    if( IsBitSetBS( maze->M[ 1 ], pos ) )
      queue[ top++ ] = pos + YSIZE;
    if( IsBitSetBS( maze->M[ 2 ], pos ) )
      queue[ top++ ] = pos - 1;
    if( IsBitSetBS( maze->M[ 3 ], pos ) )
      queue[ top++ ] = pos - YSIZE;
  }

  BitAndNotEqBS( maze->reach, treach );
  return 0;
}

void UpdateReach( MAZE *maze, PHYSID stonepos )
{
  static BitString treach;
  char a, b, c;

  MarkReachQuick( maze, maze->manpos );

  /* only need to clear treach if UnReach discards calculations */
  a = b = c = 1;
  UnsetBitBS( maze->reach, stonepos );
  Set0BS( treach );
  if( stonepos + 1 != maze->manpos &&
      UnReach( maze, stonepos + 1, treach ) ) {
    if( IsBitSetBS( treach, stonepos + YSIZE ) &&
	IsBitSetBS( maze->reach, stonepos + YSIZE ) )
      a = 0;
    if( IsBitSetBS( treach, stonepos - 1 ) &&
	IsBitSetBS( maze->reach, stonepos - 1 ) )
      b = 0;
    if( IsBitSetBS( treach, stonepos - YSIZE ) &&
	IsBitSetBS( maze->reach, stonepos - YSIZE ) )
      c = 0;
    Set0BS( treach );
  }
  if( a &&
      stonepos + YSIZE != maze->manpos &&
      UnReach( maze, stonepos + YSIZE, treach ) ) {
    if( IsBitSetBS( treach, stonepos - 1 ) &&
	IsBitSetBS( maze->reach, stonepos - 1 ) )
      b = 0;
    if( IsBitSetBS( treach, stonepos - YSIZE ) &&
	IsBitSetBS( maze->reach, stonepos - YSIZE ) )
      c = 0;
    Set0BS( treach );
  }
  if( b &&
      stonepos - 1 != maze->manpos &&
      UnReach( maze, stonepos - 1, treach ) ) {
    if( IsBitSetBS( treach, stonepos - YSIZE ) &&
	IsBitSetBS( maze->reach, stonepos - YSIZE ) )
      c = 0;
    Set0BS( treach );
  }
  if( c
      && stonepos - YSIZE != maze->manpos )
    UnReach( maze, stonepos - YSIZE, treach );

  CopyBS(maze->old_no_reach,maze->no_reach);
  BitNotAndNotAndNotBS(maze->no_reach,maze->reach,maze->out,maze->stone);
}

void MarkDead(MAZE *maze) {
/* function to mark the fields that create a deadlock if a stone is on  */
	
	PHYSID pos,p;
	int dead,dir;

	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (IsBitSetBS(maze->out,pos)) continue;
		if (maze->Phys[pos].goal>=0) continue;
		if (IsBitSetBS(maze->wall,pos)) continue;
		if (!( ( IsBitSetBS( maze->M[NORTH], pos ) &&
			 IsBitSetBS( maze->M[SOUTH], pos ) ) ||
		       ( IsBitSetBS( maze->M[WEST], pos ) &&
			 IsBitSetBS( maze->M[EAST], pos ) ) ) ) {
			SetBitBS(maze->dead,pos);
			for (dir=NORTH; dir<=WEST; dir++) {
				if (IsBitSetBS(maze->M[dir],pos)) 
					UnsetBitBS(maze->S[OppDir[dir]],
						   pos+DirToDiff[dir]);
			}
		} 
		/* check x direction only in east, since we start */
		/* lower left corner and move right */
		/* we assume dead and if not found otherwise mark */
		if (!IsBitSetBS(maze->M[WEST],pos)) dead = 1; 
		else dead = 0;
		for (p=pos; dead==1 && p<YSIZE*XSIZE; p+=YSIZE) {
			/* if we hit goal state, not dead */
			if (maze->Phys[p].goal>=0) {
				dead = 0;
				break;
			}
			/* check if both N and S are possible and break */
			if (IsBitSetBS(maze->M[NORTH],p)
			    &&IsBitSetBS(maze->M[SOUTH],p)) {
				dead = 0;
				break;
			}
			/* check if end */
			if (!IsBitSetBS(maze->M[EAST],p)) break;
		}
		/* if dead is still 1, we rerun pos and set dead flag */
		if (dead==1) for (p=pos; p<YSIZE*XSIZE; p+=YSIZE) {
			SetBitBS(maze->dead,p);
			/* set neighbours S[NEWS] flags */
			for (dir=NORTH; dir<=WEST; dir++) {
				if (IsBitSetBS(maze->M[dir],p)) 
					UnsetBitBS(maze->S[OppDir[dir]],
						   p+DirToDiff[dir]);
			}
			if (!IsBitSetBS(maze->M[EAST],p)) break;
		}
		/* now same for E/W, asume south was done */
		if (!IsBitSetBS(maze->M[SOUTH],pos)) dead = 1; 
		else dead = 0;
		for (p=pos; dead==1 && (p%YSIZE)!=0; p++) {
			/* if we hit goal state, not dead */
			if (maze->Phys[p].goal>=0) {
				dead = 0;
				break;
			}
			/* check if both W and E are possible and break */
			if (  IsBitSetBS(maze->M[WEST],p)
			    &&IsBitSetBS(maze->M[EAST],p)) {
				dead = 0;
				break;
			}
			/* check if end */
			if (!IsBitSetBS(maze->M[NORTH],p)) break;
		}
		/* if dead is still 1, we rerun pos and set dead flag */
		if (dead==1) for (p=pos; (p%YSIZE)!=0; p++) {
			SetBitBS(maze->dead,p);
			/* set neighbours S[NEWS] flags */
			for (dir=NORTH; dir<=WEST; dir++) {
				if (IsBitSetBS(maze->M[dir],p)) 
					UnsetBitBS(maze->S[OppDir[dir]],
						   p+DirToDiff[dir]);
			}
			if (!IsBitSetBS(maze->M[NORTH],p)) break;
		}
	}
}

void MarkBackDead(MAZE *maze)
{
	PHYSID pos;
	int    stonei, dead, dir;

	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (  IsBitSetBS(maze->out,pos)
		    ||IsBitSetBS(maze->wall,pos)) {
/*
			SetBitBS(maze->backdead,pos);
*/
			continue;
		}
		dead = 1;
		/* ACHTUNG:) Backward means stone locs are the goal */
		for (stonei=0; stonei<maze->number_stones&&dead==1; stonei++) {
			if (IsBitSetBS(maze->one_way,pos)) {
			   for (dir=NORTH; dir<=WEST && dead==1; dir++) {
			      if (BackGoalWeight(maze,pos,
				     maze->stones[stonei].loc,dir)<ENDPATH)
				dead = 0;
			   }
			} else {
			   if (  BackGoalWeight(maze,pos,
						maze->stones[stonei].loc,0) 
			       < ENDPATH) dead = 0;
			}
		}
		if (dead==1) SetBitBS(maze->backdead,pos);
	}
}

void MarkOneHelp(MAZE *maze, PHYSID curr, PHYSID avoid) {
/* Propagate marking, basically see where man could go, ignoring stones */
	int dir;
	if (curr==avoid) return;
	if (IsBitSetBS(maze->s_visited,curr)) return;
	SetBitBS(maze->s_visited,curr);
	for (dir=NORTH; dir<=WEST; dir++) {
		if (IsBitSetBS(maze->M[dir],curr)) 
			MarkOneHelp(maze,curr+DirToDiff[dir],avoid);
	}
}

void MarkOneTest(MAZE *maze, int in_dir, PHYSID curr) {
/* test if all reachable squares are marked, if not, mark as oneway */
	int dir;

	if (maze->s_weights[in_dir][curr] != NULL) return;
	if(  (   IsBitSetBS(maze->M[NORTH],curr)
	      &&!IsBitSetBS(maze->s_visited,curr+1))
	   ||(   IsBitSetBS(maze->M[SOUTH],curr)
	      &&!IsBitSetBS(maze->s_visited,curr-1))
	   ||(  IsBitSetBS(maze->M[EAST],curr)
	      &&!IsBitSetBS(maze->s_visited,curr+YSIZE))
	   ||(  IsBitSetBS(maze->M[WEST],curr)
	      &&!IsBitSetBS(maze->s_visited,curr-YSIZE)))
	{
		SetBitBS(maze->one_way,curr);
		maze->s_weights[in_dir][curr] = NewWeights();
		maze->bg_weights[in_dir][curr] = NewWeights();
		maze->b_weights[in_dir][curr] = NewWeights();
		for (dir=NORTH; dir<=WEST; dir++) {
			if (  IsBitSetBS(maze->M[dir],curr)
		    	    &&IsBitSetBS(maze->s_visited,curr+DirToDiff[dir])) {
				DubWeights(maze,curr,in_dir,dir);
			}
		}
	}
}

void MarkOne(MAZE *maze) {

	int i,dir;

	for (i=0; i<XSIZE*YSIZE; i++) {
		/* test if one way for every square not outside/wall */
		if (  IsBitSetBS(maze->out,i)
		    /*NOTE: ||maze->Phys.POS[i].goal>=0 #10!!! */
		    ||IsBitSetBS(maze->wall,i))
			continue;
		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->M[dir],i)) {
				Set0BS(maze->s_visited);
				MarkOneHelp(maze,i+DirToDiff[dir],i);
				MarkOneTest(maze,dir,i);
			}
		}
		if (!IsBitSetBS(maze->one_way,i)) {
			maze->s_weights[0][i] = NewWeights();
			maze->bg_weights[0][i] = NewWeights();
			maze->b_weights[0][i] = NewWeights();
		}
	}
	Set0BS(maze->s_visited);
}

void MarkTun(MAZE *maze) {

	int i,dim,j,dir;
	int min_dim=0, tunnel;

	for (i=0; i<XSIZE*YSIZE; i++) {
		/* test if one way for every square not outside/wall */
		maze->Phys[i].tunnel = 0;
		maze->Phys[i].s_tunnel = 0;
		maze->Phys[i].min_dim = 0;
		maze->Phys[i].s_min_dim = 0;
		maze->Phys[i].free = 0;
		maze->Phys[i].s_free = 0;
		if (  IsBitSetBS(maze->out,i)
		    ||IsBitSetBS(maze->wall,i)) {
			continue;
		}

		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->S[dir],i)) maze->Phys[i].s_free++;
			if (IsBitSetBS(maze->M[dir],i)) maze->Phys[i].free++;
		}
		/* collect both dimensions man tunnels */
		tunnel = YSIZE*XSIZE;

		dim=1;
		j=i;
		j+=YSIZE;
		while (!IsBitSetBS(maze->wall,j)) { dim++; j+=YSIZE; }
		j=i;
		j-=YSIZE;
		while (!IsBitSetBS(maze->wall,j)) { dim++; j-=YSIZE; }
		if (dim<tunnel) { tunnel = dim; min_dim = YSIZE; }

		dim=1;
		j=i;
		j++;
		while (!IsBitSetBS(maze->wall,j)) { dim++; j++; }
		j=i;
		j--;
		while (!IsBitSetBS(maze->wall,j)) { dim++; j--; }
		if (dim<tunnel) { tunnel  = dim; min_dim = 1; }

		maze->Phys[i].tunnel = (USHORT)tunnel;
		maze->Phys[i].min_dim = min_dim;

		/* collect both dimensions stone tunnels */
		if (IsBitSetBS(maze->dead,i)) continue;
		tunnel = YSIZE*XSIZE;
		dim=1;
		j=i;
		while (  IsBitSetBS(maze->S[EAST],j)
		       ||IsBitSetBS(maze->S[WEST],j+YSIZE)) { dim++; j+=YSIZE; }
		j=i;
		while (  IsBitSetBS(maze->S[WEST],j)
                       ||IsBitSetBS(maze->S[EAST],j-YSIZE)) { dim++; j-=YSIZE; }
		if (dim<tunnel) { tunnel = dim; min_dim = YSIZE; }

		dim=1;
		j=i;
		while (  IsBitSetBS(maze->S[NORTH],j)
                       ||IsBitSetBS(maze->S[SOUTH],j+1)) { dim++; j++; }
		j=i;
		while (  IsBitSetBS(maze->S[SOUTH],j)
                       ||IsBitSetBS(maze->S[NORTH],j-1)) { dim++; j--; }
		if (dim<tunnel) { tunnel = dim; min_dim = 1; }

		maze->Phys[i].s_tunnel = (USHORT)tunnel;
		maze->Phys[i].s_min_dim = min_dim;
	}

}

int Hinch(MAZE *maze, PHYSID pos, PHYSID prev) {
/* is pos a square that is only weakly linked to prev */
/* precond: neither pos nor prev are neither hallway nor intersection */
	PHYSID adiff, bdiff;

	adiff = pos-prev;
	if (  (IsBitSetBS(maze->wall,pos+adiff))
	    ||(IsBitSetBS(maze->wall,pos-adiff))) return(0);
	if (  (maze->Phys[pos].free!=3)
	    ||(maze->Phys[prev].free!=3)) return(0);
	if (abs(adiff)==YSIZE) bdiff=1;
	else bdiff=YSIZE;
	if (  (  IsBitSetBS(maze->wall,pos +bdiff)
	       &&IsBitSetBS(maze->wall,prev-bdiff))
	    ||(  IsBitSetBS(maze->wall,pos -bdiff)
	       &&IsBitSetBS(maze->wall,prev+bdiff)))
	{
		return(1);
	}
	return(0);
}

void MarkStruct(MAZE *maze) {

	
        static PHYSID stack[ENDPATH];
        static PHYSID from[ENDPATH];
        int next_in, next_out,dir;

	PHYSID p,pos,yoffs=0,xoffs=0,prev;
	int count;
	signed short lstruct=0,ls;

	/* allways assume we visited lower and left square before since we
	 * start lower left corner, working our way up and then over to the
	 * right. */

	/* find all hallways */
	for (p=0; p<YSIZE*XSIZE; p++) {
	    from[0]  = 0;
	    stack[0] = p;
	    next_in  = 1;
	    next_out = 0;
	    while (next_out < next_in) {
		pos  = stack[next_out];
		prev = from[next_out++];
		if (  (IsBitSetBS(maze->wall,pos))
		    ||(IsBitSetBS(maze->out,pos))
		    ||(maze->Phys[pos].lstruct>=0)) {
			continue;
		}
		if (maze->Phys[pos].free==2) {
			if (  ((lstruct=maze->Phys[pos-1].lstruct)==-1)
			    &&((lstruct=maze->Phys[pos+1].lstruct)==-1)
			    &&((lstruct=maze->Phys[pos+YSIZE].lstruct)==-1)
			    &&((lstruct=maze->Phys[pos-YSIZE].lstruct)==-1))
			{
			    /* Start of a tunnel */
				if (  (  (IsBitSetBS(maze->M[SOUTH],pos))
				       ||(IsBitSetBS(maze->M[NORTH],pos)))
				    &&(  (IsBitSetBS(maze->M[EAST],pos))
				       ||(IsBitSetBS(maze->M[WEST],pos)))) {
					/* This is a corner, so either north
					 * or south and either of east or
					 * west has to take part in it */
					if (IsBitSetBS(maze->M[EAST],pos)) 
						yoffs=YSIZE;
					else yoffs=-YSIZE;
					if (IsBitSetBS(maze->M[NORTH],pos)) 
						xoffs=1;
					else xoffs=-1;
					if(!IsBitSetBS(maze->out,pos+yoffs+xoffs)
					&&!IsBitSetBS(maze->wall,pos+yoffs+xoffs))
					{
						continue;
					}
				}
				maze->structs=(STRUCT*)My_realloc(maze->structs,
				       sizeof(STRUCT)*(maze->number_structs+1));
				maze->Phys[pos].lstruct
				       = maze->number_structs;
				memset(&maze->structs[maze->number_structs],0,
				       sizeof(STRUCT));
				maze->structs[maze->number_structs].hallway=1;
				maze->number_structs++;
			} else {
				maze->Phys[pos].lstruct = lstruct;
			}
			for (dir=NORTH; dir<=WEST; dir++) {
				if (   IsBitSetBS(maze->M[dir],pos)
				    &&(prev!=pos+DirToDiff[dir])) {
					from[next_in] = pos;
					stack[next_in++] = pos+DirToDiff[dir];
				}
			} 
		}
	    }
	}
	/* find all intersections - is a square adjacent to hallways and
	 * other intersections only - alternatively, we check if all four
	 * diagonally adjacent squares are walls and if min. 3 are walls
	 * this is a intersection */
	for (pos=0; pos<YSIZE*XSIZE; pos++) {
		if (  (IsBitSetBS(maze->wall,pos))
		    ||(IsBitSetBS(maze->out,pos))
		    ||(IsBitSetBS(maze->dead,pos))
		    ||(maze->Phys[pos].goal>=0)
		    ||(maze->Phys[pos].lstruct>=0)) continue;
		p = 0;
		if (IsBitSetBS(maze->wall,pos+YSIZE+1)) p++;
		else {xoffs=YSIZE; yoffs=1;}
		if (IsBitSetBS(maze->wall,pos+YSIZE-1)) p++;
		else {xoffs=YSIZE; yoffs=-1;}
		if (IsBitSetBS(maze->wall,pos-YSIZE+1)) p++;
		else {xoffs=-YSIZE; yoffs=1;}
		if (IsBitSetBS(maze->wall,pos-YSIZE-1)) p++;
		else {xoffs=-YSIZE; yoffs=-1;}
		if (  (p==4)
		    ||(  (p==3)
		       &&(maze->Phys[pos+xoffs+yoffs].lstruct>=0))) {
			maze->structs=(STRUCT*)My_realloc(maze->structs,
			       sizeof(STRUCT)*(maze->number_structs+1));
			maze->Phys[pos].lstruct
			       = maze->number_structs;
			memset(&maze->structs[maze->number_structs],0,
			       sizeof(STRUCT));
			maze->structs[maze->number_structs].intersection=1;
			maze->number_structs++;
		}
	}
	/* find free hinches, assume all other intersections are found and
	 * all hallways are found */
	for (pos=0; pos<YSIZE*XSIZE; pos++) {
		if (  (IsBitSetBS(maze->wall,pos))
		    ||(IsBitSetBS(maze->out,pos))
		    ||(IsBitSetBS(maze->dead,pos))
		    ||(maze->Phys[pos].goal>=0)
		    ||(maze->Phys[pos].lstruct>=0)) continue;
		if (  (maze->Phys[pos].free==4)
		    &&(  (  (IsBitSetBS(maze->wall,pos+YSIZE+1))
		          &&(IsBitSetBS(maze->wall,pos-YSIZE-1)))
		       ||(  (IsBitSetBS(maze->wall,pos+YSIZE-1))
			  &&(IsBitSetBS(maze->wall,pos-YSIZE+1))))
		    &&(maze->Phys[pos+YSIZE].lstruct==-1)
		    &&(maze->Phys[pos-YSIZE].lstruct==-1)
		    &&(maze->Phys[pos+1].lstruct==-1)
		    &&(maze->Phys[pos-1].lstruct==-1))
		{
			maze->structs=(STRUCT*)My_realloc(maze->structs,
			       sizeof(STRUCT)*(maze->number_structs+1));
			maze->Phys[pos].lstruct
			       = maze->number_structs;
			memset(&maze->structs[maze->number_structs],0,
			       sizeof(STRUCT));
			maze->structs[maze->number_structs].intersection=1;
			maze->number_structs++;
		}
	}
	/* find all rooms - rooms can be adjacent, if both corners*/
	for (p=0; p<YSIZE*XSIZE; p++) {
	    from[0]  = 0;
	    stack[0] = p;
	    next_in  = 1;
	    next_out = 0;
	    while (next_out < next_in) {
		pos  = stack[next_out];
		prev = from[next_out++];
		if (  (IsBitSetBS(maze->wall,pos))
		    ||(IsBitSetBS(maze->out,pos))
		    ||(maze->Phys[pos].lstruct>=0)) continue;
		count = 0;
		for (dir=NORTH; dir<=WEST; dir++) {
			if (  (IsBitSetBS(maze->M[dir],pos))
			    &&((ls=maze->Phys[pos+DirToDiff[dir]].lstruct)!=-1)
			    &&(maze->structs[ls].room==1)
			    &&(!Hinch(maze,pos,pos+DirToDiff[dir]))) {
				lstruct=maze->Phys[pos+DirToDiff[dir]].lstruct;
				count++;
			}
		}
		/* First square */
		if (count==0) {
			maze->structs=(STRUCT*)My_realloc(maze->structs,
			       sizeof(STRUCT)*(maze->number_structs+1));
			maze->Phys[pos].lstruct
			       = maze->number_structs;
			memset(&maze->structs[maze->number_structs],0,
			       sizeof(STRUCT));
			maze->structs[maze->number_structs].room=1;
			maze->number_structs++;
			for (dir=NORTH; dir<=WEST; dir++) {
				if (   IsBitSetBS(maze->M[dir],pos)
			    	    &&(prev!=pos+DirToDiff[dir])
				    &&(!Hinch(maze,pos+DirToDiff[dir],pos))) {
					from[next_in] = pos;
					stack[next_in++] = pos+DirToDiff[dir];
				} 
			} 
		} else if (prev!=0) {
			maze->Phys[pos].lstruct 
				= maze->Phys[prev].lstruct;
			for (dir=NORTH; dir<=WEST; dir++) {
				if (   IsBitSetBS(maze->M[dir],pos)
			    	    &&(prev!=pos+DirToDiff[dir])
				    &&(!Hinch(maze,pos+DirToDiff[dir],pos))) {
					from[next_in] = pos;
					stack[next_in++] = pos+DirToDiff[dir];
				} 
			} 
		}
	   }
	}
	/* count stones, squares and goals in all the structures, verify
	 * that every square is part of a structure */
	for (pos=0; pos<YSIZE*XSIZE; pos++) {
		if (  (IsBitSetBS(maze->wall,pos))
		    ||(IsBitSetBS(maze->out,pos))) continue;
		lstruct = maze->Phys[pos].lstruct;
		SR(Assert(lstruct>=0,"MarkStruct: Square not put in Struct\n"));
		if (  (IsBitSetBS(maze->dead,pos))
		    &&(maze->structs[maze->Phys[pos].lstruct].hallway))
			maze->structs[lstruct].manonly=1;
		maze->structs[lstruct].number_squares++;
		if (!IsBitSetBS(maze->dead,pos))
			maze->structs[lstruct].number_s_squares++;
		if (maze->Phys[pos].goal>=0) 
			maze->structs[lstruct].number_goals++;
		if (maze->PHYSstone[pos]>=0) 
			maze->structs[lstruct].number_stones++;
	}
	/* count number doors (stone and man) */
	for (pos=0; pos<YSIZE*XSIZE; pos++) {
		if (  (IsBitSetBS(maze->wall,pos))
		    ||(IsBitSetBS(maze->out,pos))) continue;
		lstruct = maze->Phys[pos].lstruct;
		for (dir=NORTH; dir<=WEST; dir++) {
			if (  (IsBitSetBS(maze->M[dir],pos))
		            &&(  maze->Phys[pos+DirToDiff[dir]].lstruct
			       !=lstruct)) {
			        maze->structs[lstruct].number_doors++;
		                if (  (IsBitSetBS(maze->S[OppDir[dir]],
						  pos+DirToDiff[dir]))
		                    &&(maze->structs[maze->Phys[pos+DirToDiff[dir]].lstruct].manonly==0))
					maze->structs[lstruct].number_s_doors++;
			}
		}
	}
	/* set max_number_stones appropriatly */
	for (lstruct = 0; lstruct<maze->number_structs; lstruct++) {
		if (   maze->structs[lstruct].hallway
		    || maze->structs[lstruct].intersection)
			maze->structs[lstruct].max_number_stones = 1;
		else 	maze->structs[lstruct].max_number_stones =
				maze->structs[lstruct].number_squares;
		if (  (maze->structs[lstruct].number_goals)
		     >(maze->structs[lstruct].max_number_stones)) {
			maze->structs[lstruct].max_number_stones 
			= maze->structs[lstruct].number_goals;
		}
/*
		if (Cur_Maze_Number == 6) {
			switch (lstruct) {
			case 6: maze->structs[lstruct].max_number_stones = 3;
				break;
			default:
			}
		}
		if (Cur_Maze_Number == 4) {
			switch (lstruct) {
			case 5: maze->structs[lstruct].max_number_stones = 7;
				break;
			case 7: maze->structs[lstruct].max_number_stones = 11;
				break;
			default:
			}
		}
*/
	}
}


int GetDistDiff(MAZE *maze, PHYSID pos, int goali, int dist[4][4]) {
	int stonedir, mandir;
	int min;

	min=ENDPATH;
	for (stonedir=NORTH; stonedir<=WEST; stonedir++) {
		for (mandir=NORTH; mandir<=WEST; mandir++) {
			if (   IsBitSetBS(maze->S[stonedir],pos)
			    && IsBitSetBS(maze->M[  mandir],pos)) {
				dist[stonedir][mandir]  = 
					GetWeightManpos(maze,
					pos+DirToDiff[stonedir],
					pos,pos+DirToDiff[mandir]);
				dist[stonedir][mandir] +=
					GetWeightManpos(maze,
					maze->goals[goali].loc,
					pos+DirToDiff[stonedir],pos);
				if (dist[stonedir][mandir]>ENDPATH)
					dist[stonedir][mandir]=ENDPATH;
				if (min>dist[stonedir][mandir])
					min = dist[stonedir][mandir];
			} else  dist[stonedir][mandir]  = ENDPATH;
		}
	}
	for (stonedir=NORTH; stonedir<=WEST; stonedir++) {
		for (mandir=NORTH; mandir<=WEST; mandir++) {
			if (dist[stonedir][mandir]<ENDPATH)
				dist[stonedir][mandir] -= min;
		}
	}
	return(min);
}

int AllSame(int a[4][4], int b[4][4]) {
	int i,j;
	for (i=0; i<4; i++) {
		for (j=0; j<4; j++) {
			if (a[i][j]!=b[i][j]) return(0);
		}
	}
	return(1);
}

void MarkEqual(MAZE *maze) 
/* Marks all squares in eqsq that have equal dist diffs to all goals */
/* This is needed for the lb increase from dl searches */
/* min is the minimum distance to a goal, if that is ENDPATH it is not
 * reachable and we are not considering it at all */
{

	PHYSID pos;
	int    goali;
	int    diff[4][4];
	int    comp[4][4];
	int    eqsq;
	int    min;

	Set0BS(maze->eqsq);
	for (pos=0; pos<YSIZE*XSIZE; pos++) {
		if (IsBitSetBS(maze->out,pos)) continue;
		if (IsBitSetBS(maze->dead,pos)) continue;
		if (IsBitSetBS(maze->goal,pos)) continue;
		eqsq=YES;
		goali=0;
		do {	/* find a reachable goal */
			min = GetDistDiff(maze,pos,goali,diff);
			goali++;
		} while (min>=ENDPATH && goali<maze->number_goals);
		while (eqsq==YES && goali<maze->number_goals) {
			min = GetDistDiff(maze,pos,goali,comp);
			if (min<ENDPATH && !AllSame(comp,diff)) {
				eqsq=NO;
			}
			goali++;
		}
		if (eqsq==YES) SetBitBS(maze->eqsq,pos);
	}
}

int GetYDim(MAZE *maze)
/* returns y-dimension of the maze */
{
	PHYSID pos;
	int    y,min;

	for (pos=0; pos<YSIZE*XSIZE; pos++) {
		if (!IsBitSetBS(maze->out,pos)) {
			y = pos%YSIZE;
			if (min > y) min = y;
		}
	}
	return( (YSIZE - min) + 1 );
}
