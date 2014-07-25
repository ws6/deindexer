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
#ifndef _DEINDEX_H
#define _DEINDEX_H

#include "read_cfg.h"
#include "collision_handler.h"
#include "khash.h"
KHASH_MAP_INIT_STR(strint, int)
#include <zlib.h>
#include  "mat.h"
/*
* remove the define of _NOTGZ will support automatically gz format as output.
* but it is significantly slower than the plain text format output
*/
#define _NOTGZ

MD_TYPE_DEFINE(FILEs, FILE ** )  
MD_TYPE_DEFINE(str, char * )  

#define MIN2(a,b) (((a) < (b)) ? (a) : (b))
#define MIN3(x, y, z)  MIN2(MIN2(x,y), z)

typedef struct {
	/*
	* interpreted by -f "rbbr" option
	*/
	int *flags; 
	/*
	* saving the config.cfg information.
	*/
	scfgs_t * config;
	/*
	* barcode reads and non-barcode reads control
	*/
	int num_bars, num_notbars;
	/*
	*one or multiple barcode libs information
	*/
	barlist_t **barlib;
	/*
	*one or multiple dimensional matrix 
	* for quick search the file handler entry using index[es]
	*/
	matrix_FILEs_t *file;  //open multiple dimension array in 1D
	FILE **noindex;
	matrix_str_t * prefix; //strings hold the prefix of mat
	/*
	* one or multiple hash barcode[i]->mat[i]
	*/
	kh_strint_t ** hashs;
	/*
	* mismatches threshold 
	*/
	int mismatch; //can be mismatch for using individual mismatch  if takes as array
	char *outdir;
	int out_used; //control of output_files
	#ifdef POST_PROCESS
	char **output_files;
	int total_files;
	#endif
	//function pointer for using which algorithm
	 
	//which distance function to use hamming/edit distance
	distance_func_t distance_func;
	//which collision function to use 1/2/3
	collision_func_t collision_func ;
	
	//if config file has 0 or -1 , using below storage
	int extra_files; /*number of extra definitions*/
	FILE ***extra; /*accept non collision non index, and collision */

}deindex_t;

/*
* deindex_new:
* init function for saving barcode libs, config file and sanity check the two input files with formatter, "-f"  option
* paramters:
*		char *fmt : formatter e.g "rb"
*		char *config_file : configuration file name
*		char *barlib_files: one or multiple barcode libs
* return: 	deindex_t * 
*/
//deindex_t * deindex_new(  char *fmt , char * config_file ,char **barlib_files, char *outdir ) ;

deindex_t * deindex_new(  char *fmt , 
			char * config_file ,
			char **barlib_files, 
			char *outdir, 
			int(*dist_fp)(char * , char *, int),
			collision_func_t collision_func  ) ;
/*
*deindex_free
*exit function of deindex_new	
*parameters:
*		deindex_t *dt: returned by deindex_new
*return:	void	
*/
void deindex_free(deindex_t * dt ) ;
/*
*deindex_main
*main driver function for deindexing one group of files
*parameters:
*		deindex_t *dt: returned by deindex_new
*		int argc: number of files in one group	, it must contains barcode reads and non barcode rreads
*			the order specified by 'char *fmt ' from deindex_new function
*		char *argv[]: file names of one group
*return:	void
*/
void deindex_main( deindex_t * dt , int argc, char **argv   ) ;
int hamming_distance( char *a, char *b, int size )  ;
int edit_distance( char *a, char *b, int size )  ;
#endif //_DEINDEX_H
