/* The MIT License

   Copyright (c) 2013 Jingtao Liu

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/
#include "parse_file.h"


inline 
int parse_numbers( char *instring, char *delimiter , int *numbers, size_t numbers_len_limit)
{
        int counter = 0;
        char *scratch, *txt ;
        while ((txt = strtok_r(!counter ? instring : NULL, delimiter, &scratch))){
                if (counter>numbers_len_limit ) break ;
                numbers[counter++]=atoi( txt );
        }
        return counter;
}



#ifdef PARSE_FILE_MAIN
int main( int argc, char *argv[])
{
	char *fn=argv[1];
	gzFile fp=gzopen( fn ,"r");
	char *line;
	char *line2;
	int numbers[2] ;
	PARSE_LINE_INT(fp, numbers,2,
		printf("%d\t%d\n", numbers[0], numbers[1] );
	);	
	gzclose(fp);
	return 0;
}


#endif //PARSE_FILE_MAIN
