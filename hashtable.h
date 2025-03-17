void     InitHashTables();
void     UnSetPathFlag(MAZE *maze);
void     SetPathFlag(MAZE *maze);
void     ClearHashTable(MAZE *maze);
HASHENTRY *StoreHashTable(MAZE *maze, int g, int down, int min_h, 
	MOVE bm, int32_t tree_size, int dl, int pen, int back, int pathflag);
HASHENTRY *GetHashTable(MAZE *maze);
void     PSSetPathFlag(MAZE *maze);
void     PSClearHashTable(MAZE *maze);
HASHENTRY *PSStoreHashTable(MAZE *maze, int g, int down, int min_h, 
	MOVE bm, int32_t tree_size, int dl, int pen, int back, int pathflag);
HASHENTRY *PSGetHashTable(MAZE *maze);
HASHKEY  NormHashKey(MAZE *maze);
HASHKEY  DeadHashKey(MAZE *maze);
HASHKEY  UpdateHashKey( MAZE *maze, UNMOVE *move);

void GGStoreHashTable(HASHKEY hashkey);
int  GGGetHashTable(HASHKEY hashkey);

extern short NextHashGen;
extern HASHKEY RandomTable[ 896 ];
extern HASHENTRY HashTableNorm[MAX_HASHENTRIES];
extern HASHENTRY HashTableElse[MAX_HASHENTRIES];

void InitRandom();
