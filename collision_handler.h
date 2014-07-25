#ifndef _COLLISION_HAND_
#define _COLLISION_HAND_
#include <stdio.h>
#include <stdlib.h>

/*gaps threshold for collision3 routine*/
#define COLLISION3_MAX_DIFF 2

typedef int (*distance_func_t)(char *, char *, int);	
typedef  int (*collision_func_t )( int , char *, int , char **, int , distance_func_t ) ;

/*collision1 : given a mismatch threshold, if multiple bacodes being found below this thresh, collision found*/
int collision1( int bar_len, char *bar, int nbars, char **bars, int mismatch, int(*dist_ptr)(char *,char *, int ) ) ;
/*collision2 : given a mismatch threshold, if more than one  min values found  and this min value is less than threshold 
 * collision found*/
int collision2( int bar_len, char *bar, int nbars, char **bars, int mismatch, int(*dist_ptr)(char *,char *, int ) ) ;
/*collision3 : given a mismatch threshold, and given a gaps value (mismatches gaps )
 * if multiple values are less than mismatch thresh 
 * AND the gaps between first and second values are less than 'COLLISION3_MAX_DIFF', collision found*/
int collision3( int bar_len, char *bar, int nbars, char **bars, int mismatch, int(*dist_ptr)(char *,char *, int ) ) ;

#endif //_COLLISION_HAND_
