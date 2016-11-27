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
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define CNC2_VERSION "0.0.1"

#define ERR_NOT_OPEN_INPUT 0x01
#define ERR_NOT_OPEN_OUTPUT 0x01
#define ERR_NOT_OPEN_TEMP 0x01

#define BS 0x08
#define HT 0x09
#define LF 0x0A
#define VT 0x0B
#define FF 0x0C
#define CR 0x0D
#define SPACE 0x20
#define DEL 0x7F

#define DISTANCE_MODE_UNK 0
#define DISTANCE_MODE_ABS 90
#define DISTANCE_MODE_INC 91

#define MACHINE_UNITS_MM 40;
#define MACHINE_UNITS_INCH 1016;
#define MACHINE_UNITS_UNK 0


const char CNC2_HELP[]= "convgerber: FlatCAM cnc file converter\n"
	                    "Written by Robert Murphy.\n"
	                    "This software is available at https://github.com/ozzyrob/convgerber\n"
	                    "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
	                    "\n"
	                    "This is free software; you are free to change and redistribute it.\n"
	                    "There is NO WARRANTY, to the extent permitted by law.\n"
	                    "\n"
	                    "Usage: cnc2hpgl -i <input CNC> -o <output HPGL> [-v] [-h]\n"
	                    "\n"
	                    "\t-i <input CNC> : Specifies which file contains the FlatCAM cnc file to convert.\n"
	                    "\t-o <output HPGL> : specifies which file the converted CNC is to be saved to.\n"
	                    "\n"
	                    "\t-v : Display current software version\n"
	                    "\t-h : Display this help.\n"
	                    "\n";
	                    
const char CNC2_PROLOGUE[] = ".@;1:IN;\n"
						     "SP;\n"
						     "VS5;\n"
						     "SP1;\n";

const char CNC2_EPILOGUE[] = "SP0;\n"
							 "SP;\n"
							 "IN;\n";



							 
struct CNC2_glb {
	int status;
	float machine_units;
	int distance_mode;				//* 90 = absolute; 91 = incremental; 0=unknown  
	char *src_filename;
	char *dest_filename;
	char *temp_filename;

};							 					   	               

/**
  * CNC2_show_version
  *
  * Display the current convgerber version
  */
int CNC2_show_version( struct CNC2_glb *glb ) {
	fprintf(stderr,"%s\n", CNC2_VERSION);
	return 0;
}

/**
  * CNC2_show_help
  *
  * Display the help data for this program
  */
int CNC2_show_help( struct CNC2_glb *glb ) {
	CNC2_show_version(glb);
	fprintf(stderr,"%s\n", CNC2_HELP);

	return 0;

}                    

/**
  * CNC2_init
  *
  * Initializes any variables or such as required by
  * the program.
  */
int CNC2_init( struct CNC2_glb *glb )
{
	glb->status = 0;
	glb->machine_units = MACHINE_UNITS_UNK;
	glb->distance_mode = DISTANCE_MODE_UNK;
	glb->src_filename = NULL;
	glb->dest_filename = NULL;
	glb->temp_filename = "CNC.TMP";
	return 0;
}

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
		c = getopt(argc, argv, "i:o:vh");
		switch (c) { 
			case EOF: /* finished */
				break;

			case 'i':
				glb->src_filename = strdup(optarg);
				break;

			case 'o':
				glb->dest_filename = strdup(optarg);
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
				
		} /* switch */
	} while (c != EOF); /* do */
	return 0;
}

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
					
					case BS:
					case HT:
					case VT:
					case FF:
					case CR:
					case SPACE:
					case DEL:
								break;
					
					default:
								fputc( char_read, f_dest);
			}
	}
	
	rewind(f_dest);	
	return 0;
}	
/**
  * CNC2_get_distance_mode
  * get & set path control mode
  * when done file pointer is reset to start
  * 
  */
int CNC2_get_distance_mode ( FILE* f_src, struct CNC2_glb *glb ) {
	
	char buff[80];
	
	while ( fgets(buff, 80, f_src) != NULL ) {
		
		
		//* Check for G90 absolute mode
		if ( strstr(buff, "G90") != NULL ) {
			printf("Absolute co-ords\n");
			glb->distance_mode = DISTANCE_MODE_ABS; 
			rewind (f_src);
			return 0;
		}
		//* Check for G91 incremental mode
		else if ( strstr(buff, "G91") != NULL ){
			printf("Incremental co-ords\n");
			glb->distance_mode = DISTANCE_MODE_INC;
			rewind (f_src);
			return 0;
		}
	}		

	//* No mode found
	printf("Mode not found\n");
	glb->distance_mode = DISTANCE_MODE_UNK;
	rewind (f_src);
	return DISTANCE_MODE_UNK;
}

/**
  * CNC_get_machine_units
  * get & set machine units
  * when done file pointer is reset to start
  * 
  */
int CNC2_get_machine_units ( FILE* f_src, struct CNC2_glb *glb ) {
	
	char buff[80];
	
	while ( fgets(buff, 80, f_src) != NULL ) {
		
		
		//* Check for G20 imperial (inch)
		if ( strstr(buff, "G20") != NULL ) {
			printf("Imperial Units\n");
			glb->machine_units = MACHINE_UNITS_INCH; 
			rewind (f_src);
			return 0;
		}
		//* Check for G21 metric (mm)
		else if ( strstr(buff, "G21") != NULL ){
			printf("Metric units\n");
			glb->machine_units = MACHINE_UNITS_MM;
			rewind (f_src);
			return 0;
		}
	}		

	//* No mode found
	printf("Units not found\n");
	glb->machine_units=MACHINE_UNITS_UNK;
	rewind (f_src);
	return MACHINE_UNITS_UNK;
}

/**
  * CNC_x_y_moves
  * copy G00 & G01 moves that have x & y
  * when done file pointer is reset to start
  * 
  */
int CNC2_x_y_moves ( FILE* f_src, FILE* f_dest, float mac_units) {
	
	int i;
	int x;
	float tmp_axis;
	float x_axis;
	float y_axis;
	char buff[32];
	char current_axis[16];
	char prev_line[32];
	char curr_line[32];

	

	while ( fgets(buff, 32, f_src) != NULL ) {
		
		
		//* Check Z (G00Z or G01Z)
		//* Not required for plotter
		//* as G00 (rapid) is equal to PU movement
		if ( strstr(buff, "Z") != NULL ) {
			
			continue;
		}

		//* Rapids (G00)
		//* Convert X & Y values to plotter values
		//* Pen Up instruction
		else if ( strstr(buff, "G00X") != NULL ) {
			
 
			
				i=0;
				while ( buff[i] != 'X' ) {

					i++;
					}
				i++;
				x=0;
				
				//* Get X cordinate and convert using supplied units
				while ( buff[i] != 'Y' ) {
					
					current_axis[x] = buff[i];
					i++;
					x++;
					}	
				//* Add string terminator (NULL)
				current_axis[x] = 0x00;
				//* Convert string to float
				tmp_axis = atof(current_axis);
				//* Multiply by required units and save rounded value
				x_axis = roundf(tmp_axis * mac_units);	
				i++;
				x=0;				

				//* Get Y cordinate and convert using supplied units
				while ( buff[i] != LF ) {
					
					current_axis[x] = buff[i];
					i++;
					x++;
					}
				//* Add string terminator (NULL)
				current_axis[x] = 0x00;	
				//* Convert string to float
				tmp_axis = atof(current_axis);
				//* Multiply by required units and save rounded value
				y_axis = roundf(tmp_axis * mac_units);
				//* Uncomment to printf below to print to stdout
				//*printf("PU %i,%i\n", (int)x_axis, (int)y_axis);

				//* Format current line
				 sprintf(curr_line, "PU %i,%i;\n", (int)x_axis, (int)y_axis);

				//* Catch consectutive duplicate lines
				//* If curr_line and prev_line are not the same
				//* write curr_line to file & copy it to prev_line
				 if ( strcmp(prev_line, curr_line) != 0 ) {		
					fputs(curr_line, f_dest);
					strcpy(prev_line, curr_line);
				}
				//* else if ( strcmp(prev_line, curr_line) != 0 ) {
				//* 
				//*}
			}
		

		//* Linear movement (G01)
		//* Convert X & Y values to plotter values
		//* Pen Down instruction
		else if ( strstr(buff, "G01X") != NULL ) {
				i=0;
				while ( buff[i] != 'X' ) {

					i++;
					}
				i++;
				x=0;
				
				while ( buff[i] != 'Y' ) {
					
					current_axis[x] = buff[i];
					i++;
					x++;
					}	
				//* Add string terminator (NULL)
				current_axis[x] = 0x00;
				//* Convert string to float
				tmp_axis = atof(current_axis);
				//* Multiply by required units and save rounded value
				x_axis = roundf(tmp_axis * mac_units);	
				i++;
				x=0;				
				while ( buff[i] != LF ) {
					
					current_axis[x] = buff[i];
					i++;
					x++;
					}
				//* Add string terminator (NULL)
				current_axis[x] = 0x00;	
				//* Convert string to float
				tmp_axis = atof(current_axis);
				//* Multiply by required units and save rounded value
				y_axis = roundf(tmp_axis * mac_units);
				//* Uncomment to printf below to print to stdout
				//*printf("PD %i,%i\n", (int)x_axis, (int)y_axis);					
				//* Format current line
				 sprintf(curr_line, "PD %i,%i;\n", (int)x_axis, (int)y_axis);

				//* Catch consectutive duplicate lines
				//* If curr_line and prev_line are not the same
				//* write curr_line to file & copy it to prev_line
				 if ( strcmp(prev_line, curr_line) != 0 ) {		
					fputs(curr_line, f_dest);
					strcpy(prev_line, curr_line);
				}
				//* else if ( strcmp(prev_line, curr_line) != 0 ) {
				//* 
				//*}
		}	
		
	
	}
	rewind (f_src);
	return 0;
}

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
}

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
}

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
}

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
		fprintf(stderr,"Error: Input filename is NULL.\n");
		exit(1);
	}
	if ((glb.dest_filename == NULL)) {
		fprintf(stderr,"Error: Output filename is NULL.\n");
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
	printf("Sanitising file:\t");
	CNC2_sanitise_src( f_source, f_temp );
	printf("Done !\n");
	fclose(f_source);

	CNC2_get_distance_mode(f_temp, &glb);
	printf("Co-ord value = %i\n", glb.distance_mode);

	CNC2_get_machine_units(f_temp, &glb);
	printf("Units Value = %f\n", glb.machine_units);
	
	
	CNC2_write_prologue(f_destination);
	CNC2_x_y_moves(f_temp, f_destination, glb.machine_units);
	CNC2_write_epilogue(f_destination);

	fclose(f_destination);
	fclose(f_temp);

	return 0;
}
