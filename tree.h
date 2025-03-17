typedef enum { BlancSquare, StoneSquare, WallSquare} SQUARE;

typedef int DLENTRY[3];

typedef struct {
	int    x_ofs;	   /* both of these are in -1 1's */
	int    y_ofs;	   /* and measured as from the stone in the midle */
	int    direction;  /* from which direction was the stone moved in */
	int    symetrical; /* is the patter symetrical? if yes, don't test
			    * mirror */
	int    deadlocks;
	int    unknowns;
	int    irrelevants;
	int    visited;
	MAZE  *maze;
} DLSUP;


typedef struct {
	DLSUP   *Sup;
	DLENTRY *Array;
	int     *Next;
	int      Max;
	int	 CurrentLength;
} TREE;

extern TREE  AllTrees[10];
extern short NumberTrees;
extern DLSUP DlSup1[], DlSup2[];

int  InitTree(DLSUP *sup);
void LoadTree(int id, char *filename);
void SaveTree(int id, char *filename);
int  DeadTree( MAZE *maze, PHYSID pos, int direction);
