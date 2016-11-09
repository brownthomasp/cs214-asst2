#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Struct used to pass multiple arguments to the compressor thread.
 * Has fields for the begin and end index of compression as well as the source and dest files.
 */
struct compressorArguments {
	int beginIndex;
	int endIndex;
	char *src;
	char *dest;
} compressorArguments;

/*
 * Mallocs and sets a new a compressorArguments structure for passing into the compress thread.
 */
compressorArguments* setArguments(int beginIndex, int endIndex, char *src, char *dest) {
	compressorArguments args = malloc(sizeof(compressorArguments));
	
	args->beginIndex = beginIndex;
	args->endIndex = endIndex;
	args->src = src;
	args->dest = dest;
	
	return args;
}

/*
 * Compresses a portion of a file via LOLS (length of long sequence) and exits its thread.
 */
void* compress(compressorArguments *args) {
	FILE *src = fopen(args->src, "r"),
             *dest = fopen(args->dest, "w"); 

	char thisChar = getc(src),
             prevChar = thisChar;
	int sequenceLength = 0;

	fseek(src, args->beginIndex, SEEK_SET);

 	//Do compression logic.
	do {
		thisChar = fgetc(src);
		
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
			}
			else {
				fprintf(dest, "%d", sequenceLength);
				fputc(prevChar, dest);
			}

			//Reset sequence length and update the previous character.
			sequenceLength = 1;
			prevChar = thisChar;
		}
	} while (thisChar != EOF);

	fclose(src);
	fclose(dest);

	pthread_exit(0);
}

int main(int argc, char **argv) {
	
	if (argc != 3) {
		fprintf(stderr, "ERROR: incorrect number of arguments given (%d), expected 2.\n", argc - 1);
		return -1;	
	}

	FILE *fp = fopen(argv[1], "r");
	if (fp == NULL) {
		perror("ERROR");
		return -2;	
	}

	int parts = atoi(argv[2]);
	if (parts < 1) {
		fprintf(stderr, "ERROR: number of parts specified must be at least 1.\n");
		return -3;
	}

	//Create strings for output file names.
	char out[strlen(argv[1]) + 10];
	char fileName[strlen(argv[1]) + 6];
	sprintf(fileName, "%s,_LOLS", argv[1]);
	fileName[strlen(argv[1]) - 4] = '_';

	if (parts > 1) sprintf(out, "%s%d", fileName, 0);
	else sprintf(out, "%s", fileName);

	//Check if this file has already been compressed and prompt user to confirm re-compressing.
	char answer = '0';
	if (access(out, F_OK) != -1) {
		printf("A compressed version of this file already exists. Do you want to overwrite it? (y/n)\n");
		
		while (answer != 'y' && answer != 'Y') {
			fscanf(stdin, "%c", &answer);

			if (answer == 'n' || answer == 'N') {
				printf("The file will not be overwritten. Exiting.\n");
				return 0;
			} else if (answer != 'y' && answer != 'Y') printf("Valid responses are Y/y (yes) or N/n (no). Try again:\n");
		}
	}

	//Get file length.
	fseek(fp, -1, SEEK_END);
	long int size = ftell(fp);
	fclose(fp);

	if (parts > size) {
		parts = size;
		answer = '0';

		printf("There are only %ld characters in given file %s, so this file can only be compressed into %ld parts. Continue? (y/n)\n");
		while (answer != 'y' && answer != 'Y') {
			fscanf(stdin, "%c", &answer);

			if (answer == 'n' || answer == 'N') {
				printf("The compression process will not continue. Exiting.\n");
				return 0;
			} else if (answer != 'y' && answer != 'Y') printf("Valid responses are Y/y (yes) or N/n (no). Try again:\n");
		}
	}

	long int begin = 0;
	long int end = size/parts;
	long int temp;
	pthread_t pthreads[parts];
	int threadNumber = 0;
	compressorArguments args = NULL;
	
	int roundup = size - end * parts;
	if (roundup != 0) {
		end++;
		roundup--;
	}

	//While pieces left, break the file down into several pieces and then spawn a thread to handle each piece.
	//If pthread_create doesn't return 0, gracefully exit the program and report we were unable to finish.
	while(begin < size) {
		fseek(fp, end - 1, SEEK_SET);

		if (parts > 1) sprintf(out, "%s%d", fileName, threadNumber);

		args = setArguments(begin, end, argv[1], out);

		if (pthread_create(&pthreads[threadNumber], NULL, compress, (void*) args) !=  0) {
			fprintf(stderr, "ERROR: unable to create new worker thread #%d to perform compression.\n", threadNumber);
			return -4;
		}

		temp = end;
		end += size/parts;
		begin = temp;

		if (roundup != 0) {
			end++;
			roundup--;
		}

		threadNumber++;
		free(args);
	}	

	//Check if for some reason we couldn't compress to the desired number of parts and report if this happened.
	if (threadNumber != parts) printf("Unfortunately, the given file %s could only be compressed into %d parts, instead of the requested number (%d).\n", argv[1], threadNumber. parts);

	//Join the various threads to this thread and hold for termination.
	while (threadNumber >= 0) {
		if (pthread_join(pthreads[threadNumber], NULL) != 0) fprintf(stderr, "ERROR: unable to join worker thread #%d.\n", threadNumber);
		else printf("Worker thread #%d successfully completed.\n", threadNumber);

		threadNumber++;		
	}
		
	return 0;
}
