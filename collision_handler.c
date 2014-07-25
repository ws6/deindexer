#include "collision_handler.h"
static
inline
int collision_search( int bar_len, char *bar, int nbars, char **bars, int mismatch, int(*dist_ptr)(char *,char *, int ))
{
	int i, score, min=1000000, min_index=0, min_found;
	for(i=0,min_found=0;i<nbars;i++ ){
		score=(*dist_ptr)( bar, bars[i], bar_len);
		if(score<min){
			min=score;
			min_index=i;
			min_found=1;	
		}else if(score==min){
			min_found++;
		}
	}
	if(min<=mismatch){
		//collisions
		return   min_found == 1 ? min_index: -1 ;	
	}else{
		return -2; // beyond mismatch threshold
	}
	return -2;
}

int collision1( int bar_len, char *bar, int nbars, char **bars, int mismatch, int(*dist_ptr)(char *,char *, int ) )
{
	int i, score, min=1000000, min_index=0, min_found;
	for(i=0,min_found=0;i<nbars;i++ ){
		score=(*dist_ptr)( bar, bars[i], bar_len);
		if(score<min){
			min=score;
			min_index=i;
		}
		if(score<=mismatch){
			min_found++;
		}
	}
	if(min_found>1)
		return -1 ; //collision
	if (min<=mismatch )
		return min_index;  //really found 
	else
		return -2 ;// out of mismatch threshold
}

int collision2( int bar_len, char *bar, int nbars, char **bars, int mismatch, int(*dist_ptr)(char *,char *, int ) )
{
	return collision_search (bar_len, bar, nbars, bars, mismatch, dist_ptr );
}

static
inline
int _cmp( const void *pa, const void *pb)
{
	const int (*a)[2] =pa;
	const int (*b)[2] =pb;
	return (*a)[1] - (*b)[1] ;
}

int collision3( int bar_len, char *bar, int nbars, char **bars, int mismatch, int(*dist_ptr)(char *,char *, int )  ) 
{
	int distance_diff=COLLISION3_MAX_DIFF;  //recompile this value , a.k.a gaps of distance difference
	int i, score, min=1000000, min_index=0, min_found;
	int index_value[nbars][2]; //temp solution need optimize
	for(i=0,min_found=0;i<nbars;i++ ){
		score=(*dist_ptr)( bar, bars[i], bar_len);
		index_value[i][0]=i; 	
		index_value[i][1]=score; 	
	}
	//sort index_value pair  on the  second column
	qsort(index_value, nbars, sizeof(index_value[0]), _cmp );
	int collision=0;
	for(i=1;i<nbars;i++){
		if( index_value[i][1] >mismatch)
			break;
		if ( (index_value[i][1] - index_value[i-1][1])<distance_diff ) {
			//this is a collision
			collision=1; break;		
		}	
	}
	if(collision)
		return -1; //collision
	if( index_value[0][1] <=mismatch )
		return index_value[0][0];
	else
		return -2 ; //out of mismatch threshold
	
}


