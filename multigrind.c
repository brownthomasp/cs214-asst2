#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>



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

  FILE * fp = fopen("200bytes.txt", "w");

  int i = 0;
  char c = 0;
  for (i = 0; i < 200; i++) {
    c = 'A' + rand()%58;
    fputc(c, fp);
  }

  fclose(fp);

  fp = fopen("3200bytes.txt", "w");
  c = 0;
 
  for (i = 0; i < 3200; i++) {
    c = 'A' + rand()%58;
    fputc(c, fp);
  }

  fclose(fp);
  fp = fopen("12800bytes.txt", "w");
  c = 0;

  for (i = 0; i < 12800; i++) {
    c = 'A' + rand()%58;
    fputc(c, fp);
  }

  fclose(fp);
  fp = fopen("102400bytes.txt", "w");
  c = 0;

  for (i = 0; i < 102400; i++) {
    c = 'A' + rand()%58;
    fputc(c, fp);
  }

  fclose(fp);

  double t_total_R = 0;
  double t_total_T = 0;

  struct timeval start, end;

  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("200bytes.txt", "1");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\n\nAverage Times:  compressR_LOLS/compressT_LOLS\n________________________________________________\n");
  printf("Compressing a 200 byte file into\n\t1 file:  %f/%f\n", 0.01*t_total_R, t_total_T);

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("200bytes.txt", "2");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t2 files: %f/%f\n", 0.01*t_total_R, t_total_T);

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("200bytes.txt", "4");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t4 files: %f/%f\n", 0.01*t_total_R, t_total_T);

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("200bytes.txt", "8");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t8 files: %f/%f\n", 0.01*t_total_R, t_total_T);
  printf("Compressing a 3200 byte file into\n");

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("3200bytes.txt", "1");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t1 file:  %f/%f\n", 0.01*t_total_R, t_total_T);

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("3200bytes.txt", "2");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t2 files: %f/%f\n", 0.01*t_total_R, t_total_T);

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("3200bytes.txt", "4");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t4 files: %f/%f\n", 0.01*t_total_R, t_total_T);

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("3200bytes.txt", "8");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t8 files: %f/%f\n", 0.01*t_total_R, t_total_T);
  printf("Compressing a 12800 byte file into\n");

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("12800bytes.txt", "1");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t1 file:  %f/%f\n", 0.01*t_total_R, t_total_T);

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("12800bytes.txt", "2");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t2 files: %f/%f\n", 0.01*t_total_R, t_total_T);

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("12800bytes.txt", "4");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t4 files: %f/%f\n", 0.01*t_total_R, t_total_T);

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("12800bytes.txt", "8");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t8 files: %f/%f\n", 0.01*t_total_R, t_total_T);
  printf("Compressing a 102400 byte file into\n");

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("102400bytes.txt", "1");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t1 file:  %f/%f\n", 0.01*t_total_R, t_total_T);

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("102400bytes.txt", "2");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t2 files: %f/%f\n", 0.01*t_total_R, t_total_T);

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("102400bytes.txt", "4");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t4 files: %f/%f\n", 0.01*t_total_R, t_total_T);

  t_total_R = 0;
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, NULL);
    compressR_LOLS("102400bytes.txt", "8");
    gettimeofday(&end, NULL);
    t_total_R += 1.0*(end.tv_sec - start.tv_sec);
    t_total_R += 1.0E-6*(end.tv_usec - start.tv_usec);
  }

  printf("\t8 files: %f/%f\n\n\n", 0.01*t_total_R, t_total_T);



  return 0;
}
