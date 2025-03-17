#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

void die(char *msg) {
	printf( msg);
	exit(1);
}

int ReadBin(char *name, int **array) {
	FILE *fp;
	struct stat buf;

	if ((fp = fopen(name,"r")) == 0)
		die("Check <in> filename\n");
	if (stat(name, &buf)!=0)
		die("Check <in> filename\n");;
	*array = (int*)malloc(buf.st_size);
	fread(*array,sizeof(int),buf.st_size,fp);
	fclose(fp);	
printf( "size: %i\n",buf.st_size/sizeof(int));
	return(buf.st_size/sizeof(int));
}

void WriteBin(char *name, int *array, int size) {
	FILE *fp;
	if ((fp = fopen(name,"w")) == NULL)
		die("Check <out> filename\n");
	fwrite(array,sizeof(int),size,fp);
	fclose(fp);

}

/******************************** Indep Stuff **************************/

int ReadIndepAscii(char *name, int **array) {
	int i;
	FILE *fp;
	int size;
	struct stat *buf;

	if ((fp = fopen(name,"r")) == 0)
                die("Check <in> filename\n");
	size = 1000;
	*array = (int*)malloc(sizeof(int)*size);
	i = 0;
	while (!feof(fp)) {
		if (i>=size) {
			size += 1000;
			*array = (int*) realloc(*array,sizeof(int)*size);
		}
		if (fscanf(fp,"%i ",(*array)+i)<1)
			die("Format error in Ascii file\n");
		i++;
	}
printf( "size: %i\n",i/sizeof(int));
	return(i);
}


void WriteIndepAscii(char *name, int *array, int size) {
	int i;
	FILE *fp;
	if ((fp = fopen(name,"w")) == 0)
		die("Check <out> filename\n");
	for (i=0; i<size; i++) {
		fprintf(fp,"%i ",array[i]);
	}
        fclose(fp);	

}



int main (int argc, char **argv, char **env) {

	int size;
	int  *array;

	printf( "sizeof(int) = %i\n",sizeof(int));
	if (argc !=4) die("Usage: convert [ToAscii|ToBin] <in> <out>\n");
	if (strcmp("ToAscii",argv[1])==0) {
		size = ReadBin(argv[2],&array);
		WriteIndepAscii(argv[3],array,size);
	} else if (strcmp("ToBin",argv[1])==0) {
		size = ReadIndepAscii(argv[2],&array);
		WriteBin(argv[3],array,size);
	} else die("Specify ToAscii or ToBin!\n");

}
