#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>



void compressR_LOLS(char * file, int split) {

  // open target file
  FILE * fp = fopen(file, "r");
  if (!fp) { 
    perror("ERROR ");
    return;
  }

  if (split < 1) { 
    fprintf(stderr, "ERROR:  cannot generate less than 1 file\n");
    return;
  }
  
  // char out[] will be the name of the compressed files
  char out[strlen(file) + 10];
  char file_name[strlen(file) + 6];
  sprintf(file_name, "%s_LOLS", file);
  file_name[strlen(file) - 4] = '_';

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
	return;
      }
    }
  }

  // determine the size of the file
  fseek(fp, -1, SEEK_END);
  long int size = ftell(fp);
  fclose(fp);


  if (split > size) {
    split = size;
    answer = 0;
    printf("There are only %ld characters in %s, so only %ld files will be made.  Do you wish to continue? (y/n): ", size, file, size);
    while(answer != 'y') {
      fscanf(stdin, "%c", &answer);
      if (answer == 'n') {
	return;
      }
    }
  }


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
      execl("./compressR_worker_LOLS", "./compressR_worker_LOLS", file, out, bg, nd, (char*)0);

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
    pid = wait(&status);
    if (status) {
      printf("Child process %d exited with status: %d", pid, status);
    }
    wk_num--;
  }

}



int main(int argc, char ** argv) {

  // check for proper number of arguments
  if (argc < 3) {
    fprintf(stderr, "ERROR: Not enough arguments\n"); 
    return -1;
  }

  compressR_LOLS(argv[1], atoi(argv[2]));

  return 0;

}
