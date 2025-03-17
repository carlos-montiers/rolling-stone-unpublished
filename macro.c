#include "board.h"

int BestScore;
BitString BestGoalArea;
int min_dist[XSIZE*YSIZE];

void InitGRoom(GROOM *groom, int gridx)
{
	groom->n              = 0;
	groom->index          = gridx;
	groom->stone_inside   = 0;
	groom->number_goals   = 0;
	groom->number_squares = 0;
	groom->deadentrances  = 0;
	groom->hashkey        = 0;
	Set0BS(groom->goals);
	Set0BS(groom->squares);
}

void AddMacro(MAZE *maze, PHYSID pos, int type, PHYSID from, 
			    PHYSID last_over, PHYSID to) {

	int n;
	n = maze->macros[pos].number_macros++;

	maze->macros[pos].macros = (MACRO*)
		My_realloc( maze->macros[pos].macros,
	        sizeof(MACRO)*maze->macros[pos].number_macros);
	maze->macros[pos].type			=type;
	maze->macros[pos].macros[n].from	=from;
	maze->macros[pos].macros[n].last_over 	=last_over;
	maze->macros[pos].macros[n].to  	=to;
/* XXX
printf("from: %i, pos: %i, last_over: %i, to %i\n", from, pos, last_over, to);
PosNr = from;
PrintBit3Maze(maze,maze->goal,maze->goal,to);
PosNr = 0;
*/
}

PHYSID  FindEndTunnel(MAZE *maze, PHYSID pos, int diff, PHYSID *last_over) {

	PHYSID next;
	int    from_dir,next_dir,waystogo,dir;

	next = pos + diff;

	next_dir = NODIR;
	/* is it a one way square and does no goal macro start here */
	if (   IsBitSetBS(maze->one_way,next)
	    && !IsBitSetBS(maze->goal,next)
	    && maze->macros[next].type!=4) {
		/* go on with tunnel */
		/* find direction of continuing at next */
		from_dir = DiffToDir(-diff);
		waystogo = 0;
		for (dir=NORTH; dir<=WEST; dir++) {
			if (   IsBitSetBS(maze->S[dir],next) 
		    	    && ConnectedDir(maze,next,from_dir,OppDir[dir])) {
				waystogo++; next_dir = DirToDiff[dir];
			}
		}
		if (waystogo==0) {
			/* DeadEnd, Don't go here */
			return(0); 
		} else if (waystogo==1) {
			/* still in tunnel, go on */
			return(FindEndTunnel(maze,next,next_dir,last_over));
		}
		/* more then one way, NOTE: could be checked if one
		 * of the possible ways is DeadEnd... return as
		 * last_over square */
		/* fall through to end of tunnel */
	}
	/* next is end of tunnel */
	*last_over = pos;
	return(next);
}

void FindStartEndTunnel(MAZE *maze, int diff, 
			PHYSID pos, PHYSID *start, PHYSID *end)
{
	int        dir;
	int	   lstruct;
	int	   odiff;

	dir = DiffToDir(diff);
	odiff = (diff==YSIZE?1:YSIZE);
	*end   = pos;
	*start = pos;
	lstruct = maze->Phys[pos].lstruct;

	while (   maze->Phys[*end + diff].lstruct==lstruct
	       && IsBitSetBS(maze->S[dir],*end)
	       && !IsBitSetBS(maze->goal,*end+diff)
	       && !IsBitSetBS(maze->wall,*end+2*diff)
	       && IsBitSetBS(maze->wall,*end+odiff)
	       && IsBitSetBS(maze->wall,*end-odiff)
	       && maze->macros[*end + diff].type==0) *end += diff;
	dir = OppDir[dir];
	while (   maze->Phys[*start - diff].lstruct==lstruct
	       && IsBitSetBS(maze->S[dir],*start)
	       && !IsBitSetBS(maze->goal,*start-diff)
	       && !IsBitSetBS(maze->wall,*start-2*diff)
	       && IsBitSetBS(maze->wall,*start+odiff)
	       && IsBitSetBS(maze->wall,*start-odiff)
	       && maze->macros[*start - diff].type==0) *start -= diff;
}

void RemoveGRoom(MAZE *maze, int gridx)
{
	int j;

	for (j=0; j<XSIZE*YSIZE; j++) {
		if (maze->groom_index[j]==gridx)
				maze->groom_index[j]=-1;
	}
	maze->gmtrees[gridx] = NULL;
	maze->number_grooms--;
}

void FindMacros(MAZE *maze) {
	int i,j,gridx;
	int diff,size;
	PHYSID last,last_over,pos,start,end;
	/* find macros that shoot stones to goals */
/*
	if (Options.mc_gm==NO) goto TUNNEL;
*/
	gridx = 0;
	/* init hashtables, that are used in GrowRoom3() */
	InitHashTables();
	for (i=0; i<maze->number_goals; i++) {
		/* init was to -2, if is only -1 we tried goal already */
		if (maze->groom_index[maze->goals[i].loc]<-1) {
			gridx = maze->number_grooms;
			maze->gmtrees = (GMNODE**)
				My_realloc(maze->gmtrees,
				sizeof(GMNODE*) *(maze->number_grooms+1));
			maze->grooms=(GROOM*)
				My_realloc(maze->grooms,
				sizeof(GROOM) *(maze->number_grooms+1));

			maze->number_grooms++;
			InitGRoom(&(maze->grooms[gridx]),gridx);
			if (!GrowRoom3(maze,maze->goals[i].loc,
					0,&(maze->grooms[gridx]))) {
				RemoveGRoom(maze,gridx);
				continue;
			}
			PickUpEntrances(maze,gridx);
			if (!StartBuildGMTree(maze,maze->grooms+gridx)) {
				RemoveGRoom(maze,gridx);
			}
		}
	}
TUNNEL:
	/* Get the Tunnel macros */
	if (Options.mc_tu==NO) return;
	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		/* if macro here already, go on */
		if (maze->macros[pos].number_macros>0) continue;
		/* if in goal area, don't create tunnel macros */
		if (IsBitSetBS(maze->goal,pos)) continue;
		if (maze->groom_index[pos]>=0) continue;
		/* We go from lower left to upper right, use this fact to
		 * check if we must have looked at this square already */
		if (maze->Phys[pos].tunnel==1) {
		    if (IsBitSetBS(maze->one_way,pos)) {
			/* Get the Oneway Tunnel macros */
			/* Found one, check if left/lower one is already
			 * been found, if no go on, otherwise next */
			if (maze->Phys[pos].min_dim==1) diff=YSIZE;
			else diff=1;
			last = FindEndTunnel(maze,pos,diff,&last_over);
			if (last != 0) 
				AddMacro(maze,pos,2,pos-diff,last_over,last);
			last = FindEndTunnel(maze,pos,-diff,&last_over);
			if (last != 0) 
				AddMacro(maze,pos,2,pos+diff,last_over,last);
		    } else {
			/* Get the two-way Tunnel macros */
			if (maze->Phys[pos].min_dim==1) diff=YSIZE;
			else diff=1;
			FindStartEndTunnel(maze,diff,pos,&start,&end);
			if (start!=end && maze->macros[start+diff].type == 0) {
				AddMacro(maze,start+diff,2,start,end,end+diff);
				AddMacro(maze,end-diff,2,end,start,start-diff);
			}
		    }
		}
	}
}

int GrowRoom(MAZE *maze, PHYSID pos, GROOM *groom)
{
	int dir;

	if (IsBitSetBS(maze->wall,pos)) return(1);
	if (maze->groom_index[pos] >=0 ) return(1);
	if (  (maze->Phys[pos].goal>=0)
	    ||(  (maze->structs[maze->Phys[pos].lstruct].number_stones==0)
	       &&(maze->structs[maze->Phys[pos].lstruct].number_goals>0))) {
		GroomIncPos(maze,pos,groom);
		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->M[dir],pos)) 
				GrowRoom(maze,pos+DirToDiff[dir],groom);
		}
		return(0);
	}
	return(1);
}

void GroomExcPos(MAZE *maze, PHYSID pos, GROOM *groom)
/* Simply excludes pos from groom */
{
	if (maze->PHYSstone[pos]>=0) groom->stone_inside=1;
	if (maze->Phys[pos].goal>=0) {
		groom->number_goals--;
		UnsetBitBS(groom->goals,pos);
	}
	maze->groom_index[pos] = -1;
	groom->number_squares--;
	UnsetBitBS(groom->squares,pos);
	groom->hashkey ^= RandomTable[pos];
}

void GroomIncPos(MAZE *maze, PHYSID pos, GROOM *groom)
/* Simply includes pos into groom */
{
	if (maze->Phys[pos].goal>=0) {
		groom->number_goals++;
		SetBitBS(groom->goals,pos);
	}
	maze->groom_index[pos] = groom->index;
	groom->number_squares++;
	SetBitBS(groom->squares,pos);
	groom->hashkey ^= RandomTable[pos];
}

int EvaluateGroom(GROOM *groom)
{
	if (groom->n==0 || groom->deadentrances==1) return( 0 );
	return( (20-groom->n)*1000 + groom->number_squares );
}

void AsimGoals(MAZE *maze, PHYSID pos, GROOM *groom)
/* This is to assimilate all goals into the groom that are adjasent to one
 * another */
{
	int dir;

	if (IsBitSetBS(maze->wall,pos)) return;
	if (maze->groom_index[pos] >=0 ) return;
	if (maze->PHYSstone[pos]>=0) return;
	if (maze->Phys[pos].goal>=0) {
		GroomIncPos(maze,pos,groom);
		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->M[dir],pos)) 
				AsimGoals(maze,pos+DirToDiff[dir],groom);
		}
		return;
	}
	return;
}

void SetMinDist(MAZE *maze)
{
	int goali,dist;
	PHYSID pos;

	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (IsBitSetBS(maze->out ,pos)) continue;
		if (IsBitSetBS(maze->wall,pos)) continue;
		min_dist[pos] = ENDPATH;
		for (goali=0; goali<maze->number_goals; goali++) {
		     dist = ManWeight(maze,maze->goals[goali].loc,pos);
		     if (min_dist[pos] > dist) min_dist[pos] = dist; 
		}		
	}
}

static int EntrComp(const void *e1, const void *e2) {
        return(min_dist[*(PHYSID*)e1] - min_dist[*(PHYSID*)e2]);
}

void StoneReach(MAZE *maze, BitString v, PHYSID entr, PHYSID location)
{
/* pseudo-recursive function to mark the fields in v from which location is
 * reachable via entr */
/* v contains all bits set for locations of squares that can be served */
/* by this entr. */

/* The man is not allowed to touch any goal area squares of ANY goal area.
 * That is not completely correct, since we might not even know yet what
 * will be a goal area in the future, but we have to live with this for now. */

        static PHYSID stack[ENDPATH];
        static PHYSID from[ENDPATH];
        PHYSID pos,fro;
        int next_in, next_out, dir, curr_dir;
	static short     touched_from[XSIZE*YSIZE];
	static short     dir2bit[4] = {1,2,4,8};

	from[0]  = entr;
        stack[0] = location;
        next_in  = 1;
        next_out = 0;
	Set0BS(v);
	memset(touched_from,0,sizeof(short)*XSIZE*YSIZE);
        while (next_out < next_in) {
		fro = from[next_out];
                pos = stack[next_out++];
		if (IsBitSetBS(maze->one_way,pos)) {
			/* if we touched that square from every dir, we are
			 * done with it */
			curr_dir = DiffToDir(fro-pos);
			if (touched_from[pos] & dir2bit[curr_dir]) continue;
			for (dir=NORTH; dir<=WEST; dir++) {
				if (  (!IsBitSetBS(maze->M[dir],pos))
				    ||(ConnectedDir(maze,pos,curr_dir,dir))) {
					touched_from[pos] |= dir2bit[dir];
				}
			}
		} else {
			if (IsBitSetBS(v,pos)) continue;
		}
		SetBitBS(v,pos);

		for (dir=NORTH; dir<=WEST; dir++) {
                	if(  IsBitSetBS(maze->M[dir],pos)
			   &&IsBitSetBS(maze->S[OppDir[dir]],pos+DirToDiff[dir])
		    	   &&maze->groom_index[pos+DirToDiff[dir]] < 0) {
				from[next_in] = pos;
				stack[next_in++] = pos+DirToDiff[dir];
			}
		}
        }
}

void ValidateEntrances(MAZE *maze, GROOM *groom)
/* This routine tries to find out if there are useless entrances and if all
 * stones can make it to an entrance */
{
	int i;
	PHYSID pos;
	BitString reach,tmp,all_reach;

	Set0BS(all_reach);
	for (i=0; i<groom->n; i++) {
		/* Which stones can make it to this entrance */
		StoneReach(maze,reach,groom->entrances[i],groom->locations[i]);
		BitOrEqBS(all_reach,reach);
	}
	/* If there are stones that can't reach any of the entrances, see if
	 * they could make it at all, even through goal areas. If they can't
	 * reach any of the entrances, fine, if they could, set n=0, since
	 * this is not a good goal area then */
	BitAndNotBS(tmp,maze->stone,all_reach);
	if (Isnt0BS(tmp)) {
		for (pos=0; pos<XSIZE*YSIZE; pos++) {
			if (!IsBitSetBS(tmp,pos)) continue;
			/* Can this stone make it at all? Then no goos! */
			for (i=0; i<groom->n; i++) {
			  if (  GetOptWeight(maze,groom->locations[i],pos,NODIR)
			      < ENDPATH) {
printf("Badness!\n");
				groom->deadentrances = 1;
				return;
			  }
			}
		}
	}
}

void GrowDFS(MAZE *maze, GROOM *groom, int g)
/* receives a groom, Entrances not set yet, no eval done and tries to
 * improve on it and over Bound */
{
	int i,n,dir;
	int score;
	PHYSID pos;
	PHYSID entrances[MAX_LOCATIONS];
	BitString tmp;

	/* We assume that if we found a BestGoalArea, it can only be grown,
	 * so throw everything away that is smaller than BestGoalArea */
	IncNodeCount(g);
	if (IdaInfo->node_count > 1000) return;
	if (groom->n > 20) return;

	BitAndNotBS(tmp,BestGoalArea,groom->squares);
	if (Isnt0BS(tmp)) return;
	
	/* check if already looked at */
	if (GGGetHashTable(groom->hashkey)) return;
	GGStoreHashTable(groom->hashkey);
	PickUpEntrances(maze,groom->index);
	score = EvaluateGroom(groom);
	/*if (score < (20000 - (20000 - BestScore)*2)) return;*/
	if (BestScore > 19000 && score < 18000) return;
	if (score > BestScore) {
		BestScore = score;
		CopyBS(BestGoalArea,groom->squares);
	}

	/* sort entrances so near squares are first added */
	n = groom->n;
	memcpy(entrances,groom->entrances,sizeof(PHYSID)*n);
	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (IsBitSetBS(groom->squares,pos)) {
			for (dir=NORTH; dir<=WEST; dir++) {
                		if(  IsBitSetBS(maze->M[dir],pos)
				   &&!IsBitSetBS(maze->stone,pos+DirToDiff[dir])
				   /* no need to check for real entr-> TT! */
		    	   	   &&maze->groom_index[pos+DirToDiff[dir]]<0) {
				   entrances[n] = pos+DirToDiff[dir];
				   n++;
				}
			}
		}
	}
	My_qsort(entrances,n,sizeof(PHYSID),EntrComp);

	for (i=0; i<n; i++) {
		/* foreach entrance do */
		pos = entrances[i];
		if (  (maze->PHYSstone[pos]>=0)
		    ||(IsBitSetBS(groom->squares,pos)))
			continue;
		GroomIncPos(maze,pos,groom);
		GrowDFS(maze,groom,g+(IsBitSetBS(maze->dead,pos)?1:0));
		GroomExcPos(maze,pos,groom);
	}
}

int GrowRoom3(MAZE *maze, PHYSID pos, PHYSID prev, GROOM *groom)
/* run a DFS Branch and Bound through the goal area search space to find 
 * the "best" goal area topology, least entrances and then most squares. */
{
	PHYSID p;

	/* setup distance array */
	SetMinDist(maze);
	IdaInfo->CurrentHashGen = NextHashGen++;
	IdaInfo->node_count = 0;

	/* Asimilate all goals connected to pos */
	AsimGoals(maze,pos,groom);
if (groom->number_goals < 3) return(0);
	PickUpEntrances(maze,groom->index);
	BestScore = EvaluateGroom(groom);
	CopyBS(BestGoalArea,groom->squares);
	GrowDFS(maze,groom,0);
	for (p=0; p<XSIZE*YSIZE; p++) {
		if (IsBitSetBS(groom->squares,p))
			GroomExcPos(maze,p,groom);
		if (IsBitSetBS(BestGoalArea,p))
			GroomIncPos(maze,p,groom);
	}
	return(1);
}


int GrowRoom2(MAZE *maze, PHYSID pos, PHYSID prev, GROOM *groom) {
/* use topological information to grow goal areas */
	int j;
	int dir,dir2;
	
	if (IsBitSetBS(maze->wall,pos)) return(0);
	if (maze->groom_index[pos] >=0 ) return(0);
	/* if same struct as prev, include */
	if (maze->Phys[pos].lstruct == maze->Phys[prev].lstruct) {
		goto PROPAGATE;
	}
	/* if this is a room with no stones in it, include it */
	if (  (maze->structs[maze->Phys[pos].lstruct].number_stones==0)
	    &&(maze->structs[maze->Phys[pos].lstruct].room==1)) {
		goto PROPAGATE;
	}
	if (maze->Phys[pos].goal>=0) {
		goto PROPAGATE;
	}
	if (maze->PHYSstone[pos]>=0) {
		return(0);
	}
	if (IsBitSetBS(maze->one_way,pos)) {
		goto PROPAGATE;
	}
	/* now, this is a square not a goal, nor stone, check now if we
	 * should make this part of the goal area */
	/* if the min_dim does not contain stones make all of it part of the
	 * goal area */
	if (maze->Phys[pos].tunnel>8) return(0);
	j = pos;
	j += maze->Phys[pos].min_dim;
	while (  (IsBitSetBS(maze->wall,j))
	       &&(maze->groom_index[j] < 0)
	       &&(maze->PHYSstone[j]<0)) {
		j += maze->Phys[pos].min_dim;
	}
	if (maze->PHYSstone[j]<0) {
		j = pos;
		j -= maze->Phys[pos].min_dim;
		while (  (!IsBitSetBS(maze->wall,j))
		       &&(maze->groom_index[j] < 0)
	       	       &&(maze->PHYSstone[j]<0)) {
			j -= maze->Phys[pos].min_dim;
		}
	}
	if (maze->PHYSstone[j]<0) {
		/* OK, there are no stones in the min distance */
		goto PROPAGATE;
	}
	return(1);
PROPAGATE:
	GroomIncPos(maze,pos,groom);
	if (IsBitSetBS(maze->one_way,pos)&&prev!=0&&maze->Phys[pos].goal<0)
	{
		dir = pos - prev;
		dir = DiffToDir(dir);
		for (dir2=NORTH; dir2<=WEST; dir2++) {
			if ( ConnectedDir(maze,pos,dir,dir2)
		    	    &&(IsBitSetBS(maze->M[dir2],pos))
		    	    &&(IsBitSetBS(maze->S[OppDir[dir2]],
					  pos+DirToDiff[dir2]))) {
				GrowRoom2(maze,pos+DirToDiff[dir2],pos,groom);
			}
		}
	} else {
		for (dir2=NORTH; dir2<=WEST; dir2++) {
	     		if (IsBitSetBS(maze->M[dir2],pos)) 
				GrowRoom2(maze,pos+DirToDiff[dir2],pos,groom);
		}
	}
	return(0);
}

int GrowRoom1(MAZE *maze, PHYSID pos, PHYSID prev, GROOM *groom) {
/* return 0 if pos is not part of the goal area, 1 if it is */
	int j,count,dir;

	
	if (IsBitSetBS(maze->wall,pos)) return(0);
	if (maze->groom_index[pos] >=0 ) return(0);
	if (maze->Phys[pos].goal>=0) {
		goto PROPAGATE;
	}
/*
	if (IsBitSetBS(maze->dead,pos)) return(0);
*/
	if (  (maze->structs[maze->Phys[pos].lstruct].number_stones==0)
	    &&(maze->structs[maze->Phys[pos].lstruct].room==1)) {
		goto PROPAGATE;
	}
	if (maze->PHYSstone[pos]>=0) return(0);
	if (IsBitSetBS(maze->one_way,pos)) {
		if (  (maze->Phys[pos].free>2)
		    &&(IsBitSetBS(maze->one_way,prev))) return(0);
		else goto PROPAGATE;
	}
	/* now, this is a square not a goal, nor stone, check now if we
	 * should make this part of the goal area */
	/* if the min_dim does not contain stones make all of it part of the
	 * goal area */
	if (maze->Phys[pos].tunnel>8) return(0);
	j = pos;
	j += maze->Phys[pos].min_dim;
	while (  (!IsBitSetBS(maze->wall,j))
	       &&(maze->groom_index[j] < 0)
	       &&(maze->PHYSstone[j]<0)) {
		j += maze->Phys[pos].min_dim;
	}
	if (maze->PHYSstone[j]<0) {
		j = pos;
		j -= maze->Phys[pos].min_dim;
		while (  (!IsBitSetBS(maze->wall,j))
		       &&(maze->groom_index[j] < 0)
	       	       &&(maze->PHYSstone[j]<0)) {
			j -= maze->Phys[pos].min_dim;
		}
	}
	if (maze->PHYSstone[j]<0) {
		/* OK, there are no stones in the min distance */
		goto PROPAGATE;
	}
	return(1);
PROPAGATE:
	GroomIncPos(maze,pos,groom);
	count = 0;
	for (dir=NORTH; dir<=WEST; dir++) {
		count += GrowRoom1(maze,pos+DirToDiff[dir],pos,groom);
	}
	return(0);
}

void PickUpEntrances(MAZE *maze, int index) {
	PHYSID pos;
	int dir;

	maze->grooms[index].n = 0;
	maze->grooms[index].maninout = 0;
	maze->grooms[index].deadentrances = 0;
	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (  IsBitSetBS(maze->out,pos)
		    ||IsBitSetBS(maze->wall,pos)
		    ||maze->groom_index[pos] < 0)
			continue;
		if (index != maze->groom_index[pos]) continue;
		for (dir=NORTH; dir<=WEST; dir++) {
			if (  maze->groom_index[pos+DirToDiff[dir]]<0) {
			    if (IsBitSetBS(maze->S[OppDir[dir]],pos+DirToDiff[dir])) {
				maze->grooms[index].
					locations[maze->grooms[index].n] 
					= pos;
				maze->grooms[index].
					entrances[maze->grooms[index].n]
					= pos+DirToDiff[dir];
				maze->grooms[index].n++;
				if (maze->grooms[index].n==MAX_LOCATIONS) {
					maze->grooms[index].n = 0;
					return;
				}
			    } else if (IsBitSetBS(maze->M[OppDir[dir]],
					pos+DirToDiff[dir])) {
				/* count the man only entrances */
				maze->grooms[index].maninout++;
			    }
			}
		}
	}
	ValidateEntrances(maze,maze->grooms+index);
}

int SubMacro(MAZE *maze, MOVE *moves, int *move_number) {
/* Substitute a move onto a macro square with a macro move */
/* return 1 only if the macro should be the only move(s) to try in this
 * position */
	int    i,j;
	MACRO  *macro;
	GMNODE *gmnode;
	MOVE   *m = &(moves[*move_number]);
	MOVE   *o_m;
	int     o_n;
	int     dist;

	if (maze->macros[m->to].type != 0) {
	   switch (maze->macros[m->to].type) {
	   case 1:
	   case 3:
		break;
	   case 2:
		if (Options.mc_tu==0) break;
		/* This is the One way Tunnel Macro */
		for (i=0; i<maze->macros[m->to].number_macros; i++) {
			macro = &(maze->macros[m->to].macros[i]);
			if (m->last_over==macro->from) {
				/* Check for stones in Tunnel first */
				j = DistToGoal(maze,m->from,
					       macro->to,&(m->last_over));
				if (j!=-1) {
				       m->to=macro->to;
				       m->macro_id  = 2;
				       m->move_dist = j;
				       return(SubMacro(maze,moves,move_number));
				} else {
				       /* Stone was in the way, so it creates a
				        * deadlock moving this way! */
				       *m = DummyMove;
				       return(0);
				}
			}
		}
		break;
	   case 4:
		if (Options.mc_gm==0) break;
		gmnode = maze->gmtrees[maze->groom_index[m->to]];
		if (gmnode == NULL) return(0);
		o_m = moves;
		o_n = 0;
		for (i=0; i<gmnode->number_entries; i++) {
			if (gmnode->entries[i].entrance_loc==m->last_over){
				/* check if Macro move is possible if more
				 * then one entrance to the goal exist */
			      dist = DistToGoal(maze,
					m->from,gmnode->entries[i].goal_loc,
					&(o_m->last_over));
			      if (dist==-1) 
					continue;
			      o_m->move_dist = dist;
			      o_m->from      = m->from;
			      o_m->to        = gmnode->entries[i].goal_loc;
			      o_m->macro_id  = 4;
			      o_m++;
			      o_n++;
			}
		}
		if (o_n > 0) {
			*move_number = o_n;
			return(1);
		} else {
			/* if we could have made a macro move, but something
			   is in the way, we are really screwed, NOTE: we
			   should never have that problem! */
			return(0);
		}
		break;
	   }
	}
	return(0);
}

void DelGMTree(GMNODE *gmnode) {
	int     i;
	if (gmnode==NULL) return;
	gmnode->references--;
	if (gmnode->references>0) return;
	GMDelHashEntry(gmnode->hashkey);
	for (i=0; i<gmnode->number_entries; i++) {
		DelGMTree(gmnode->entries[i].next);
	}
	My_free(gmnode->entries);
	My_free(gmnode);
}

GMNODE *GetGMTree() {
	GMNODE *gmnode;

	gmnode = (GMNODE*)My_malloc(sizeof(GMNODE));
	gmnode->references	= 1;
	gmnode->number_entries	= 0;
	gmnode->min_dist	= 255;
	gmnode->hashkey		= 0;
	gmnode->entries		= NULL;
	return(gmnode);
}
	


int StartBuildGMTree(MAZE *start_maze, GROOM *groom) {
/* starts the building of the GMtree, copies working maze */
/* no stone is in goal area */

	MAZE	   *maze;
	IDA         idainfo,*old_idainfo;

	int i;
	if (groom->stone_inside) {
		Debug(2,0,"No Macro: Stone inside\n");
		return(0);
	}
	if (groom->n > 4) {
		Debug(2,0,"No Macro: Too many entries (%i)\n",groom->n);
		return(0);
	}

	GMInitHashTable();

	/* setup IdaInfo */
	old_idainfo = IdaInfo;
	InitIDA(&idainfo);
	IdaInfo			= &idainfo;
	idainfo.IdaMaze		= maze = CopyMaze(start_maze);

	/* setup work maze */
	/* remove all stones */
	for (i=0; i<maze->number_stones; i++ ) {
		UnsetBitBS(maze->stone,maze->stones[i].loc);
		maze->PHYSstone[maze->stones[i].loc]=-1;
	}
	maze->number_stones=0;

	start_maze->gmtrees[groom->index] = BuildGMTree(0,groom);
	Debug(0,0,"%s MACRO, nodes: %li n: %i\n",
		(start_maze->gmtrees[groom->index] != NULL)?"YES":"NO",
		IdaInfo->node_count,groom->n);
	IdaInfo = old_idainfo;
	DelCopiedMaze(maze);

	if (start_maze->gmtrees[groom->index] != NULL) {
	    for (i=0; i<groom->n; i++ ) {
	   	start_maze->macros[groom->locations[i]].type = 4;
	      	start_maze->macros[groom->locations[i]].number_macros = 1;
	      	start_maze->macros[groom->locations[i]].macros = NULL;
	    }
	    return(1);
	}
	return(0);
}

#define OUTSIDE(try) ( (try&1))
#define OPTIMAL(try) (!(try&2))
#define QSISAFE(try) ( (try&4))

GMNODE *BuildGMTree(int depth, GROOM *groom) {
/* This routine is responsible to create and fill up the gmnode for the
   current IdaMaze position, it will create at least one macro for the
   situation that we can't use the outside.
*/

	int       entri,stonei,goali,try,index,dead;
	BitString safe, fixed;
	BitString targets,reach;
	MAZE     *maze = IdaInfo->IdaMaze;
	PHYSID    goalpos;
	GMNODE   *gmnode, *ret_gmnode;
	int	  no_outside_created;	/* was there at least one macro not
					   using the outside area? */
	int	  num_entries_per_entri;/* ensure at least 1 entry per entri */
	int	  max_try;		/* record how far we had to go to 
					   stop futility cutoffs */

	SR(Debug(4,depth,"Enter GMBuilt\n"));
	if (IdaInfo->node_count>4000) {
		SR(Debug(4,depth,"node_count limit exceeded\n"));
		return(NULL);
	}
	IdaInfo->node_count++;

	/* check for a transposition */
	gmnode = GMGetHashTable(NormHashKey(maze));
	if (gmnode!=NULL) {
		gmnode->references++;
		SR(Debug(4,depth,"found in GM TT\n"));
		return(gmnode);
	} else gmnode = GetGMTree();

	if (maze->number_stones==groom->number_goals) {
		SR(Debug(4,depth,"'GOAL' found\n"));
		gmnode->min_dist=0;
		return(gmnode);
	}

	FixedGoals(maze, fixed, groom->index);
	max_try = 0;
	for (entri=0; entri<groom->n; entri++) {
		/* First find out a few interesting things, such as
		   which are safe squares, make sure
		   everything empty is still reachable and if not exit
		   with ERROR message - should never happen, since the
		   node above is responsible to pick only squares that
		   keep everything reachable. */

		SR(Debug(4,depth,"next entrance: %i\n",entri));
		no_outside_created = NO;
		num_entries_per_entri = 0;
		for (try=0; try<8; try++) {
			/* we can try 8 different ways to find target
			   squares, if we found some, try if we can make 
			   a tree underneeth, if not keep trying */
			SR(Debug(4,depth,"try: %i\n",try));

			/* this shortcuts in case we cound macros, but now
			   are searching only for one that is not using the 
			   outside */
			if (   num_entries_per_entri > 0 
			    && no_outside_created==NO
			    && OUTSIDE(try)) 
				continue;

			if (QSISAFE(try)) {
				/* search this with all liberty, find
				   anything that would go */
				dead =QuasiSafeSquares(maze, safe, groom->index,
					1, 0);
			} else {
				dead =SafeSquares(maze, safe, groom->index,
					OUTSIDE(try), OPTIMAL(try));
			}
			if (dead && !OPTIMAL(try) && OUTSIDE(try)) {
				SR(Debug(4,depth,"Dead square(s)!\n"));
				DelGMTree(gmnode);
				return(NULL);
			}
			if (Is0BS(safe)) continue;
			GoalReach(maze,reach,groom->locations[entri],
				groom->entrances[entri],
				OUTSIDE(try)?-1:groom->index, OPTIMAL(try));
			BitAndBS(targets,safe,reach);
			if (Is0BS(targets)) continue;
			BitAndEqBS(targets,fixed);
			if (Is0BS(targets))
				BitAndBS(targets,safe,reach);
			/* being here means there are squares in target */

			/* Now go through the targets to create the goal macros
		   	and recursively call BuildGMTree */
			stonei = maze->number_stones;
			maze->number_stones++;
			while (Isnt0BS(targets)) {
				goalpos = GetBestTarget(maze,targets);
				goali = maze->Phys[goalpos].goal;
				UnsetBitBS(targets,goalpos);
				maze->stones[stonei].loc = goalpos;
				maze->PHYSstone[goalpos] = stonei;
				SetBitBS(maze->stone,goalpos);
	
				ret_gmnode = BuildGMTree(depth+1,groom);
				if (ret_gmnode!=NULL) {
					index = gmnode->number_entries++;
					gmnode->entries = (GMENTRY*)My_realloc(
						gmnode->entries,sizeof(GMENTRY)*
						gmnode->number_entries);
					if (  gmnode->min_dist
					    > ret_gmnode->min_dist)
						gmnode->min_dist
						= ret_gmnode->min_dist;
					gmnode->entries[index].goal      
						= (signed char)goali;
					gmnode->entries[index].entrance  
						= (signed char)entri;
					gmnode->entries[index].distance  
						= (signed char)
							GetWeightManpos(maze,
					   	maze->goals[goali].loc,
					   	groom->locations[entri],
					   	groom->entrances[entri]);
					gmnode->entries[index].last_over = 0;
					gmnode->entries[index].goal_loc  
						= maze->goals[goali].loc;
					gmnode->entries[index].entrance_loc 
						= groom->entrances[entri];
					gmnode->entries[index].next =ret_gmnode;
					no_outside_created =
					    no_outside_created || !OUTSIDE(try);
					num_entries_per_entri++;
				}
				UnsetBitBS(maze->stone,	
					   maze->stones[stonei].loc);
				maze->PHYSstone[goalpos] = -1;
				maze->stones[stonei].loc = 0;
				/* enough if most restricted growing worked */
				/* Does not work (#51!!) */
/*
*/
				if (try==0 && gmnode->min_dist==0) {
					SR(Debug(4,depth,"futility cut\n"));
					break;
				}
			}
			maze->number_stones--;
			if (try > max_try) max_try = try;
			/* if we found macros and at least one not using the
			   outside of the maze, we are done */
			if (num_entries_per_entri > 0 && no_outside_created) 
				break;
		}
		/* in case one of the entrances can't find any goals! */
		if (num_entries_per_entri==0) {
			max_try = 100;
		}
	}
	/* if we found no "optimal_no_outside" macro, don't
	   allow futility cutoffs further up! */
	if (gmnode->min_dist==0 && max_try>0) gmnode->min_dist=1;
	NormHashKey(maze);
	GMStoreHashTable(maze,gmnode);
	SR(Debug(4,depth,"normal exit, entries: %i\n", gmnode->number_entries));
	return(gmnode);
}

void MarkReachGRoom(MAZE *maze, int index) {
/* recursive function to mark the fields that are reachable */
	
	static PHYSID stack[ENDPATH];
	PHYSID pos;
	int next_in, next_out,dir;
	CleanReach(maze);

	stack[0] = maze->manpos;
	next_in  = 1;
	next_out = 0;
	while (next_out < next_in) {
		pos = stack[next_out++];
		if (IsBitSetBS(maze->reach, pos) ) continue;
		if (maze->PHYSstone[pos] >= 0 && pos!=maze->manpos) continue;
		if (   index >=0
			/* allow the man on all dead squares since they cant
			   be blocked by stones */
		    && !IsBitSetBS(maze->dead,pos)
		    && pos != stack[0]
		    && maze->groom_index[pos] != index) continue;
		/* The following is used to make local move generation
		 * possible */
		if (AvoidThisSquare==pos) continue;
		SetBitBS(maze->reach,pos);
		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->M[dir],pos)) 
				stack[next_in++] = pos +DirToDiff[dir];
		}
	}
	CopyBS(maze->old_no_reach,maze->no_reach);
	BitNotAndNotBS(maze->no_reach,maze->reach,maze->out);
	BitAndNotEqBS(maze->no_reach,maze->stone);
}

void GoalReach(MAZE *maze, BitString v, PHYSID start, PHYSID manpos, 
		int index, int optimal)
{
/* pseudo-recursive function to mark the fields that are reachable */
/* v contains all bits set for locations of goals that are reachable */
/* index is the goal room index, 
   1.) if index < 0 then allow the man to go all over the place 
   2.) if optimal==YES then allow only to reach goals the optimal way */

/* The man cannot leave the goal room, since we only know where the stones
 * are in the goal room, we cannot assume, that the outside world cooperates
 * in allowing the man to move around coming in through other entrances
 * (#38) */

        static PHYSID stack[ENDPATH];
        static PHYSID from[ENDPATH];
        static int    dist[ENDPATH];
        PHYSID pos,fro,old_stone,old_man;
        int next_in, next_out, dir, dir1, dis;
	BitString s_visited[4];

	memset(s_visited,0,sizeof(BitString)*4);

	from[0]  = manpos;
	old_stone= start;
	old_man  = maze->manpos;
        stack[0] = start;
	dist[0]  = 0;
        next_in  = 1;
        next_out = 0;
	Set0BS(v);
        while (next_out < next_in) {
		fro = from[next_out];
		dis = dist[next_out];
                pos = stack[next_out++];
                if (maze->PHYSstone[pos] >= 0) 
			continue;
		if (   optimal
		    && !IsBitSetBS(maze->one_way,pos)
		    && dis > GetWeightManpos(maze,pos,start,old_man))
			continue;
		dir = DiffToDir(fro-pos);
		if (IsBitSetBS(s_visited[dir],pos)) continue;

		/* set maze up to make the reach analysis for the man */
		MANTO(maze,fro);
		AvoidThisSquare = pos;
		MarkReachGRoom(maze,index);

		for (dir=NORTH; dir<=WEST; dir++) {
			if (IsBitSetBS(maze->reach,pos+DirToDiff[dir]))
				SetBitBS(s_visited[dir],pos);
		}
		if (maze->Phys[pos].goal>=0) {
			SetBitBS(v,pos);
		}

		for (dir=NORTH; dir<=WEST; dir++) {
                	if (   IsBitSetBS(maze->S[dir],pos)
		    	    && maze->groom_index[pos+DirToDiff[dir]]>=0
		    	    && IsBitSetBS(maze->reach, pos-DirToDiff[dir] ))  {
				from[next_in] = pos;
				dist[next_in] = dis + 1;
				stack[next_in++] = pos+DirToDiff[dir];
			}
		}
        }
	MANTO(maze,old_man);
	AvoidThisSquare = 0;
}

int DeadGoal(MAZE *maze, PHYSID pos) {
/* Returns 1 if the goal at that place is a dead goal, meaning a stone
 * placed there will not be moveable anymore */


	/* check two-wall patterns */
	if (  (  (IsBitSetBS(maze->wall,pos+YSIZE))
	       ||(IsBitSetBS(maze->wall,pos-YSIZE)))
	    &&(  (IsBitSetBS(maze->wall,pos+1))
	       ||(IsBitSetBS(maze->wall,pos-1)))) return(1);
	/* check four-block */
	if (   (  (IsBitSetBS(maze->wall,pos+YSIZE+1))
		||(maze->PHYSstone[pos+YSIZE+1]>=0))
	    && (  (IsBitSetBS(maze->wall,pos+YSIZE))
		||(maze->PHYSstone[pos+YSIZE]>=0))
	    && (  (IsBitSetBS(maze->wall,pos+1))
		||(maze->PHYSstone[pos+1]>=0))) return(1);
	if (   (  (IsBitSetBS(maze->wall,pos+YSIZE-1))
		||(maze->PHYSstone[pos+YSIZE-1]>=0))
	    && (  (IsBitSetBS(maze->wall,pos+YSIZE))
		||(maze->PHYSstone[pos+YSIZE]>=0))
	    && (  (IsBitSetBS(maze->wall,pos-1))
		||(maze->PHYSstone[pos-1]>=0))) return(1);
	if (   (  (IsBitSetBS(maze->wall,pos-YSIZE-1))
		||(maze->PHYSstone[pos-YSIZE-1]>=0))
	    && (  (IsBitSetBS(maze->wall,pos-YSIZE))
		||(maze->PHYSstone[pos-YSIZE]>=0))
	    && (  (IsBitSetBS(maze->wall,pos-1))
		||(maze->PHYSstone[pos-1]>=0))) return(1);
	if (   (  (IsBitSetBS(maze->wall,pos-YSIZE+1))
		||(maze->PHYSstone[pos-YSIZE+1]>=0))
	    && (  (IsBitSetBS(maze->wall,pos-YSIZE))
		||(maze->PHYSstone[pos-YSIZE]>=0))
	    && (  (IsBitSetBS(maze->wall,pos+1))
		||(maze->PHYSstone[pos+1]>=0))) return(1);
	return(0);
}

