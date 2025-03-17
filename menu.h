typedef enum {               /* Commands types */

   /* General menu commands. */
   CmdUnknown,               /* Unknown command. */
   CmdQuit,                  /* Quit in menu. */
   CmdHelp,                  /* List menu options. */

   /* Main menu commands. */
   CmdSolve,		     /* Solve Maze */
   CmdBack,		     /* Back Solve Maze */
   CmdBidir,		     /* BiDirect Solve Maze */
   CmdReal,		     /* Real Time Solve Maze */
CmdRandom,
   CmdAbort,		     /* Abort Node Count set */
   CmdTestAll,		     /* Test All Mazes in screen */
   CmdPrint,		     /* */
   CmdBounds,		     /* */
   CmdPosNr,		     /* */
   CmdMove,		     /* */
   CmdTestX,		     /* */

   CmdOptions,		     /* */
   CmdOptionEX,  	     /* E Examine all settings */
   CmdOptionTT,   	     /* T [on/off] Toggle TT */
   CmdOptionDL,   	     /* D [on/off] Toggle deadlock det. movegen */
   CmdOptionDZ,   	     /* Z [on/off] Toggle deadlock2 det. movegen */
   CmdOptionDS,   	     /* S [on/off] Toggle deadMove search */
   CmdOptionPS,   	     /* N [on/off] Toggle penMove search */
   CmdOptionMI,   	     /* I [on/off] Toggle multi insert */
   CmdOptionST,   	     /* X [on/off] Toggle store tested */
   CmdOptionPT,   	     /* P [number] Switch Pattern DBs */
   CmdOptionMP,   	     /* M [on/off] Toggle LB manpos */
   CmdOptionCF,   	     /* C [on/off] Toggle LB conflict */
   CmdOptionTM,   	     /* Tunnel Macro switch */
   CmdOptionGM,   	     /* General Goal Macro switch */
   CmdOptionGO,   	     /* Goal Macro switch */
   CmdOptionCG,   	     /* Cut Goal Macro */
   CmdOptionXD,   	     /* Cut Goal Macro */
   CmdOptionLC,   	     /* Local Cut (k,m) */
   CmdOptionLA,   	     /* Auto Set Local Cut Params */
   CmdOptionMO,   	     /* Move Order Index */

   CmdShow,		     /* */
   CmdShowConfl,	     /* */
   CmdShowXDist,	     /* */
   CmdShowSDist,	     /* */
   CmdShowMDist,	     /* */

   /* Navigator Menu Commands */
   CmdNavigator,	     /* Navigator Menu */
   CmdNavEX,		     /* Navigator Menu */
   CmdNavPatt,		     /* Navigate through patterns */
   CmdNavOnDeep,	     /* Set on_deepest */
   CmdNavOnPS,		     /* Set on_ps_start */

   CmdNavPrNM,		     /* print? number_moves */
   CmdNavPrUA,		     /* set update at count */

   CmdLastCommand	     /* Just last command, not used */
} COMMAND;


typedef struct {
  char    *keys;             /* Key code for command. */
  COMMAND  cmd;              /* Command type. */
  char    *text;             /* Information text. */
} CMDMENU;

#define SZ_CMDSTR             100      /* Size of the input command buffer */
#define COMMAND_IS(buff,cmd) !strncmp(cmd,buff,MIN(strlen(cmd),strlen(buff)) )

extern int Cur_Maze_Number;
extern IDA MainIdaInfo;

void MainMenu();
void OptionsMenu();
void ShowMenu();
COMMAND  GetCommand(char prompt[], CMDMENU *pcmdMenu, char cmdstr[] );
char    *CmdParam( char cmdstr[], int no );
void     ParseMakeMoves(char *param);

