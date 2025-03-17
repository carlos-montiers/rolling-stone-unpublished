#include "board.h"

void InitMaze(MAZE *maze)  {
	PHYSID pos;
	int i,dir;

	maze->lbtable	     = NULL;
	memset(&(maze->Phys),0,sizeof(PHYS)*XSIZE*YSIZE);
	maze->number_grooms  = 0;
	maze->number_structs = 0;
	maze->number_goals   = 0;
	maze->number_stones  = 0;
	maze->grooms         = NULL;
	maze->structs        = NULL;
	maze->gmtrees        = NULL;
	maze->stones         = NULL;
	maze->goals          = NULL;
	maze->h		     = 0;
	maze->pen	     = 0;
	maze->g		     = 0;
	maze->hashkey	     = 0;
	maze->conflicts      = My_malloc(sizeof(CONFLICTS));
	InitConflicts(maze->conflicts);
	maze->goal_sqto      = -1;
	maze->currentmovenumber = 0;
	for (dir=NORTH; dir<=WEST; dir++) {
		Set1BS(maze->S[dir]);
		Set1BS(maze->M[dir]);
	}
	Set0BS(maze->wall);
	Set0BS(maze->dead);
	Set0BS(maze->backdead);
	Set1BS(maze->out);
	Set0BS(maze->m_visited);
	Set0BS(maze->s_visited);
	Set0BS(maze->one_way);
	Set0BS(maze->stone);
	Set0BS(maze->goal);
	Set0BS(maze->reach);
	Set0BS(maze->no_reach);
	Set0BS(maze->old_no_reach);
	Set0BS(maze->stones_done);
	Set0BS(maze->eqsq);
	
	for (pos = 0; pos < XSIZE*YSIZE; pos++) {
		maze->Phys[pos].tunnel    =  1;
		maze->Phys[pos].min_dim   =  0;
		maze->Phys[pos].free      =  0;
		maze->Phys[pos].s_tunnel  =  1;
		maze->Phys[pos].s_min_dim =  0;
		maze->Phys[pos].s_free    =  0;
		maze->Phys[pos].lstruct   = -1;
		maze->PHYSstone[pos]     = -1;
		maze->Phys[pos].goal      = -1;
		for (i=0; i<4; i++) {
			maze->s_weights[i][pos]=NULL;
			maze->bg_weights[i][pos]=NULL;
			maze->b_weights[i][pos]=NULL;
		}
		maze->m_weights[pos]=NewWeights();
		maze->d_weights[pos]=NewWeights();
		maze->groom_index[pos] = -2;
		maze->macros[pos].type = 0;
		maze->macros[pos].number_macros = 0;
		maze->macros[pos].macros = NULL;
	}
	memset(maze->avg_influence_to,0,sizeof(float)*XSIZE*YSIZE);
	memset(maze->avg_influence_from,0,sizeof(float)*XSIZE*YSIZE);
}

void ResetMaze(MAZE *maze)  {
	PHYSID pos,dir;
	int i;

	memset(&(maze->Phys),0,sizeof(PHYS)*XSIZE*YSIZE);
	Set0BS( maze->reach );
	maze->number_structs = 0;
	maze->number_goals   = 0;
	maze->number_stones  = 0;
	maze->h		     = 0;
	maze->pen	     = 0;
	maze->g		     = 0;
	maze->hashkey	     = 0;
	maze->goal_sqto      = -1;
	maze->currentmovenumber = 0;
	DelConflicts(maze->conflicts);
	for (dir=NORTH; dir<=WEST; dir++) {
		Set1BS(maze->S[dir]);
		Set1BS(maze->M[dir]);
	}
	Set0BS(maze->wall);
	Set0BS(maze->dead);
	Set0BS(maze->backdead);
	Set1BS(maze->out);
	Set0BS(maze->m_visited);
	Set0BS(maze->s_visited);
	Set0BS(maze->one_way);	
	Set0BS(maze->stone);	
	Set0BS(maze->goal);
	Set0BS(maze->reach);
	Set0BS(maze->no_reach);
	Set0BS(maze->old_no_reach);
	Set0BS(maze->stones_done);
	Set0BS(maze->eqsq);

	for (pos = 0; pos < XSIZE*YSIZE; pos++) {
		maze->Phys[pos].tunnel    =  1;
		maze->Phys[pos].min_dim   =  0;
		maze->Phys[pos].free      =  0;
		maze->Phys[pos].s_tunnel  =  1;
		maze->Phys[pos].s_min_dim =  0;
		maze->Phys[pos].s_free    =  0;
		maze->Phys[pos].lstruct   = -1;
		maze->PHYSstone[pos]     = -1;
		maze->Phys[pos].goal      = -1;
		for (i=0; i<4; i++) {
			FreeWeights(maze->s_weights[i][pos]);
			FreeWeights(maze->bg_weights[i][pos]);
			FreeWeights(maze->b_weights[i][pos]);
			maze->s_weights[i][pos] = NULL;
			maze->bg_weights[i][pos] = NULL;
			maze->b_weights[i][pos] = NULL;
		}
		ResetWeights(maze->m_weights[pos]);
		ResetWeights(maze->d_weights[pos]);
		maze->groom_index[pos] = -2;
		if (maze->macros[pos].type!=4) 
			My_free(maze->macros[pos].macros);
		maze->macros[pos].type = 0;
		maze->macros[pos].number_macros = 0;
		maze->macros[pos].macros = NULL;
	}
	memset(maze->avg_influence_to,0,sizeof(float)*XSIZE*YSIZE);
	memset(maze->avg_influence_from,0,sizeof(float)*XSIZE*YSIZE);
	for (i=0; i<maze->number_grooms; i++) {
		DelGMTree(maze->gmtrees[i]);
	}
	My_free(maze->gmtrees);
	maze->gmtrees = NULL;
	maze->number_grooms = 0;
	My_free(maze->grooms);
	maze->grooms = NULL;
}

void DelMaze(MAZE *maze) {
	PHYSID pos;
	int    i;

	My_free(maze->lbtable);
	My_free(maze->structs);
	My_free(maze->stones);
	My_free(maze->goals);
	maze->structs = NULL;
	maze->stones  = NULL;
	maze->goals   = NULL;
	maze->number_structs = 0;
	maze->number_goals   = 0;
	maze->number_stones  = 0;
	for (pos=0; pos<XSIZE*YSIZE; pos++) {
		if (maze->macros[pos].type!=4) 
			My_free(maze->macros[pos].macros);
		maze->macros[pos].macros = NULL;
		FreeWeights(maze->m_weights[pos]);
		FreeWeights(maze->d_weights[pos]);
		maze->m_weights[pos] = NULL;
		maze->d_weights[pos] = NULL;
		for (i=0; i<4; i++) {
			FreeWeights(maze->s_weights[i][pos]);
			FreeWeights(maze->bg_weights[i][pos]);
			FreeWeights(maze->b_weights[i][pos]);
			maze->s_weights[i][pos] = NULL;
			maze->bg_weights[i][pos] = NULL;
			maze->b_weights[i][pos] = NULL;
		}
	}
	for (i=0; i<maze->number_grooms; i++) {
		DelGMTree(maze->gmtrees[i]);
	}
	DelConflicts(maze->conflicts);
	My_free(maze->conflicts);
	My_free(maze->gmtrees);
	maze->gmtrees = NULL;
	maze->number_grooms = 0;
	My_free(maze->grooms);
	maze->grooms = NULL;
}

MAZE *CopyMaze(MAZE *maze) {

	MAZE *ret_maze;

	ret_maze = 
		(MAZE *)My_malloc(sizeof(MAZE));
	memcpy(ret_maze,maze,sizeof(MAZE));

	/* now copy all pointers to structures */
	/* Weightarrays are static and will never change, don't copy */
	/* but stones and goals have to be copied */
	ret_maze->lbtable=
		(LBENTRY*) My_malloc(sizeof(LBENTRY)*maze->number_goals);
	memcpy(ret_maze->lbtable,maze->lbtable,
		sizeof(LBENTRY)*maze->number_goals);
	ret_maze->gmtrees=
		(GMNODE**) My_malloc(sizeof(GMNODE*)*maze->number_grooms);
	memcpy(ret_maze->gmtrees,maze->gmtrees,
		sizeof(GMNODE*)*maze->number_grooms);
	ret_maze->structs=(STRUCT*)
		My_malloc(sizeof(STRUCT)*maze->number_structs);
	memcpy(ret_maze->structs,maze->structs,
	       sizeof(STRUCT)*maze->number_structs);
	ret_maze->stones=
		(STN*)My_malloc(sizeof(STN)*maze->number_stones);
	memcpy(ret_maze->stones,maze->stones,sizeof(STN)*maze->number_stones);
	ret_maze->goals=
		(GOL*)My_malloc(sizeof(GOL)*maze->number_goals);
	memcpy(ret_maze->goals,maze->goals,sizeof(GOL)*maze->number_goals);

	return(ret_maze);
}

void DelCopiedMaze(MAZE *maze) {

	if (maze == NULL) return;
	My_free(maze->lbtable);
	My_free(maze->gmtrees);
	My_free(maze->structs);
	My_free(maze->stones);
	My_free(maze->goals);
	My_free(maze);
}

