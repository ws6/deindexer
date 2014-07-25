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
int main(int argc, char *argv [] )
{
	char *fmt=NULL, *config=NULL ,*outdir=NULL;
	char **barlibs=malloc(sizeof( char *)*1024);
	int num_bars=0, mismatch=0;
	int i, j, c, tuple ,groups;
	int distance_function=0; //default hamming other value will be edit distance
	int collision_function=0; //default collision 1 , accepatable values 1 2 3
	#ifdef POST_PROCESS
	int post_gzip=0;//not post gzip by default
	#endif
	while( (c=getopt(argc, argv, "f:b:c:m:hzo:D:C:")) !=-1 ){
		switch(c){
			case 'f':/*formmater, can't be empty*/
				fmt=strdup(optarg);
				break;
			case 'c':/*config file, can't be empty*/
				config=strdup(optarg);
				break;
			case 'b':/*barlib, one or multiple*/
				assert(num_bars<1023);
				barlibs[num_bars++]=strdup(optarg);
				break;
			case 'm':/*mismatch threshold*/
				mismatch=atoi( optarg);
				break;
			case 'o':/*all output directory */
				outdir=strdup(optarg);
				break;
			#ifdef POST_PROCESS
			case 'z':/*need to gzip after deindexing*/
				post_gzip=1;
				break;
			case 'D': /*distance function , 0=hamming, otherwise=edit distance*/
				distance_function=atoi(optarg);
				break;		
			case 'C': /*collision function ,  only accept 1, 2, 3 now */
				collision_function=atoi(optarg);
				if (collision_function <1 || collision_function >3){
					fprintf(stderr,"Collision function only accept 1,2 or 3\n");
					exit(-1);
				}
				break;		
			#endif
			case 'h':
			default :
				fprintf(stderr, 
					"./deindexer -f 'rb' -c config.cfg -b ILMN_barlist.txt lane1_R1*gz lane1_R2*gz\n"
					"./deindexer -f 'rbr' -c config.cfg -b ILMN_barlist.txt lane1_R1*gz lane1_R2*gz lane1_R3*gz\n"
					"./deindexer -f 'rbbr' -m 1 -c config.cfg -b ILMN_dualN700t.txt  -b ILMN_dualN500t.txt lane1_R1*gz lane1_R2*gz lane1_R3*gz lane1_R4*gz \n"
				);
				exit(-1);
				break;
		}
	}
	/*select a collision function*/
	collision_func_t which_collision=NULL;
	switch(collision_function){
		case 1:
			which_collision=collision1;
			fprintf(stderr, "using collision function 1\n");
			break;
		case 3:
			fprintf(stderr, "using collision function 3\n");
			which_collision=collision3;
			break;
		case 2:
		default:
			fprintf(stderr, "using collision function 2\n");
			which_collision=collision2;
			break;
	}
	/*select a distance function*/
	distance_func_t which_distance=NULL;	
	switch (distance_function ){
		case 0:
			which_distance=hamming_distance;
			fprintf(stderr, "using Hamming Distance function\n");
			break;
		default:
			which_distance=edit_distance;
			fprintf(stderr, "using Edit Distance function\n");
			break;
	}
		
	deindex_t * dt=deindex_new( fmt, config, barlibs, outdir, which_distance , which_collision  );
	dt->mismatch=mismatch ;
	
	/*
	*sannity checking of groups of files. it needs to  follow the fomatter inputs
	*
	*/
	ASSERT(  (argc-optind ) % (dt->num_bars+dt->num_notbars)  ==0 , 
		"error: number of non options parameters should be divided by %d [%d]\n",
		(dt->num_bars+dt->num_notbars) , (argc-optind ) % (dt->num_bars+dt->num_notbars)   );
	//grouping all non optional parameters
	tuple=(dt->num_bars+dt->num_notbars) ;
	groups=(argc-optind ) / tuple ;		
	for(i=0; i<groups; i++){
		fprintf(stderr, "group :%d\n",i);
		char **reads=malloc(sizeof( char *)*tuple );	
		for(j=0;j<tuple; j++){
			reads[j]= argv [ optind  +j*groups+i ] ;
			fprintf(stderr, "%d->%s\n", j, reads[j]);
		}
		//call one group
		deindex_main(dt, tuple ,reads );
		free(reads);
	}
	//exit of deindex_new
	#ifdef POST_PROCESS
	if(post_gzip){
		fprintf(stderr, "deindexing finished, start gzip.[overwritten mode] \n");
		int zipped=0;
		for(i=0;i<dt->total_files;i++ ){
			char *cmd;
			asprintf(&cmd, "gzip -q -f  %s", dt->output_files[i]);
			if(system(cmd)){
				fprintf(stderr, "error: can not gzip %s\n",  dt->output_files[i] );
			}
			else{
				zipped++;	
			}
			if(cmd)
				free(cmd);
		}	
		fprintf(stderr, "%d/%d file have been gzippped\n", zipped, dt->total_files );
	}
	#endif

	deindex_free(dt);
	//collect garbage generated by command line
	if(outdir)
		free(outdir);
	if(fmt)
		free(fmt);
	if(config)
		free(config);
	for(i=0;i<num_bars;i++){
		if(barlibs[i])
			free(barlibs[i]);
	}
	free(barlibs);
	//exit 
	//purposely leave no return statment for gcc to auto fit a value as return successful
}
