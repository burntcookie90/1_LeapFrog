#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define FILENAME "/proc/bleep_entry"
#define DEBUG 1

long getRandom(){
		long random;
		FILE *fp;
		if(DEBUG) printf("Opening file for write\n");
		fp = fopen(FILENAME,"r");
		while (random != EOF){
			random = fscanf(fp,"%ld",random);
			printf("%c", random);
		}
		fclose(fp);
		return random;
}

void setSeed(long seed){
	FILE *fp;
	if(DEBUG) printf("Opening file for write\n");
	if(fp = fopen(FILENAME,"w")){
			if(DEBUG) printf("Opened file for write\n");
			if(DEBUG) printf("seed: %ld\n",seed);
			fprintf(fp,"%ld",seed);
			fclose(fp);
	}
}

int main(){
	setSeed(0);
	printf("%ld\n",getRandom());
}
