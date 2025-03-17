#include "board.h"

MOVE DummyMove = {ENDPATH,ENDPATH,ENDPATH};
int  PP;
int32_t dl_pos_nc=0, dl_neg_nc=0; /* node counts for pos/neg searches */
int  dl_pos_sc=0, dl_neg_sc=0;	/* search count for pos/neg */
int32_t pen_pos_nc=0, pen_neg_nc=0;	/* node counts for pos/neg searches */
int  pen_pos_sc=0, pen_neg_sc=0;	/* search count for pos/neg */
