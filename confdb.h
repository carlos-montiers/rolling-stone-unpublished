#ifndef CONFDB_DEFH
#define CONFDB_DEFH

#define DBFILE "database"

typedef struct {
  char xsize, ysize;
  unsigned char walls[ 6 ];
  unsigned char stones[ 6 ];
} dbentry;

extern dbentry *_db;
extern int _dbsize;
extern int _dbused;
extern PENALTY _dbpatterns;

void LoadConflicts( MAZE *maze, CONFLICTS *c );
void DumpConflicts( MAZE *maze, CONFLICTS *c );

#endif
