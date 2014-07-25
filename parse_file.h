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
#ifndef _PARSE_FILE_H
#define _PARSE_FILE_H

#define _GNU_SOURCE
#include <stdio.h> //free 	
#include <stdlib.h>
#include <assert.h>

//ForEachLine( FILE *fp , char *line, ... )
#define  FOREACHLINE( fp ,line, ... )	{ 				\
	line=NULL;							\
	ssize_t read;							\
	size_t len=0;							\
	if(fp==NULL)							\
		exit(EXIT_FAILURE);					\
	while((read=getline(&line, &len,fp))!=-1){			\
		{__VA_ARGS__}						\
	}								\
	if(line) free(line) ;						\
	}

#include "kseq.h"
#include <zlib.h>
#define BUF_SIZE 4096
//KSTREAM_INIT(gzFile, gzread, BUF_SIZE)
KSEQ_INIT(gzFile, gzread)
#define  k_FOREACHLINE(  gfp, line , ...)	{\
	kstream_t  *ks;					\
	ks=ks_init(gfp);				\
	kstring_t *s=calloc(1, sizeof(kstring_t));	\
	int dret;					\
	while(ks_getuntil(ks, '\n', s, &dret)>=0){	\
		line=s->s;				\
		{__VA_ARGS__}				\
	}						\
	ks_destroy(ks);					\
	if(s->s)free(s->s); 				\
	if (s)free(s);					\
}
#define  k_FOREACH_SEQ( gfp,  ...  )	{\
	int  l;				\
	kseq_t *seq;			\
	seq = kseq_init(gfp);		\
	while((l = kseq_read(seq))>=0){	\
		{  __VA_ARGS__ }	\
	}				\
	kseq_destroy(seq);		\
}




#define MAX_FIELDS 1000
int parse_numbers( char *instring, char *delimiter , int *numbers, size_t numbers_len_limit);
#define  PARSE_LINE_INT(fp , numbers, expect_fields, ... ) {\
		char *delimiter="\t";	\
		char *line ;\
		int counts;\
		k_FOREACHLINE(fp, line,\
			{\
				counts=parse_numbers( line, delimiter, numbers,  MAX_FIELDS );	\
				if(counts <expect_fields) continue ;\
				{__VA_ARGS__}\
			}\
		);	\
	}


#endif //_PARSE_FILE_H
