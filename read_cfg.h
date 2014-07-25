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
#ifndef _READ_CFG_H
#define _READ_CFG_H
#include "parse_file.h"
//error handle macro

#define ASSERT( stmt , ... ) \
	{			\
		if(!(stmt)){	\
			fprintf(stderr, __VA_ARGS__ );	\
			exit(-1);	\
		}	\
	};

typedef struct{
	int *barlens; 	//individual bar code can have difference length
	int barlength; 	//average bar code length , most cases are this. 
	int alloc;	//real memory space got allocated
	int used;	//actual barcodes loaded 
	char **bars;
}barlist_t ;

enum SAMPLE_CFG_DESC{
SAMPLE_NAME,
CLIENT_NAME,
DESC
};
typedef struct{
	int barcode_index;
	char** cols; // from sample name, client name, descriptions to be extends
	int used;
	int alloc;
}sample_cfg_t;
typedef sample_cfg_t scfg_t;

typedef struct{
	sample_cfg_t  ** cfg_array;
	int used;  /*used entry of cfg_array*/
	int alloc; /*allocated size of cfg_array, private*/
	/*track number of extra files for non barcode configs
		e.g. non indexed read, or collision reads*/
	int extra_files;  
}sample_cfgs_t;

typedef sample_cfgs_t scfgs_t;

int *  fmt_check( const char *formatter, int fmt_len,int * num_bar , int *num_notbar  ); 
barlist_t ** barlist_load_all(int argc, char **argv) ;
void barlist_free_all(int num_barlist, barlist_t ** lists) ;
scfgs_t *  scfgs_load(char *filename);
void scfgs_free( scfgs_t * ret );
int scfgs_verify_by_barlib( int num_bars, scfgs_t *config, barlist_t **barlib  ) ;
int atoi_s(char * s, int *is_error);
#endif  //_READ_CFG_H
