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

#define ERR_NOT_OPEN_INPUT 0x01
#define ERR_NOT_OPEN_OUTPUT 0x01
#define ERR_NOT_OPEN_TEMP 0x01

#define ERR_NOT_REMOVE_TEMP 0x01

#define ACSII_NULL 0x00
#define ACSII_BS 0x08
#define ACSII_HT 0x09
#define ACSII_LF 0x0A
#define ACSII_VT 0x0B
#define ACSII_FF 0x0C
#define ACSII_CR 0x0D
#define ACSII_SPACE 0x20
#define ACSII_DEL 0x7F

#define DISTANCE_MODE_UNK 0
#define DISTANCE_MODE_ABS 90
#define DISTANCE_MODE_INC 91

//* Metric Units = 1/0.025
#define MACHINE_UNITS_MM 40

//* Imperial units = 25.4/0.025
#define MACHINE_UNITS_INCH 1016
#define MACHINE_UNITS_UNK 0


const char CNC2_HELP[]= "convgerber: FlatCAM cnc file converter\n"
	                    "Written by Robert Murphy.\n"
	                    "This software is available at https://github.com/ozzyrob/convgerber\n"
	                    "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
	                    "\n"
	                    "This is free software; you are free to change and redistribute it.\n"
	                    "There is NO WARRANTY, to the extent permitted by law.\n"
	                    "\n"
	                    "Usage: cnc2hpgl -i <input CNC> -o <output HPGL> [-p] [-v] [-h]\n"
	                    "\n"
	                    "\t-i <input CNC> : Specifies which file contains the FlatCAM cnc file to convert.\n"
	                    "\t-o <output HPGL> : specifies which file the converted CNC is to be saved to.\n"
	                    "\t-p : Do not write Prologue & Epilogue to output file.\n" 
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
	bool plain;
	int status;
	float machine_units;
	int distance_mode;				//* 90 = absolute; 91 = incremental; 0=unknown  
	char *src_filename;
	char *dest_filename;
	char *temp_filename;

};
