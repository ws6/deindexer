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
#include <stdlib.h>
#include <assert.h>
#include "read_cfg.h"

// top level interpretor
int *  fmt_check( const char *formatter, int fmt_len,int * num_bar , int *num_notbar  )
{
	ASSERT(formatter!=NULL, "error: formmater is empty\n" );
	//int fmt_len=strlen(formatter);
	int i, num_barcode_read=0, num_notbar_read=0, num_total=0;
	//format checking 
	//r:reads  b: barcodes 
	for(i=0;i<fmt_len; i++)
		if ( !(formatter[i]!='r' || formatter[i]!='b' ) ){ 
			fprintf(stderr, "error formatter %s\n" , formatter);
			return NULL;
		}else{
			num_total++;
		}
	//two extra space for returns
	int *ret=calloc(num_total, sizeof(int));  //two values 0/1 , 1 indicates barcode/index read
	for(i=0;i<fmt_len; i++){
		if (formatter[i]=='b'){
			ret[i]=1;
			num_barcode_read++;
		}else if( formatter[i] =='r'){
			num_notbar_read++;
		}	
	}
	assert ( num_total == ( num_barcode_read + num_notbar_read ) );
	*num_bar=num_barcode_read; // number of barcode reads
	*num_notbar=num_notbar_read ;
	return ret;
}




/*
*
* class barlist_t
*
*/
barlist_t * barlist_new()
{
	//int i;
	barlist_t *ret =malloc(sizeof(barlist_t));
	ret->alloc=1024;
	ret->used=0;
	ret->barlength=0;
	ret->bars=malloc(sizeof( char *) * ret->alloc );
	ret->barlens=calloc( ret->alloc , sizeof(int));
	return ret;
}


void barlist_free(barlist_t *bl)
{
	int i;
	if(!bl) return ;
	if(bl->barlens) 
		free(bl->barlens);
	if(bl->bars){
		for(i=0;i<bl->used; i++){
			if(bl->bars[i]) free(   bl->bars[i]);
		}	
		free(bl->bars);
	}
	free(bl);
}
void barlist_add( barlist_t *bl, char * bar, int bar_len)
{
	if((bl->used+1)>=bl->alloc)
	{
		bl->alloc<<=1 ;
		bl->barlens=realloc(bl->barlens, sizeof(int)*bl->alloc );
		bl->bars=realloc(bl->bars, sizeof(char*)*bl->alloc);	
	}
	assert (strlen(bar)== bar_len) ;
	bl->barlens[bl->used]= strlen(bar) ;  
	bl->bars[bl->used]=strdup( bar );
	bl->used++;
}

int barlist_mean_barlen( barlist_t *bl )
{
	int i, sum=0;
	for(i=0;i<bl->used;i++){ 
		sum+=(bl->barlens[i] );
	}
	bl->barlength= sum/bl->used;
	return bl->barlength ; 
}

void barlist_print(barlist_t *bl, FILE *out)
{
	int i;
	for(i=0;i<bl->used;i++){
		fprintf(out, "%s\t%d\n", bl->bars[i], bl->barlens[i]);
	}
	fprintf(out, "Average barcode length: %d\n", bl->barlength);
}

void load_barcode_table(gzFile gfp, barlist_t *barlist )
{
	char *line=NULL;
	char *delimiter="\t";	
	k_FOREACHLINE( gfp, line ,
		{
			if( !line || line[0]=='\0'||line[0]=='#' || line[0]==' ' || line[0] =='\n'  ) continue ; //allow comments 
			int counter = 0;
			char *scratch, *txt ;
			while ((txt = strtok_r(!counter ? line : NULL, delimiter, &scratch))){
				
				if(counter ==0 ){
					barlist_add( barlist, txt , strlen(txt) );	
				}else break;

				counter++;
			}
		}
	);

}

barlist_t ** barlist_load_all(int argc, char **argv)
{
	int i;
	barlist_t ** lists=malloc(sizeof(barlist_t*)*argc);
	for(i=0;i<argc; i++){
		lists[i]=barlist_new();
		char *barlib=argv[i];
		gzFile gfp=gzopen(barlib, "r");
		ASSERT(gfp!=NULL,"error: can't open barlib %d [%s]\n", i, argv[i] );
		load_barcode_table( gfp,  lists[i] );
		gzclose(gfp);
		lists[i]->barlength=barlist_mean_barlen(lists[i] ) ;
	}
	return lists; 
}

void barlist_free_all(int num_barlist, barlist_t ** lists)
{
	int i;
	if(lists){
		for(i=0;i<num_barlist;i++){
			barlist_free( lists[i]);
		}
		free(lists);
	}
}


/*
*
* end of  class barlist_t
*
*/


/*
*
* class scfg_t  a.k.a sample_config_t 
*
*/
scfg_t * scfg_new()
{
	scfg_t * ret;
	ret=malloc(sizeof(scfg_t) );
	ret->used=0;
	ret->alloc=1024;
	ret->cols=malloc( sizeof(char * )*ret->alloc );
	ret->barcode_index=-1;
	return ret;
}

void scfg_free(scfg_t * cfg)
{
	int i ;
	if(cfg){
		if (cfg->cols){ 
			for(i=0;i<cfg->used;i++){
				if(cfg->cols[i])
					free(cfg->cols[i]);
			}
			free(cfg->cols);
		}	
		free(cfg);
	}
}

void scfg_add(scfg_t *cfg, char * col )
{
	if(!col) return ;
	if(cfg->used+1 >= cfg->alloc ){
		cfg->alloc<<=1;
		cfg->cols=realloc(cfg->cols, sizeof(char *)*cfg->alloc);
	}
	cfg->cols[cfg->used]=strdup(col);
	cfg->used++;
}

void scfg_print(scfg_t *cfg, FILE * out)
{
	int i ;
	fprintf(out, "%d", cfg->barcode_index);
	for(i=0;i<cfg->used;i++ ){
		fprintf(out, "\t%s", cfg->cols[i]);
	}
	fprintf(out, "\n");
}
/*
*
*  end of class scfg_t  a.k.a sample_config_t 
*
*/


/*
*
* class scfgs_t  a.k.a sample_configs_t 
*
*/
scfgs_t * scfgs_new()
{
	scfgs_t * ret;
	ret=malloc(sizeof(scfg_t) );
	ret->extra_files=0;
	ret->used=0;
	ret->alloc=1024;
	ret->cfg_array=malloc( sizeof(scfg_t * )*ret->alloc );
	return ret;
}

void scfgs_free( scfgs_t * ret )
{
	int i;
	if(ret){
		if(ret->cfg_array){
			for(i=0;i<ret->used; i++){
				if(ret->cfg_array[i])
					scfg_free( ret->cfg_array[i] );
			}
			free( ret->cfg_array );
		}
		free(ret);
	}
}

void scfgs_add( scfgs_t * cfgs , scfg_t * in)
{
	if(cfgs->used+1 >=cfgs->alloc){
		cfgs->alloc<<=1;
		cfgs->cfg_array=realloc(cfgs->cfg_array, sizeof( scfgs_t *) * cfgs->alloc);
	}
	cfgs->cfg_array[cfgs->used]=in;
	cfgs->used++;
}

void load_cfg_table(gzFile gfp, scfgs_t * cfgs )
{
	char *line=NULL;
	char *delimiter="\t";
	k_FOREACHLINE( gfp, line,
		{
			if( !line || line[0]=='\0' ||  line[0]=='#' || line[0]==' ' || line[0]=='\n'){ 
				continue ; //allow comments 
			}
			int counter = 0;
			char *scratch, *txt ;
			scfg_t * cfg=scfg_new();
			while ((txt = strtok_r(!counter ? line : NULL, delimiter, &scratch))){
				scfg_add( cfg, txt );
				counter++;
			}
			scfgs_add(cfgs, cfg );
		}
	);

}

void scfgs_print(scfgs_t * cfgs, FILE *out )
{
	int i;
	for(i=0; i<cfgs->used;i++){
		scfg_print( cfgs->cfg_array[i] ,  out  );
	}
}

/*
*
*  end of class scfgs_t  a.k.a sample_configs_t 
*
*/


/*
*
*  class barcode2config  a.k.a sample_configs_t 
*
*/

int atoi_s(char * s, int *is_error)
{
	*is_error=0;
	char *p;
	p=s;
	for( p=s; *p!='\0'; p++ ){
		if (*p<'0' || *p>'9' ){
			*is_error=1;
			return 0;
		}
	}
	return atoi(s);
}



scfgs_t *  config_load(char *filename)
{
	scfgs_t * cfgs= scfgs_new();
	gzFile gfp=gzopen(filename, "r");
	ASSERT(gzopen!=NULL, "error: can't open file %s\n", filename);
	load_cfg_table(gfp, cfgs );
	gzclose(gfp);
	return cfgs ;
}

scfgs_t *  scfgs_load(char *filename)
{
	ASSERT( filename!=NULL , "error: config files empty\n");
	return  config_load(filename); //create alias
}

//sanity check on if configs  followed by barlib
int scfgs_verify_by_barlib( int num_bars, scfgs_t *config,  barlist_t ** barlib  )
{
	int i, j, is_error,bar_index;
	for(i=0;i<config->used;i++){
		int first_value=atoi( config->cfg_array[i]->cols[0]);
		if(first_value<=0){
			//first value is 0 stands for  non_collision_no_indexed reads layout
			//first value is -1 stands for collision reads layout
			//if multiple indexes , padding 0 or -1 to match the other columns definitions
			//and more definitions 
			switch (first_value){
				case 0: 
					fprintf(stderr, "config: non collision non indexed read defined\n");
					break;
				case -1:
					fprintf(stderr, "config: collision reads defined\n");
					break;
				default:
					fprintf(stderr, "config: unknown defintion %d (row[%d]) , exit.\n", first_value, i);
					exit(-1);
					break;
			}
			config->extra_files++; //saving extra number of files need to open later
		}
		else
		for(j=0;j<num_bars;j++){
			bar_index=atoi_s(config->cfg_array[i]->cols[j], &is_error );
			ASSERT(!is_error, 
				"error: config file at row[%d] column[%d] shoulde be a number, got[%s]\n" ,
				i,j,config->cfg_array[i]->cols[j]
			);
			ASSERT( bar_index >=1 && bar_index <=barlib[j]->used,
				"error: config file at row[%d] column[%d] index[%d] outof range[1-%d] \n" ,
				i,j,bar_index, bar_index <=barlib[j]->used 
			);
		}
		ASSERT(config->cfg_array[i]->used >num_bars, "error: config files[%d row] need at least one more column for file names\n",i );
	}
	return 1;
}


/*
*
* end of  class barcode2config  a.k.a sample_configs_t 
*
*/


#ifdef CFG_MAIN



int main( int argc, char *argv[]){
}

#endif //CFG_MAIN


