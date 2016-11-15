#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>



int main(int argc, char ** argv) {

  FILE * rd = fopen(argv[1], "r");  // pointer to the file spedified by argv[1]
  FILE * wr = fopen(argv[2], "w");  // pointer to the file being writen, file name specified by argv[2]

  int begin = atoi(argv[3]);        // starting index of the portion of the file to be compressed
  int end = atoi(argv[4]);          // ending index of portion to be compressed

  fseek(rd, begin, SEEK_SET);       // set our reading pointer to the starting index


  char c = getc(rd);  // holds the next character
  while (!isalpha(c)) { c = getc(rd); }  // make sure c is a valid character
  int bad_section = (ftell(rd) > end)? 1 : 0; // flag if there are no valid characters
  char val = c;       // holds the value of previous character
  int num = 1;        // holds the number of times val has occured in sequence


  // while the index of the reading pointer is within the portion to be compressed
  if (!bad_section) {
    do {
      //read next character
      c = fgetc(rd);

      // if the character is not alphabetic, ignore it
      if (!isalpha(c)) { continue; }

      // if we have reached the end of the section, signal to end and write current values
      if (ftell(rd) > end) { c = EOF; }

      // if the current val is the same as the previous, increment and continue
      if (c == val) { num++; }

      // otherwise, write num and val to the compressed file then reset them
      else {  

	if (num == 1) {

	  fputc(val, wr);

	} else if (num == 2) {

	  fputc(val, wr);
	  fputc(val, wr);

	} else {

	  fprintf(wr, "%d", num);
	  fputc(val, wr);

	}

	num = 1;
	val = c;

      }

    } while (c != EOF);
  }

  fclose(rd);
  fclose(wr);

  return 0;
}
