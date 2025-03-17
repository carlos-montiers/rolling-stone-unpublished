#include "board.h"

void Mprintf( int priority, char *format, ... )
{
   va_list argptr;
   static char msg[2000];

#ifndef NAVIGATOR
   if (priority <= IdaInfo->PrintPriority) {
   	va_start( argptr, format );
   	vsprintf( msg, format, argptr );
   	va_end( argptr );

        printf( "%s", msg );	/* This is the only printf in the entire
				 * program!!!!!! */
        fflush(stdout);
   }
#else
   int i;

   va_start( argptr, format );
   vsprintf( msg, format, argptr );
   va_end( argptr );
   i = strlen( msg ) - 1;
   if ( msg[ i ] == '\n' || msg[ i ] == '\r' ) {
	i--;
	while ( msg[ i ] == ' ' || msg[ i ] == '\t' ) i--;
	msg[ i+1 ] = '\n';
	msg[ i+2 ] = '\0';
   }
   Fwaddstr(MyNavigator.fdefault,msg);
#endif
}

void Debug( int level, int indent, char *format, ... )
/*
	0: Highest priority, prints exit messages and error stuff
	2: Highest level stuff, like loading and saving mazes
	4: High level search stuff, calls to mea
	6: Lower level stuff, path searcher routines
	8: all the rest
*/
{

   va_list argptr;
   char msg[2000];
   #define xxx " | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |"
   char xxx2[800];

      /*
         The level of this debug call is less or equal to current debug level,
         therefore debug messages are printed.
      */
   if (IdaInfo->PrintPriority >= level) {
      if ( format != NULL ) {
         va_start( argptr, format );
         vsprintf( msg, format, argptr );
         va_end( argptr );
      }
      indent += IdaInfo->base_indent;
      if (indent>=0) {
      	strncpy(xxx2,xxx,indent);
      	xxx2[indent] = '\0';
      	Mprintf( level, ". %3d %s ", indent, xxx2);
      }
      Mprintf( level, "%s", msg );
   }
}

void Assert(int cond, char *format, ...) {

   va_list argptr;
   char msg[2000];


   if (!cond) {
	if ( format != NULL ) {
        	va_start( argptr, format );
        	vsprintf( msg, format, argptr );
        	va_end( argptr );
	}
	My_exit(1,msg);
   }
}
