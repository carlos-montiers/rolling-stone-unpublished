#include "board.h"

GMHASHENTRY GMHashTable[GMMAX_HASHENTRIES];

void GMInitHashTable() {
	memset(GMHashTable,0,sizeof(GMHASHENTRY)*GMMAX_HASHENTRIES);
}

void GMStoreHashTable(MAZE *maze, GMNODE *n)
{
	HASHKEY gmhashkey = maze->hashkey&GMHASHMASK;
	if (   GMHashTable[gmhashkey].lock != 0
	    && GMHashTable[gmhashkey].lock != maze->hashkey) 
		IdaInfo->gmtt_cols++;
	GMHashTable[gmhashkey].lock       = maze->hashkey;
	GMHashTable[gmhashkey].gmnode     = n;
	n->hashkey = maze->hashkey;
}

GMNODE *GMGetHashTable(HASHKEY key) {
	HASHKEY gmhashkey = key&GMHASHMASK;
	IdaInfo->gmtt_reqs++;
	if (   GMHashTable[gmhashkey].lock == key ) {
		IdaInfo->gmtt_hits++;
		return(GMHashTable[gmhashkey].gmnode);
	}
	return(NULL);
}

void GMDelHashEntry(HASHKEY key) {

	HASHKEY gmhashkey = key&GMHASHMASK;
	if (   GMHashTable[gmhashkey].lock == key ) {
		GMHashTable[gmhashkey].lock = 0;
		GMHashTable[gmhashkey].gmnode = NULL;
	}
}
