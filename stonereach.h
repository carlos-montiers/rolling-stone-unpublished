#ifndef STONEREACH_DEFH
#define STONEREACH_DEFH

void FindStoneReach( MAZE *maze, int stone, BitString reach );
void SetStoneReach( MAZE *maze );
int StoneReachChanged( BitString old, BitString new, PHYSID from, PHYSID to );

#endif
