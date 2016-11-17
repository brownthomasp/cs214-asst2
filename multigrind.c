#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <ctype.h>

/*
 * Struct used to pass multiple arguments to the compressor thread.
 * Has fields for the begin and end index of compression as well as the source and dest files.
 */
typedef struct compressorArguments {
	int beginIndex;
	int endIndex;
	char *src;
	char *dest;
} compressorArguments;

/*
 * Mallocs and sets a new a compressorArguments structure for passing into the compress thread.
 */
compressorArguments* setArguments(int beginIndex, int endIndex, char *src, char *dest) {
	compressorArguments *args = malloc(sizeof(compressorArguments));
	
	args->beginIndex = beginIndex;
	args->endIndex = endIndex;
	args->src = src;
	args->dest = dest;
	
	return args;
}

/*
 * Compresses a portion of a file via LOLS (length of long sequence) and exits its thread.
 */
void* compress(void *argsVoidPtr) {
    compressorArguments *args = (compressorArguments *)argsVoidPtr;

	FILE *src = fopen(args->src, "r"),
         *dest = fopen(args->dest, "w"); 

	char thisChar = getc(src),
         prevChar = thisChar;
	int sequenceLength = 0;

	fseek(src, args->beginIndex, SEEK_SET);
	
	if (ftell(src) <= args->endIndex) {
	 	//Do compression logic.
		do {
			thisChar = fgetc(src);
	
			if (!isalpha(thisChar)) continue;
	
			//If we're past end index, we hit EOF (for this thread).
			//If this is the same char as the last, it's a sequence and we need to advance to the next char to know what to do.
			//Otherwise, decide what to output.
			if (ftell(src) > args->endIndex) thisChar = EOF;
			if (thisChar == prevChar) sequenceLength++;
			else {
			//If there was only one or two chars in the sequence, output them as they are.
			//Otherwise, output <n><c> where n is the number of recurrences of the char c.
				if (sequenceLength == 1) fputc(prevChar, dest);
				else if (sequenceLength == 2) {
					fputc(prevChar, dest);
					fputc(prevChar, dest);
				} else {
					fprintf(dest, "%d", sequenceLength);
					fputc(prevChar, dest);
				}

				//Reset sequence length and update the previous character.
				sequenceLength = 1;
				prevChar = thisChar;
			}
		} while (thisChar != EOF);
	}

	fclose(src);
	fclose(dest);

	pthread_exit(0);
}


void compressT_LOLS(char *src, char *partsArg) {
	FILE *fp = fopen(src, "r");
	if (fp == NULL) {
		perror("ERROR");
		return;	
	}

//	printf("Will try to compress %s into %s part(s)\n", src, partsArg);

	int parts = atoi(partsArg);

	//Create strings for output file names.
	char out[strlen(src + 10)];
	char fileName[strlen(src) + 6];
	sprintf(fileName, "%s_LOLS", src);
	fileName[strlen(src) - 4] = '_';

	if (parts > 1) sprintf(out, "%s%d", fileName, 0);
	else sprintf(out, "%s", fileName);

	//printf("output file name will be %s\n", fileName);

	//Get file length.
	fseek(fp, -1, SEEK_END);
	long int size = ftell(fp);
	fclose(fp);

	long int begin = 0;
	long int end = size/parts;
	long int temp;
	pthread_t pthreads[parts];
	int threadNumber = -1;
	compressorArguments *args = NULL;
	
	int roundup = size - end * parts;
	if (roundup != 0) {
		end++;
		roundup--;
	}

	//While pieces left, break the file down into several pieces and then spawn a thread to handle each piece.
	//If pthread_create doesn't return 0, gracefully exit the program and report we were unable to finish.
	while(begin < size) {

		//printf("will try to compress bytes %ld to %ld\n", begin, end);

		threadNumber++;

		if (parts > 1) sprintf(out, "%s%d", fileName, threadNumber);

		args = setArguments(begin, end, src, out);

		if (pthread_create(&pthreads[threadNumber], NULL, &compress, (void*)args) !=  0) {
			fprintf(stderr, "ERROR: unable to create new worker thread #%dest to perform compression.\n", threadNumber);
			return;
		} //else printf("Created new thread #%d.\n", threadNumber);

		temp = end;
		end += size/parts;
		begin = temp;

		if (roundup != 0) {
			end++;
			roundup--;
		}

		free(args);
		args = NULL;
	}	

	//Join the various threads to this thread and hold for termination.
	while (threadNumber >= 0) {
		pthread_join(pthreads[threadNumber], NULL);

		//if (pthread_join(pthreads[threadNumber], NULL) != 0) fprintf(stderr, "Unable to join worker thread #%d; it may have already finished.\n", threadNumber);
		//else printf("Worker thread #%d successfully joined and completed.\n", threadNumber);

		threadNumber--;		
	}
}

void compressR_LOLS(char * arg1 , char * arg2) {

  // open target file
  FILE * fp = fopen(arg1, "r");
  if (!fp) { 
    perror("ERROR ");
    return;
  }

  int split = atoi(arg2);   // number of child processes to make
  
  // char out[] will be the name of the compressed files
  char out[strlen(arg1) + 10];
  char file_name[strlen(arg1) + 6];
  sprintf(file_name, "%s_LOLS", arg1);
  file_name[strlen(arg1) - 4] = '_';

  if (split > 1) {
    sprintf(out,"%s%d", file_name, 0);
  } else {
    sprintf(out, "%s", file_name);
  }

  // determine the size of the file
  fseek(fp, -1, SEEK_END);
  long int size = ftell(fp);
  fclose(fp);

  long int begin = 0;           // the begining of the section to be given to a worker
  char bg[20];                  // a string to store begin
  long int end = size/split;    // end of the section
  char nd[20];                  // a string to store end
  long int temp;                // storage variable
  pid_t workers[split];         // to store the childrens' pids
  int wk_num = 0;               // stores worker number

  int roundup = size - end*split;  // to keep track of how many times we must round up
  if (roundup) { end++; roundup--; }

  while (begin < size) {

    // format the name of the compressed file
    if (split > 1) {
      sprintf(out, "%s%d", file_name, wk_num);
    }
    
    workers[wk_num] = fork();
    if (!workers[wk_num]) {
      sprintf(bg, "%ld", begin);
      sprintf(nd, "%ld", end);
      execl("./compressR_worker_LOLS", "./compressR_worker_LOLS", arg1, out, bg, nd, (char*)0);

      fprintf(stderr, "ERROR: Worker %d failed to exec\n", wk_num);
      return;
    }

    temp = end;
    end += size/split;
    begin = temp;

    if (roundup) { end++; roundup--; }
    wk_num++;

  }

  int status;
  pid_t pid;
  while (wk_num) {
    pid =  wait(&status);
    // printf("Child process %d has finished\n", pid);
    wk_num--;

  }

}





int main(int argc, char ** argv) {

  FILE * fp;

  int i = 0, j = 0;
  char c = 0;

  double R_total_1 = 0, R_total_2 = 0, R_total_4 = 0, R_total_8 = 0;
  double T_total_1 = 0, T_total_2 = 0, T_total_4 = 0, T_total_8 = 0;

  struct timeval start, end;

  printf("Compressing a 200 byte file into\n");
  for (i = 0; i < 100; i++) {
	  fp = fopen("200bytes.txt", "w");
	  for (j = 0; j < 200; j++) {
	    c = 'A' + rand()%58;
	    fputc(c, fp);
	  }
	  fclose(fp);

	  gettimeofday(&start, NULL);
	  compressR_LOLS("200bytes.txt", "1");
	  gettimeofday(&end, NULL);
	  R_total_1 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("200bytes.txt", "1");
	  gettimeofday(&end, NULL);
	  T_total_1 += 1.0E-6*(end.tv_usec - start.tv_usec); 

	  gettimeofday(&start, NULL);
	  compressR_LOLS("200bytes.txt", "2");
	  gettimeofday(&end, NULL);
	  R_total_2 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("200bytes.txt", "2");
	  gettimeofday(&end, NULL);
	  T_total_2 += 1.0E-6*(end.tv_usec - start.tv_usec); 

	  gettimeofday(&start, NULL);
	  compressR_LOLS("200bytes.txt", "4");
	  gettimeofday(&end, NULL);
	  R_total_4 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("200bytes.txt", "4");
	  gettimeofday(&end, NULL);
	  T_total_4 += 1.0E-6*(end.tv_usec - start.tv_usec); 

	  gettimeofday(&start, NULL);
	  compressR_LOLS("200bytes.txt", "8");
	  gettimeofday(&end, NULL);
	  R_total_8 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("200bytes.txt", "8");
	  gettimeofday(&end, NULL);
	  T_total_8 += 1.0E-6*(end.tv_usec - start.tv_usec); 
  }

  printf("\t1 files: %f/%f\n", 0.01*R_total_1, T_total_1);
  printf("\t2 files: %f/%f\n", 0.01*R_total_2, T_total_2);
  printf("\t4 files: %f/%f\n", 0.01*R_total_4, T_total_4);
  printf("\t8 files: %f/%f\n", 0.01*R_total_8, T_total_8);
  printf("Compressing a 3200 byte file into\n");

  R_total_1 = 0;
  R_total_2 = 0;
  R_total_4 = 0;
  R_total_8 = 0;

  for (i = 0; i < 100; i++) {
	  fp = fopen("3200bytes.txt", "w");
	  for (j = 0; j < 3200; j++) {
	    c = 'A' + rand()%58;
	    fputc(c, fp);
	  }
	  fclose(fp);

	  gettimeofday(&start, NULL);
	  compressR_LOLS("3200bytes.txt", "1");
	  gettimeofday(&end, NULL);
	  R_total_1 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("3200bytes.txt", "1");
	  gettimeofday(&end, NULL);
	  T_total_1 += 1.0E-6*(end.tv_usec - start.tv_usec); 

	  gettimeofday(&start, NULL);
	  compressR_LOLS("3200bytes.txt", "2");
	  gettimeofday(&end, NULL);
	  R_total_2 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("3200bytes.txt", "2");
	  gettimeofday(&end, NULL);
	  T_total_2 += 1.0E-6*(end.tv_usec - start.tv_usec); 

	  gettimeofday(&start, NULL);
	  compressR_LOLS("3200bytes.txt", "4");
	  gettimeofday(&end, NULL);
	  T_total_4 += 1.0E-6*(end.tv_usec - start.tv_usec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("3200bytes.txt", "4");
	  gettimeofday(&end, NULL);
	  T_total_4 += 1.0E-6*(end.tv_usec - start.tv_usec); 

	  gettimeofday(&start, NULL);
	  compressR_LOLS("3200bytes.txt", "8");
	  gettimeofday(&end, NULL);
	  R_total_8 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("3200bytes.txt", "8");
	  gettimeofday(&end, NULL);
	  T_total_8 += 1.0E-6*(end.tv_usec - start.tv_usec); 
  }

  printf("\t1 files: %f/%f\n", 0.01*R_total_1, T_total_1);
  printf("\t2 files: %f/%f\n", 0.01*R_total_2, T_total_2);
  printf("\t4 files: %f/%f\n", 0.01*R_total_4, T_total_4);
  printf("\t8 files: %f/%f\n", 0.01*R_total_8, T_total_8);
  printf("Compressing a 12800 byte file into\n");

  R_total_1 = 0;
  R_total_2 = 0;
  R_total_4 = 0;
  R_total_8 = 0;

  for (i = 0; i < 100; i++) {
	  fp = fopen("12800bytes.txt", "w");
	  for (j = 0; j < 12800; j++) {
	    c = 'A' + rand()%58;
	    fputc(c, fp);
	  }
	  fclose(fp);

	  gettimeofday(&start, NULL);
	  compressR_LOLS("12800bytes.txt", "1");
	  gettimeofday(&end, NULL);
	  R_total_1 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("12800bytes.txt", "1");
	  gettimeofday(&end, NULL);
	  T_total_1 += 1.0E-6*(end.tv_usec - start.tv_usec); 

	  gettimeofday(&start, NULL);
	  compressR_LOLS("12800bytes.txt", "2");
	  gettimeofday(&end, NULL);
	  R_total_2 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("12800bytes.txt", "2");
	  gettimeofday(&end, NULL);
	  T_total_2 += 1.0E-6*(end.tv_usec - start.tv_usec); 

	  gettimeofday(&start, NULL);
	  compressR_LOLS("12800bytes.txt", "4");
	  gettimeofday(&end, NULL);
	  R_total_4 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("12800bytes.txt", "4");
	  gettimeofday(&end, NULL);
	  T_total_4 += 1.0E-6*(end.tv_usec - start.tv_usec); 

	  gettimeofday(&start, NULL);
	  compressR_LOLS("12800bytes.txt", "8");
	  gettimeofday(&end, NULL);
	  R_total_8 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("12800bytes.txt", "8");
	  gettimeofday(&end, NULL);
	  T_total_8 += 1.0E-6*(end.tv_usec - start.tv_usec); 
  }

  printf("\t1 files: %f/%f\n", 0.01*R_total_1, T_total_1);
  printf("\t2 files: %f/%f\n", 0.01*R_total_2, T_total_2);
  printf("\t4 files: %f/%f\n", 0.01*R_total_4, T_total_4);
  printf("\t8 files: %f/%f\n", 0.01*R_total_8, T_total_8);
  printf("Compressing a 102400 byte file into\n");

  R_total_1 = 0;
  R_total_2 = 0;
  R_total_4 = 0;
  R_total_8 = 0;

  for (i = 0; i < 100; i++) {
	  fp = fopen("102400bytes.txt", "w");
	  for (j = 0; j < 102400; j++) {
	    c = 'A' + rand()%58;
	    fputc(c, fp);
	  }
	  fclose(fp);

	  gettimeofday(&start, NULL);
	  compressR_LOLS("102400bytes.txt", "1");
	  gettimeofday(&end, NULL);
	  R_total_1 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("102400bytes.txt", "1");
	  gettimeofday(&end, NULL);
	  T_total_1 += 1.0E-6*(end.tv_usec - start.tv_usec); 

	  gettimeofday(&start, NULL);
	  compressR_LOLS("102400bytes.txt", "2");
	  gettimeofday(&end, NULL);
	  R_total_2 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("102400bytes.txt", "2");
	  gettimeofday(&end, NULL);
	  T_total_2 += 1.0E-6*(end.tv_usec - start.tv_usec); 

	  gettimeofday(&start, NULL);
	  compressR_LOLS("102400bytes.txt", "4");
	  gettimeofday(&end, NULL);
	  R_total_4 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("102400bytes.txt", "4");
	  gettimeofday(&end, NULL);
	  T_total_4 += 1.0E-6*(end.tv_usec - start.tv_usec); 

	  gettimeofday(&start, NULL);
	  compressR_LOLS("102400bytes.txt", "8");
	  gettimeofday(&end, NULL);
	  R_total_8 += 1.0*(end.tv_sec - start.tv_sec);

	  gettimeofday(&start, NULL);
	  compressT_LOLS("102400bytes.txt", "8");
	  gettimeofday(&end, NULL);
	  T_total_8 += 1.0E-6*(end.tv_usec - start.tv_usec); 
  }

  printf("\t1 files: %f/%f\n", 0.01*R_total_1, T_total_1);
  printf("\t2 files: %f/%f\n", 0.01*R_total_2, T_total_2);
  printf("\t4 files: %f/%f\n", 0.01*R_total_4, T_total_4);
  printf("\t8 files: %f/%f\n", 0.01*R_total_8, T_total_8);

  return 0;
}