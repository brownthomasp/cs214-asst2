#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>


int main(int argc, char ** argv) {

  // check for proper number of arguments
  if (argc < 3) {
    fprintf(stderr, "ERROR: Not enough arguments\n"); 
    return -1;
  }

  // open target file
  FILE * fp = fopen(argv[1], "r");
  if (!fp) { 
    perror("ERROR ");
    return -1;
  }

  int split = atoi(argv[2]);   // number of child processes to make
  if (split < 1) { 
    fprintf(stderr, "ERROR: argv[1] must be at least 1\n");
    return -1;
  }
  
  // char out[] will be the name of the compressed files
  char out[strlen(argv[1]) + 10];
  char file_name[strlen(argv[1]) + 6];
  sprintf(file_name, "%s_LOLS", argv[1]);
  file_name[strlen(argv[1]) - 4] = '_';

  if (split > 1) {
    sprintf(out,"%s%d", file_name, 0);
  } else {
    sprintf(out, "%s", file_name);
  }

  // check for an already existing compressed version of the file that would be writen over
  char answer = 0;
  if (access(out, F_OK) != -1) {
    printf("A compressed version of this file already exists, do you wish to ovewrrite it? (y/n):  ");

    while (answer != 'y') {

      fscanf(stdin, "%c", &answer);

      if (answer == 'n') {
	return 0;
      }
    }
  }

  // determine the size of the file
  fseek(fp, 0, SEEK_END);
  long int size = ftell(fp);


  long int begin = 0;           // the begining of the section to be given to a worker
  char bg[20];                  // a string to store begin
  long int end = size/split;    // end of the section
  char nd[20];                  // a string to store end
  long int temp;                // storage variable
  pid_t workers[split];         // to store the childrens' pids
  int wk_num = 0;               // stores worker number
  char c = 0;                   // a storage variable for testing

  int roundup = size - end*split;  // to keep track of how many times we must round up
  if (roundup) { end++; roundup--; }

  while (begin < size) {
    fseek(fp, end-1, SEEK_SET);
    // make sure end is not in the middle of a compressable sequence
    c = fgetc(fp);
    while (fgetc(fp) == c && c != EOF) { end++; }

    if (c == EOF) { break; }

    if (wk_num == split-1) {
      end = size;
    }

    // format the name of the compressed file
    if (split > 1) {
      sprintf(out, "%s%d", file_name, wk_num);
    }
    
    workers[wk_num] = fork();
    if (!workers[wk_num]) {
      sprintf(bg, "%ld", begin);
      sprintf(nd, "%ld", end);
      execl("./compressR_worker_LOLS", "./compressR_worker_LOLS", argv[1], out, bg, nd, (char*)0);

      fprintf(stderr, "ERROR: Worker %d failed to exec\n", wk_num);
      return -1;
    }

    temp = end;
    end += size/split;
    begin = temp;

    if (roundup) { end++; roundup--; }
    wk_num++;

  }

  // Report if the arrangement or number of characters in the files results in fewer files than requested
  if (wk_num < split) {
    printf("Due to the arrangement and number of characters in %s, only %d compressed files where made\n", argv[1], wk_num + 1);
  }

  fclose(fp);

  int status;
  pid_t pid;
  while (wk_num) {
    pid =  wait(&status);
    printf("Child process %d has finished\n", pid);
    wk_num--;

  }

  return 0;

}
