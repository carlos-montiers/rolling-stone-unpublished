#CC = purify -freeze-on-error=yes -inuse-at-exit=yes gcc
CC = gcc

OPT = -O2
#OPT = -O0 -m32
#OPT = -g
#WARN = -Wall
#INCLUDE = -I$(HOME)/iris4d/include
#LIBDIRS = -L$(HOME)/iris4d/lib $(NAVILIBS)
#LIB = -lfl
#LIB = -lncurses -ltermcap 
#LIB = -lm

PC = -DPC




#DEBUG = -DDEBUG
#NAVI = -DNAVIGATOR
#GTV = -DGTV
#DDB = -DDYNAMICDB
RATIO = -DRATIO
RRR = -DRRR

#MACHINE = -DHOME

OPTIONS = $(PC) $(DEBUG) $(NAVI) $(GTV) $(DDB) $(RATIO) $(RRR)
CFLAGS = $(OPT) $(WARN) $(INCLUDE) $(LIBDIRS) $(OPTIONS)

##########################################################
#
#  DL (deadlock) is a flag that changes program behaviour 
#  to solve deadlock analysis.
#  P1 or P2 are responsible for which pattern is used, this 
#     will have to change in the future since several patterns
#     will have to co-exist
#  COPY_LB_TABLE uses a backup of the lb_table in UNMOVE
#
##########################################################

DL:

OBJ = moves.o debug.o board.o ida.o realtime.o\
 revsearch.o deadlock.o bitstring.o pensearch.o\
 init.o io.o mark.o conflicts.o deadsearch.o bisearch.o\
 dl.o mymem.o gmhashtable.o safesquare.o backsearch.o histogram.o\
 weights.o lowerbound.o hashtable.o stats.o tree.o menu.o macro.o gtv.o\
 navigator.o confdb.o stonereach.o random.o priority.o

board.o: board.h  init.h

init.o: board.h

DL: $(OBJ) board.h  init.h dl.c
	$(CC) $(CFLAGS) -o DL $(OBJ) $(LIB)

DLtest: $(OBJ) board.h  init.h dl.c
	$(CC) $(CFLAGS) -o DLtest $(OBJ) $(LIB)

All: clean Maze

clean:
	rm -f $(OBJ) DL

dis: display.c
	$(CC) $(CFLAGS) -o dis display.c $(LIB)

convert: convert.c
	gcc -g convert.c -o convert

bs: bitstring.c bitstring.h bs.c
	gcc -g -o bs bitstring.c bs.c

