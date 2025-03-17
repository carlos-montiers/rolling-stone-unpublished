#ifdef DEBUG
#define SR(func)  func
#else
#define SR(func)
#endif

void Mprintf( int priority, char *format, ... );
void Debug( int level, int indent, char *format, ... );
void Assert(int cond, char *format, ...);

