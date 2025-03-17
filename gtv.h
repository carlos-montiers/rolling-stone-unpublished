#define MAX_GTV_DEPTH 200

#ifdef GTV
#define GTVAny(x)	x
#else
#define GTVAny(x)
#endif

int  GTVOpen( int d, char fen[] );
void GTVNodeEnter( int d, int alpha, int beta, char move[], int type );
void GTVNodeExit( int d, int score, char *best_move );
void GTVClose( );
char *GTVFen(MAZE *maze);
char *GTVMove(MOVE move);
