#define MAXHIST 500

typedef int H_int;

typedef struct {
	long  total_sum;
	H_int total_count;
	int   max_index;
	H_int max_count;
	H_int count[MAXHIST];
} HISTOGRAM;

void  InitHist(HISTOGRAM *hist);
void  ResetHist(HISTOGRAM *hist);
void  IncCounter(HISTOGRAM *hist, int index);
float GetAvgHist(HISTOGRAM *hist);
void  PrintHist(HISTOGRAM *hist);
void  PrintHist2(HISTOGRAM *hist, HISTOGRAM *hist2);
