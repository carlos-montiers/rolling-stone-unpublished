#include "board.h"

/************************************************************************/
/*									*/
/*	Classical IDA* routine, compares if g+h<threshold		*/
/*	before searching deeper. 					*/
/*									*/
/*									*/
/************************************************************************/

#define MAXEFFORT 100

IDA *IdaInfo;
int PATTERNRATIO = 20;

int IsLocalCut;

#ifdef RRR
int MAGICNUMBER = 500000;
#define INCMAGIC 50000
#endif

#ifdef RANDOM
double percent;
#endif

int StartIda(int nomacro) {
/* Sets up all data structures and repeatedly calls ida with increasing 
   threshold to guarantee optimal solutions, returns 0 if solution found 
   otherwise the smallest heuristic value seen at any leaf node if this is
   ENDPATH there is no solution - deadlock */

	int       result=ENDPATH;
	int32_t   last_tree_size;
	MAZE     *maze;
	PHYSID    pos;
	int       i, j;
#ifdef DYNAMICDB
	int 	  tnc;
#endif
#ifdef RRR
	int CurrAbort, prev_patt;
	int save_count;
	int trialnum;

	CurrAbort = IdaInfo->AbortNodeCount;
	IdaInfo->AbortNodeCount = MAGICNUMBER;
	PATTERNRATIO += 3;
	prev_patt = 0;
	save_count = 0;
	trialnum = 0;
#endif

for( i = 0; i < XSIZE * YSIZE; i++ )
  for( j = i; j < XSIZE * YSIZE; j++ ) {
    IdaInfo->IdaMaze->influence[ i ][ j ] = 0;
    IdaInfo->IdaMaze->influence[ j ][ i ] = 0;
  }

	/* initialize data structures */
	total_node_count = 0;
	penscount = penmcount = deadscount = deadmcount = 0;
	IdaInfo->node_count = 0;
#ifdef STONEREACH
	SetStoneReach( IdaInfo->IdaMaze );
	IdaInfo->reach_cuts = 0;
#endif
#ifdef DYNAMICDB
	LoadConflicts( IdaInfo->IdaMaze, IdaInfo->IdaMaze->conflicts );
	tnc = total_node_count;
	total_node_count = 0;
	printf( "%i nodes searched in database load\n", tnc );
#endif
#ifdef RANDOM
	percent = 1.0;
#endif
	init_stats();
START_IDA:
	InitHashTables();
	/* IdaInfo->IdaArray[0].solution = (MOVE){0,0,0,0,0,0}; */
        memset( &IdaInfo->IdaArray[ 0 ].solution, 0, sizeof( MOVE ) );
	dl_pos_nc = dl_pos_sc = dl_neg_nc = dl_neg_sc = 0;
	pen_pos_nc = pen_pos_sc = pen_neg_nc = pen_neg_sc = 0;
	IdaInfo->CurrentHashGen = NextHashGen++;
	AvoidThisSquare = 0;
	Set0BS(IdaInfo->IdaManSquares);
	Set0BS(IdaInfo->IdaStoneSquares);
	IdaInfo->CurrentSolutionDepth = ENDPATH;
	for (IdaInfo->Threshold = IdaInfo->IdaMaze->h;
	       (IdaInfo->CurrentSolutionDepth > IdaInfo->Threshold)
	     &&(  (IdaInfo->Threshold < (IdaInfo->IdaMaze->h<<1) )
		||(IdaInfo->Threshold < 100))
	     &&(!AbortSearch());
	     IdaInfo->Threshold += IdaInfo->ThresholdInc) {
		Debug(2,0,"Threshold %i (%i)\n",
			IdaInfo->Threshold, IdaInfo->IdaMaze->h);
		GTVAny(GTVOpen(IdaInfo->Threshold,
			       GTVFen(IdaInfo->IdaMaze)));
		last_tree_size = IdaInfo->r_tree_size;
		IdaInfo->r_tree_size=0;
		IdaInfo->v_tree_size=0;
		IdaInfo->IdaMaze->goal_sqto = -1;
IsLocalCut = NO;
		result = Ida(0,0,0); /**********************************/
		GTVAny(GTVClose());
#ifdef RRR
		IdaInfo->node_count -= save_count;
		print_stats(2);
		IdaInfo->node_count += save_count;
#else
		print_stats(2);
#endif
		if (result>=ENDPATH) 
			IdaInfo->Threshold = ENDPATH + IdaInfo->ThresholdInc;
	}
	IdaInfo->Threshold -= IdaInfo->ThresholdInc;
	if (result<ENDPATH) result = IdaInfo->Threshold - IdaInfo->IdaMaze->h;


#ifdef RRR
	if( AbortSearch() && total_node_count < CurrAbort ) {
	  if( IdaInfo->IdaMaze->conflicts->number_patterns == prev_patt ) {
	    MAGICNUMBER = CurrAbort;
	  } else {
	    MAGICNUMBER += INCMAGIC;
	  }
	  prev_patt = IdaInfo->IdaMaze->conflicts->number_patterns;
	  if( total_node_count + MAGICNUMBER < ( CurrAbort >> 2 ) ) {
	    IdaInfo->AbortNodeCount += MAGICNUMBER;
	  } else {
	    IdaInfo->AbortNodeCount = CurrAbort;
	  }
	  save_count = IdaInfo->node_count;
	  PATTERNRATIO--;
	  trialnum++;
	  goto START_IDA;
	}
#endif

	/* if we used goal macros and did not find a asolution and did not
	 * exhaust the search effort (#38), rerun search without goal macros */
	if (   (   (result >= ENDPATH)
	        || (IdaInfo->Threshold+IdaInfo->ThresholdInc >= (IdaInfo->IdaMaze->h<<1)))
	    && nomacro==YES
	    && IdaInfo->IdaMaze->number_grooms > 0
	    && (  IdaInfo->node_count < IdaInfo->AbortNodeCount
		||IdaInfo->AbortNodeCount==-1)) {
		/* Turn off macros */
printf("removing goal macro\n");
		maze = IdaInfo->IdaMaze;
		for (pos = 0; pos < XSIZE*YSIZE; pos++) {
			maze->groom_index[pos] = -2;
			if (maze->macros[pos].type == 4)
				maze->macros[pos].type = 0;
		}
		for (i=0; i<maze->number_grooms; i++) {
			DelGMTree(maze->gmtrees[i]);
		}
		My_free(maze->gmtrees);
		maze->gmtrees = NULL;
		maze->number_grooms = 0;
		My_free(maze->grooms);
		maze->grooms = NULL;

		goto START_IDA;
	}
#ifdef DYNAMICDB
	total_node_count += tnc;
	printf( "Total nodes searched = %i\n", total_node_count );
#endif
	PrintSolution();

	printf( "pensearch nodes: %" PRIi32 "( %" PRIi32 " )\n", penscount, penmcount );
	printf( "deadsearch nodes: %" PRIi32 "( %" PRIi32 " )\n", deadscount, deadmcount );

/* for( i = 0; i < XSIZE * YSIZE; i++ ) */
/*   if( !IsBitSetBS( IdaInfo->IdaMaze->out, i ) && */
/*       !IsBitSetBS( IdaInfo->IdaMaze->dead, i ) && */
/*       !IsBitSetBS( IdaInfo->IdaMaze->goal, i ) ) { */
/*     printf( "%3i:", i ); */
/*     for( j = 0; j < XSIZE * YSIZE; j++ ) */
/*       if( !IsBitSetBS( IdaInfo->IdaMaze->out, j ) && */
/* 	  !IsBitSetBS( IdaInfo->IdaMaze->dead, j ) && */
/* 	  !IsBitSetBS( IdaInfo->IdaMaze->goal, j ) ) */
/* 	printf( " %i", IdaInfo->IdaMaze->influence[ i ][ j ] ); */
/*     printf( "\n" ); */
/*   } */

#ifdef DYNAMICDB
	DumpConflicts( IdaInfo->IdaMaze, IdaInfo->IdaMaze->conflicts );
#endif

	return(result);
}

void PrintSolution()
{
	MAZE     *maze;
	MOVE      lastmove;
	MOVE      solution[ENDPATH];
	UNMOVE    unmove;
	int       i,g;

	Debug(0,-1,"Path: ");
	i = 0; g = 0;
	lastmove = DummyMove;
	maze = CopyMaze(IdaInfo->IdaMaze);
	while (IdaInfo->IdaArray[i].solution.from != 0 && maze->h>0) {
		solution[i]=IdaInfo->IdaArray[i].solution;
		if (lastmove.to==IdaInfo->IdaArray[i].solution.from) {
			Debug(0,-1,"%s", 
				HumanMove(IdaInfo->IdaArray[i].solution)+2);
		} else {
			Debug(0,-1," %s", 
				HumanMove(IdaInfo->IdaArray[i].solution));
		}	
		lastmove = IdaInfo->IdaArray[i].solution;
		MakeMove(maze,&(IdaInfo->IdaArray[i].solution),&unmove);
		if (IdaInfo->IdaArray[i].unmove.move_dist>1) {
			Debug(0,-1,"*");
		}
		g += IdaInfo->IdaArray[i].unmove.move_dist;
		i++;
	}
	Debug(0,-1,"\n(moves: %i depth: %i)\n",g,i);
	solution[i] = DummyMove;
	DelCopiedMaze(maze);

	maze = CopyMaze(IdaInfo->IdaMaze);
	if (ValidSolution(maze,solution)==0) {
		Mprintf(0,"****** Invalid Solution ******\n");
	}
	DelCopiedMaze(maze);
}

int IsGoalNodeNorm(int g)
{
	int i;

	if (IdaInfo->IdaMaze->h!=0) {
		return(0);
	} else {
		if (g < IdaInfo->CurrentSolutionDepth) {
			IdaInfo->CurrentSolutionDepth=g;
			for (i=0; i<g; i++) {
				IdaInfo->IdaArray[i].solution
					= IdaInfo->IdaArray[i].currentmove;
			}
		}
		if (g > IdaInfo->Threshold) {
			Debug(0,0,"Premature Goal Found! Depth: %d\n",g);
			return(0);
		} else return(1);
	}
}

int Ida(int treedepth, int g, int prev_pen) {
/* the procedure that does the work at one node. it returns 
	X - the smallest h underneath this node */

	IDAARRAY  *S;
#ifdef STONEREACH
	IDAARRAY  *lastS;
#endif
	HASHENTRY *entry;
	HASHKEY    h;
	MOVE       bestmove;
	int 	   min_h,result,i;
	int32_t    tree_size_save;
	int32_t    max_tree_size, tmp_tree_size;
	int        dir,cut,old_IsLocalCut;
int cutttl = 0;
	int	   targetpen;
	int	   dlsearched = 0, pensearched = 0;
	int old_h = IdaInfo->IdaMaze->h-IdaInfo->IdaMaze->pen;
	int in_h = IdaInfo->IdaMaze->h;
#ifdef STONEREACH
	int oldneilflag;

	IdaInfo->depth = treedepth;
#endif

        SR(Debug(4,treedepth,"starting ida (h=%i) (%s) %d\n",
		IdaInfo->IdaMaze->h, 
		treedepth==0?"a1a1":PrintMove(IdaInfo->IdaArray[treedepth-1].currentmove),IdaInfo->node_count));
	S = &(IdaInfo->IdaArray[treedepth]);

	GTVAny(GTVNodeEnter(treedepth,g,0,GTVMove(treedepth?IdaInfo->IdaArray[treedepth-1].currentmove:DummyMove),0));
	if (AbortSearch()) {
		GTVAny(GTVNodeExit(treedepth,0,"Abort_Search"));
		return(IdaInfo->IdaMaze->h);
	}
	tree_size_save = IdaInfo->v_tree_size;
	IncNodeCount(treedepth);

	/* check for goal state, if yes return(0) */
	if (IsGoalNodeNorm(g)) {
		SR(Debug(4,treedepth,"Found goal (%i %i)******************\n",
			IdaInfo->IdaMaze->h, IdaInfo->IdaMaze->number_stones));
		GTVAny(GTVNodeExit(treedepth,0,"Goal_found"));
if (IsLocalCut==YES) {
  printf("\nLocalCut this Goal\n");
}
		return(0);
	}

	/* The following order is important, we might find a goal
	   an iteration earlier! */
	/* check for cutoff: is g+h > threshold => return(0) (fail) */
	if (g+IdaInfo->IdaMaze->h>IdaInfo->Threshold) {
		SR(Debug(4,treedepth,"Threshold cutoff (%i=%i+%i)\n", 
			g+IdaInfo->IdaMaze->h,g,IdaInfo->IdaMaze->h));
		GTVAny(GTVNodeExit(treedepth,IdaInfo->IdaMaze->h,
			"Threshold_Cutoff"));
		return(IdaInfo->IdaMaze->h);
	}

	/* check trans table if searched already */
	entry = GetHashTable(IdaInfo->IdaMaze);
	if (entry != NULL) {
		if (entry->pathflag == 1) {
			SR(Debug(4,treedepth, "Cycle (TT)\n"));
			GTVAny(GTVNodeExit(treedepth, ENDPATH,"Cycle (TT)"));
			IdaInfo->v_tree_size+=entry->tree_size;
			IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
			return(IdaInfo->IdaMaze->h);
		}
		if (IdaInfo->Threshold - g <= entry->down) {
			SR(Debug(4,treedepth, "Futil (TT) %i<=%i\n",
				IdaInfo->Threshold-g,entry->down));
			GTVAny(GTVNodeExit(treedepth,ENDPATH,"Futil (TT)"));
			IdaInfo->v_tree_size+=entry->tree_size;
			IdaInfo->IdaMaze->goal_sqto = entry->goal_sqto;
			return(entry->min_h);
		}
		bestmove = entry->bestmove;
		SetPathFlag(IdaInfo->IdaMaze);
	} else {
		bestmove = DummyMove;
		entry = StoreHashTable(IdaInfo->IdaMaze,g,IdaInfo->Threshold-g,
			IdaInfo->IdaMaze->h,bestmove,0,dlsearched,
			pensearched,0,1);
	}

	/* Only if was not found in TT,this search makes sense */
	if (
#ifndef RANDOM
#ifdef RATIO
	    total_node_count / IdaInfo->node_count < PATTERNRATIO &&
#else
	    ( g <= ( IdaInfo->Threshold/4 ) ) &&
#endif
	    treedepth > 0 &&
#endif
	    DeadMove(IdaInfo->IdaMaze,entry,
		&(IdaInfo->IdaArray[treedepth-1].currentmove),
		treedepth,g,&dlsearched,PATTERNSEARCH)) {
#ifdef RANDOM
	percent = percent * PATTERNRATIO *
	  IdaInfo->node_count / total_node_count;
#endif
		SR(Debug(4,treedepth,"Deadlock detected (DeadMove)\n"));
		GTVAny(GTVNodeExit(
			treedepth,entry->min_h,"DeadMove: deadlck"));
		return(ENDPATH);
	}
	targetpen = ((IdaInfo->Threshold-g)-IdaInfo->IdaMaze->h) + 2;
	if (
#ifndef RANDOM
#ifdef RATIO
	    total_node_count / IdaInfo->node_count < PATTERNRATIO &&
#else
	    ( g <= ( IdaInfo->Threshold/4 ) ) &&
#endif
	    treedepth > 0 &&
#endif
	    PenMove(IdaInfo->IdaMaze,entry,
		&(IdaInfo->IdaArray[treedepth-1].currentmove),
		treedepth,g,targetpen,&pensearched,PATTERNSEARCH)>=targetpen) {
#ifdef RANDOM
	percent = percent * PATTERNRATIO *
	  IdaInfo->node_count / total_node_count;
#endif
		SR(Debug(4,treedepth,"Penalty detected (PenMove)\n"));
		GTVAny(GTVNodeExit(
			treedepth,entry->min_h,"PenMove: penalty"));
		StoreHashTable(IdaInfo->IdaMaze,g,IdaInfo->Threshold-g,
				IdaInfo->IdaMaze->h,
				DummyMove,0,dlsearched,pensearched,0,0);
		return(IdaInfo->IdaMaze->h+targetpen);
	}
	if (treedepth >= IdaInfo->ForwDepthLimit) {
		SR(Debug(4,treedepth,"Too deep in the tree!(%i)\n",treedepth));
		GTVAny(GTVNodeExit(treedepth,0,"Too_deep_in_tree"));
		return(IdaInfo->IdaMaze->h);
	}

	min_h    = ENDPATH;
	max_tree_size = -1;

	/* create and sort (bestmove first + extension of previous stone) 
	   moves */
	S->number_moves = GenerateMoves(IdaInfo->IdaMaze,&(S->moves[0]));
	S->number_moves = (*IdaInfo->MoveOrdering)
				(treedepth,S->number_moves,bestmove);

if (S->number_moves==0) return(IdaInfo->IdaMaze->h);
old_IsLocalCut = IsLocalCut;

	/* foreach move call ida */
	for (i=0; i<S->number_moves; i++) {
		if (ISDUMMYMOVE(S->moves[i])) continue;
		S->currentmove = S->moves[i];
		S->currentindex = i;

		if (   Options.cut_goal==1 
		    && S->moves[i].macro_id!=4
		    && IdaInfo->IdaMaze->goal_sqto != -1 ) {
			SR(Debug(4,treedepth,"Goal Cut move\n")); 
			continue;
		}
		if (   Options.local == 1
		    /* && i > 0 make sure moves to the goal are allowed? */
		    && RegisterMove(&(S->moves[i]),treedepth)) {
			SR(Debug(4,treedepth,"Local Cut move %s\n",
                                PrintMove(S->moves[i])));
        /* toss a coin to see if we will execute the cut or not */
       	h =   IdaInfo->IdaMaze->hashkey
                    ^ RandomTable[S->currentmove.from]
                    ^ RandomTable[S->currentmove.to];
        if (h%100 > 50) {
                        SR(Debug(4,treedepth,"Local Cut move %s\n",
                                PrintMove(S->moves[i])));
                        IsLocalCut = YES;
                        continue;
        }
		}
		/* check for deadlock */
		dir = DiffToDir(S->currentmove.to-S->currentmove.last_over);
		if (dir!=NODIR) {
			if (DeadTree(IdaInfo->IdaMaze,S->currentmove.to,dir)) {
				SR(Debug(6,treedepth,
					"DeadTree fd deadlock (%i-%i)\n",
					S->currentmove.from,
					S->currentmove.to)); 
IsLocalCut = old_IsLocalCut;
				continue;
			}
		}
		SR(Debug(6,treedepth,"MakeMove %s (%i of %i)\n", 
			PrintMove(S->currentmove),i+1,S->number_moves));
		if( !MakeMove( IdaInfo->IdaMaze, &S->currentmove,
			       &S->unmove ) ) {
IsLocalCut = old_IsLocalCut;
		  continue;
		}

		tmp_tree_size = IdaInfo->v_tree_size;

		result = Ida(treedepth+1,g+S->unmove.move_dist,
			     IdaInfo->IdaMaze->pen);
if( IsLocalCut && old_IsLocalCut == NO ) cutttl++;
if (result == 0 && IsLocalCut==YES && old_IsLocalCut==NO) {
  printf("\nLocalCut this Goal\n");
  PrintMaze(IdaInfo->IdaMaze);
}
		tmp_tree_size = IdaInfo->v_tree_size - tmp_tree_size;
		UnMakeMove(IdaInfo->IdaMaze,&(S->unmove));
		if (old_h != IdaInfo->IdaMaze->h-IdaInfo->IdaMaze->pen)
			BetterLowerBound(IdaInfo->IdaMaze);
if (result == 0 && IsLocalCut==YES && old_IsLocalCut==NO) {
  PrintMaze(IdaInfo->IdaMaze);
}
IsLocalCut = old_IsLocalCut;
		if (result < min_h) {
			min_h = result;
			bestmove = S->currentmove;
		}
		if (tmp_tree_size>max_tree_size) {
			max_tree_size = tmp_tree_size;
		}
		/* one solution found? */
		if (result == 0) {
			S->solution = S->currentmove;
			bestmove = S->currentmove;
			SetManStoneSquares(IdaInfo->IdaMaze,bestmove);
			goto END_IDA;
		}
		/* if a deadlock score came back, maybe we are already in a
			deadlock??? NOTE: A test for RevMove here could
			propagate the deadlock up  - did not work - why? */
	}

END_IDA:
/* Fix stats */
    if( old_IsLocalCut == YES )
    {
	/* This node would have been cutoff prior to reaching here */
	IdaInfo->no_lcut_nodes[treedepth] ++;
	IdaInfo->no_lcut_moves[treedepth] += S->number_moves+1;
	IdaInfo->no_lcut_h[treedepth]+=in_h;
	IdaInfo->no_lcut_g[treedepth]+=g;
    }
    else if( cutttl > 0 )
    {
/* A new cut node.  Count how many moves were searched out of how many */
	IdaInfo->lcut_nodes[treedepth] ++;
	IdaInfo->lcut_moves[treedepth] += S->number_moves+1-cutttl;
	IdaInfo->lcut_allmoves[treedepth] += S->number_moves+1;
	IdaInfo->lcut_h[treedepth]+=in_h;
	IdaInfo->lcut_g[treedepth]+=g;
    }
    else /* if ( cutttl == 0 */
    {
/* No relevence cut, but this is a node examined by both algorithms */
	IdaInfo->both_nodes[treedepth] ++;
	IdaInfo->both_moves[treedepth] += S->number_moves+1;
	IdaInfo->both_h[treedepth]+=in_h;
	IdaInfo->both_g[treedepth]+=g;
    }

	/* write Transposition table */
	StoreHashTable(IdaInfo->IdaMaze,g,IdaInfo->Threshold-g,min_h,bestmove,
		IdaInfo->v_tree_size-tree_size_save,dlsearched,pensearched,0,0);
	SR(Debug(4,treedepth,"return from ida %i (%s %i)\n", 
		treedepth, PrintMove(bestmove), min_h));
	GTVAny(GTVNodeExit(treedepth,min_h,"Normal_Exit"));
	return(min_h);
}

void SetManStoneSquares(MAZE *maze, MOVE bestmove)
{
	BitString stone,man;
	int p,m;

	PushesMoves(maze,bestmove.from,bestmove.to,&p,&m,stone,man);
	BitOrEqBS(IdaInfo->IdaStoneSquares,stone);
	BitOrEqBS(IdaInfo->IdaManSquares,man);
}

void SetManStoneSquares2(MAZE *maze, MOVE bestmove)
{
	BitString stone,man;
	int p,m;

	PushesMoves2(maze,bestmove.from,bestmove.to,&p,&m,stone,man);
	BitOrEqBS(IdaInfo->IdaStoneSquares,stone);
	BitOrEqBS(IdaInfo->IdaManSquares,man);
}

int AbortSearch() {

	/* stop any search if its limit is reached */
	if (   MainIdaInfo.AbortNodeCount >= 0 
	    && total_node_count >= MainIdaInfo.AbortNodeCount) return(1);
	if (   IdaInfo->AbortNodeCount >= 0 
	    && IdaInfo->node_count >= IdaInfo->AbortNodeCount) return(1);

	return(0);
}

void InitIDA(IDA *ida)
{
	Set0BS(ida->IdaManSquares);
	Set0BS(ida->IdaStoneSquares);
	ida->IdaMaze        = NULL;
	ida->Threshold      = 0;
	ida->ThresholdInc   = 2;
        /* ida->IdaArray[0].solution = (MOVE){0,0,0,0,0,0}; */
        memset( &ida->IdaArray[ 0 ].solution, 0, sizeof( MOVE ) );
	ida->AbortNodeCount = -1;
	ida->ForwDepthLimit = MAX_DEPTH-1;
	ida->BackDepthLimit = MAX_DEPTH-1;
	ida->CurrentHashGen = 0;
	ida->last_tree_size = 0;
	ida->base_indent    = 0;
	ida->MiniFlag       = 0;

	ida->CurrentSolutionDepth = ENDPATH;

	ida->HashTable	    = HashTableNorm;
	ida->MoveOrdering   = NewMoveOrdering;

	ida->reach_at_goal  = 0;

	ida->goal_last_to   = 0;
	ida->closest_confl  = 0;
	Set0BS(ida->shadow_stones);
	Set0BS(ida->no_reach);

	ida->r_tree_size = ida->v_tree_size = ida->start_time =
	ida->tt_hits = ida->tt_cols = ida->tt_reqs = ida->gmtt_hits =
	ida->gmtt_cols = ida->gmtt_reqs = ida->node_count     = 0;

	ida->PrintPriority  = 0;

#ifdef STONEREACH
ida->depth = 0;
#endif
}

void SetLocalCut(int k, int m, int d)
/* any of k,m set to -1 will turn local cut off
   any of k,m set to  0 will auto set parameters
   else set as sent in */
{
	if (k < 0 || m < 0 || k>m) {
		Mprintf(0,"Strange local cut parameter setting: %i,%i.\n",k,m);
		Mprintf(0,"Turning local cut off!\n");
		Options.local_k = -1;
		Options.local_m = -1;
		Options.local_d = max(YSIZE,XSIZE);
		Options.local = 0;
	} else if ((k < 1 || m < 1) && Options.autolocal==1) {
		k = 1;
		XDistHist(IdaInfo->IdaMaze, &d, &m);
		d = max(6,d);
		m = min(10,m);
		Mprintf(0,"Auto Set local cut parameter: %i,%i, %i.\n",k,m,d);
		Options.local_k = k;
		Options.local_m = m;
		Options.local_d = d;
	} else {
		Mprintf(0,"Set local cut parameter: %i,%i, %i.\n",k,m,d);
		Options.local_k = k;
		Options.local_m = m;
		Options.local_d = d;
		Options.local = 1;
		Options.autolocal = 0;
	}
}

int DistantSquares(PHYSID s1, PHYSID s2, int growding)
/* Return YES if the two squares are distant */
{
       return( ((XDistMan(IdaInfo->IdaMaze,s1,s2)>(Options.local_d+growding)))
	     &&((XDistMan(IdaInfo->IdaMaze,s2,s1)>(Options.local_d+growding))));
}

int DistantMove(MAZE *maze, MOVE *last_move, MOVE *test_move)
/* Returns YES if test_move is considered a distant move to last_move */
{
	if ( DistantSquares(test_move->from,last_move->from,
		Growding(maze,last_move->to))
	    && (test_move->macro_id != 4)
	    && (last_move->macro_id != 4))
		return(YES);
	return(NO);
}

int Growding(MAZE *maze, PHYSID sq)
/* return a growding number. The more stones in the vicinity, the larger the
 * growding number */
/* for now, the growding number is the number stones "local" to sq */
{
	int growding;
	int i;
/*	int tbl[MAXSTONES] = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
			      2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,
			      4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,
			      7,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7};
	int tbl[MAXSTONES] = {-2,-1,0,0,1,1,1,2, 2,2,2,2,1,1,1,1,*/
	static int tbl[MAXSTONES] = {0,0,0,0,0,0,0,0, 1,1,1,1,1,1,1,1,
			      2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,
			      4,4,4,4,4,4,4,4, 5,5,5,5,5,5,5,5};
	growding = 0;
	for (i=0; i<maze->number_stones; i++) {
		if (!DistantSquares(sq,maze->stones[i].loc,0)) growding++;
	}
	return(tbl[growding]);
}

int RegisterMove(MOVE *move, int depth)
/* Return YES if move should be cut off */
/* do two things, do not allow switches back into previously visited areas
          and,    do not allow too many switches between previous moves */
{
	int i,n;
	int firstswitch;

	if (   depth > 0
	    && DistantMove(IdaInfo->IdaMaze,
			   &(IdaInfo->IdaArray[depth-1].currentmove),
			   move)) {
		firstswitch = YES;
	} else {
		firstswitch = NO;
	}
	n = 0;
	/* last move was non-local, now see if we where in that area
	 * before, and if so, how many times. if we exceed k, cut */
	i = 1;
	while ( ( depth-i ) >= 0 && ( i < Options.local_m ) ) {
	   /* if we hit a goal macro move stop */
	   if ( IdaInfo->IdaArray[depth-i].currentmove.macro_id == 4 )
			break;
	   /* if move was local to ancestor, count */
	   /* AND not the same as previous stone */
	   if (   firstswitch == YES
	       && !DistantMove(IdaInfo->IdaMaze,
		   	       &(IdaInfo->IdaArray[depth-i].currentmove),
		   	       move)
	       && DistantMove(IdaInfo->IdaMaze,
			      &(IdaInfo->IdaArray[depth-i].currentmove),
			      &(IdaInfo->IdaArray[(depth-i)+1].currentmove))) {
			return( YES );
	   }

	   if (DistantMove(IdaInfo->IdaMaze,
			   &(IdaInfo->IdaArray[depth-i].currentmove),
			   &(IdaInfo->IdaArray[(depth-i)+1].currentmove)))
		n++;
	   if ( n > Options.local_k )
		return( YES );
	   i++;
	}
	return( NO );
}

