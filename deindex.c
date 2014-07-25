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
#include "deindex.h"

MD_METHOD_DEFINE(FILEs, FILE ** )  
//prefix in matrix
MD_METHOD_DEFINE(str, char *)

void deindex_hash_load_config(deindex_t *dt );
void deindex_hash_assign(deindex_t * dt);
void deindex_fopen(deindex_t *dt  );
void deindex_fclose(deindex_t *dt  );
void deindex_extra_open(deindex_t * dt) ;
void deindex_extra_close(deindex_t * dt) ;

/*
 *deindex_new : contains config/barcode lib/checking/file opening/barcode hashes control
 * */

deindex_t * deindex_new(  char *fmt , /*formatter*/ 
			char * config_file , /*config file name*/
			char **barlib_files,  /*barcode lib fileS*/
			char *outdir, 	/*outputdirectory optional NULL if not provided*/
			int(*dist_fp)(char * , char *, int), /*distance function pointer*/
			collision_func_t collision_func /*collision handling function pointer , see collision_handle.h */ ) 
{
	int i ;
	deindex_t *dt=calloc(1, sizeof(deindex_t));
	dt->flags=fmt_check(fmt, strlen(fmt),  &(dt->num_bars), &(dt->num_notbars) );
	dt->distance_func=dist_fp ;
	dt->collision_func =collision_func ;
	//output directory on the top
	if(outdir)
		dt->outdir=strdup(outdir);
	else
		dt->outdir=NULL;
	//process cfg
	dt->config=scfgs_load(config_file);

	//process barlibs
	dt->barlib=barlist_load_all(dt->num_bars, barlib_files);
	//verify both config and barlib with num_bars
	scfgs_verify_by_barlib( dt->num_bars, dt->config, dt->barlib  );
	dt->extra_files=dt->config->extra_files;
	fprintf(stderr, "nobarcode config : %d \n", dt->extra_files);
	
	//open matrix gzFiles and str
	dt->out_used=0;
	#ifdef POST_PROCESS
	dt->total_files=((dt->config->used) * dt->num_notbars );
	dt->output_files=malloc(sizeof(char *)*dt->total_files);
	#endif
	if(dt->config->extra_files)
		deindex_extra_open(dt);
	deindex_fopen(dt);
	//init hashes
	dt->hashs=calloc(dt->num_bars, sizeof(kh_strint_t* ));
	for(i=0;i<dt->num_bars; i++){
		dt->hashs[i]=kh_init( strint);
	}
	//clear mismatch flag 
	dt->mismatch=0;	
	deindex_hash_load_config(dt);
	deindex_hash_assign(dt);

		
	return dt;
}

/*
 *deindex_free: free off everything once used by deindex_new
 * */
void deindex_free(deindex_t * dt )
{
        int i;
        if(dt){
                if(dt->flags)
                        free(dt->flags);
                
                deindex_fclose(dt);
                if(dt->hashs){
                        for(i=0;i<dt->num_bars; i++){
                                char *key;
                                int val;
                                kh_foreach(dt->hashs[i] ,
                                        key,
                                        val,
                                        {
                                                if(key)
                                                        free(key);
                                        }
                                );
                                kh_destroy( strint, dt->hashs[i]);
                        }
                        free(dt->hashs);
                }
                barlist_free_all(dt->num_bars, dt->barlib);
		
		if(dt->config->extra_files)
			deindex_extra_close(dt);
                scfgs_free(dt->config);
		#ifdef POST_PROCESS
		if(dt->output_files){
			for(i=0;i<dt->out_used;i++){
				if(dt->output_files[i])
					free( dt->output_files[i] );
			}
			free(dt->output_files);
		}
		#endif
		if(dt->outdir)
			free(dt->outdir);
                free(dt);
		
        }
}
/*
 *deindex_extra_open: if 0 or -1 or more negative values defined handle it
 * */

void deindex_extra_open(deindex_t * dt)
{
	int i,j,k;
	dt->extra=calloc(dt->extra_files  , sizeof(FILE**) );
	for(i=0;i<dt->extra_files; i++){
		dt->extra[i]=calloc(dt->num_notbars, sizeof(FILE *) );
	}	
	for(i=0;i<dt->config->used; i++){
		int code=atoi(dt->config->cfg_array[i]->cols[0] )  ;
		if(code>0 )
			continue ; //exclusive of non extra files option
		ASSERT(  dt->config->cfg_array[i]->used >dt->num_bars ,"error: config file[row %d] has no file names specified\n", i);

		ASSERT(code>=-1, "error: config[row %d] non barcode index[%d] <-1 had not defined yet\n",i ,code );
		char *path=calloc(4096, sizeof(char ) ); 
		if(dt->outdir){
			strcat(path ,dt->outdir);
			strcat(path, "/" );
		}
		for(j=dt->config->cfg_array[i]->used-1;j>=dt->num_bars ; j--){
			strcat(path, dt->config->cfg_array[i]->cols[j] );
			strcat(path, "/" );
		}
		char *cmd=NULL ;
		asprintf(&cmd, "mkdir -p %s", path);
		if(system(cmd)){
			fprintf(stderr, "error: can't create directory %s\n", path);
			exit(-1);
		}	
		char *prefix= dt->config->cfg_array[i]->cols[dt->num_bars];
		for(k=0;k<dt->num_notbars; k++){
			char *filename=NULL;
			if (path[0])
				asprintf(&filename, "%s/%s_R%d.fastq", path, prefix, k+1 );
			else
				asprintf(&filename, "%s_R%d.fastq", prefix, k+1 );
			dt->extra[(-1*code)][k]=fopen( filename, "w" );
			ASSERT(dt->extra[(-1*code)][k] , "error:can't open extra file %s\n", filename );
			dt->output_files[dt->out_used++]=strdup(filename); ;
			if(filename)
				free(filename);
		}
		if(cmd)
			free(cmd);

		free(path);
	}
}
/**
 *deindex_extra_close : free off deindex_extra_open
 */
void deindex_extra_close(deindex_t * dt)
{
	if(!dt->extra)
		return ;
	int i,j;
	for(i=0;i<dt->extra_files; i++){
		for(j=0;j<dt->num_notbars;j++)
			if(dt->extra[i][j])
				fclose( dt->extra[i][j] );
	}	
	free( dt->extra ) ;
}
/*
 *deindex_fopen: open slots for valid barcode config entries, not 0 or -1 
 * */

void deindex_fopen( deindex_t * dt)
{
	//compute dimensions 
	int i,j, k, is_error;
	int dim[dt->num_bars];	
	for(i=0;i<dt->num_bars;i++){
		dim[i]=dt->barlib[i]->used;
	}
	//alloc file
	dt->file=matrix_FILEs_new(dt->num_bars, dim );
	//alloc prefix
	dt->prefix=matrix_str_new(dt->num_bars, dim );
	for(i=0;i<dt->config->used;i++){
		if (atoi(dt->config->cfg_array[i]->cols[0] )<=0  )  /*leave <=0 values to dt->extra*/
			continue ;
		for(j=0;j<dt->num_bars;j++){
			dim[j]=atoi_s(dt->config->cfg_array[i]->cols[j], &is_error);
			//is_error
			ASSERT(!is_error,"error: config file failed at row[%d] col[%d]\n", i,j)
			dim[j]--;
		}
		FILE *** this=matrix_FILEs_get(dt->file, dim );
		*this=malloc(sizeof(FILE*)*dt->num_notbars);
		char **pre=matrix_str_get(dt->prefix, dim);
		//prefix - multiple directory level construction
		char *path=NULL;
		int total_len=0;
		for(k=dt->config->cfg_array[i] ->used-1; k>=j;k-- ){
			total_len+=strlen( dt->config->cfg_array[i]->cols[k] );
			total_len++;		
		}
		path=calloc( total_len, sizeof(char));
		path[0]='\0';
		for(k=dt->config->cfg_array[i] ->used-1; k>j;k-- ){
			strcat(path, dt->config->cfg_array[i]->cols[k] );
			strcat(path, "/"); //dir separator
		}
		*pre=strdup(dt->config->cfg_array[i]->cols[j]);
		for(k=0;k<dt->num_notbars;k++){
			char *outfile=NULL;
			if(path[0]){
				char *cmd;
				if(dt->outdir)
				asprintf(&cmd, "mkdir -p %s/%s", dt->outdir,path);
				else
				asprintf(&cmd, "mkdir -p %s", path);
				//fprintf(stderr, "%s\n", cmd );
				if(system( cmd )){
					ASSERT(0, "error: can't create directory %s\n", path);
				}
				free(cmd);
				if(dt->outdir)
				asprintf(&outfile, "%s/%s/%s_R%d.fastq", dt->outdir, path , (*pre),k+1 );
				else
				asprintf(&outfile, "%s/%s_R%d.fastq", path , (*pre),k+1 );
			}
			else
			if(dt->outdir)
			asprintf(&outfile, "%s/%s_R%d.fastq",dt->outdir, (*pre),k+1 );
			else
			asprintf(&outfile, "%s_R%d.fastq",(*pre),k+1 );

			(*this)[k]=fopen(outfile,"w");
			ASSERT( (*this)[k] !=NULL, "error: can't open file %s for write\n", outfile );
			if(outfile){
				#ifdef POST_PROCESS
				dt->output_files[dt->out_used++]=strdup(outfile);
				#endif
				free(outfile);
			}
		}
		if(path)
			free(path);	
	}
	
}
/*
 * deindex_fclose: closure of deindex_fopen
 * */
void deindex_fclose(deindex_t * dt)
{
	int i,j;
	if(dt->file){
		for(i=0;i<dt->file->size;i++){
			if(dt->file->data[i]){
				for(j=0;j<dt->num_notbars;j++){
					if( dt->file->data[i][j] )
						fclose(  dt->file->data[i][j] );
				}
				free( dt->file->data[i]);	
			}
		}
		matrix_FILEs_free( dt->file);
	}
	/*
	if(dt->noindex){
		for(i=0;i<dt->num_notbars; i++){
			fclose(dt->noindex[i]);
		}	
		free(dt->noindex);		
	}
	*/
	if(dt->prefix){
		for(i=0;i<dt->prefix->size;i++){
			if(dt->prefix->data[i]){
				free( dt->prefix->data[i]);	
			}
		}
		matrix_str_free( dt->prefix);
	}
}
/*
 *kseq_all_new: open multiple files at same time
 *parameters:
		argc: number of files
		argv: file names
 * */
kseq_t **  kseq_all_new( int argc, char *argv[]  )
{
	int i;
	kseq_t **ret=malloc( sizeof(kseq_t *) *argc );
	for(i=0;i<argc;i++){
		gzFile gfp=gzopen(argv[i], "r" );
		ASSERT(gfp!=NULL, "error: can't openfile %s\n", argv[i]);
		ret[i]=kseq_init(gfp);
	}
	return ret;
}
/*
 *kseq_all_free: closure of kseq_all_new
 * */
void kseq_all_free( kseq_t ** seq_all, int argc)
{
	int i;
	if(seq_all){
		for ( i=0;i<argc;i++){
			gzclose( seq_all[i ]->f->f )  ;
			kseq_destroy( seq_all[i] );
		}
		free(seq_all);
	}
}
/*
 *kseq_all_next: iterator multiple files , read one fastq/a entry from each file
 *parameters:
 *	seq_all: type returned from kseq_all_open
 *	argc: number of files opened
 * */
int kseq_all_next( kseq_t ** seq_all, int argc )
{
	int i , l;
	int errors=0;
	for(i=0;i<argc; i++ ){
		if((l=kseq_read(seq_all[i])) <0){
			errors++;
		} 
	}
	if(errors){
		if(errors!=argc ) 
			fprintf(stderr, "Jagged Files detected\n");
	}
	return errors==0 ? 1 : 0 ;
}
/*
 *deindex_hash_load_config: save valid barcodes into hashes by config files indications
 * */
void deindex_hash_load_config(deindex_t *dt )
{
	int i,j, kret,is_num;								
	khiter_t kit;
	for(i=0; i<dt->config->used; i++){						
		int code=atoi(dt->config->cfg_array[i]->cols[0] )  ;
		if(code<=0 )
			continue ; //exclusive of non extra files option
		size_t target[dt->num_bars];						
		for(j=0;j<dt->num_bars;j++){						
			kh_strint_t *hash=dt->hashs[j];
			target[j]=atoi_s(dt->config->cfg_array[i]->cols[j], &is_num );	
			char *barcode=dt->barlib[j]->bars[ target[j] -1 ];
			//int barlen=dt->barlib[i]->barlens[target[j] ] ;
			kit=kh_put(strint, hash, barcode, &kret);
			if(kret==1){
				kh_key(hash, kit) =strdup(barcode );
			}
		}
	}
}
/*
 *deindex_hash_preload: load all barcodes into hashes
 *parameters:
 *	dt: types contains all infomation
 *	bar_reads: all files are barcode file
 *note: 
 *	this function only load every barcode into hashes, it doesn't handle/solve mismatches/collisions
 * */
void deindex_hash_preload (deindex_t * dt, char **bar_reads)
{	

	int i,kret;								
	khiter_t kit;
	//preload bars	
	for(i=0;i<dt->num_bars;i++)
	fprintf(stderr, "mismatch:precompute barcode read %s\n", bar_reads[i] );

	if(bar_reads!=NULL){
		kseq_t** seq_all=kseq_all_new( dt->num_bars, bar_reads);
		while( kseq_all_next(seq_all, dt->num_bars ) ){
			for(i=0;i<dt->num_bars;i++){
				char *barcode=seq_all[i]->seq.s ;
				int barlen=seq_all[i]->seq.l;
				ASSERT( barlen >= ((dt->barlib[i] )->barlength), "error: index read length[%d] < barlib length[%d] [%s]\n", 
					barlen,dt->barlib[i]->barlength,
					barcode  );
				barlen=(dt->barlib[i] )->barlength ;
				barcode[barlen]='\0';
				kit=kh_put(strint, dt->hashs[i], barcode, &kret);
				if(kret==1){
					kh_key(dt->hashs[i], kit) =strdup(barcode );
				}
			}	
		}
		kseq_all_free( seq_all, dt->num_bars);
	}
	//post process hashs with mismatches threshold (include 0 mm)
}
//edit distance with same length string comparision
/*
 *edit_distance: compute the edit distance against two strings, both has the same length
 * */
int edit_distance( char *s1 , char *s2, int size )
{
	int  sz1=size;
	int sz2=size;
	
	 int  i, j;

	int  matrix[size+1][size+1];

	for(i=0;i<=sz2;i++){
		matrix[0][i]=i;
	}
	for(i=0;i<=sz1;i++){
		matrix[i][0]=i;
	}

	for(i=1; i<=sz1; i++){
		for (j=1; j<=sz2;j++){
			matrix[i][j]=MIN3(matrix[i-1][j]+1 , matrix[i][j-1]+1, matrix[i-1][j-1]+ ((s1[i-1]==s2[j-1])?0:1 ) );
		}
	}
	return matrix[sz1][sz2];	
}
//hamming distance option for compare same length strings
/*hamming_distance: compute the hamming distance against two strings with same length
 *
 * */
int hamming_distance( char *a, char *b, int size )
{
	int score, i;
	for(i=0,score=0;i<size;i++){
		if(a[i]!=b[i])
			score++;
	}
	return score;

}
/*
 *deindex_hash_assign: mismatches and collision handling routine
 * */
void deindex_hash_assign(deindex_t * dt)
{
	int i, index;
	int collisions=0;
	int not_collision=0;
	for(i=0;i<dt->num_bars;i++){
		char *bar;
		int val;
		kh_foreach( dt->hashs[i],
				bar,
				val,
				{
					index=(* dt->collision_func )(strlen(bar) ,  bar, dt->barlib[i]->used, dt->barlib[i]->bars, dt->mismatch , dt->distance_func );
					kh_val( dt->hashs[i] , __i)=index;
					//counting howmany collisions if value is -1
					if(index==-1)
						collisions++;
					//counting howmany okay barcodes
					if(index>=0)
						not_collision++;
					/*counting howmany out of mismatches cases
 * 					if(index==-2)
 * 						out_of_mismatches++;
 					*/
				}
		);
	
	}
	
	//reporting  collisions/non collisions
	fprintf(stderr, "mismatch: %d non collision barcodes found and  %d collisons within %d mismatch\n", not_collision , collisions, dt->mismatch);
	for(i=0;i<dt->num_bars;i++){
		fprintf(stderr, "mismatch: hash size %d\n", kh_size(dt->hashs[i] ));
	}
}
/*
 *kseq_all_out: output group of sequence into designated array of file hanlders
 *parameter:
	FILE ** out: array of file handlers
	kseq_t ** notbars: types holding not barcodes reads 
	int num_notbars: control of notbars
	kseq_t ** bars: types holding barcodes, this is for putting barcodes at the description sections
	int num_bars: control of bars
 *
 * */
void kseq_all_out(FILE ** out,  kseq_t ** notbars, int num_notbars, kseq_t ** bars, int num_bars)
{
	int i, bar_cat_size;
	char *bar_cat;
	for(i=0,bar_cat_size=0;i<num_bars;i++)
		bar_cat_size+=bars[i]->seq.l ;
	bar_cat=calloc( bar_cat_size, sizeof(char)*(bar_cat_size+1));
	for(i=0;i<num_bars;i++){
		strcat(bar_cat, bars[i]->seq.s);
	}
	bar_cat[ bar_cat_size ] ='\0';
	for( i=0;i<num_notbars; i++){
		fprintf(out[i] , "@%s %s%s\n%s\n+\n%s\n", notbars[i]->name.s ,notbars[i]->comment.s,bar_cat,notbars[i]->seq.s ,notbars[i]->qual.s);
	}
	free(bar_cat);
}
/*
 *deindex_main: main function start deindexing one groups of files
 *parameters:
 *	dt: type hold all information
 *	argc: number of files with barcode reads and non-barcode reads
 *	argv: filenames with barcode reads and non-barcode reads 	
 * */
void deindex_main( deindex_t * dt , int argc, char **argv   )
{
	//deindex each groups of files
	int i,j,k,  skip, not_in_config=0, is_collision;
	khiter_t kit;
	ASSERT(( dt->num_bars+dt->num_notbars) ==argc, "error: index reads and non indexed reads  are not matched\n"  ) ;
	char **bars=malloc( sizeof(char *)*dt->num_bars);
	char **notbars=malloc(sizeof(char *)*dt->num_notbars);
	for(i=0,j=0,k=0;i<argc;i++){
		if(dt->flags[i])
			bars[j++]=argv[i];
		else
			notbars[k++]=argv[i];
	}
	if(dt->mismatch!=0 ){
		fprintf(stderr, "mismatch: mismatch was set with %d\n", dt->mismatch );
		deindex_hash_preload (dt, bars);
		deindex_hash_assign(dt);
		fprintf(stderr, "mismatch: precomputed, start deindexing\n" );
	}	
	kseq_t ** bar_seqall=kseq_all_new(dt->num_bars, bars );	
	kseq_t ** notbar_seqall=kseq_all_new(dt->num_notbars, notbars );	
	while( kseq_all_next( bar_seqall,dt->num_bars ) &&  
		kseq_all_next( notbar_seqall, dt->num_notbars) ){
		int target[ dt->num_bars];
		skip=0;
		is_collision=0;
		for(i=0;i<dt->num_bars; i++){
			char * barcode=bar_seqall[i]->seq.s ;
			int barlen= dt->barlib[i]->barlength ;
			barcode[barlen]='\0';
			kit=kh_get( strint, dt->hashs[i], barcode);
			if(kit==kh_end( dt->hashs[i] )){  //not have this key, not collision
				skip=1;
				break;
			}
			int val=kh_value(dt->hashs[i] , kit);
			if(val<0){ //key exists but not in mismatches scope 
				if(val==-1)
					is_collision=1;
				skip=1;
				break;
			}
			target[ i] =val;
		}
		if(skip){
			if(is_collision  && dt->extra_files>=2)
				kseq_all_out( dt->extra[1],  notbar_seqall, dt->num_notbars, bar_seqall, dt->num_bars);
			else if(dt->extra_files>=1 )
				kseq_all_out( dt->extra[0],  notbar_seqall, dt->num_notbars, bar_seqall, dt->num_bars);
			continue ;
		}
		FILE ***out=matrix_FILEs_get(dt->file, target);	
		if( !(*out)){
			//kseq_all_out( dt->noindex,  notbar_seqall, dt->num_notbars, bar_seqall, dt->num_bars);
			if(dt->extra_files>=1 )
				kseq_all_out( dt->extra[0],  notbar_seqall, dt->num_notbars, bar_seqall, dt->num_bars);
			not_in_config++;
		}
		else  
		kseq_all_out( *out,  notbar_seqall, dt->num_notbars, bar_seqall, dt->num_bars);
	}
	kseq_all_free( bar_seqall, dt->num_bars  );
	kseq_all_free( notbar_seqall,dt->num_notbars );
	free(bars);
	free(notbars);
}
