void *My_realloc(void *p, int s);
void *My_malloc(int s);
int My_free(void *p);

extern void My_qsort(void *base, int nel, int width,
          int (*compar) (const void *, const void *));

void My_exit(int code, char *format, ... );
