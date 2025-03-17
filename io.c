#include "board.h"

int PosNr=0;

void PrintMaze(MAZE *maze) {
	int x,y,num_empty,pos;
	char buff[XSIZE*2+2];

	if (maze == NULL) return;
	num_empty=0;
	for (y = YSIZE-1; y>=0 && num_empty<XSIZE ; y--) {
		buff[0]='\0';
		num_empty = 0;
		for (x = 0; x<XSIZE; x++) {
			pos = XY2ID(x,y);
			if (pos == PosNr) strcat(buff,"?");
/*
			if(IsBitSetBS(maze->dead,pos)) strcat(buff,"+");
			else
			if(IsBitSetBS(maze->backdead,pos)) strcat(buff,"X");
			else
			if (maze->Phys[pos].lstruct>=0) {
			   if (maze->structs[maze->Phys[pos].lstruct].room==1)
				sprintf(&buff[strlen(buff)],"R");
			   else
			   if(maze->structs[maze->Phys[pos].lstruct].hallway==1)
				sprintf(&buff[strlen(buff)],"H");
			   else
			   if(maze->structs[maze->Phys[pos].lstruct].intersection==1)
				sprintf(&buff[strlen(buff)],"I");
			   else
			   if(maze->structs[maze->Phys[pos].lstruct].deadend==1)
				sprintf(&buff[strlen(buff)],"D");
			}
			else
			if (   maze->to_goals[pos].fw>0 
			    || maze->to_goals[pos].bw>0)
				sprintf(&buff[strlen(buff)],"X");
			else
			if (maze->Phys[pos].lstruct>=0&&maze->PHYSstone[pos]<0) 
				sprintf(&buff[strlen(buff)],"%c",
					'0'+maze->Phys[pos].lstruct);
			else
			if (IsBitSetBS( maze->reach, pos ) 
				sprintf(&buff[strlen(buff)],"1");
			else
			if (  maze->to_goals[x*YSIZE+y].fw>0
			    ||maze->to_goals[x*YSIZE+y].bw>0) 
				sprintf(&buff[strlen(buff)],"%i",
					maze->to_goals[x*YSIZE+y].n%10);
			else
			if (maze->Phys[pos].one_way) strcat(buff,"~");
			else
			if (maze->Phys[pos].one_way) 
				switch (WhereMan(maze,x*YSIZE+y)) {
				case 0: strcat(buff,"+"); break;
				case NORTH: strcat(buff,"^"); break;
				case SOUTH: strcat(buff,"v"); break;
				case EAST: strcat(buff,">"); break;
				case WEST: strcat(buff,"<"); break;
				default: My_exit(1,"Where did HE go?\n");
				}
			else
			if (maze->groom_index[x*YSIZE+y] > 0) strcat(buff,"+");
                        else
			else if (IsBitSetBS(maze->dead,pos)) strcat(buff,"X");
				sprintf(&buff[strlen(buff)],"%c",
				      'G'+maze->PHYSstone[pos]
				      +((maze->Phys[pos].goal>=0)?('a'-'A'):0));
			else if (   !IsBitSetBS(maze->out,pos)
				 && PosNr!=0
				 && (DistWeight(maze,PosNr,pos)<10000))
				if (DistWeight(maze,PosNr,pos)<10)
					sprintf(&buff[strlen(buff)],"%i",
						DistWeight(maze,PosNr,pos)%10);
				else if (DistWeight(maze,PosNr,pos)<36)
					sprintf(&buff[strlen(buff)],"%c",
					    'a'+DistWeight(maze,PosNr,pos)-10);
				else if (DistWeight(maze,PosNr,pos)<62)
					sprintf(&buff[strlen(buff)],"%c",
					'A'+DistWeight(maze,PosNr,pos)-36);
				else strcat(buff,">");
*/
			else if (IsBitSetBS(maze->wall,pos)) strcat(buff,"#");
			else if (maze->PHYSstone[pos]>=0) strcat(buff,"$");
			else if (   maze->PHYSstone[pos]>=0
			    && maze->Phys[pos].goal>=0) strcat(buff,"*");
			else if (  maze->manpos==pos
				 &&maze->Phys[pos].goal>=0) strcat(buff,"+");
			else if (maze->manpos==pos) strcat(buff,"@");
			else if (maze->Phys[pos].goal>=0) strcat(buff,".");
			else if (IsBitSetBS(maze->out,pos)) {
				strcat(buff," ");
				num_empty++;
			}
/*
*/
			else if (maze->groom_index[x*YSIZE+y]>=0) 
				sprintf(&buff[strlen(buff)],"%i",
					maze->groom_index[x*YSIZE+y]%10);
			else strcat(buff," ");
/*
			else
			if (maze->groom_index[x*YSIZE+y]>=0) 
				sprintf(&buff[strlen(buff)],"%i",
					maze->groom_index[x*YSIZE+y]%10);
			else {
			  sprintf(&buff[strlen(buff)],"%i",
				GetWeight(maze,PosNr,XY2ID(x,y))%10);
			}
			else sprintf(&buff[strlen(buff)],"%i",
				maze->Phys[pos].free%10);
			else if(IsBitSetBS( maze->reach, pos) strcat(buff,"+");
			else sprintf(&buff[strlen(buff)],"%i",
				maze->Phys[pos].s_tunnel%10);
			else if (maze->Phys[pos].partof>=0) {
			  sprintf(&buff[strlen(buff)],"%c",
					'0'+maze->Phys[pos].partof);
			}
			else 
				sprintf(&buff[strlen(buff)],"%i",
					maze->to_goals[x*YSIZE+y].n%10);
*/
		}
		Mprintf( 0, "%s\n",buff);
	}
	/* if (PosNr != 0) sprintf(buff,
				"avg_inf's: to: %5.2f, from: %5.2f\n",
				maze->avg_influence_to[PosNr],
				maze->avg_influence_from[PosNr]); */
	Mprintf( 0, "%s\n",buff);
/*
	for (x=0; x<maze->number_structs; x++) {
		Mprintf( 0, "\n%c: sq: %2i st: %2i go: %2i do: %2i s_do: %2i", 'A'+x,
			maze->structs[x].number_squares,
			maze->structs[x].number_stones,
			maze->structs[x].number_goals,
			maze->structs[x].number_doors,
			maze->structs[x].number_s_doors);
		if (maze->structs[x].hallway==1)      Mprintf( 0, " HALL");
		if (maze->structs[x].room==1) 	      Mprintf( 0, " ROOM");
		if (maze->structs[x].intersection==1) Mprintf( 0, " INTS");
		if (maze->structs[x].manonly==1)      Mprintf( 0, " MANO");
		if (maze->structs[x].deadend==1)      Mprintf( 0, " DEAD");
		if (maze->structs[x].exit==1)         Mprintf( 0, " EXIT");
	} Mprintf( 0, "\n");
	for (x=0; x<maze->number_stones; x++) {
		Mprintf( 0, "%2i: loc: %2i %2i\n", x, ID2XY(maze->stones[x].loc));
	}
	for (x=0; x<maze->number_goals; x++) {
		Mprintf( 0, "%2i: loc: %2i %2i weight: %d\n", 
						 x, 
						 ID2XY(maze->goals[x].loc),
						 maze->goals[x].weight);
	}
*/
	Mprintf( 0, "manpos: %i h: %i pen: %i search nodes: %" PRIi32 " patterns: %d total nodes: %" PRIi32 "\n",
		 maze->manpos,maze->h,maze->pen,
		 IdaInfo->node_count,maze->conflicts->number_patterns,
		 total_node_count );
}


void PrintTable(MAZE *maze) {
	int goali, stonei;
	int w,sum = 0;

	for (stonei=0; stonei<maze->number_stones; stonei++) {
		Mprintf( 0, "%2i: %2i (%3i),", stonei, maze->lbtable[stonei].goalidx,
				        maze->lbtable[stonei].distance);
		sum += maze->lbtable[stonei].distance;
		for (goali=0; goali<maze->number_goals; goali++) {
			w = GetWeight(maze,maze->goals[goali].loc,
                                                     maze->stones[stonei].loc);
			if (w<ENDPATH) Mprintf( 0, " %2i", w);
			else Mprintf( 0, " XX");
		}
		Mprintf( 0, "\n");
	}
	Mprintf( 0, "h: %i, sum: %i\n",maze->h, sum);
}

void ReadMaze(FILE *fp, MAZE *maze ) {
	int sq,x,y,pos;

	x = 0;
	y = YSIZE-1;
	ResetMaze(maze);
	while ( (sq = getc(fp)) != EOF) {
		pos = XY2ID(x,y);
		if (sq=='\n' && x==0) goto END_INPUT;
		if (y<0) {
			My_exit(1, "Maze too large for YSIZE, recompile with larger YSIZE!\n");
		}
		UnsetBitBS(maze->s_visited,pos);
		UnsetBitBS(maze->m_visited,pos);
		if (x == 0) {
			UnsetBitBS(maze->M[WEST],pos);
			UnsetBitBS(maze->S[WEST],pos);
		}
		if (x == XSIZE-1) {
			UnsetBitBS(maze->M[EAST],pos);
			UnsetBitBS(maze->S[EAST],pos);
		}
		if (y == 0) {
			UnsetBitBS(maze->M[SOUTH],pos);
			UnsetBitBS(maze->S[SOUTH],pos);
		}
		if (y == YSIZE-1) {
			UnsetBitBS(maze->M[NORTH],pos);
			UnsetBitBS(maze->S[NORTH],pos);
		}
		switch (sq) {
		case '\n': 
			if (x==0) goto END_INPUT;
			if (x>XSIZE) {
				My_exit(1, "Maze too large for XSIZE, recompile with larger XSIZE!\n");
			}
			for (;x<XSIZE;x++) SetBitBS(maze->out,pos);
			y--;
			x = 0;
			break;
		case '#':
			SetBitBS(maze->wall,pos);

			if (IsBitSetBS(maze->M[NORTH],pos)) {	
				UnsetBitBS(maze->M[SOUTH],pos+1);
				UnsetBitBS(maze->S[SOUTH],pos+1);
				UnsetBitBS(maze->S[NORTH],pos+1);
			}
			if (IsBitSetBS(maze->M[EAST],pos)) {
				UnsetBitBS(maze->M[WEST],pos+YSIZE);
				UnsetBitBS(maze->S[WEST],pos+YSIZE);
				UnsetBitBS(maze->S[EAST],pos+YSIZE);
			}
			if (x>0) {
				UnsetBitBS(maze->M[EAST],pos-YSIZE);
				UnsetBitBS(maze->S[EAST],pos-YSIZE);
				UnsetBitBS(maze->S[WEST],pos-YSIZE);
			}
			if (y>0) {
				UnsetBitBS(maze->M[NORTH],pos-1);
				UnsetBitBS(maze->S[NORTH],pos-1);
				UnsetBitBS(maze->S[SOUTH],pos-1);
			}
			UnsetBitBS(maze->M[NORTH],pos);
			UnsetBitBS(maze->M[EAST],pos);
			UnsetBitBS(maze->M[WEST],pos);
			UnsetBitBS(maze->M[SOUTH],pos);
			UnsetBitBS(maze->S[NORTH],pos);
			UnsetBitBS(maze->S[EAST],pos);
			UnsetBitBS(maze->S[WEST],pos);
			UnsetBitBS(maze->S[SOUTH],pos);
			x++;
			break;
		case '+':
		case '%':
		case '@':
			maze->manpos = XY2ID(x,y);
                        if (sq=='@') x++;
			else goto GOALSQUARE;
                        break;
		case '*': /* Stone on Goal, jump next break!!!!! */
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
		case 's': case 't': case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z':
		case '$':
			maze->PHYSstone[pos] = maze->number_stones++;
			SR(Assert(MAXSTONES>maze->number_stones,
				"Too many stones!\n"));
			maze->stones
			   =(STN*)My_realloc(maze->stones,
				sizeof(STN)*maze->number_stones);
			maze->stones[maze->number_stones-1].loc=XY2ID(x,y);
			SetBitBS(maze->stone,XY2ID(x,y));
                        if (sq=='$' || (sq>='A'&&sq<='Z')) {
                        	x++;
				break;
			}
			/* fall throuh in case it is a stone on a goal */
GOALSQUARE:
		case '.':
			maze->Phys[pos].goal = maze->number_goals++;;
			SR(Assert(MAXGOALS>maze->number_goals,
				"Too many goals!\n"));
			maze->goals
			   =(GOL*)My_realloc(maze->goals,
				sizeof(GOL)*maze->number_goals);
			maze->goals[maze->number_goals-1].loc=XY2ID(x,y);
			maze->goals[maze->number_goals-1].weight=0;
			maze->goals[maze->number_goals-1].tried=0;
			SetBitBS(maze->goal,XY2ID(x,y));
                        x++;
                        break;
		case '=':
		case ' ':
                        x++;
                        break;
		default:
			My_exit(1, "unknown maze square: \"%c\"\n",sq);
		}
	}
END_INPUT:
	maze->goal_manpos = maze->manpos;
	/* sanity check for some trivial numbers */
	if (maze->number_goals < maze->number_stones) {
		My_exit(1,
			"Too few goals (%i) for all stones (%i) in the maze\n",
			maze->number_goals, maze->number_stones);
	}
	for (;y>=0;y--) {
		for (x=0;x<XSIZE;x++) SetBitBS(maze->out,XY2ID(x,y));
	}
        maze->lbtable=
                (LBENTRY*)My_realloc(maze->lbtable,
			sizeof(LBENTRY)*maze->number_goals);
	MarkAll(maze);
	SetGlobalWeights(maze);
        SetGoalWeights(maze);
#ifdef NEEDED
	MarkNeeded(maze);
#endif
	MarkEqual(maze);
	MarkBackDead(maze);
	if (maze->number_stones==maze->number_goals) {
		BetterLowerBound(maze);
	} else {
		Debug(0,0,"ReadMaze: DeadLowerBound called, stones<goals!\n");
		DeadLowerBound(maze);
	}
	NormHashKey(maze);
	FindMacros(maze);

	if (Options.autolocal == 1) SetLocalCut(0,0,0);
	/*PrintSquare(maze, 27);
	PrintSquare(maze, 28);
	PrintSquare(maze, 44);
	PrintSquare(maze, 61);
	PrintSquare(maze, 60);*/
}

char *PrintMove(MOVE move) {

	static char buff[32];

	sprintf(buff,"%02i-%02i",move.from,move.to);
	return(buff);
}

void PrintSquare(MAZE *maze, PHYSID pos) {

	Mprintf( 0, "POS %i: ", pos);
	if(maze->PHYSstone[pos]>=0) Mprintf( 0, "stone ");
	if(maze->Phys[pos].goal>=0) Mprintf( 0, "goal ");
	if(IsBitSetBS(maze->reach, pos)) Mprintf( 0, "reach ");
	if(IsBitSetBS(maze->wall,pos)) Mprintf( 0, "wall ");
	if(maze->manpos==pos) Mprintf( 0, "man ");
	if(IsBitSetBS(maze->dead,pos)) Mprintf( 0, "dead ");
	if(IsBitSetBS(maze->M[NORTH],pos)) Mprintf( 0, "MN ");
	if(IsBitSetBS(maze->M[EAST],pos)) Mprintf( 0, "ME ");
	if(IsBitSetBS(maze->M[WEST],pos)) Mprintf( 0, "MW ");
	if(IsBitSetBS(maze->M[SOUTH],pos)) Mprintf( 0, "MS ");
	if(IsBitSetBS(maze->S[NORTH],pos)) Mprintf( 0, "SN ");
	if(IsBitSetBS(maze->S[EAST],pos)) Mprintf( 0, "SE ");
	if(IsBitSetBS(maze->S[WEST],pos)) Mprintf( 0, "SW ");
	if(IsBitSetBS(maze->S[SOUTH],pos)) Mprintf( 0, "SS ");
	if(IsBitSetBS(maze->out,pos)) Mprintf( 0, "out");
	Mprintf( 0, "\n");

}

char *HumanMove(MOVE move) {
	static char buff[50];

	if (ISDUMMYMOVE(move)) {
		/*strcpy(buff,"AA-AA DummyMove");*/
		strcpy(buff,"Aa-Aa");
	} else {
		buff[0] = 'A'+move.from/YSIZE;
		buff[1] = 'a'+(YSIZE - move.from%YSIZE -1);
		buff[2] = '-';
		buff[3] = 'A'+move.to/YSIZE;
		buff[4] = 'a'+(YSIZE - move.to%YSIZE -1);
		buff[5] = '\0';
	}
	return(buff);
}

void PrintBit2Maze(MAZE *maze,BitString marks) {
	int x,y,num_empty,pos;
	char buff[XSIZE*2+2];

	num_empty=0;
	for (y = YSIZE-1; y>=0 && num_empty<XSIZE ; y--) {
		buff[0]='\0';
		num_empty = 0;
		for (x = 0; x<XSIZE; x++) {
			pos = XY2ID(x,y);
			if (pos == PosNr) strcat(buff,"?");
			else if (IsBitSetBS(maze->wall,pos)) strcat(buff,"#");
			else if (IsBitSetBS(marks,pos)) strcat(buff,"*");
			else if (IsBitSetBS(maze->out,pos)) {
				strcat(buff," ");
				num_empty++;
			}
			else strcat(buff," ");
		}
		Mprintf( 0, "%s\n",buff);
	}
}

void PrintBit3Maze(MAZE *maze,BitString marks,BitString mark2, PHYSID manpos) {
	int x,y,num_empty,pos;
	char buff[XSIZE*2+2];

	num_empty=0;
	for (y = YSIZE-1; y>=0 && num_empty<XSIZE ; y--) {
		buff[0]='\0';
		num_empty = 0;
		for (x = 0; x<XSIZE; x++) {
			pos = XY2ID(x,y);
			if (pos == PosNr) strcat(buff,"?");
			else if (pos == manpos) strcat(buff,"@");
			else if (IsBitSetBS(maze->wall,pos)) strcat(buff,"#");
			else if (IsBitSetBS(marks,pos)) strcat(buff,"*");
			else if (IsBitSetBS(mark2,pos)) strcat(buff,"+");
			else if (IsBitSetBS(maze->out,pos)) {
				strcat(buff," ");
				num_empty++;
			}
			else strcat(buff," ");
		}
		Mprintf( 0, "%s\n",buff);
	}
}

