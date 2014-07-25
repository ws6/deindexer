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
/*
* mat.h
* allocate multiple dimensional array into 1D
* using generic function macro
* with new/free/get functions
* created by Jingtao Liu
* 01/22/2013
*/
#ifndef _MATRIX_H
#define _MATRIX_H
#include <stdlib.h>
#include <stdio.h>

#define MD_TYPE_DEFINE(name, data_t )	\
	typedef struct {	\
		int  dims;	\
		int  *dim_define;\
		int *pre; 	\
		data_t *data;	\
		int  size ;	\
	}matrix_##name##_t ;

#define MD_METHOD_DEFINE(name, data_t)	\
	void matrix_##name##_pre_compute(matrix_##name##_t *mat)\
	{\
		int i,t;\
		for(i=0;i<mat->dims;i++) mat->pre[i]=1;\
		for(i=1,t=1;i<mat->dims; i++){\
			t*=mat->dim_define[i-1];\
			mat->pre[i]=t;\
		}\
	}\
	matrix_##name##_t * matrix_##name##_new( int  dims, int  *dim_define)\
	{\
		int  i, size=1;\
		for(i=0;  i<dims; i++)\
			size*=dim_define[i];\
		matrix_##name##_t *ret=malloc(  sizeof(matrix_##name##_t));\
		ret->size=size; \
		ret->dims=dims;\
		ret->data=calloc(ret->size, sizeof(data_t) );\
		ret->pre=calloc( ret->dims, sizeof(int));\
		ret->dim_define=malloc(sizeof(int )*dims ) ;\
		for(i=0;i<dims;i++)\
			ret->dim_define[i]=dim_define[i] ;\
		matrix_##name##_pre_compute(ret);\
		return ret;\
	}\
	void matrix_##name##_free (matrix_##name##_t * mat)\
	{\
	  if (mat)\
	    {\
	      if (mat->data)\
		free (mat->data);\
	      if (mat->dim_define)\
		free (mat->dim_define);\
	      if (mat->pre)\
		free (mat->pre);\
	      free (mat);\
	    }\
	}\
	static inline int matrix_##name##_offset(matrix_##name##_t *mat, int *target) \
	{\
		int i, index;\
		for(i=0,index=0;i<mat->dims;i++)\
			index+=(target[i]*mat->pre[i]);\
		return index;\
	}\
	data_t *matrix_##name##_get(matrix_##name##_t *mat, int *target )\
	{\
		int offset=matrix_##name##_offset(mat, target);\
		if(offset >=0 && offset<mat->size )     \
			return mat->data+offset;\
		else\
			return NULL;\
	}
#define matrix_new(name,dims, dim) matrix_##name##_new(dims, dim )
#define matrix_free(name, m) matrix_##name##_free( m )
#define matrix_get(name, m, target) matrix_##name##_get( m, target )
#define matrix_offset(name, m, target) matrix_##name##_offset( m, target )


#endif //_MATRIX_H

#ifdef _MATRIX_MAIN
//compile : 
//cat mat.h | gcc -xc  -D_MATRIX_MAIN  -
//usage
MD_TYPE_DEFINE(int, int ) ;
MD_METHOD_DEFINE(int, int) 
int main(){
        int dims=1;
        int d[1]={10 };
        int target[1]={7};
        int *dim=d;
        matrix_int_t *m=matrix_new( int,dims, dim);
        *matrix_get(int, m,  target)=17441;
        printf("index %d, value %d\n", matrix_offset(int,m,  target),   *matrix_get(int, m,  target) );
        matrix_free(int,m);

}
#endif //_MATRIX_MAIN
