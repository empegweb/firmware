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
# (:Empeg Source Release  13-Mar-2003 18:15 rob:)
#

# Standard stuff

TRUE=1
FALSE=0

# The languages we know how to deal with

LANGUAGE_LIST=["English", "British", "French", "German", "Italian", "Spanish", "Japanese" ]

# Used in the '.mc' output file to define the language outputs - should match language list above

LANGUAGE_MCIDS=["English=0x409:MSG00409", "British=0x809:MSG00809", "French=0x40c:MSG0040c", "German=0x407:MSG00407", "Italian=0x410:MSG00410", "Spanish=0xc0a:MSG00c0a", "Japanese=0x411:MSG00411" ]

# Default properties of an error message object

DEF_LANGUAGE="English"
DEF_COMPONENT=0
DEF_FACILITY=0
DEF_SEVERITY=0
DEF_CODE=0

import sys
import getopt
import os
import re

from string import strip, upper, atoi, count
from time import time, ctime
#from os import path, unlink, getcwd, name

###############################################
# RE stuff
###############################################

# Whitespace definitions
ws_re = "\s*"
leading_ws_re = "^" + ws_re
trailing_ws_re = ws_re + "$"

# Allow for tabs in the gaps?
searchpattern = re.compile (                
	'^\s*(Symbol|Severity|Facility|Component|Code|Language|Text)=\s*([^ ]+.*)')

# Blank line
blank_re = re.compile(leading_ws_re + "$")

# For parsing previous output header

# expected lines (can ignore these)
expected_lines_re1 = re.compile("^#(ifndef|define) ([A-Z0-9_]+)$")
expected_lines_re2 = re.compile("^#endif")
expected_lines_re3 = re.compile("^//")

# definition lines (need to get values out of these)
definition_lines_re = re.compile("^#define ([A-Z0-9_]+)\tMAKE_STATUS2\((0x[0-9a-f]+), (0x[0-9a-f]+), (0x[0-9a-f]+), (0x[0-9a-f]+)\)")

# list of stuff

global_codelist = {}
global_symbollist = {}

# The list of symbols to be output

symbol_ordered_list = []	# list of symbols - need both because dictionarys are unordered
symbol_list = {}		# dictionary with symbol = entry

# list of the definitions used previously

previous_symbol_list = {}		# dictionary with symbol = value

###############################################
#
# Helper class to simplify pattern matching
#
# (Make if's simplier to use with new RE interface)
#
###############################################

class ReHelper:
	def ReMatch(self, input_re, input_string):
		self.match_group = input_re.match(input_string)
		if (self.match_group == None):
			return FALSE
		else:
			return TRUE

	def MatchGroup(self, group_number):
		return self.match_group.group(group_number)

###############################################
#
# ErrorPosition function
#
###############################################

def ErrorPosition(filename, linecount):
	if (os.name == "posix"):		# Give line output according to os
		return "%s:%d:" % (filename, linecount)
	else:
		return "%s(%d) :" % (os.getcwd() + filename, linecount)

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
		Self.Values = {}		# dictionary of language = text
		#Self.Text=""
		Self.Error = FALSE
		Self.ErrorResult = ""	

	def SetSymbol (Self, Data):
		Self.Symbol = Data
		Self.Values = {}
		
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
		#Self.Text = InputMessage.Text

	def AddLanguageText(Self, InputLanguage, LanguageText):
		if (InputLanguage in Self.Values.keys()):
			return TRUE
		else:
			Self.Values[ InputLanguage ] = LanguageText
			return FALSE
			
	def AddToList(Self, LanguageText):
		if Self.Symbol in symbol_ordered_list:
			add = symbol_list[ Self.Symbol ];
		else:
			add = ParseMessage()
			add.MakeCopy (Self)
			symbol_ordered_list.append(add.Symbol)

		if (add.AddLanguageText(Self.Language, LanguageText)):
			Self.Error = TRUE
			Self.ErrorResult = "Multiple defined language - " + Self.Language

		symbol_list[ Self.Symbol ] = add;

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
		if (Self.Language in ParseMessage.Values.keys()):
			Text = ParseMessage.Values[ Self.Language ];
			prev = ""
			output_text = ""
			for char in Text:
				if (char == '"'):
					if (prev == '\\'):
						output_text = output_text + char
					else:
						output_text = output_text + '\\"'
				else:
					output_text = output_text + char					
				prev = char;

			Self.OutputFile.write ("    if (PrintableStatus(%s) == test_status)\n" % ParseMessage.Symbol)
			Self.OutputFile.write ("        return \"%s (%s)\";\n" % (output_text, ParseMessage.Symbol))
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
		
		Self.DefineName = upper (os.path.basename (FileName)) + '_H'

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
		if (Self.Language in ParseMessage.Values.keys()):
			Text = ParseMessage.Values[ Self.Language ];
			Self.OutputFile.write ("// %s\n" % Text)
			Self.OutputFile.write ("#ifndef %s\n" % ParseMessage.Symbol)
			Self.OutputFile.write ("#define %s\tMAKE_STATUS2(0x%x, 0x%x, 0x%x, 0x%03x) /* 0x%x */ \n" %
				(ParseMessage.Symbol, ParseMessage.Severity, ParseMessage.Facility, 
				ParseMessage.Component, ParseMessage.Code, ParseMessage.GetValue()))
#			Self.OutputFile.write ("#define %s\t((STATUS) 0x%08x)\n" %
#					       (ParseMessage.Symbol, ParseMessage.GetValue()))
			Self.OutputFile.write ("#endif\n\n")
	
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

#lastmessageoutput = ParseMessage()

class McOutput:
	def __init__(Self):
		Self.Language=""
		
	def McidFromLanguage (Self, language):
		ix = LANGUAGE_LIST.index(language)
		return LANGUAGE_MCIDS[ix]
		
	def OutputLanguageNames (Self):
		if Self.Language == "":
			mcid_list = LANGUAGE_MCIDS
		else:
			mcid_list = Self.McidFromLanguage(Self.Language)
		prefix="LanguageNames=("
#		Self.OutputFile.write ("LanguageNames=(")
		for mcid in LANGUAGE_MCIDS:
			if (strip (prefix) == ""):
				Self.OutputFile.write ("\n")
			Self.OutputFile.write ("%s%s" % (prefix, mcid))
			prefix = "               "	
		Self.OutputFile.write (")\n")
		
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

		Self.OutputLanguageNames()
		Self.OutputFile.write ("\n")

		return TRUE

	def AddOutput (Self, ParseMessage):
#		if ((ParseMessage.Severity != lastmessageoutput.Severity) or
#		    (ParseMessage.Code != lastmessageoutput.Code) or
#		    (ParseMessage.Component != lastmessageoutput.Component)):

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

		if Self.Language == "":
			lang_list = LANGUAGE_LIST
		else:
			lang_list = [ Self.Language ]

		for lang in lang_list:
			if lang in ParseMessage.Values.keys():
				Self.OutputFile.write ("Language=%s\n" % lang)
				Self.OutputFile.write ("%s\n" % ParseMessage.Values[ lang ])
				Self.OutputFile.write (".\n")
			else:
				sys.stderr.write ("Warning: missing language %s for message %s\n" % (lang, ParseMessage.Symbol) )
			
		#lastmessageoutput.MakeCopy (ParseMessage)
		
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
#
# Parse the given file and extract error message definition entries
#
# add entries to global list of entries
#
###############################################

def parsefile(filename):

	errmess = ParseMessage()

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
	Text = ""
	m = ReHelper()
	
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
		
		if (m.ReMatch(searchpattern, line)):
			type = m.MatchGroup(1)
			data = m.MatchGroup(2)

#			print " Matched:", type
#			print " Data:", data
		
			# Basically take the option found and assign to the appropriate class member
			
			if (type == "Symbol"):	
				errmess.SetSymbol (data)

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
					sys.stderr.write ("%s WARNING: Double quotes around text removed - not required\n" % 
						(ErrorPosition(filename, linecount)))
					
				Text = data
				if not continuation:
					errmess.AddToList(Text)
#					outputclass.AddOutput (errmess)
				
				if TRUE:
					testcode="%08x:%s" % (errmess.GetValue(), errmess.Language)

					if (codelist.has_key (testcode)):
						failure = TRUE
						sys.stderr.write ("%s Duplicate code insert attempted: 0x%08x (see line %d)\n" % 
							(ErrorPosition(filename, linecount), errmess.GetValue(), codelist[ testcode ]))
					else:
						codelist[ testcode ] = linecount

						if (global_codelist.has_key (testcode)):
							sys.stderr.write ("%s WARNING: Global duplicate code insert: 0x%08x\n" % 
								(ErrorPosition(filename, linecount), errmess.GetValue()))
							sys.stderr.write ("%s: Code 0x%08x first added here\n" % 
								(global_codelist[ testcode ], errmess.GetValue()))
						else:
							global_codelist[ testcode ] = ("%s" % (ErrorPosition(filename, linecount)))

					testsymbol="%s:%s" % (errmess.Symbol, errmess.Language)

					if (symbollist.has_key(testsymbol)):
						failure = TRUE
						sys.stderr.write ("%s Duplicate symbol insert attempted: %s (see line %d)\n" % 
							(ErrorPosition(filename, linecount), errmess.Symbol, symbollist[ testsymbol ]))
					else:
						symbollist[ testsymbol ] = linecount

						if (global_symbollist.has_key (testsymbol)):
							sys.stderr.write ("%s WARNING: Global duplicate symbol insert: %s\n" % 
								(ErrorPosition(filename, linecount), errmess.Symbol))
							sys.stderr.write ("%s: Symbol %s first added here\n" % 
								(global_symbollist[ testsymbol ], errmess.Symbol))
						else:
							global_symbollist[ testsymbol ] = ("%s" % (ErrorPosition(filename, linecount)))

		elif continuation:                                   # Possible to continue over more than one line?
			Text = Text + ' ' + line
			errmess.AddToList(Text)
#			outputclass.AddOutput (errmess)
			continuation = FALSE
		else:
			failure = TRUE
			sys.stderr.write ("%s Invalid line '%s'\n" % (ErrorPosition(filename, linecount), line));

		if errmess.Error:
			failure = TRUE
			sys.stderr.write ("%s Error with definition: %s\n" % (ErrorPosition(filename, linecount), errmess.ErrorResult))
		
	if failure:
#		outputclass.Abort()
		sys.exit(1)
	
#	for sym in symbol_ordered_list:
#		print "%s = %x" % (sym, symbol_list[ sym ].GetValue())
		

###############################################
#
# Parse the header output file given and get the previous values output
#
###############################################

def get_previous_hsymbols(output_file):
	global previous_symbol_list
	
	try:
		read_file = open(output_file + ".h", 'r')
	except:
		sys.stderr.write ("Failed to open input file for '%s'\n" % (output_file + ".h"))
		return;
	
	previous = ParseMessage()
	
	ln = 0
	
	previous_ok = TRUE;

	m = ReHelper()

	while previous_ok:
		line = read_file.readline()

		ln = ln + 1

		if not line:
			break;		# We're outta here ...

		#print line
		
		if (m.ReMatch(expected_lines_re1, line)):
			#print "IGNORE1: %s" % expected_lines_re1.MatchGroup(1)
			continue;			

		elif (m.ReMatch(expected_lines_re2, line)):
			#print "IGNORE2: %s" % line
			continue;			

		elif (m.ReMatch(expected_lines_re3, line)):
			#print "IGNORE2: %s" % line
			continue;			

		elif (m.ReMatch(blank_re, line)):
			#print "BLANK: %s" % line
			continue;			
			
		elif (m.ReMatch(definition_lines_re, line)):
			symbol = m.MatchGroup(1)
			
			#print line
			previous.ClearError()
			previous.SetSeverity(m.MatchGroup(2));
			previous.SetFacility(m.MatchGroup(3));
			previous.SetComponent(m.MatchGroup(4));
			previous.SetCode(m.MatchGroup(5));

			# Not really correct since we'll only get the last error but ...
			if (previous.Error):
				sys.stderr.write ("%s %s\n" % (ErrorPosition(output_file + ".h", ln), previous.ErrorResult))
				previous_ok = FALSE
			
#			print "DEFN: %s %s %s %s %s %x" % (symbol, 
#				definition_lines_re.MatchGroup(2),
#				definition_lines_re.MatchGroup(3),
#				definition_lines_re.MatchGroup(4),
#				definition_lines_re.MatchGroup(5),
#				previous.GetValue())

			previous_symbol_list[ symbol ] = previous.GetValue();
		else:
			sys.stderr.write ("%s Unknown line type\n" % 
				ErrorPosition(output_file + ".h", ln))
			previous_ok = FALSE
			#previous_symbol_list.append(symbol)

	read_file.close
	
	if not previous_ok:
		previous_symbol_list = {}
		
#	for sym in previous_symbol_list.keys():
#		print "%s = %x" % (sym, previous_symbol_list[ sym ])

###############################################
#
# Return true if there are no changes in the include values
#
###############################################

def no_changes():
	#print "old %d new %d" % (len(previous_symbol_list.keys()), len(symbol_ordered_list))

	if len(previous_symbol_list.keys()) != len(symbol_ordered_list):
		return FALSE

	for symbol in symbol_ordered_list:
		if not symbol in previous_symbol_list.keys():
			return FALSE
		
		if previous_symbol_list[ symbol ] != symbol_list[ symbol ].GetValue():
			return FALSE

	return TRUE

############################################################
#
# Usage function below
#
############################################################

def usage():
	print ("Usage: %s [-f] [-h|-c|-m] [ -l <language-string> ] -o <output-file> (inputfile1|@inputfilelist1) ... " % 
		os.path.basename (sys.argv[ 0 ]))
	
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
		opts, args = getopt.getopt(sys.argv[1:], "?fhcml:o:", ["help", "force-build", "header", "cpp", "mc", "language", "output"])
	except:
		# print help information and exit:
		usage()
		sys.exit(2)

	filelist = []
	output_file=""
	outputlist=""
	if (os.name == "posix"):		# Give line output according to os	
		force_build = FALSE
	else:
		force_build = TRUE
	
	language = ""
    
	for o, a in opts:					# Deal with options first
		if o in ("-?", "--help"):
			usage()	
			sys.exit()
		if o in ("-f", "--force-build"):
			if (os.name == "posix"):		# Give line output according to os	
				force_build = TRUE
			else:
				force_build = FALSE
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
			output_file=a

	# Must have an output file to continue
	
	if not output_file:
		usage()
		sys.exit(2)
   
	for option in args:					# Process file arguments now
		if option[ 0 ] == '@':
			readfilelist (option[1:], filelist)
		elif option[ 0 ] != '-':
			filelist.append (option)	
	
	if (outputlist == ""):					# Output all file type if none defined	
		outputlist = "hcm"

	# Parse previous output header and save symbols
	
	if (not force_build) and ("h" in outputlist):
		get_previous_hsymbols(output_file)
	
	# Parse input file - put symbols in global list
	
	for name in filelist:
		print "Processing:", name
		parsefile (name)				

	# Check for programmer error	
	if (len(symbol_ordered_list) != len(symbol_list.keys())):
		sys.stderr.write ("Programmer error\n")
		sys.exit(1)
		
	# Next output each file type from generated list
	
	for outputtype in outputlist:	
	
		if outputtype == "h":
			print "Process header"

			if (not force_build) and (no_changes()):
				print "No header changes detected - ignoring"
				continue
				
			fileproc = HeaderOutput()
			if language == "":
				fileproc.Language = DEF_LANGUAGE
			else:
				fileproc.Language = language
		elif outputtype == "c":
			print "Process code file"
			fileproc = CppOutput()
			if language == "":
				fileproc.Language = DEF_LANGUAGE
			else:
				fileproc.Language = language
		elif outputtype == "m":
			print "Process mc file"
			fileproc = McOutput ()
			fileproc.Language = language

		if not fileproc.Open (output_file):
			sys.stderr.write ("Failed to open output file: '%s'\n" % output_file)
			sys.exit (1)

		for sym in symbol_ordered_list:
			#print "OUTPUT %s = %x" % (sym, symbol_list[ sym ].GetValue())
			fileproc.AddOutput(symbol_list[ sym ])

		fileproc.Close ()			# Close single output file outside loop

# Simply run main program if executed as top level

if __name__ == "__main__":
    main()
