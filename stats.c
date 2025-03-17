#include "board.h"

OPTIONS Options;

int32_t total_node_count = 0;
int32_t penscount, penmcount, deadscount, deadmcount;
int32_t pattern_counter[256];

char *CreateStringDepth(short numberinfo)
/* Create a string that represents the nodes at certain dpeths in the tree */
{
	static char msg[4000];
	static char depth[16], nn[32];
	char   *cp;
	int    d;

	d = 0;
	msg[0] = '\0';
	cp = msg;
	while ( IdaInfo->nodes_depth[d] > 0 ) {
		if (d%10 == 0) sprintf(depth,"%d:",d);
		else depth[0] = '\0';
		if (numberinfo) sprintf(nn,"-%d/%d",
			IdaInfo->IdaArray[d].currentindex+1,
			IdaInfo->IdaArray[d].number_moves);
		else nn[0] = '\0';
		sprintf(cp,"%s%" PRId32 "%s ",depth,IdaInfo->nodes_depth[d],nn);
		cp = msg + strlen(msg);
		d++;
	}
	return( msg );
}

void IncNodeCount(int dth) {
	
	FILE *fp;

	total_node_count++;
	IdaInfo->node_count++;
	IdaInfo->r_tree_size++;
	IdaInfo->v_tree_size++;
	IdaInfo->nodes_depth[dth]++;
	if (total_node_count%100000==0) {
		fp = fopen("nodecount","w");
		if (fp != NULL) {
			fprintf(fp,"%" PRIi32,total_node_count);
			fclose(fp);
		}
	}
	NavigatorIncNodeCount(dth);
}

void init_opts() {
	Options.tt=1;
	Options.dl_mg=1;
	Options.dl2_mg=1;
	Options.dl_srch=1;
	Options.pen_srch=1;
	Options.multiins=1;
	Options.st_testd=1;
	Options.dl_db=7;
	Options.lb_mp=1;
	Options.lb_cf=1;
	Options.mc_tu=1;
	Options.mc_gm=1;
	Options.cut_goal=1;
	Options.xdist=1;
	Options.local=1;
	Options.autolocal=1;
	Options.local_k=-1;
	Options.local_m=-1;
	Options.local_d=max(YSIZE,XSIZE);
}

void init_stats() {

	int i;

	for (i=0; i<MAX_DEPTH; i++) {
		IdaInfo->nodes_depth[i]=0;
		IdaInfo->no_lcut_nodes[i]=0;
		IdaInfo->no_lcut_moves[i]=0;
		IdaInfo->no_lcut_h[i]=0;
		IdaInfo->no_lcut_g[i]=0;
		IdaInfo->lcut_nodes[i]=0;
		IdaInfo->lcut_moves[i]=0;
		IdaInfo->lcut_allmoves[i]=0;
		IdaInfo->lcut_h[i]=0;
		IdaInfo->lcut_g[i]=0;
		IdaInfo->both_nodes[i]=0;
		IdaInfo->both_moves[i]=0;
		IdaInfo->both_h[i]=0;
		IdaInfo->both_g[i]=0;
	}
	IdaInfo->r_tree_size= 0;
	IdaInfo->v_tree_size= 0;

	IdaInfo->tt_hits    = 0;
	IdaInfo->tt_cols    = 0;
	IdaInfo->tt_reqs    = 0;
	IdaInfo->start_time = time(NULL);
}

void print_stats(int pri) {
	int i,ttl;
	time_t t;
	
	Debug(pri,0, "tt: %c, dl_mg: %c, dl2_mg: %c, dl_srch: %c, pen_srch: %c, multiins: %c\n",
		Options.tt==1?'Y':'N', Options.dl_mg==1?'Y':'N',
		Options.dl2_mg==1?'Y':'N', Options.dl_srch==1?'Y':'N', 
		Options.pen_srch==1?'Y':'N', Options.multiins==1?'Y':'N');
	Debug(pri,0,"st_testd: %c, dl_db: %i, cut_goal: %c\n",
		Options.st_testd==1?'Y':'N', Options.dl_db, 
		Options.cut_goal==1?'Y':'N');
	Debug(pri,0,"xdist: %c, auto: %c, local: %c(%i,%i,%i)\n",
		Options.xdist==1?'Y':'N',
		Options.autolocal==1?'Y':'N', Options.local==1?'Y':'N',
		Options.local_k,Options.local_m,Options.local_d);
	Debug(pri,0, "lb_mp: %c, lb_cf: %c, mc_tu: %c, mc_gm: %c\n",
		Options.lb_mp==1?'Y':'N', Options.lb_cf==1?'Y':'N', 
		Options.mc_tu==1?'Y':'N', Options.mc_gm==1?'Y':'N');
	Debug(pri,0,"nodes searched: total: %" PRIi32 ", top level: %" PRIi32 "\n",
		total_node_count, IdaInfo->node_count);
	Debug(pri,0,"TT hits: %" PRIi32 ", TT collisions: %" PRIi32 " Req: %" PRIi32 ", (%f)\n",
		IdaInfo->tt_hits,
		IdaInfo->tt_cols,
		IdaInfo->tt_reqs,
		((float)100*IdaInfo->tt_hits)/IdaInfo->tt_reqs);
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, "nodes:");
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->nodes_depth[i]);
		ttl += IdaInfo->nodes_depth[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nno_lcut_nodes:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->no_lcut_nodes[i]);
		ttl += IdaInfo->no_lcut_nodes[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nno_lcut_moves:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->no_lcut_moves[i]);
		ttl += IdaInfo->no_lcut_moves[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nno_lcut_h:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->no_lcut_h[i]);
		ttl += IdaInfo->no_lcut_h[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nno_lcut_g:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->no_lcut_g[i]);
		ttl += IdaInfo->no_lcut_g[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nlcut_nodes:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->lcut_nodes[i]);
		ttl += IdaInfo->lcut_nodes[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nlcut_moves:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->lcut_moves[i]);
		ttl += IdaInfo->lcut_moves[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nlcut_h:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->lcut_h[i]);
		ttl += IdaInfo->lcut_h[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nlcut_g:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->lcut_g[i]);
		ttl += IdaInfo->lcut_g[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nlcut_allmoves:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->lcut_allmoves[i]);
		ttl += IdaInfo->lcut_allmoves[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nboth_nodes:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->both_nodes[i]);
		ttl += IdaInfo->both_nodes[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nboth_moves:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->both_moves[i]);
		ttl += IdaInfo->both_moves[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nboth_h:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->both_h[i]);
		ttl += IdaInfo->both_h[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\nboth_g:",ttl);
	i = ttl = 0;
	while (IdaInfo->nodes_depth[i] && IdaInfo->PrintPriority >= pri)  {
		Mprintf( 0, " %" PRIi32, IdaInfo->both_g[i]);
		ttl += IdaInfo->both_g[i];
		i++;
	}
	if (IdaInfo->PrintPriority >= pri)
		Mprintf( 0, ":%" PRIi32 "\n",ttl);
	t = time(NULL) - IdaInfo->start_time;
	if (t==0) t=1;
	Debug(pri,0,"Nodes per Second: %8.0f\n",(float)total_node_count/t);
	Debug(pri,0,"DeadMove search stats:\n");
	Debug(pri,0,"DL POS: #: %5i (%2i%%) #n: %8" PRIi32 "  nodes/search: %5i\n",
		  dl_pos_sc,
		  (int)(100*dl_pos_sc)/
		       (dl_pos_sc+dl_neg_sc+(dl_pos_sc+dl_neg_sc==0?1:0)),
		  dl_pos_nc,(dl_pos_sc==0)?0:(int)dl_pos_nc/dl_pos_sc);
	Debug(pri,0,"DL NEG: #: %5i (%2i%%) #n: %8" PRIi32 "  nodes/search: %5i\n",
		  dl_neg_sc,
		  (int)(100*dl_neg_sc)/
		       (dl_pos_sc+dl_neg_sc+(dl_pos_sc+dl_neg_sc==0?1:0)),
		  dl_neg_nc,(dl_neg_sc==0)?0:(int)dl_neg_nc/dl_neg_sc);
	Debug(pri,0,"PEN POS: #: %5i (%2i%%) #n: %8" PRIi32 "  nodes/search: %5i\n",
		  pen_pos_sc,
		  (int)(100*pen_pos_sc)/
		       (pen_pos_sc+pen_neg_sc+(pen_pos_sc+pen_neg_sc==0?1:0)),
		  pen_pos_nc,(pen_pos_sc==0)?0:(int)pen_pos_nc/pen_pos_sc);
	Debug(pri,0,"PEN NEG: #: %5i (%2i%%) #n: %8" PRIi32 "  nodes/search: %5i\n",
		  pen_neg_sc,
		  (int)(100*pen_neg_sc)/
		       (pen_pos_sc+pen_neg_sc+(pen_pos_sc+pen_neg_sc==0?1:0)),
		  pen_neg_nc,(pen_neg_sc==0)?0:(int)pen_neg_nc/pen_neg_sc);
	Debug(pri,0,"\n");
}

unsigned char GetPatternIndex(MAZE *maze, int pos)
{
	unsigned char index=0;
	
	if (IsBitSetBS(maze->S[NORTH],pos)) index |= 1;
	if (IsBitSetBS(maze->S[EAST],pos)) index |= 2;
	if (IsBitSetBS(maze->S[SOUTH],pos)) index |= 4;
	if (IsBitSetBS(maze->S[WEST],pos)) index |= 8;
	if (IsBitSetBS(maze->M[NORTH],pos)) index |= 16;
	if (IsBitSetBS(maze->M[EAST],pos)) index |= 32;
	if (IsBitSetBS(maze->M[SOUTH],pos)) index |= 64;
	if (IsBitSetBS(maze->M[WEST],pos)) index |= 128;
	return(index);
}

void RecordPatterns(MAZE *maze)
{
	PHYSID i;
	
	for (i=0; i<XSIZE*YSIZE; i++) {
		if (IsBitSetBS(maze->out,i)) continue;
		if (IsBitSetBS(maze->wall,i)) continue;
		pattern_counter[GetPatternIndex(maze,i)]++;
	}
}

void CountPatterns()
{
	int i=0;
	int index;

	for (index=0; index<256; index++)
		if (pattern_counter[index]>0) {
			i++;
		}
	Mprintf( 0, " Number Patterns: %i\n",i);
}

void InitPatterns()
{
	int index;

	for (index=0; index<256; index++) pattern_counter[index]=0;
}
