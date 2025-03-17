int VerifyConflict(CONFLICTS *c, BitString conflict, BitString reach, int pen);
void InitConflicts(CONFLICTS *c);
void DelConflicts(CONFLICTS *c);
void AddPenalties(CONFLICTS *c);
void InsertPenalty(CONFLICTS *c, int i, int p);
void InitPenalty(PENALTY *p, int penalty);
PENALTY *FindPenalty(CONFLICTS *c, int penalty);
void AddConflicts(PENALTY *p);
void InsertConflict(PENALTY *p, BitString c, BitString no_reach);
void AddConflict(CONFLICTS *c, BitString conflict,
	BitString no_reach, BitString reach, int penalty, char fromdb);

void InitDelayedConflict();
void AddDelayedConflict(CONFLICTS *c, BitString conflict,
	BitString no_reach, BitString reach, int penalty, char fromdb );
void InsertDelayedConflict();

int  GetPriorPostPen(MAZE *maze, int penalty, int *prior, int *post);
int  GetPenaltyOld(CONFLICTS *c, BitString stones, PHYSID manpos );
int  GetPenaltyMaximize(CONFLICTS *c, BitString stones, PHYSID manpos );


void PrintConflicts(MAZE *maze, CONFLICTS *c);

void AddTestedPen(CONFLICTS *c, BitString relevant, BitString stones, 
				PHYSID manpos, PHYSID stonepos);
void AddTestedDead(CONFLICTS *c, BitString relevant, BitString stones,
				PHYSID manpos, PHYSID stonepos);
int WasTestedPen(CONFLICTS *c, BitString stones, PHYSID manpos, 
				PHYSID stonepos);
int WasTestedDead(CONFLICTS *c, BitString stones, PHYSID manpos,
				PHYSID stonepos);

#define GetPenalty(a,b,c) 	GetPenaltyMaximize(a,b,c)
