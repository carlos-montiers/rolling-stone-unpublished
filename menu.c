/*=============================================================================
|
| This file includes User Interface Functions for the DEADLOCK program.
|  
=============================================================================*/
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "board.h"

/*   Menu structures.
*/
CMDMENU cmdMainMenu[] = { 
   { "S", CmdSolve,		"S nam|num kind      Solve Maze" },
   { "B", CmdBack,		"B nam|num kind      Back Solve Maze"},
   { "D", CmdBidir, 		"D nam|num kind      Bidirect Solve Maze"},
   { "R", CmdReal,     		"R nam|num kind      RealTime solve Maze" },
{ "F", CmdRandom,     		"C nam|num kind      Random solve Maze" },
   { "A", CmdAbort,		"A                   Set Abort Node Count" },
   { "T", CmdTestAll,		"T a b kind          Test a to b in screen" },
   { "P", CmdPrint,		"P nam|num kind      Print Maze" },
   { "L", CmdBounds,		"L a b kind          All 90 Lower Bounds" },
   { "N", CmdPosNr,		"N num               Set PosNr with num" },
   { "M", CmdMove,		"M num-num|XX-YY     move from num to num" },
   { "X", CmdTestX,		"X [lrud]*           move center for xdist" },
   { "Z", CmdShow,		"Z                   Show Menu" },
   { "O", CmdOptions,		"O                   Options Menu" },
#ifdef NAVIGATOR
   { "V", CmdNavigator,		"V                   Navigator Menu" },
#endif
   { "Q", CmdQuit,		"Q                   Quit Program" },
   { NULL } };

 CMDMENU cmdShowMenu[] = { 
   { "D", CmdShowMDist,		"D                   Hist of man-distances"},
   { "S", CmdShowSDist,		"S                   Hist of stone-distances"},
   { "X", CmdShowXDist,		"X                   Hist of X-distances"},
   { "C", CmdShowConfl,		"C                   Print Conflicts prev search"},
   { NULL } };

 CMDMENU cmdOptionsMenu[] = { 
   { "E", CmdOptionEX,   "E		Examine all settings" },
   { "H", CmdOptionTT,   "H [on/off] 	HashTable on/off" },
   { "D", CmdOptionDL,   "D [on/off]	deadlock det. movegen on/off" },
   { "Z", CmdOptionDZ,   "Z [on/off]	deadlock2 det. movegen on/off" },
   { "S", CmdOptionDS,   "S [on/off]	deadsearch on/off" },
   { "N", CmdOptionPS,   "N [on/off]	pensearch on/off" },
   { "I", CmdOptionMI,   "I [on/off]	multi insert on/off" },
   { "X", CmdOptionST,   "X [on/off]	store tested on/off" },
   { "P", CmdOptionPT,   "P number 	Switch Pattern DBs on/off (0-7)" },
   { "M", CmdOptionMP,   "M [on/off] 	LB manpos on/off" },
   { "C", CmdOptionCF,   "C [on/off] 	LB conflict on/off" },
   { "T", CmdOptionTM,   "T [on/off] 	Tunnel Macro on/off" },
   { "G", CmdOptionGM,   "G [on/off] 	Goal Macro on/off" },
   { "U", CmdOptionCG,   "U [on/off] 	Cut Goal Macro on/off" },
   { "A", CmdOptionXD,   "A [on/off] 	eXtended Distance on/off" },
   { "L", CmdOptionLC,   "L k m d    	Local Cut (k,m,d), -1 -1 turns off" },
   { "B", CmdOptionLA,   "B [on/off]	Auto Set Local Cut Parameter" },
   { "O", CmdOptionMO,   "O number 	Set Move order index (0-off)" },
   { NULL } };

int  Cur_Maze_Number;
IDA  MainIdaInfo;
MAZE Maze;

void SimpleMakeName(char *name, int *curr_number, int number, char *str_kind)
{
	*curr_number = number;
	if (str_kind == NULL)
		sprintf(name,"screens/screen.%d",number);
	else
		sprintf(name,"screens/%s.%d",str_kind,number);
}

void MakeName(char *name, int *curr_number, char *str_number, char *str_kind)
{
	int no;

	no = atoi(str_number);
	if (no == 0) {
		*curr_number = -1;
		strcpy(name,str_number);
	} else {
		*curr_number = no;
		if (str_kind == NULL)
			sprintf(name,"screens/screen.%d",no);
		else
			sprintf(name,"screens/%s.%d",str_kind,no);
	}
}

void
/*=============================================================================
|
| Descr.:    Character based interface for the main menu.
|
| Return:    None.
|
| Side eff.: SearchDepth can be updated. Also when playing a game the 
|            chess related global structures are modified.
|
=============================================================================*/
MainMenu()
{
   COMMAND cmd;
   char    cmdstr[SZ_CMDSTR+1];
   char    name[100];
   char    *param;
   FILE    *fp;
   int     no,no2,i,g,dir;
   MOVE    move;
   UNMOVE  unmove;
	
   InitMaze(&Maze);
   param = NULL;
   g = 0;
   for ( ;; ) {

      cmd=GetCommand("Command",cmdMainMenu,cmdstr);

      switch ( cmd ) {

	case CmdBack:
	case CmdBidir:
	case CmdSolve:
	case CmdReal:
case CmdRandom:
			MakeName(name,&Cur_Maze_Number, CmdParam(cmdstr,1),
							CmdParam(cmdstr,2));
			if ((fp = fopen(name,"r")) != NULL) {
   				Mprintf( 0, "Maze %s:\n", name );
				MainIdaInfo.IdaMaze = &Maze;
				ReadMaze(fp,&Maze);
				fclose(fp);
				NavigatorSetStartMaze(&Maze,name);
				IdaInfo = &MainIdaInfo;
				switch (cmd) {
				case CmdSolve: 
						StartIda(YES);
						break;
				case CmdBack:  BackStartIda();
						break;
				case CmdBidir: BidirectStartIda();
						break;
				case CmdReal: RealSearch(
					 	 IdaInfo->AbortNodeCount!=-1
						?IdaInfo->AbortNodeCount
						:10000);
						break;
				case CmdRandom: StartRandom();
				  break;
				default:	break;
				}
			} else {
                                My_exit(1,"Menu: %s %s\n",name,strerror(errno));
			}
			g = 0;
			break;
	case CmdAbort:
			param = CmdParam(cmdstr,1);
			if (param != NULL) 
				MainIdaInfo.AbortNodeCount = atol(param);
			else MainIdaInfo.AbortNodeCount = -1;
			break;
	case CmdTestAll:
			if ((param = CmdParam(cmdstr,1)) != NULL) {
				no = atoi(param);
				if (no<1) no = 1;
			} else no = 1;
			if ((param = CmdParam(cmdstr,2)) != NULL) {
				no2 = atoi(param);
			} else no2 = 90;
			for (i=no; i<=no2; i++) {
				SimpleMakeName(name,&Cur_Maze_Number, i, 
						CmdParam(cmdstr,3));
				if ((fp = fopen(name,"r")) != NULL) {
					IdaInfo = &MainIdaInfo;
					MainIdaInfo.IdaMaze = &Maze;
					ReadMaze(fp,&Maze);
					Mprintf(0,"Maze: %i\n", i);
					BackStartIda();
					fclose(fp);
				} else {
                                	My_exit(1,"Menu: %s %s\n",
						name,strerror(errno));
				}
			}
			break;
	case CmdPrint:
			MakeName(name,&Cur_Maze_Number, CmdParam(cmdstr,1),
							CmdParam(cmdstr,2));
			if ((fp = fopen(name,"r")) != NULL) {
				MAZE *BackMaze;
				int   plainlb;

				MainIdaInfo.IdaMaze = &Maze;
				ReadMaze(fp,&Maze);
				fclose(fp);
				plainlb = PlainLowerBound(&Maze);
				BetterLowerBound(&Maze);
				BackMaze = CopyMaze(&Maze);
				BackGoalsStones(BackMaze);
				Mprintf(0,"%15s: ",name);
				Mprintf(0,"forw lb: %d (pen: %d), plain lb: %d, ",
					Maze.h,Maze.pen,plainlb);
				Mprintf(0,"back lb: %d\n", BackMaze->h);
				DelCopiedMaze(BackMaze);
				BetterLowerBound(&Maze);
				NavigatorSetStartMaze(&Maze,name);
			} else {
                        	My_exit(1,"Menu: %s %s\n",name,strerror(errno));
			}
			g = 0;
			break;
	case CmdBounds:
			if ((param = CmdParam(cmdstr,1)) != NULL) {
				no = atoi(param);
				if (no<1) no = 1;
			} else no = 1;
			if ((param = CmdParam(cmdstr,2)) != NULL) {
				no2 = atoi(param);
			} else no2 = 1;
		        for (i=90; i>0; i--) {
				SimpleMakeName(name,&Cur_Maze_Number, i, 
						CmdParam(cmdstr,3));
                		if ( (fp = fopen(name,"r")) == NULL) {
					MainIdaInfo.IdaMaze = &Maze;
                			ReadMaze(fp,&Maze);
                			Mprintf(0,"%i: %i\n",i,Maze.h);
                			fflush(stdout);
                			fclose(fp);
				} else {
                        		My_exit(1,"Menu: %s %s\n",
						name,strerror(errno));
                		}
        		}
			break;
	case CmdOptions: OptionsMenu(); break;
#ifdef NAVIGATOR
	case CmdNavigator: NavigatorMenu(); break;
#endif
	case CmdShow:    ShowMenu(); break;
	case CmdPosNr: 
			param = CmdParam(cmdstr,1);
			if (param != NULL) PosNr = atoi(param);
			else PosNr = 0;
			g = 0;
			break;
	case CmdMove: 
			move.move_dist = 1;
			move.value = 0;
			move.from = atoi(CmdParam(cmdstr,1));
			if (move.from != 0) {
				move.to   = atoi(CmdParam(cmdstr,2));
				move.last_over = move.from;
				MakeMove(&Maze,&move,&unmove);
				PrintMaze(&Maze);
			} else {
			   ParseMakeMoves(cmdstr);
			}
			break;
	case CmdTestX: 
			PosNr = Maze.manpos;
			param = CmdParam(cmdstr,1);
			while (   param != NULL
			       && param[0] != '\0'
			       && param[0] != ' ') {
				switch (param[0]) {
			  	case 'i':
			  	case 'u': 
					dir = NORTH;
					break;
			  	case 'j':
			  	case 'l': 
					dir = WEST;
					break;
			  	case 'k':
			  	case 'r': 
					dir = EAST;
					break;
			  	case 'd':
			  	case 'm': 
					dir = SOUTH;
					break;
			  	default:	dir = NODIR;
				}
				if (dir != NODIR && IsBitSetBS(Maze.M[dir],PosNr)) {
					PosNr += DirToDiff[dir];
					MANTO((&Maze),PosNr);
					PrintMaze(&Maze);
				}
				param++;
			}
			PosNr = 0;
			/*
			MakeName(name,&Cur_Maze_Number, CmdParam(cmdstr,1),
							CmdParam(cmdstr,2));
			if ((fp=fopen(CmdParam(cmdstr,1),"r"))!=NULL) {
				HASHENTRY entry;
				entry.pensearched = 0;

				move.move_dist = 1;
				move.value = 0; move.macro_id = 0;
				move.from = atoi(CmdParam(cmdstr,2));
				move.to   = atoi(CmdParam(cmdstr,3));
				move.last_over = move.from;
				IdaInfo = &MainIdaInfo;
				MainIdaInfo.IdaMaze = &Maze;
				ReadMaze(fp,&Maze);
				DConflictStartIda(0);
				int pensearched = 0;
				PenMove(&Maze,&entry,&move,1,0,2,&pensearched);
				BackStartIda();
			} else {
                               My_exit(1,"Menu: %s %s\n",param,strerror(errno));
			}
			*/
			break;
	case CmdQuit:
   			DelMaze(&Maze);
			return;
	default: break;
      }
   }
}

void ShowMenu()
{
   COMMAND cmd;
   char    cmdstr[SZ_CMDSTR+1];

   while ( (cmd=GetCommand("Show",cmdShowMenu,cmdstr)) != CmdQuit ) {

      	switch ( cmd ) {

   	case CmdShowConfl:
			PrintConflicts(&Maze,Maze.conflicts);
			break;
   	case CmdShowXDist:
			XDistHist(&Maze,NULL,NULL);
			break;
   	case CmdShowSDist:
			SDistHist(&Maze);
			break;
   	case CmdShowMDist:
			DistHist(&Maze);
			break;
	default: break;
	}
    }
}

void OptionsMenu()
{
   COMMAND cmd;
   char    cmdstr[SZ_CMDSTR+1];
   char *param,*param2,*param3;
   int  i;

   while ( (cmd=GetCommand("Option",cmdOptionsMenu,cmdstr)) != CmdQuit ) {

      	switch ( cmd ) {

        case CmdOptionEX:
		print_stats(2);
		break;
        case CmdOptionTT:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.tt = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.tt = NO;
		}
		else Options.tt = !Options.tt;
		break;
        case CmdOptionDL:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.dl_mg = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.dl_mg = NO;
		}
		else Options.dl_mg = !Options.dl_mg;
		break;
	case CmdOptionDZ:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.dl2_mg = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.dl2_mg = NO;
		}
		else Options.dl2_mg = !Options.dl2_mg;
		break;
	case CmdOptionPS:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.pen_srch = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.pen_srch = NO;
		}
		else Options.pen_srch = !Options.pen_srch;
		break;	
	case CmdOptionMI:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.multiins = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.multiins = NO;
		}
		else Options.multiins = !Options.multiins;
		break;
	case CmdOptionST:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.st_testd = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.st_testd = NO;
		}
		else Options.st_testd = !Options.st_testd;
		break;
	case CmdOptionDS:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.dl_srch = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.dl_srch = NO;
		}
		else Options.dl_srch = !Options.dl_srch;
		break;
        case CmdOptionPT:
		param = CmdParam(cmdstr,1);
		if ( param != NULL ) {
			i = atoi(param);	
			Options.dl_db = min(7,max(0,i));
		}
		break;
        case CmdOptionMP:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.lb_mp = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.lb_mp = NO;
		}
		else Options.lb_mp = !Options.lb_mp;
		break;
        case CmdOptionCF:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.lb_cf = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.lb_cf = NO;
		}
		else Options.lb_cf = !Options.lb_cf;
		break;
        case CmdOptionTM:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.mc_tu = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.mc_tu = NO;
		}
		else Options.mc_tu = !Options.mc_tu;
		break;
        case CmdOptionGM:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.mc_gm = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.mc_gm = NO;
		}
		else Options.mc_gm = !Options.mc_gm;
		break;
        case CmdOptionCG:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.cut_goal = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.cut_goal = NO;
		}
		else Options.cut_goal = !Options.cut_goal;
		break;
       case CmdOptionXD:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.xdist = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.xdist = NO;
		}
		else Options.xdist = !Options.xdist;
		break;
	case CmdOptionLC:
		param  = CmdParam(cmdstr,1);
		param2 = CmdParam(cmdstr,2);
		param3 = CmdParam(cmdstr,3);
		SetLocalCut(atoi(param),atoi(param2),atoi(param3));
		break;        
       case CmdOptionLA:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			if (strstr(param,"on") || strstr(param,"ON"))
				Options.autolocal = YES;
			if (strstr(param,"off") || strstr(param,"OFF"))
				Options.autolocal = NO;
		}
		else Options.autolocal = !Options.autolocal;
		break;
	case CmdOptionMO:
		param = CmdParam(cmdstr,1);
		if (param != NULL) {
			i = atoi(param);	
			switch (i) {
			case 0: IdaInfo->MoveOrdering = NoMoveOrdering;
				break;
			case 1: IdaInfo->MoveOrdering = BestMoveOrdering;
				break;
			case 2: IdaInfo->MoveOrdering = InertiaMoveOrdering;
				break;
			case 3: IdaInfo->MoveOrdering = ManDistMoveOrdering;
				break;
			case 4: IdaInfo->MoveOrdering = NewMoveOrdering;
				break;
			default: Mprintf(0,
					"WARNING: NoMoveOrdering set!\n");
				IdaInfo->MoveOrdering = NoMoveOrdering;
				break;
			}
		}
		break;
	default: break;
	}
   }
}

COMMAND
/*=============================================================================
|
| Descr.:    Get a command from the user.  By default the input is standard
|            I/O.  Every command entered is logged in a file so they can
|            be replayed.
|
| In:   prompt   The command prompt string.
|       cmdMenu  The menu structure for the menu to display.
|
| Out:  cmdstr   The exact command string as entered by the user.
|                The command CmdParam(cmdstr,nr) can be used to determine
|                the parameters of the cmdstr.
|
| Return:    The command type entered.  The command type is determined by
|            the first word in the command string.
| 
| Side eff.: None.
|
=============================================================================*/
GetCommand(char prompt[], CMDMENU cmdMenu[], char cmdstr[] )
{
   int  i;

   /*
      Read input string and log to file.
   */
   Mprintf( 0, "%s:", prompt);
#ifdef NAVIGATOR   /* make sure we echo in NAVIGATOR mode */
   i = 0;
   cbreak();
   do {
   	cmdstr[ i ] = getc( stdin );
	if ( cmdstr[ i ] == 8 ) {
		if ( i > 0 ) {
			Mprintf( 0, "%c %c", 8, 8);
			i--;
		}
	} else {
		Mprintf( 0, "%c", cmdstr[ i ]);
		i++;
	}
   } while ( cmdstr[ i-1 ] != '\n' && cmdstr[ i-1 ] != '\r' );
   cmdstr[ i ] = '\0';
   nocbreak();
#else
   if (fgets( cmdstr, SZ_CMDSTR, stdin ) == NULL) {
	return(CmdQuit);
   }
#endif

   /* 
      Make sure string is zero terminated and get rid of newline at end.
   */
   i = 0;
   cmdstr[SZ_CMDSTR] = '\0';   
   if ( strlen(cmdstr) ) 
      cmdstr[strlen(cmdstr)-1] ='\0';

   if ( cmdstr[0] == '\n' ) return ( CmdUnknown );
   if ( cmdstr[0] == '\0' ) return ( CmdUnknown );
   if ( cmdstr[0] ==  '%' ) return ( CmdUnknown );
   if ( cmdstr[0] ==  '#' ) return ( CmdUnknown );
   if ( cmdstr[0] == '?' ) {
      /*
         Help command.  Displays availible menu options.
      */
      Mprintf( 0, "MENU OPTIONS:" );
      while ( cmdMenu[i].keys != NULL ) {
         Mprintf(0, "\n  %-10s - %s", cmdMenu[i].keys, cmdMenu[i].text );
         i++;
      }
      Mprintf( 0, "\n  %-10s - %s", "<", "Back (quit)" );
      Mprintf( 0, "\n  %-10s - %s", "?", "Help" );
      Mprintf( 0, "\n");
      return( CmdHelp );
   }
   else if ( cmdstr[0] == '<' ) {
      /*
         Quit commmand.  Return command CmdQuit.
      */
      return( CmdQuit );
   }
   else {
      /*
         Check if entered command is legal.  If so return the
         command type.
      */
      while ( cmdMenu[i].keys != NULL ) {
         if ( strchr(cmdMenu[i].keys,cmdstr[0]) != NULL )
            return ( cmdMenu[i].cmd );
         i++;
      }
   }   

   /* 
     Command entered is unknown, so display an error message. 
   */
   Mprintf( 0, "Unknown command '%s'\n", cmdstr );

   return ( CmdUnknown );
}



char 
/*=============================================================================
|
| Descr.:    Returns a specified paramenter of a given command string.
|
| In:   cmdstr   The command string 
|       no       The number of the parameter we want to get.
|
| Return:    Pointer to paramenter number no in the command string
| 
| Side eff.: None.
|
=============================================================================*/
*CmdParam( char cmdstr[], int no )
{
   static char tmpcmdstr[SZ_CMDSTR+1];
   char *p, *found;

   strncpy( tmpcmdstr, cmdstr, SZ_CMDSTR );
   found = NULL;
   p = strtok( tmpcmdstr, " " );
   while ( p != NULL ) {
      if ( no == 0 )
         found = p;
      no--;
      p = strtok( NULL, " " );
   }

   return ( found );
}

void ParseMakeMoves(char *param)
/* Makes moves from the string param, enters into the IDA history */
/* Uses IdaInfo and IdaMaze */
{
	int no,cut;
	IDAARRAY *S;
#ifdef STONEREACH
	int i, srcut;
	IDAARRAY *lastS;
#endif

#ifdef STONEREACH
	IdaInfo->neilflag = -1;
#endif
	no=2;
	while (param[no] != '\0' && param[no] != '\n') {
		S = &IdaInfo->IdaArray[IdaInfo->IdaMaze->currentmovenumber];
		while (param[no]==' ' || param[no]=='*') no++;
		if (param[no]<'A'||param[no]>('A'+XSIZE)) {
			Mprintf( 0, "Error in move string\n");
			goto READ_END;
		}
		S->currentmove.from = (param[no++]-'A')*YSIZE;
		S->currentmove.from +=(YSIZE - (param[no++]-'a')) -1;
READ_TO:
		S = &IdaInfo->IdaArray[IdaInfo->IdaMaze->currentmovenumber];
		while (param[no]=='-' || param[no]=='*') no++;
		if (param[no]<'A'||param[no]>('A'+XSIZE)) {
			Mprintf( 0, "Error in move string\n");
			goto READ_END;
		}
		S->currentmove.to  = (param[no++]-'A')*YSIZE;
		S->currentmove.to += (YSIZE - (param[no++]-'a')) -1;
		S->currentmove.last_over = S->currentmove.from;
		S->currentmove.move_dist =
			DistToGoal(IdaInfo->IdaMaze,S->currentmove.from,
			S->currentmove.to,&S->currentmove.last_over);
		if (  (IdaInfo->IdaMaze->groom_index[S->currentmove.to]>=0)
		    &&(IdaInfo->IdaMaze->gmtrees[IdaInfo->IdaMaze->groom_index[S->currentmove.to]]
		       != NULL)) {
			S->currentmove.macro_id = 4;
		} else S->currentmove.macro_id = 0;
		Mprintf( 0, "Moving: %s\n",HumanMove(S->currentmove));
		cut = RegisterMove(&S->currentmove,
			IdaInfo->IdaMaze->currentmovenumber);

#ifdef STONEREACH
		if( IdaInfo->neilflag >= 0 ) {
		  lastS = &( IdaInfo->IdaArray[IdaInfo->IdaMaze->currentmovenumber - 1 ] );
		  for( i = 0; i < IdaInfo->IdaMaze->number_stones; i++ )
		    if( StoneReachChanged( lastS->unmove.stone_reach[ i ],
					   IdaInfo->IdaMaze->stone_reach[ i ],
					   lastS->unmove.stonefrom,
					   lastS->unmove.stoneto ) ) {
		      IdaInfo->neilflag = -1;
		      break;
		    }
		}
		SetStoneReach( IdaInfo->IdaMaze );
		srcut = 0;
		if( IdaInfo->neilflag >= 0 &&
		    S->currentmove.from != IdaInfo->neilflag ) {
		  /* printf( "Cut: %i,%i to %i,%i\n",
		     S->currentmove.from / YSIZE,
		     S->currentmove.from % YSIZE, S->currentmove.to / YSIZE,
		     S->currentmove.to % YSIZE );
		     printf( "last move was to %i,%i from %i,%i\n",
		     lastS->unmove.stoneto / YSIZE,
		     lastS->unmove.stoneto % YSIZE,
		     lastS->unmove.stonefrom / YSIZE,
		     lastS->unmove.stonefrom % YSIZE );
		     PrintMaze( IdaInfo->IdaMaze ); */
		  srcut = 1;
		}
#endif
		MakeMove(IdaInfo->IdaMaze,&S->currentmove,&S->unmove);
#ifdef STONEREACH
IdaInfo->neilflag = S->currentmove.to;
#endif
		PrintMaze(IdaInfo->IdaMaze);
#ifdef STONEREACH
		Mprintf( 0, "XX g: %3d, h: %3d, g+h: %3d, distant: %c, cut: %c, sr cut: %c\n",
#else
		Mprintf( 0, "XX g: %3d, h: %3d, g+h: %3d, distant: %c, cut: %c, sr cut: %c\n",
#endif
			IdaInfo->IdaMaze->g, IdaInfo->IdaMaze->h,
			IdaInfo->IdaMaze->g+IdaInfo->IdaMaze->h,
			S->distant?'Y':'N',cut?'Y':'N'
#ifdef STONEREACH
			 , srcut ? 'Y' : 'N'
#endif
			 );
		while (param[no]==' ' || param[no]=='*') no++;
		if (param[no]=='-') {
			IdaInfo->IdaArray[IdaInfo->IdaMaze->currentmovenumber].
				currentmove.from = S->currentmove.to;
			goto READ_TO;
		}
	}
READ_END:
	;
}
