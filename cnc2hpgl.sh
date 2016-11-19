#!/bin/bash
# Author: Robert Murphy
# Date: 16 November 2016
# Name: cnc2hpgl
# Revision: Initial
# Description:
# Quick and dirty method of converting FlatCAM output to
# format for plotting with HP or Roland plotters
# Required utilities:
# bc - https://www.gnu.org/software/bc/
# hp2xx - https://www.gnu.org/software/hp2xx/ 
# pstoedit - http://www.pstoedit.net/
# To do:
# Add commandline options for src file name & other options
FILE_BASE=esp-test-bcu

# Base name plus extension from FlatCAM
FILE=$FILE_BASE.cnc

# Name of temporary file
TMP_FILE=$FILE_BASE.tmp

# Name of HPGL output file
MASK_FILE=$FILE_BASE.plt

# Encapsulated Postscript file
EPS_FILE=$FILE_BASE.eps

# PDF file
PDF_FILE=$FILE_BASE.pdf

# Instructions required at beginning of file (eg initial plotter choose pen set speed)
# modify this file to suit your plotter these are site specific to your plotter
PROLOGUE=prologue.plt


# Instructions required at end of file ( eg home plotter),
# modify this file to suit your plotter these are site specific to your plotter

EPILOGUE=epilogue.plt

# Pen width in .1mm ie 5=0.5mm
PEN_WIDTH=5

# Offset from Bottom Left cormer in mm
X_OFFSET=40
Y_OFFSET=40

# Should do sanity check to ensure BC & hp2xx are installed
# We should fail on bc not installed
# We can continue with hp2xx not installed, but no eps output
# will be generated.
BC=`which bc`
HP2XX=`which hp2xx`
PSTOEDIT=`which pstoedit`

# Save our FlatCAM generated cnc file
cp $FILE $TMP_FILE

# Remove all Z movements
sed -i '/Z/d' $TMP_FILE

# Convert G00 (rapid movements) to PU (pen up)
sed -i 's/G00 X/PU /g' $TMP_FILE

# CONVERT G01 (non rapid movements) to PD (pen down)
sed -i 's/G01 X/PD /g' $TMP_FILE

# Convert 1234.5678Y9012.3456 to  1234.5678 9012.3456 
sed -i 's/Y/ /g' $TMP_FILE

# Process file as to leave only PU & PD commands
sed -i -n '/^P/p' $TMP_FILE

# Get number of lines in file
TOTAL_LINES=`awk 'END {print NR}' $TMP_FILE`
COUNT=1

# Copy instructions required for housekeeping at start
# also overwrites existing file
cat $PROLOGUE > $MASK_FILE

# Check for metric or imperial coordinates 
# G21 For metric input file 40 plotter units per mm
# G20 For imperial input file 1016 plotter units per inch (1016 DPI)
# Is check required G21 & G20 statements in cnc source file ?
if grep -q G21 $FILE 
then RES=40 
else RES=1016 
fi


# Read "processed" FlatCAM cnc file
while read CODE x y; do

# Convert cnc values to ploter values
    X=`echo "scale=0;($x*$RES+0.5)/1" | $BC `
    Y=`echo "scale=0;($y*$RES+0.5)/1" | $BC`



# Wirte converted values to file
    echo -e "$CODE $X,$Y;" >> $MASK_FILE

# Display current line being converted    
    echo "Line $COUNT of $TOTAL_LINES"
    ((COUNT++))
done < $TMP_FILE

# Copy instructions required for housekeeping at end
cat $EPILOGUE >> $MASK_FILE

# -t ---->used to true size
# -m eps ----> mode is enscapulated postscript
# Uncomment below to create encapsulated postscript file
$HP2XX $MASK_FILE -t -m eps \
-p $PEN_WIDTH \
--xoffset $X_OFFSET \
--yoffset $Y_OFFSET  \
-f $EPS_FILE

# EPS file needs editing for suitable sheet (A4 or A5)
#
# A4 Landscape 0 0 842 595
# A4 Portrait 0 0 595 842
# A5 Landscape 0 0 595 420
# A5 Portrait 0 0 420 595 
sed -i 's/^%%BoundingBox:.*$/%%BoundingBox: 0 0 842 595/' $EPS_FILE

# Create pdf from ensacpulated postscript file
$PSTOEDIT -f gs:pdfwrite $EPS_FILE $PDF_FILE


# Remove temporary file
rm $TMP_FILE
