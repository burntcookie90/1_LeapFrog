#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define FILENAME "/proc/bleep_entry"
#define DEBUG 0

char* getRandom(){
		char* random;
		FILE *fp;
		random = malloc(sizeof(char*));

		/*if(DEBUG) printf("Opening file for read\n");*/

		if((fp = fopen(FILENAME,"r"))){
				/*if(DEBUG) printf("Opened file for read\n");*/
				/*random = fscanf(fp,"%ld",&random);*/
				fgets(random,100,fp);
				/*printf("Random is: %s\n", random);*/
				fclose(fp);
				return random;
		}
		
		if(DEBUG) printf("File wasn't found\n");
		return NULL;
}

void setSeed(long seed){
	FILE *fp;
	if(DEBUG) printf("Opening file for write\n");
	if((fp = fopen(FILENAME,"w"))){
			if(DEBUG) printf("Opened file for write\n");
			if(DEBUG) printf("seed: %ld\n",seed);
			fprintf(fp,"%ld",seed);
			fclose(fp);
	}
}

int main(){
		int th_id;
		omp_set_num_threads(20);
#pragma omp parallel
{
	setSeed(10);
}
#pragma omp parallel private(th_id)
{
	th_id = omp_get_thread_num();
	printf("\nThread: %d number:: %s\n",th_id,getRandom());
	printf("\nThread: %d number:: %s\n",th_id,getRandom());
	printf("\nThread: %d number:: %s\n",th_id,getRandom());
}
	 
	return 0;
}
