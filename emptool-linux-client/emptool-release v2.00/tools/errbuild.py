#!/usr/bin/env python
#
# empeg tools library
#
# (C) 2001 empeg ltd.
#
# Tool to turn error definition file(s) into '.h', '.cpp' and '.mc' files (for windows)
#
# This software is licensed under the GNU General Public Licence (see file
# COPYING), unless you possess an alternative written licence from empeg ltd.
#
# (:Empeg Source Release  01-Apr-2003 18:52 rob:)
#

# Standard stuff

TRUE=1
FALSE=0

# The languages we know how to deal with

LANGUAGE_LIST=["English", "British", "French"]

# Used in the '.mc' output file to define the language outputs - should match language list above

LANGUAGE_MCIDS=["English=0x409:MSG00409", "British=0x809:MSG00809", "French=0x40c:MSG0040c"]

# Default properties of an error message object

DEF_LANGUAGE="English"
DEF_COMPONENT=0
DEF_FACILITY=0
DEF_SEVERITY=0
DEF_CODE=0

import sys
import getopt
import regex
import regex_syntax

from string import strip, upper, atoi, count
from time import time, ctime
from os import path, unlink

###############################################
#
# Container for a compiled error message
#
###############################################

class ParseMessage:
	def __init__(Self):
		Self.Language=DEF_LANGUAGE
		Self.Component=DEF_COMPONENT
		Self.Facility=DEF_FACILITY
		Self.Severity=DEF_SEVERITY
		Self.Code=DEF_CODE
		Self.Symbol=""
		Self.Text=""
		Self.Error = FALSE
		Self.ErrorResult = ""	
		
	def ClearError (Self):
		Self.Error = FALSE
		Self.ErrorResult = ""
		
	def SetSeverity (Self, Data):
		temp = atoi (Data, 0)
		if ((temp >= 0) and (temp <= 3)):
			Self.Severity = temp
		else:
			Self.Error = TRUE
			Self.ErrorResult = "Severity value '" + Data + "' out of range (0 -> 3)"

	def SetFacility (Self, Data):
		temp = atoi (Data, 0)
		if ((temp == 0) or (temp == 4) or (temp == 7)):
			Self.Facility = temp
		else:
			Self.Error = TRUE
			Self.ErrorResult = "Facility value '" + Data + "' out of range (0, 4 or 7)"

	def SetComponent (Self, Data):
		temp = atoi (Data, 0)
		if ((temp >= 0) and (temp <= 15)):
			Self.Component = temp
		else:
			Self.Error = TRUE
			Self.ErrorResult = "Component value '" + Data + "' out of range (0 -> 15)"

	def SetCode (Self, Data):
		temp = atoi (Data, 0)
		if ((temp >= 0) and (temp <= 4095)):
			Self.Code = temp
		else:
			Self.Error = TRUE
			Self.ErrorResult = "Code value '" + Data + "' out of range (0 -> 4095)"

	def SetLanguage (Self, Data):
		if (Data in LANGUAGE_LIST):
			Self.Language = Data			
		else:
			Self.Error = TRUE
			Self.ErrorResult = "Unknown language - " + Data
			Self.Language = "Invalid"

	def GetMessageId (Self):
		return (Self.Component << 12) | Self.Code
		
	def GetValue (Self):
		return (Self.Severity << 30) | (Self.Facility << 16) | (Self.Component << 12) | Self.Code

	def MakeCopy (Self, InputMessage):
		Self.Language = InputMessage.Language
		Self.Component = InputMessage.Component
		Self.Facility = InputMessage.Facility
		Self.Severity = InputMessage.Severity
		Self.Code = InputMessage.Code
		Self.Symbol = InputMessage.Symbol
		Self.Text = InputMessage.Text
		
###############################################
#
# Class for outputting '.cpp' files
#
###############################################

class CppOutput:
	def __init__(Self):
		Self.Language=DEF_LANGUAGE
	
	def Open (Self, FileName):
		Self.FileName = FileName + '.cpp'

		try:
			Self.OutputFile = open(Self.FileName, 'w')
		except:
			return FALSE

#		runname = sys.argv[0]

#		if (runname[0:2] == "./"):
#			runname = runname[2:]

		Self.OutputFile.write ("//\n")
		Self.OutputFile.write ("// Generated on %s\n" % ctime (time()))
		Self.OutputFile.write ("//\n")
		Self.OutputFile.write ("// Language: %s\n" % Self.Language)
		Self.OutputFile.write ("//\n")
		Self.OutputFile.write ("// PLEASE DON'T EDIT THIS FILE MANUALLY\n")
		Self.OutputFile.write ("//\n")
		Self.OutputFile.write ("\n")
		Self.OutputFile.write ("#include <string>\n");
		Self.OutputFile.write ("#include <stdio.h>\n");
		Self.OutputFile.write ("\n")
		Self.OutputFile.write ("#include \"empeg_error.h\"\n");
		Self.OutputFile.write ("\n")
		Self.OutputFile.write ("#include \"%s.h\"\n" % FileName);
		Self.OutputFile.write ("\n")
		Self.OutputFile.write ("std::string FormatErrorMessage(STATUS status)\n")
		Self.OutputFile.write ("    {\n")
		Self.OutputFile.write ("    int test_status = PrintableStatus (status);\n")
		Self.OutputFile.write ("\n")
#		Self.OutputFile.write ("    string result;\n")
#		Self.OutputFile.write ("\n")
#		Self.OutputFile.write ("    switch (print_status)\n")
#		Self.OutputFile.write ("        {\n")

		return TRUE

	def AddOutput (Self, ParseMessage):
		if (Self.Language == ParseMessage.Language):
			prev = ""
			output_text = ""
			for char in ParseMessage.Text:
				if (char == '"'):
					if (prev == '\\'):
						output_text = output_text + char
					else:
						output_text = output_text + '\\"'
				else:
					output_text = output_text + char					
				prev = char;

			Self.OutputFile.write ("    if (PrintableStatus(%s) == test_status)\n" % ParseMessage.Symbol)
			Self.OutputFile.write ("        return \"%s\";\n" % output_text)
			Self.OutputFile.write ("\n")
			
#			Self.OutputFile.write ("        case %s:\n" % ParseMessage.Symbol)
#			Self.OutputFile.write ("            {\n")
#			Self.OutputFile.write ("            return \"%s\";\n" % ParseMessage.Text)		
#			Self.OutputFile.write ("            break;\n")
#			Self.OutputFile.write ("            }\n")
	
	def Close (Self):
#		Self.OutputFile.write ("        }\n")
#		Self.OutputFile.write ("\n")
		Self.OutputFile.write ("    char tmpbuf[ 30 ];\n")    
		Self.OutputFile.write ("    sprintf (tmpbuf, \"Error 0x%08x\", test_status);\n")
		Self.OutputFile.write ("    return std::string (tmpbuf);\n")
#		Self.OutputFile.write ("    return \"Unknown error\";\n")
		Self.OutputFile.write ("    }\n")
		Self.OutputFile.close

	def Abort (Self):
		Self.OutputFile.close
		try:
			unlink (Self.FileName)
		except:
			return

###############################################
#
# Class for outputting '.h' files
#
###############################################

class HeaderOutput:
	def __init__(Self):
		Self.Language=DEF_LANGUAGE

	def Open (Self, FileName):
		Self.FileName = FileName + '.h'

		try:
			Self.OutputFile = open(Self.FileName, 'w')
		except:
			return FALSE
		
		Self.DefineName = upper (path.basename (FileName)) + '_H'

#		runname = sys.argv[0]

#		if (runname[0:2] == "./"):
#			runname = runname[2:]

		Self.OutputFile.write ("//\n")
		Self.OutputFile.write ("// Generated on %s\n" % (ctime (time())))
		Self.OutputFile.write ("//\n")
		Self.OutputFile.write ("// Language: %s\n" % Self.Language)
		Self.OutputFile.write ("//\n")
		Self.OutputFile.write ("// PLEASE DON'T EDIT THIS FILE MANUALLY\n")
		Self.OutputFile.write ("//\n")
		Self.OutputFile.write ("\n")
		Self.OutputFile.write ("#ifndef %s\n" % Self.DefineName)
		Self.OutputFile.write ("#define %s\n" % Self.DefineName)
		Self.OutputFile.write ("\n")

		return TRUE

	def AddOutput (Self, ParseMessage):
		if (Self.Language == ParseMessage.Language):
			Self.OutputFile.write ("// %s\n" % ParseMessage.Text)
			Self.OutputFile.write ("#ifndef %s\n" % ParseMessage.Symbol)
			Self.OutputFile.write ("#define %s\tMAKE_STATUS2(0x%x, 0x%x, 0x%x, 0x%03x) /* 0x%x */ \n" %
				(ParseMessage.Symbol, ParseMessage.Severity, ParseMessage.Facility, 
				ParseMessage.Component, ParseMessage.Code, ParseMessage.GetValue()))
#			Self.OutputFile.write ("#define %s\t((STATUS) 0x%08x)\n" %
#					       (ParseMessage.Symbol, ParseMessage.GetValue()))
			Self.OutputFile.write ("#endif \n\n")
	
	def Close (Self):
		Self.OutputFile.write ("#endif // %s\n" % Self.DefineName)
		Self.OutputFile.close

	def Abort (Self):
		Self.OutputFile.close
		try:
			unlink (Self.FileName)
		except:
			return

###############################################
#
# Class for outputting '.mc' files
#
###############################################

lastmessageoutput = ParseMessage()

class McOutput:
	def Open (Self, FileName):
		Self.FileName = FileName + '.mc'

		try:
			Self.OutputFile = open(Self.FileName, 'w')
		except:
			return FALSE

#		runname = sys.argv[0]

#		if (runname[0:2] == "./"):
#			runname = runname[2:]

		Self.OutputFile.write (";\n")
		Self.OutputFile.write ("; Generated on %s\n" % ctime (time()))
		Self.OutputFile.write (";\n")
		Self.OutputFile.write ("; PLEASE DON'T EDIT THIS FILE MANUALLY\n")
		Self.OutputFile.write (";\n")
		Self.OutputFile.write ("\n")
#		Self.OutputFile.write ("MessageIdTypedef=\n")
		
		Self.OutputFile.write ("SeverityNames=(Success=0x0\n")
		Self.OutputFile.write ("               PlayerWarning=0x1\n")
		Self.OutputFile.write ("               WindowError=0x2\n")
		Self.OutputFile.write ("               PlayerError=0x3)\n")
		Self.OutputFile.write ("FacilityNames=(System=0x0\n")
		Self.OutputFile.write ("               Player=0x4\n")
     		Self.OutputFile.write ("               Windows=0x7)\n")

		prefix="LanguageNames=("
#		Self.OutputFile.write ("LanguageNames=(")
		for mcid in LANGUAGE_MCIDS:
			if (strip (prefix) == ""):
				Self.OutputFile.write ("\n")
			Self.OutputFile.write ("%s%s" % (prefix, mcid))
			prefix = "               "
			
		Self.OutputFile.write (")\n")
		Self.OutputFile.write ("\n")

		return TRUE

	def AddOutput (Self, ParseMessage):
		if ((ParseMessage.Severity != lastmessageoutput.Severity) or
		    (ParseMessage.Code != lastmessageoutput.Code) or
		    (ParseMessage.Component != lastmessageoutput.Component)):
			Self.OutputFile.write ("\n")
			
#			Self.OutputFile.write ("MessageId=0x%08x\n" % ParseMessage.GetValue())
			Self.OutputFile.write ("MessageId=0x%04x\n" % ParseMessage.GetMessageId())
			
			if (ParseMessage.Severity == 0):
				Self.OutputFile.write ("Severity=Success\n")
			elif (ParseMessage.Severity == 1):
				Self.OutputFile.write ("Severity=PlayerWarning\n")
			elif (ParseMessage.Severity == 2):
				Self.OutputFile.write ("Severity=WindowError\n")
			elif (ParseMessage.Severity == 3):
				Self.OutputFile.write ("Severity=PlayerError\n")
			else:
				sys.stderr.write ("Application error in severity\n")

			if (ParseMessage.Facility == 0):
				Self.OutputFile.write ("Facility=System\n")
			elif (ParseMessage.Facility == 4):
				Self.OutputFile.write ("Facility=Player\n")
			elif (ParseMessage.Facility == 7):
				Self.OutputFile.write ("Facility=Windows\n")
			else:
				sys.stderr.write ("Application error in facility\n")

			Self.OutputFile.write ("SymbolicName=%s\n" % ParseMessage.Symbol)

		Self.OutputFile.write ("Language=%s\n" % ParseMessage.Language)
		Self.OutputFile.write ("%s\n" % ParseMessage.Text)
		Self.OutputFile.write (".\n")
			
		lastmessageoutput.MakeCopy (ParseMessage)
		
	def Close (Self):
		Self.OutputFile.close

	def Abort (Self):
		Self.OutputFile.close
		try:
			unlink (Self.FileName)
		except:
			return

###############################################
#
# Read each non-blank line from the given file and add it to the filelist 
#
###############################################

def readfilelist(filename, filelist):
	try:
		input = open(filename, 'r')
		
		while 1:
			line = input.readline()

			if not line:
				break;        # We're outta here ...
			line = strip (line)
			if line <> "":
				filelist.append (line)
	except:
		sys.stderr.write ("Failed to open file: %s\n" % filename)
		sys.exit(1)

###############################################
# RE stuff
###############################################

regex.set_syntax (regex_syntax.RE_SYNTAX_GREP) # Easier in perl type format but ...

searchpattern = regex.compile (                # Allow for tabs in the gaps?
	'^ *\(Symbol\|Severity\|Facility\|Component\|Code\|Language\|Text\)= *\([^ ]\+.*\)')

###############################################
#
# Parse the given file and extract error message definition entries - output complete messages to the output file using the 'outputclass' defined
#
###############################################

global_codelist = {}
global_symbollist = {}

def parsefile(filename, outputclass, testglobal):
	errmess=ParseMessage()


	try:
		input = open(filename, 'r')
	except:
		sys.stderr.write ("Failed to open file: %s\n" % filename)
		sys.exit(1)

	failure = FALSE			# Assume success unless we get a failure below
	continuation = FALSE
	linecount = 0
	codelist = {}
	symbollist = {}
	
	while TRUE:
		line = input.readline()

		linecount = linecount + 1

		if not line:
			break;		# We're outta here ...

		line = strip (line)	# Not too sure whether we should do this ...

		if line == "":
			continue;

		if line[ 0 ] == '#':
			continue;

		errmess.ClearError()
		
		if searchpattern.match (line) > 0:
			type = searchpattern.group(1)
			data = searchpattern.group(2)

#			print " Matched:", type
#			print " Data:", data
		
			# Basically take the option found and assign to the appropriate class member
			
			if (type == "Symbol"):	
				errmess.Symbol = data

			if (type == "Severity"):				
				errmess.SetSeverity (data)

			if (type == "Facility"):				
				errmess.SetFacility (data)

			if (type == "Component"):				
				errmess.SetComponent (data)

			if (type == "Code"):				
				errmess.SetCode (data)

			if (type == "Language"):				
				errmess.SetLanguage (data)

			if (type == "Text"):		    	
				if (data[ len (data) - 1 ] == '\\'):# Check for final character line continuation character '\'
					continuation = TRUE
					data = data[:-1]
				elif ((data[ 0 ] == '"') and (data[ len(data) - 1 ] == '"') and
					(count(data, '"') == 2)):   # if first & last are "'s and they're are only two of them then remove!
					data = data[1:-1]
					sys.stderr.write ("%s:%d: WARNING: Double quotes around text removed - not required\n" % 
						(filename, linecount))
					
				errmess.Text = data
				if not continuation:
					outputclass.AddOutput (errmess)
				
				if (testglobal):
					testcode="%08x:%s" % (errmess.GetValue(), errmess.Language)

					if (codelist.has_key (testcode)):
						failure = TRUE
						sys.stderr.write ("%s:%d: Duplicate code insert attempted: 0x%08x (see line %d)\n" % 
							(filename, linecount, errmess.GetValue(), codelist[ testcode ]))
					else:
						codelist[ testcode ] = linecount

						if (global_codelist.has_key (testcode)):
							sys.stderr.write ("%s:%d: WARNING: Global duplicate code insert: 0x%08x\n" % 
								(filename, linecount, errmess.GetValue()))
							sys.stderr.write ("%s: Code 0x%08x first added here\n" % 
								(global_codelist[ testcode ], errmess.GetValue()))
						else:
							global_codelist[ testcode ] = ("%s:%d" % (filename, linecount))

					testsymbol="%s:%s" % (errmess.Symbol, errmess.Language)

					if (symbollist.has_key(testsymbol)):
						failure = TRUE
						sys.stderr.write ("%s:%d: Duplicate symbol insert attempted: %s (see line %d)\n" % 
							(filename, linecount, errmess.Symbol, symbollist[ testsymbol ]))
					else:
						symbollist[ testsymbol ] = linecount

						if (global_symbollist.has_key (testsymbol)):
							sys.stderr.write ("%s:%d: WARNING: Global duplicate symbol insert: %s\n" % 
								(filename, linecount, errmess.Symbol))
							sys.stderr.write ("%s: Symbol %s first added here\n" % 
								(global_symbollist[ testsymbol ], errmess.Symbol))
						else:
							global_symbollist[ testsymbol ] = ("%s:%d" % (filename, linecount))

		elif continuation:                                   # Possible to continue over more than one line?
			errmess.Text = errmess.Text + ' ' + line
			outputclass.AddOutput (errmess)
			continuation = FALSE
		else:
			failure = TRUE
			sys.stderr.write ("%s:%d: Invalid line '%s'\n" % (filename, linecount, line));

		if errmess.Error:
			failure = TRUE
			sys.stderr.write ("%s:%d: Error with definition: %s\n" % (filename, linecount, errmess.ErrorResult))
		
	if failure:
		outputclass.Abort()
		sys.exit(1)
	
############################################################
#
# Usage function below
#
############################################################

def usage():
	print ("Usage: %s [-h|-c|-m] [ -l <language-string> ] [ -o <output-file> ] (inputfile1|@inputfilelist1) ... " % 
		path.basename (sys.argv[ 0 ]))
	
############################################################
#
# Main program below
#
############################################################

def main():
	if len (sys.argv) == 1:
		usage()
		sys.exit(2)
		
	try:
		opts, args = getopt.getopt(sys.argv[1:], "?hcml:o:", ["help", "header", "cpp", "mc", "language", "output"])
	except:
		# print help information and exit:
		usage()
		sys.exit(2)

	filelist = []
	outputfile=""
	outputlist=""
	
	language = DEF_LANGUAGE
    
	for o, a in opts:					# Deal with options first
		if o in ("-?", "--help"):
			usage()	
			sys.exit()
		if o in ("-h", "--header"):
			outputlist = outputlist + "h"
		if o in ("-c", "--cpp"):
			outputlist = outputlist + "c"
		if o in ("-m", "--mc"):
			outputlist = outputlist + "m"
		if o in ("-l", "--language"):
			language = a
			if not a in LANGUAGE_LIST:
				print "Unknown language:", a
				sys.exit (2)
		if o in ("-o", "--output"):
			outputfile=a
   
	for option in args:					# Process file arguments now
		if option[ 0 ] == '@':
			readfilelist (option[1:], filelist)
		elif option[ 0 ] != '-':
			filelist.append (option)	
	
	if (outputlist == ""):					# Output all file type if none defined
		outputlist = "hcm"
	
	testglobal = TRUE;
	
	for outputtype in outputlist:	
	
		if outputtype == "h":
#			print "Process header"
			fileproc = HeaderOutput()			
			fileproc.Language = language
		elif outputtype == "c":
#			print "Process code file"
			fileproc = CppOutput()
			fileproc.Language = language
		elif outputtype == "m":
#			print "Process mc file"
			fileproc = McOutput ()

		if (outputfile != ""):				# Single output file so open here NOT in loop
			if not fileproc.Open (outputfile):
				sys.stderr.write ("Failed to open output file for '%s'\n" % outputfile)
				sys.exit (1)

		for name in filelist:
			print "Processing:", name

			if (outputfile == ""):			# Open output file here if not set above
				if not fileproc.Open (name):
					sys.stderr.write ("Failed to open output file for '%s'\n" % name)
					sys.exit (1)

			parsefile (name, fileproc, testglobal)	# Parse input file - write to output file using 'fileproc'

			if (outputfile == ""):
				fileproc.Close ()

		if (outputfile != ""):
			fileproc.Close ()			# Close single output file outside loop

		testglobal = FALSE;

# Simply run main program if executed as top level

if __name__ == "__main__":
    main()
