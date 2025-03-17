extern GMHASHENTRY GMHashTable[GMMAX_HASHENTRIES];

void GMInitHashTable();
void GMStoreHashTable(MAZE *maze, GMNODE *n);
GMNODE *GMGetHashTable(HASHKEY key);
void GMDelHashEntry(HASHKEY key);
