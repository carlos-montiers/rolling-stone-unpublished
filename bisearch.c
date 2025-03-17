#include "board.h"

int BidirectStartIda()
{
/* This is a fun routine... It will call both, forward and backward searches
   to create a bidirectional search. It does all the overhead of the
   StartIdas, since we need control over all the initialization, such as
   HashTables, IdaMazes... */


	int       result=ENDPATH;
	long      forw_node_count, back_node_count;
	PHYSID    pos;
	MAZE     *forw_maze;
	MAZE     *back_maze;
	OPTIONS   old_opts;

	/* initialize data structures */
	old_opts = Options;
	Options.dl_db = 0;

	/* remove all traces of macros */
/*
	Options.mc_gm = 0;
	Options.mc_tu = 0;
	for (i=0; i<IdaInfo->IdaMaze->number_grooms; i++) {
		DelGMTree(IdaInfo->IdaMaze->gmtrees[i]);
	}
	My_free(IdaInfo->IdaMaze->gmtrees);
	IdaInfo->IdaMaze->gmtrees = NULL;
	IdaInfo->IdaMaze->number_grooms = 0;
	My_free(IdaInfo->IdaMaze->grooms);
	IdaInfo->IdaMaze->grooms = NULL;
	for (pos = 0; pos < XSIZE*YSIZE; pos++) {
		IdaInfo->IdaMaze->groom_index[pos] = -2;
		if (IdaInfo->IdaMaze->macros[pos].type!=4) 
			My_free(IdaInfo->IdaMaze->macros[pos].macros);
		IdaInfo->IdaMaze->macros[pos].macros = NULL;
		IdaInfo->IdaMaze->macros[pos].type = 0;
		IdaInfo->IdaMaze->macros[pos].number_macros = 0;
	}
		
	Options.lb_cf = 0;
	Options.cut_goal = 0;
	Options.st_testd = 0;
	Options.multiins = 0;
	Options.pen_srch = 0;
	Options.dl_srch  = 0;
	Options.dl2_mg   = 0;
	Options.dl_mg    = 0;
	Options.tt       = 1;
*/

	forw_maze = IdaInfo->IdaMaze;
	IdaInfo->IdaMaze = CopyMaze(IdaInfo->IdaMaze);
	BackGoalsStones(IdaInfo->IdaMaze);
	back_maze = IdaInfo->IdaMaze;

	InitHashTables();
	total_node_count = 0;
	IdaInfo->node_count = 0;
	dl_pos_nc = dl_pos_sc = dl_neg_nc = dl_neg_sc = 0;
	pen_pos_nc = pen_pos_sc = pen_neg_nc = pen_neg_sc = 0;
	IdaInfo->CurrentHashGen = NextHashGen++;
	AvoidThisSquare = 0;
	Set0BS(IdaInfo->IdaManSquares);
	Set0BS(IdaInfo->IdaStoneSquares);
	IdaInfo->CurrentSolutionDepth = ENDPATH;

	IdaInfo->Threshold = IdaInfo->IdaMaze->h;
	IdaInfo->ForwDepthLimit = IdaInfo->Threshold/2;
	IdaInfo->BackDepthLimit = IdaInfo->Threshold-IdaInfo->ForwDepthLimit;
	forw_node_count = 0;
	back_node_count = 0;
	for (;
	       (IdaInfo->CurrentSolutionDepth > IdaInfo->Threshold)
	     &&(!AbortSearch());
	     IdaInfo->Threshold += IdaInfo->ThresholdInc) {

		if (forw_node_count<=back_node_count || forw_node_count==0) {
			/* forward search */
			GTVAny(GTVOpen(IdaInfo->Threshold, 
				GTVFen(IdaInfo->IdaMaze)));
			init_stats();
			IdaInfo->r_tree_size=0;
			IdaInfo->v_tree_size=0;
			IdaInfo->IdaMaze->goal_sqto = -1;
			SR(Debug(2,0,"Forw Thr: %i (%i) Frw: %d Bck: %d [%i]\n",
				IdaInfo->Threshold, IdaInfo->IdaMaze->h,
				IdaInfo->ForwDepthLimit,IdaInfo->BackDepthLimit,
				IdaInfo->node_count));
			IdaInfo->IdaMaze = forw_maze;
			result = Ida(0,0,0); /**********************************/
			forw_node_count += IdaInfo->r_tree_size;
			GTVAny(GTVClose());
			print_stats(2);
		}
		if (back_node_count<=forw_node_count || back_node_count==0) {
			/* backward search */
			GTVAny(GTVOpen(IdaInfo->Threshold, 
				GTVFen(IdaInfo->IdaMaze)));
			init_stats();
			IdaInfo->r_tree_size=0;
			IdaInfo->v_tree_size=0;
			IdaInfo->IdaMaze->goal_sqto = -1;
			SR(Debug(2,0,"Back Thr: %i (%i) Frw: %d Bck: %d [%i]\n",
				IdaInfo->Threshold, IdaInfo->IdaMaze->h,
				IdaInfo->ForwDepthLimit,IdaInfo->BackDepthLimit,
				IdaInfo->node_count));
			/* mark where man needs to end up and mark all squares 
		 	* as reachable to allow for every possible "last" 
		 	* move (man location) */
			for (pos=0; pos<YSIZE*XSIZE; pos++) {
				if (  IsBitSetBS(IdaInfo->IdaMaze->out,pos)
		    	    	||IsBitSetBS(IdaInfo->IdaMaze->stone,pos)
		    	    	||IsBitSetBS(IdaInfo->IdaMaze->wall,pos)) {
					continue;
				}
				SetBitBS(IdaInfo->IdaMaze->reach,pos);
/* 				IdaInfo->IdaMaze->PHYSreach[ pos ]=0; */
			}
			IdaInfo->IdaMaze = back_maze;
			result = BackIda(0,0); /******************************/
			back_node_count += IdaInfo->r_tree_size;
			GTVAny(GTVClose());
			print_stats(2);
		}
		if (result>=ENDPATH) 
			IdaInfo->Threshold = ENDPATH + IdaInfo->ThresholdInc;
		if (forw_node_count<=back_node_count) {
			IdaInfo->ForwDepthLimit += IdaInfo->ThresholdInc;
		} else {
			IdaInfo->BackDepthLimit += IdaInfo->ThresholdInc;
		}
	}
	IdaInfo->Threshold -= IdaInfo->ThresholdInc;
	if (result<ENDPATH) result = IdaInfo->Threshold - IdaInfo->IdaMaze->h;
	
	IdaInfo->IdaMaze = forw_maze;
	DelCopiedMaze(back_maze);

	BackPrintSolution();
	PrintSolution();

	Options = old_opts;
	return(result);
}
    

/*
 - set depth limits in IDaInfo
 - stop searching if depth limit is reached
 - made TT/Goal/Depth check same in back and ida!! ->log!!!!!!!!!
 - change TT code not to override frontier 
*/
