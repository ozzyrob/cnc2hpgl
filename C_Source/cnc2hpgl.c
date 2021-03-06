/**
  * cnc2hpgl - convert FlatCAM cnc to hpgl
  *
  * Written and maintained by Robert Murphy [ robert.murphy@gmx.com ]
  *
  * The purpose of this program is to convert FlatCAM cnc to hpgl
  *
  * Code logic based on hpgl-distiller by Paul L Daniels
  * 
  * Original version written: Tuesday 22 Nov 2016
  *
  *
  * Process description:
  *
  * cnc2hpgl -i src_filenam -o dest_filename
  *
**/
  
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "cnc2hpgl.h"

#define CNC2_VERSION "0.0.1"
							 					   	               
//* START FUNCTIONS
/**
  * CNC2_show_version
  *
  * Display the current convgerber version
  */
int CNC2_show_version( struct CNC2_glb *glb ) {
	fprintf(stderr,"%s\n", CNC2_VERSION);
	return 0;
} //* END_FUNC: CNC2_show_version

/**
  * CNC2_show_help
  *
  * Display the help data for this program
  */
int CNC2_show_help( struct CNC2_glb *glb ) {
	CNC2_show_version(glb);
	fprintf(stderr,"%s\n", CNC2_HELP);

	return 0;

} //* END_FUNC: CNC2_show_help                  

/**
  * CNC2_init
  *
  * Initializes any variables or such as required by
  * the program.
  */
int CNC2_init( struct CNC2_glb *glb )
{
	glb->status = 0;
	glb->plain = false;
	glb->keep_tmp = false;
	glb->machine_units = MACHINE_UNITS_ERR;
	glb->distance_mode = DISTANCE_MODE_ERR;
	glb->src_filename = NULL;
	glb->dest_filename = NULL;
	glb->temp_filename = "cnc2hpgl.tmp";
	return 0;
} //* END_FUNC: CNC2_init

/**
  * CNC2_parse_parameters
  *
  * Parses the command line parameters and sets the
  * various convgerber settings accordingly
  */
int CNC2_parse_parameters( int argc, char **argv, struct CNC2_glb *glb )
{

	char c;

	do {
		c = getopt(argc, argv, "i:o:pkvh");
		switch (c) { 
			case EOF: /* finished */
				break;

			case 'i':
				glb->src_filename = strdup(optarg);
				break;

			case 'o':
				glb->dest_filename = strdup(optarg);
				break;
			
			case 'p' :
				glb->plain = true;
				break;

			case 'k' :
				glb->keep_tmp = true;
				break;

			case 'h':
				CNC2_show_help(glb);
				exit(1);
				break;

			case 'v':
				CNC2_show_version(glb);
				exit(1);
				break;

			case '?':
				break;

			default:
				fprintf(stderr, "internal error with getopt\n");
				exit(1);
				
		} //* END: switch (c) 
	} while (c != EOF); /* do */
	return 0;
} //* END_FUNC: CNC2_parse_parameters

/**
 * CNC_sanitise_src
 * remove all whitespace except NL from file
 * when done file pointer is reset to start
 * 
 */
 int CNC2_sanitise_src( FILE* f_src, FILE* f_dest)
 {
				int char_read;
	 
	 			while (  ( char_read= fgetc(f_src) ) != EOF ) {	
			
					switch (char_read) {
					
					case ACSII_BS:
					case ACSII_HT:
					case ACSII_VT:
					case ACSII_FF:
					case ACSII_CR:
					case ACSII_SPACE:
					case ACSII_DEL:
								break;
					
					default:
								fputc( char_read, f_dest);
			} //* END: switch (char_read)
	} //* END: while (  ( char_read= fgetc(f_src) ) != EOF )
	
	rewind(f_dest);	
	return 0;
} //* END_FUNC: CNC2_sanitise_src	
/**
  * CNC2_get_distance_mode
  * get & set path control mode
  * when done file pointer is reset to start
  * 
  */
int CNC2_get_distance_mode ( FILE* f_src, struct CNC2_glb *glb ) {
	
	char buff[32];
	while ( fgets(buff, 32, f_src) != NULL ) {
		
		
		//* Check for G90 absolute mode
		if ( strstr(buff, "G90") != NULL ) {
			printf("G90\tAbsolute co-ords\n");
			glb->distance_mode = DISTANCE_MODE_ABS; 
			rewind (f_src);
			return true;
		}
		//* Check for G91 incremental mode
		else if ( strstr(buff, "G91") != NULL ){
			printf("G91\tIncremental co-ords\n");
			glb->distance_mode = DISTANCE_MODE_INC;
			rewind (f_src);
			return true;
		}
	} //* END: while ( fgets(buff, 80, f_src) != NULL )		

	//* No mode found
	glb->distance_mode = DISTANCE_MODE_ERR;
	rewind (f_src);
	return DISTANCE_MODE_ERR;
} //* END_FUNC: CNC2_get_distance_mode

/**
  * CNC_get_machine_units
  * get & set machine units
  * when done file pointer is reset to start
  * 
  */
int CNC2_get_machine_units ( FILE* f_src, struct CNC2_glb *glb ) {
	
	char buff[32];
	
	while ( fgets(buff, 32, f_src) != NULL ) {
		
		
		//* Check for G20 imperial (inch)
		if ( strstr(buff, "G20") != NULL ) {
			printf("G20\tImperial Units\n");
			glb->machine_units = MACHINE_UNITS_INCH; 
			rewind (f_src);
			return true;
		}
		//* Check for G21 metric (mm)
		else if ( strstr(buff, "G21") != NULL ){
			printf("G21\tMetric units\n");
			glb->machine_units = MACHINE_UNITS_MM;
			rewind (f_src);
			return true;
		}
	} //* END: while ( fgets(buff, 80, f_src) != NULL )		

	//* No mode found
	glb->machine_units=MACHINE_UNITS_ERR;
	rewind (f_src);
	return MACHINE_UNITS_ERR;
} //* END_FUNC: CNC2_get_machine_units

/**
  * CNC2_write_hpgl
  * copy G00 & G01 moves that have x & y
  * when done file pointer is reset to start
  * 
  */
int CNC2_write_hpgl ( FILE* f_src, FILE* f_dest, float mac_units) {
	
	float tmp_axis;
	float x_axis;
	float y_axis;
	char buff[32];
	char prev_line[32];
	char curr_line[32];
	
	char *line_to_process = buff;
	char *x_split;
	char *y_split;

	

	while ( fgets(line_to_process, 32, f_src) != NULL ) {
		
//*---------------------------------------------------------------------		
		//* Check for Z  axis (G00Z or G01Z)
		//* Not required for plotter
		//* as G00 (rapid) is equal to PU movement
		if ( strstr(line_to_process, "Z") != NULL ) {
			
			continue;
		} //* END: if ( strstr(line_to_process, "Z") != NULL )
		
		//* Linear moves (G01)
		//* Convert X & Y values to plotter values
		//* Pen Up instruction
		else if ( strstr(line_to_process, "G01") != NULL ) {

				//* Get location in string of X & Y characters
				//* and replace them with ACSII_NULL character
				x_split = strpbrk(line_to_process, "X");
				y_split = strpbrk(line_to_process, "Y");  
				*x_split = ACSII_NULL;
				*y_split = ACSII_NULL;

				x_split++;	//* x_split now points to char after ASCII_NUL
				y_split++;  //* y_split now points to char after ASCII_NULL

				//* Convert Y value string to float, multiply by machine units
				//* and round it
				tmp_axis = atof(x_split); 
				x_axis = roundf(tmp_axis * mac_units);
 
				//* Convert X value string to float, multiply by machine units
				//* and round it
			    tmp_axis = atof(y_split);
			    y_axis = roundf(tmp_axis * mac_units);

				//* Uncomment to printf below to print to stdout
				//*printf("PD %i,%i\n", (int)x_axis, (int)y_axis);

				//* Format current line
				//* For Pen Down 

				sprintf(curr_line, "PD %i,%i;\n", (int)x_axis, (int)y_axis);
				
				//* Catch consectutive duplicate lines
				//* If curr_line and prev_line are not the same
				//* write curr_line to file & copy it to prev_line
				 if ( strcmp(prev_line, curr_line) != 0 ) {		
					fputs(curr_line, f_dest);
					strcpy(prev_line, curr_line);
				}

			} //* END: else if ( strstr(line_to_process, "G01") != NULL )			

		//* Rapids (G00)
		//* Convert X & Y values to plotter values
		//* Pen Up instruction		
		else if ( strstr(line_to_process, "G00") != NULL ) {


				//* Get location in string of X & Y characters
				//* and replace them with ACSII_NULL character
				x_split = strpbrk(line_to_process, "X");
				y_split = strpbrk(line_to_process, "Y");  
				*x_split = ACSII_NULL;
				*y_split = ACSII_NULL;

				x_split++;	//* x_split now points to char after ASCII_NUL
				y_split++;  //* y_split now points to char after ASCII_NULL

				//* Convert Y value string to float, multiply by machine units
				//* and round it
				tmp_axis = atof(x_split); 
				x_axis = roundf(tmp_axis * mac_units);
 
				//* Convert X value string to float, multiply by machine units
				//* and round it
			    tmp_axis = atof(y_split);
			    y_axis = roundf(tmp_axis * mac_units);

				//* Uncomment to printf below to print to stdout
				//*printf("PU %i,%i\n", (int)x_axis, (int)y_axis);

				//* Format current line
				//* For Pen Up 
				sprintf(curr_line, "PU %i,%i;\n", (int)x_axis, (int)y_axis);
				
				//* Catch consectutive duplicate lines
				//* If curr_line and prev_line are not the same
				//* write curr_line to file & copy it to prev_line
				 if ( strcmp(prev_line, curr_line) != 0 ) {		
					fputs(curr_line, f_dest);
					strcpy(prev_line, curr_line);
				}

			} //* END: else if ( strstr(line_to_process, "G00") != NULL )			

	} //* END: while ( fgets(line_to_process, 32, f_src) != NULL )

	rewind (f_src);
	return 0;
} // END_FUNC: CNC2_write_hpgl

/**
 * CNC2_get_file_size
 * return size of file in bytes
 */ 

int CNC2_get_file_size (FILE* fp)
{
	struct stat st;
	int fd;

	/* Convert file pointer to file number (integer) */
	fd = fileno(fp);
	/* Get info about file */
	if ( fstat(fd, &st) ) { 
        printf("\nfstat error: [%s]\n",strerror(errno));  
        return -1; 
    }
    /* Return size of file */ 
	return st.st_size;
} //* END_FUNC: CNC2_get_file_size

/**
  * CNC2_write_prologue
  *
  * Writer header to output file
  * 
  */
int CNC2_write_prologue( FILE *f_new )
{
	fprintf(f_new,"%s", CNC2_PROLOGUE);

	return 0;
} //* END_FUNC: CNC2_write_prologue

/**
  * CNC2_write_epilogue
  *
  * Writer header to output file
  * 
  */
int CNC2_write_epilogue( FILE *f_new )
{
	fprintf(f_new,"%s", CNC2_EPILOGUE);

	return 0;
} //* END_FUNC: CNC2_write_epilogue

//* END FUNCTIONS

/**
  * cnc2hpgl,  main body
  *
  * This program takes a FlatCAM cnc file and ouputs a HPGL
  * suitable for plotting or converting to EPS
  */
int main(int argc, char **argv) {

	struct CNC2_glb glb;
	FILE *f_source, *f_destination, *f_temp;

	
	/* Initialize our global data structure */
	CNC2_init(&glb);


	/* Parse & decypher the command line parameters provided */
	CNC2_parse_parameters(argc, argv, &glb);
	

	/* Check the fundamental sanity of the variables in
		the global data structure to ensure that we can
		actually perform some meaningful work
		*/
	if ((glb.src_filename == NULL)) {
		fprintf(stderr,"Error: No input filename given.\n");
		exit(1);
	}
	if ((glb.dest_filename == NULL)) {
		fprintf(stderr,"Error: No output filename given.\n");
		exit(1);
	}
	if ((glb.temp_filename == NULL)) {
		fprintf(stderr,"Error: Output filename is NULL.\n");
		exit(1);
	}	

//* Input		
	/* Attempt to open input file as read only */ 
	f_source = fopen(glb.src_filename,"r");
	if (!f_source) {
		fprintf(stderr,"Error: Cannot open input file '%s' for reading (%s)\n", glb.src_filename, strerror(errno));
		return ERR_NOT_OPEN_INPUT;
	}

//* Output
	/* Attempt to open the output file in read\write mode */
	/*	(no appending is done, any existing file will be */
	/*	overwritten */
	f_destination = fopen(glb.dest_filename,"w+");
	if (!f_destination) {
		fprintf(stderr,"Error: Cannot open output file '%s' for read\\write (%s)\n", glb.dest_filename, strerror(errno));
		return ERR_NOT_OPEN_OUTPUT;
	}
	
//* Temporary
	/* Attempt to open the temporary file in read\write mode */
	/*	(no appending is done, any existing file will be */
	/*	overwritten */
	f_temp = fopen(glb.temp_filename,"w+");
	if (!f_temp) {
		fprintf(stderr,"Error: Cannot open temporary file '%s' for read\\write (%s)\n", glb.temp_filename, strerror(errno));
		return ERR_NOT_OPEN_TEMP;
	}	


	//* Remove whitespace from input and save in temp
	printf("Please wait\nSanitising input file %s\n",glb.src_filename);
	CNC2_sanitise_src( f_source, f_temp );
	fclose(f_source);
	
	//* Imperial or metric units 
	if ( CNC2_get_machine_units(f_temp, &glb) == false ){
			fprintf(stderr, "Missing G20 or G21 command\nUnable to determine if metric or imperial measurements used\n");
			exit(1);
		}
	//* Not really used
	if ( CNC2_get_distance_mode(f_temp, &glb) == false ){
			fprintf(stderr, "Missing G90 or G91 command\nUnable to determine if absolute or incremental distance mode\n");
			exit(1);
		}


	
	printf("Writing output file %s\n", glb.dest_filename);

	//* Include epilogue & prologue
	if ( glb.plain ==false ) {
		CNC2_write_prologue(f_destination);
		CNC2_write_hpgl(f_temp, f_destination, glb.machine_units);
		CNC2_write_epilogue(f_destination);
	}

	//* Do not include epilogue & prologue
	else if ( glb.plain == true) {
		CNC2_write_hpgl(f_temp, f_destination, glb.machine_units);
	}
	printf("All done\n");

	fclose(f_destination);
	fclose(f_temp);
	
	if ( glb.keep_tmp == false ) {	
		if ( remove(glb.temp_filename) != 0 ) {
			fprintf(stderr, "Error: Can not remove temp file %s\n", glb.temp_filename);
			return ERR_NOT_REMOVE_TEMP;
		}
	}
	return 0;
}
