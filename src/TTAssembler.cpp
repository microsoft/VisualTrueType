/*****
 *
 *	TTAssembler.c
 *
 *	Copyright (c) Microsoft Corporation.
 *  Licensed under the MIT License.
 *
 *	
 * Notes on TrueType Assembler GregH 12/2011
 * Instruction matching is a linear search. Should be easy to move to a binary search
 * Numbers can be hexadecimal when prefixed by a 0x (not 0X). Hex digits can be upper or lower case
 * Maximum number is 0xFFFF if positive or 0x8FFF if negative
 * + or - can prefix numbers
 * 256 (PUSH_ARG) is the maximum number of arguments for one #PUSH
 * A label must be less than 22 (MAXLABELLENGTH) characters
 * 200 (MAXJRPAIR) total PUSHs in a block
 *
 *****/
#define _CRT_SECURE_NO_DEPRECATE 
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <stdio.h> /* for swprintf  */
#include <string.h> /* for  wcslen */
#include <limits.h> /* for SHRT_MIN MAX */
#include "pch.h"

#define SHORTMAX 32767
#define SHORTMIN -32768
#define USHORTMAX 65535

#define MAXINSTRUCTIONCOUNT 16000
#define MAXARGUMENTS (0x8000L - 4L) // don't push the limit
#define ARGTYPEBUFFER_SIZE 300

/* maximumembeding of #BEGIN #END block */
#define MAXBLOCKEMBEDDING	6

/* ClaudeBe for the DELTA command, DeltaBase is constant 9 and DeltaShift is constant 3,
   use the DLT command with  relative ppem (0..15) and relative offset (-8..8) if you need
   a specific DeltaBase or DeltaShift */
#define DELTABASE				9

#define MAXIFRECURSION 20

/* maximal number of delta in one command */
#define MAXDELTA 256

/* for the DovMan partial compilation feature that flash points referenced by the current command */
#define tt_MAXFLASHINGPOINTS 4

#define tt_MAXINSTRUCTIONS	8000  /* size needed for the chicago Japanese fonts */

typedef struct {
	unsigned short flashingPoints[tt_MAXFLASHINGPOINTS];
	short NumberOfFlashingPoint;
} tt_flashingPoints;

#define tt_NoError				0
#define	tt_EmbeddedComment		1
#define tt_UnterminatedComment	2
#define tt_UnknownSwitch		3
#define tt_UnknownInstruction	4
#define tt_TwoInstructionsInSameLine	5 /* could mean garbage, too many arguments,... */
#define tt_BooleanFlagsMissing	6
#define tt_WrongNumberOfBoolean	7
#define tt_TooManyBooleans		8
#define tt_UnrecognizedBoolean	9
#define tt_MissingClosingBracket	10
#define tt_SLOOPArgumentBufferTooSmall	11  /* reduce the count for SLOOP, the compiler cannot handle such a big number */
#define tt_EmptyParameterList	12 /* missing comma between parameters or empty parameter list */
#define tt_UnableToParseArgument	13
#define tt_MissingParameters	14
#define tt_PointNbOutOfRange	15
#define tt_CVTIndexOutOfRange	16
#define tt_StorageIndexOutOfRange	17
#define tt_ContourNbOutOfRange	18
#define tt_FunctionNbOutOfRange	19
#define tt_ArgumentOutOfRange	20
#define tt_ArgumentIndexOutOfRange	21 /* this error should never happend, this comes from a programming error */
#define tt_NotEnoughMemory	22
#define tt_DeltaListMissing	23
#define tt_DeltaOpeningParenthesisMissing	24
#define tt_PointSizeOutOfRange	25
#define tt_DeltaDenominatorMissing 26
#define tt_DeltaWrongDenominator	27
#define tt_DeltaAtSignMissing	28
#define tt_TooManyDeltas	29
#define tt_DeltaClosingBracketMissing	30
#define tt_DeltaOORangePpem	31
#define tt_TooManyLabels	32
#define tt_LabelTooLong		33
#define tt_DuplicateLabel	34
#define tt_EndWithoutBegin	35
#define tt_MissingEndBlock	36
#define tt_TooManyEnbeddedBlocks	37
#define tt_CompositeCode	38
#define tt_VoidLabel	39
#define tt_LabelNotFound	40
#define tt_ExpectingAComma	41
#define tt_TooManyPushArgs	42
#define tt_ParseOverflow	44
#define tt_JRExpectingABracket	45
#define tt_JRExpectingABWLabel	46
#define tt_JRExpectingAEqual	47
#define tt_JRExpectingALabel	48

#define tt_JumpTooBigForByte	49

#define tt_EIFwithoutIF	50
#define tt_ELSEwithoutIF	51
#define tt_ELSEwithinELSE	52
#define tt_TooManyEmbeddedIF	53
#define tt_ExpectingaBEGIN	54

#define tt_FDEFInsideFDEF	55
#define tt_ENDFwithoutFDEF	56

#define tt_IFwithoutEIF	57
#define tt_FDEFwithoutENDF	58
#define tt_IDEFwithoutENDF	59

#define tt_PUSHONwhenAlreadyOn	60
#define tt_PUSHOFFwhenAlreadyOff	61
#define tt_IFgoingAcrossBlocks	62
#define tt_FDEFgoingAcrossBlocks	63
#define tt_IDEFgoingAcrossBlocks	64

#define tt_FDEFInsideIDEF	65
#define tt_IDEFInsideFDEF	66
#define tt_IDEFInsideIDEF	67
#define tt_IDEF_FDEFinGlyphProgram	68
#define tt_INSTCTRLnotInPreProgram	69
#define tt_ProgramTooBig	70
#define tt_TooManyArguments	71
#define tt_DELTAWithoutArguments	72

#define tt_DeltaClosingParenthesisMissing	73

#define tt_DELTAWithArguments	74

#define tt_PUSHBWInPushON	75
#define tt_WildCardInPush	76
#define tt_JumpNegativeForByte	77

#define tt_NotImplemented	9999
#define tt_Push_Switch			1
#define tt_PushOn_Switch		2
#define tt_PushOff_Switch		3
#define tt_Begin_Switch			4
#define tt_End_Switch			5
#define tt_GHTBlockBegin_Switch	6
#define tt_GHTBlockEnd_Switch	7

#define co_NoError				0

/* start at 100 for the composite errors to avoid overlapping */

#define co_TwoInstructionsInSameLine		101
#define co_BooleanFlagsMissing				102
#define co_UnrecognizedBoolean				103
#define co_WrongNumberOfBoolean				104
#define co_MissingClosingBracket			105
#define co_TooManyBooleans					106

#define co_EmptyParameterList				107
#define co_PointNbOutOfRange				108
#define co_GlyphIndexOutOfRange				109
#define co_ArgumentOutOfRange				110
#define co_MissingParameters				111

#define co_2_14Overflow	114

#define co_NotImplemented	9999

typedef struct {
	short	WeAreInsideAnIF;
	short	NumberOfEmbeddedIF; /* for #PUSHON mode */			
	short	WeAreInsideAnFDEF;			
	short	WeAreInsideAnIDEF;			
	short	WeAreInPushOnMode;
	short	WeAreInPrePushMode;
	short	WeAreInsideGHBlock;
	short	LastCommandWasAnIF; /* used to check for #BEGIN (beginning of a block) */
	short	LastCommandWasAnELSE; /* used to check for #BEGIN (beginning of a block) */
	short	LastCommandWasAnFDEF; /* used to check for #BEGIN (beginning of a block) */
	short	LastCommandWasAnIDEF; /* used to check for #BEGIN (beginning of a block) */
	short	LastCommandWasAnJUMP; /* used to check for #BEGIN (beginning of a block) */
	short	LastCommandWasAnEND; /* to check if the ENDF, ELSE  or EIF is preceeded by #END (end of block) */
	short	ELSEStatus[MAXIFRECURSION];	/* to keep track in embedded IF statement
									if we are already in the ELSE clause */		
} tt_CompilationStatus;


void TT_memSwap( char *a,char *b,char *tmp,long len );
void TT_memSwap( char *a,char *b,char *tmp,long len )
{
	memcpy( tmp, a, len );
	memcpy( a, b, len );
	memcpy( b, tmp, len );
}


typedef struct {
	unsigned short point;
	unsigned short relppem;
	short num; /* numerator */
	unsigned short denum; /* denominator */
} tt_deltaPType;


typedef struct {
	const wchar_t			*name;			/* compiler switch string */
	const wchar_t			*description;	/* descriptive info */
	short			index;			/* switch index   */
} tt_CompilerSwitchType;


typedef struct {
	wchar_t  type;
	wchar_t	 code;
	wchar_t	 result;
} asm_BooleanTranslationType;

typedef struct {
	wchar_t	type;
	wchar_t	Reserved1; /* for struct alignment */
	wchar_t	Reserved2;
	wchar_t	Reserved3;
	const wchar_t	*explanation;
	short     lowestValidValue;		/* 6-7-90 JTS Range Checking Adds */
	short		highestValidValue;		/* 6-7-90 JTS Range Checking Adds */
} asm_PushAndPopDescriptionType;

/* lower case is reserved for loop variable dependent pops */
asm_PushAndPopDescriptionType asm_ppDescription1[] = {
	{ L'P',L' ',L' ',L' ',	L"Point Number" ,0 ,0},
	{ L'E',L' ',L' ',L' ',	L"Zone Pointer" ,0 ,0},  /* This is handled differently */
	{ L'D',L' ',L' ',L' ',	L"Distance in pixels, I pixel = 64 units" ,SHORTMIN ,SHORTMAX},
	{ L'B',L' ',L' ',L' ',	L"bool, true or false" ,SHORTMIN ,SHORTMAX},
	{ L'R',L' ',L' ',L' ',	L"Vector Projection" ,-16384 ,16384},
	{ L'F',L' ',L' ',L' ',	L"Function Number" ,0 ,255},
	{ L'I',L' ',L' ',L' ',	L"Index into control value table" ,0 ,0},
	{ L'J',L' ',L' ',L' ',	L"Index into storage area" ,0 ,0},
	{ L'*',L' ',L' ',L' ',	L"Anything" ,SHORTMIN ,SHORTMAX},
	{ L'&',L' ',L' ',L' ',	L"Entire Stack" ,SHORTMIN ,SHORTMAX},
	{ L'C',L' ',L' ',L' ',  L"Contour Number" ,0 ,0},
	{ L'N', L' ',L' ',L' ', L"Small Number" ,0 ,255},
	{ L'V',L' ',L' ',L' ',  L"Positive Value" ,0 ,SHORTMAX},	/* New values added to use RangeChecking*/
	{ L'L', L' ',L' ',L' ', L"Label" ,SHORTMIN ,SHORTMAX}, /* new ClaudeBe, to parse the label of a JR instruction */
	{ L'A', L' ',L' ',L' ', L"Unsigned Byte" ,0 ,255}, /* NPUSH, number of puses */
	{ L'H', L' ',L' ',L' ', L"Signed Word" ,SHORTMIN ,SHORTMAX}  /* PUSHW argument */
};	/* See typedef asm_PushAndPopDescriptionType (above) 6-7-90 JTS Range Checking Adds */

#define NumberOfPPEntries  15 

tt_CompilerSwitchType tt_CompilerSwitch[] = {
/****  Instruction name, descriptive info,    index *****/
	{ L"PUSH",		L"Push arguments on the stack", 	tt_Push_Switch },
	{ L"PUSHON",	L"Set direct push mode on", 		tt_PushOn_Switch },
	{ L"PUSHOFF",	L"Set direct push mode off", 		tt_PushOff_Switch },
	{ L"BEGIN",		L"Beginning of a block", 			tt_Begin_Switch },
	{ L"END",		L"End of a block", 					tt_End_Switch },
	{ L"GHTBLOCK",	L"Beginning of a new Sampo block", 	tt_GHTBlockBegin_Switch },
	{ L"GHTB",		L"Beginning of a new Sampo block", 	tt_GHTBlockBegin_Switch },
	{ L"GHTE",		L"End of a new Sampo block", 		tt_GHTBlockEnd_Switch }
};

#define asm_SLOOP 0x17

typedef struct {
	const wchar_t			*name;			/* Apple instruction name */
	const wchar_t			*description;	/* descriptive info */
	const wchar_t			*pops;			/* What the instruction pops   */
	const wchar_t			*pushes;		/* What the instruction pushes */
	unsigned short			 baseCode;		/* base code, need to be short because of the fake code and the duplicate DELTA */
	const wchar_t			*booleans;		/* booleans */
} tt_InstructionType;

/******
 array defining the TrueType instructions, it look like :
tt_InstructionType tt_instruction[] = {
/+  Instruction name, descriptive info,                          pops, pushes, base code, booleans -/
	{ "SVTCA",		L"Set Vectors To Coordinate Axis",				L"",		L"",		0x00, "A" },
.......

*****/

#define tt_TOTALNUMBEROFINSTRUCTIONS 178

const tt_InstructionType tt_instruction[] = {
/****  Instruction name, descriptive info,                          pops, pushes, base code, booleans *****/
	{ L"SVTCA",		L"Set Vectors To Coordinate Axis",				L"",		L"",		0x00, L"A"},
	{ L"SPVTCA",	L"Set Projection Vector To Coordinate Axis",	L"",		L"",		0x02, L"A"}, 
	{ L"SFVTCA",	L"Set Freedom Vector To To Cordinate Axis",		L"",		L"",		0x04, L"A"},
	{ L"SPVTL",		L"Set Projection Vector To Line",				L"PP",		L"",		0x06, L"R"},
	{ L"SFVTL",		L"Set Freedom Vector To Line",					L"PP",		L"",		0x08, L"R"},
	{ L"SPVFS",		L"Set Projection Vector From Stack",			L"RR",		L"",		0x0A, L"" },
	{ L"SFVFS",		L"Set Freedom Vector From Stack",				L"RR",		L"",		0x0B, L"" },
	{ L"GPV",		L"Get Projection Vector",						L"",		L"RR",		0x0C, L"" },
	{ L"GFV",		L"Get Freedom Vector",							L"",		L"RR",		0x0D, L"" },
	{ L"SFVTPV",	L"Set Freedom Vector To Projection Vector",		L"",		L"",		0x0E, L"" },
	{ L"ISECT",		L"InterSECT",									L"PPPPP",	L"",		0x0F, L"" },	
	{ L"SRP0",		L"Set Reference Point 0",						L"P",		L"",		0x10, L"" },
	{ L"SRP1",		L"Set Reference Point 1",						L"P",		L"",		0x11, L"" },
	{ L"SRP2",		L"Set Reference Point 2",						L"P",		L"",		0x12, L"" },	
	{ L"SZP0",		L"Set Zone Pointer 0",							L"E",		L"",		0x13, L"" }, /* New Name */
	{ L"SGEP0",		L"Set Glyph Element Pointer 0",					L"E",		L"",		0x13, L"" }, /* Old Name */
	{ L"SZP1",		L"Set Zone Pointer 1",				 			L"E",		L"",		0x14, L"" }, /* New Name */
	{ L"SGEP1",		L"Set Glyph Element Pointer 1",					L"E",		L"",		0x14, L"" }, /* Old Name */
	{ L"SZP2",		L"Set Zone Pointer 2",							L"E",		L"",		0x15, L"" }, /* New Name */
	{ L"SGEP2",		L"Set Glyph Element Pointer 2",					L"E",		L"",		0x15, L"" }, /* Old Name */
	{ L"SZPS",		L"Set Zone PointerS",							L"E",		L"",		0x16, L"" }, /* New Name */
	{ L"SGEPS",		L"Set Glyph Element PointerS",					L"E",		L"",		0x16, L"" }, /* Old Name */
	{ L"SLOOP",		L"Set Loop Variable",							L"V",		L"",	asm_SLOOP, L"" },
	{ L"RTG",		L"Round To Grid",								L"",		L"",		0x18, L"" },
	{ L"RTHG",		L"Round To Half Grid",							L"",		L"",		0x19, L"" },
	{ L"SMD",		L"Set Minimum Distance",						L"D",		L"",		0x1A, L"" },
	{ L"ELSE",		L"Else",										L"",		L"",		0x1B, L"" },
	{ L"JR",		L"Jump Relative",								L"L",		L"",		0x1C, L"" },
	{ L"JMPR",		L"Jump Relative",								L"L",		L"",		0x1C, L"" }, /* name documented in TrueType doc */
	{ L"SCVTCI",	L"Set Control Value Table Cut In",				L"D",		L"",		0x1D, L"" },
	{ L"SSWCI",		L"Set Single Width Cut In",						L"D",		L"",		0x1E, L"" },
	{ L"SSW",		L"Set Single Width",							L"D",		L"",		0x1F, L"" },	
	{ L"DUP",		L"Duplicate",									L"*",		L"**",		0x20, L"" },
	{ L"POP",		L"POP top element off the stack",				L"*",		L"",		0x21, L"" },
	{ L"CLEAR",		L"CLEAR entire stack",							L"&",		L"",		0x22, L"" },
	{ L"SWAP",		L"SWAP two top elements",						L"**",		L"**",		0x23, L"" },
	{ L"DEPTH",		L"DEPTH of stack",								L"",		L"V",		0x24, L"" },
	{ L"CINDEX",	L"Copy INDEXed element to the top of the stack",	L"V",	L"*",		0x25, L"" },
	{ L"MINDEX",	L"Move INDEXed element to the top of the stack",	L"V",	L"",		0x26, L"" },
	{ L"ALIGNPTS",	L"ALIGN PoinTS",									L"PP",	L"",		0x27, L"" },
	{ L"RAW",		L"Read Advance Width",							L"",		L"D",		0x28, L"" },
	{ L"UTP",		L"UnTouch Point",								L"P",		L"",		0x29, L"" },
	{ L"LOOPCALL",	L"LOOP while CALLing function",					L"VF",		L"",		0x2A, L"" },
	{ L"CALL",		L"CALL function",								L"F",		L"",		0x2B, L"" },
	{ L"FDEF",		L"Function DEFinition",							L"F",		L"",		0x2C, L"" },
	{ L"ENDF",		L"END Function definition",						L"",		L"",		0x2D, L"" },
	{ L"MDAP",		L"Move Direct Absolute Point",					L"P",		L"",		0x2E, L"R" },
	{ L"IUP",		L"Interpolate Untouched Points",				L"",		L"",		0x30, L"A" },
	{ L"SHP",		L"SHift Point",									L"p",		L"",		0x32, L"1" },
	{ L"SHC",		L"SHift Contour",								L"C",		L"",		0x34, L"1" },
	{ L"SHZ",		L"SHift Zone",									L"E",		L"",		0x36, L"1"}, /* NEW NAME */
	{ L"SHE",		L"SHift Element",								L"E",		L"",		0x36, L"1"}, /* OLD NAME */
	{ L"SHPIX",		L"SHift by fractional PIXel amount",			L"pD",		L"",		0x38, L"" },
	{ L"IP",		L"Interpolate Point",							L"p",		L"",		0x39, L"" },
	{ L"MSIRP",		L"Move Stack Indirect Relative Point",			L"PD",		L"",		0x3A, L"M" },
	{ L"ALIGNRP",	L"ALIGN Relative Point",						L"p",		L"",		0x3C, L"" },
	{ L"RTDG",		L"Round To Double Grid",						L"",		L"",		0x3D, L"" },
	{ L"MIAP",		L"Move Indirect Absolute Point",				L"PI",		L"",		0x3E, L"R" },
	{ L"NPUSHB",	L"PUSH n Bytes",								L"NB",		L"",		0x40, L"" },
	{ L"NPUSHW",	L"PUSH n Words",								L"NW",		L"",		0x41, L"" },
	{ L"WS",		L"Write Store",									L"J*",		L"",		0x42, L"" },
	{ L"RS",		L"Read Store",									L"J",		L"D",		0x43, L"" },
	{ L"WCVTP",		L"Write Control Value Table in Pixel units",	L"ID",		L"",		0x44, L"" },
	{ L"RCVT",		L"Read Control Value Table",					L"I",		L"D",		0x45, L"" },
	{ L"GC",		L"Get Coordinate value",						L"P",		L"D",		0x46, L"O" },
	{ L"SCFS",		L"Set Coordinate value From Stack",				L"PD",		L"",		0x48, L"" },
	{ L"MD",		L"Measure Distance",							L"PP",		L"D",		0x49, L"O"},
	{ L"MPPEM",		L"Measure Pixels Per EM",						L"",		L"V",		0x4B, L"" },
	{ L"MPS",		L"Measure Pointsize",							L"",		L"V",		0x4C, L"" },
	{ L"FLIPON",	L"set autoFLIP boolean ON",						L"",		L"",		0x4D, L"" },
	{ L"FLIPOFF",	L"set autoFLIP boolean OFF",					L"",		L"",		0x4E, L"" },
	{ L"DEBUG",		L"DEBUGger call",								L"*",		L"",		0x4F, L"" },
	{ L"LT",		L"Less Than",									L"**",		L"B",		0x50, L"" },
	{ L"LTEQ",		L"Less Than or EQual",							L"**",		L"B",		0x51, L"" },
	{ L"GT",		L"Greater Than",								L"**",		L"B",		0x52, L"" },
	{ L"GTEQ",		L"Greater Than or EQual",						L"**",		L"B",		0x53, L"" },
	{ L"EQ",		L"EQual",										L"**",		L"B",		0x54, L"" },
	{ L"NEQ",		L"Not EQual",									L"**",		L"B",		0x55, L"" },
	{ L"ODD",		L"ODD",											L"*",		L"B",		0x56, L"" },
	{ L"EVEN",		L"EVEN",										L"*",		L"B",		0x57, L"" },
	{ L"IF",		L"IF",											L"B",		L"",		0x58, L"" },
	{ L"EIF",		L"End IF",										L"",		L"",		0x59, L"" },
	{ L"AND",		L"AND",											L"BB",		L"B",		0x5A, L"" },
	{ L"OR",		L"OR",											L"BB",		L"B",		0x5B, L"" },
	{ L"NOT",		L"NOT",											L"B",		L"B",		0x5C, L"" },
	{ L"DELTAP1",	L"DELTA Point 1",								L"***",		L"",		0x5D, L"" },
	{ L"SDB",		L"Set Delta Base",								L"*",		L"",		0x5E, L"" },
	{ L"SDS",		L"Set Delta Shift",								L"*",		L"",		0x5F, L"" },
	{ L"ADD",		L"ADD",											L"**",		L"*",		0x60, L"" },
	{ L"SUB",		L"SUBTRACT",									L"**",		L"*",		0x61, L"" },
	{ L"DIV",		L"DIVide",										L"**",		L"*",		0x62, L"" },
	{ L"MUL",		L"MULtiply",									L"**",		L"*",		0x63, L"" },
	{ L"ABS",		L"ABSolute value",								L"*",		L"*",		0x64, L"" },
	{ L"NEG",		L"NEGate",										L"*",		L"*",		0x65, L"" },
	{ L"FLOOR",		L"FLOOR",										L"*",		L"*",		0x66, L"" },
	{ L"CEILING",	L"CEILING",										L"*",		L"*",		0x67, L"" },
	{ L"ROUND",		L"ROUND",										L"*",		L"*",		0x68, L"Cc" },
	{ L"NROUND",	L"No ROUND",									L"*",		L"*",		0x6C, L"Cc" },
	{ L"WCVTF",		L"Write Control Value Table in Funits",			L"I*",		L"",		0x70, L"" },
	{ L"DELTAP2",	L"DELTA Point 2",								L"***",		L"",		0x71, L"" },
	{ L"DELTAP3",	L"DELTA Point 3",								L"***",		L"",		0x72, L"" },
	{ L"DELTAC1",	L"DELTA Cvt 1",									L"***",		L"",		0x73, L"" },
	{ L"DELTAC2",	L"DELTA Cvt 2",									L"***",		L"",		0x74, L"" },
	{ L"DELTAC3",	L"DELTA Cvt 3",									L"***",		L"",		0x75, L"" },
	{ L"SROUND",	L"Super Round",									L"*",		L"",		0x76, L"" },
	{ L"S45ROUND",	L"Super 45 Round",								L"*",		L"",		0x77, L"" },
	{ L"JROT",		L"Jump Relative On True",						L"LB",		L"",		0x78, L"" },
	{ L"JROF",		L"Jump Relative On False",						L"LB",		L"",		0x79, L"" },
	{ L"ROFF",		L"Rounding Off",								L"",		L"",		0x7A, L"" },
	{ L"RUTG",		L"Round Up To Grid",							L"",		L"",		0x7C, L"" },
	{ L"RDTG",		L"Round Down To Grid",							L"",		L"",		0x7D, L"" },
	{ L"SANGW",		L"Set Angle Weight",							L"*",		L"",		0x7E, L"" },
	{ L"AA",		L"Adjust Angle",								L"P",		L"",		0x7F, L"" },	
	{ L"FLIPPT",	L"Flip Point",									L"p",		L"",		0x80, L"" },
	{ L"FLIPRGON",	L"Flip Range On",								L"PP",		L"",		0x81, L"" },
	{ L"FLIPRGOFF",	L"Flip Range Off",								L"PP",		L"",		0x82, L"" },
	{ L"USER83",	L"User Defined 83",								L"",		L"",		0x83, L"" },
	{ L"USER84",	L"User Defined 84",								L"",		L"",		0x84, L"" },
	{ L"SCANCTRL",	L"Scan Converter Control",						L"*",		L"",		0x85, L"" },
	{ L"SDPVTL",	L"Set Dual Projection Vector To Line",			L"PP",		L"",		0x86, L"R" },
	{ L"GETINFO",	L"GET miscellaneous INFO",						L"*",		L"*",		0x88, L"" },
	{ L"IDEF",		L"Instruction DEFinition",						L"*",		L"",		0x89, L"" },
	{ L"ROLL",		L"ROLL 3 top stack elements",					L"***",		L"***",		0x8a, L"" }, /* NEW NAME */
	{ L"ROT",		L"ROTate 3 top stack elements",					L"***",		L"***",		0x8a, L"" }, /* OLD NAME */
	{ L"MAX",		L"MAXimum",										L"**",		L"*",		0x8b, L"" },
	{ L"MIN",		L"MINimum",										L"**",		L"*",		0x8c, L"" },
	{ L"SCANTYPE",	L"Scan Type",									L"*",		L"",		0x8d, L"" },
	{ L"INSTCTRL",	L"Instruction Control",							L"*",		L"",		0x8e, L"" },		
	{ L"USER8F",	L"User Defined 8F",								L"",		L"",		0x8F, L"" },
	{ L"USER90",	L"User Defined 90",								L"",		L"",		0x90, L"" },
	{ L"USER91",	L"User Defined 91",								L"",		L"",		0x91, L"" },
	{ L"GETVARIATION", L"Get Variation",							L"",		L"V",		0x91, L"" },
	{ L"USER92",	L"User Defined 92",								L"",		L"",		0x92, L"" },
	{ L"USER93",	L"User Defined 93",								L"",		L"",		0x93, L"" },
	{ L"USER94",	L"User Defined 94",								L"",		L"",		0x94, L"" },
	{ L"USER95",	L"User Defined 95",								L"",		L"",		0x95, L"" },
	{ L"USER96",	L"User Defined 96",								L"",		L"",		0x96, L"" },
	{ L"USER97",	L"User Defined 97",								L"",		L"",		0x97, L"" },
	{ L"USER98",	L"User Defined 98",								L"",		L"",		0x98, L"" },
	{ L"USER99",	L"User Defined 99",								L"",		L"",		0x99, L"" },
	{ L"USER9A",	L"User Defined 9A",								L"",		L"",		0x9A, L"" },
	{ L"USER9B",	L"User Defined 9B",								L"",		L"",		0x9B, L"" },
	{ L"USER9C",	L"User Defined 9C",								L"",		L"",		0x9C, L"" },
	{ L"USER9D",	L"User Defined 9D",								L"",		L"",		0x9D, L"" },
	{ L"USER9E",	L"User Defined 9E",								L"",		L"",		0x9E, L"" },
	{ L"USER9F",	L"User Defined 9F",								L"",		L"",		0x9F, L"" },
	{ L"USERA0",	L"User Defined A0",								L"",		L"",		0xA0, L"" },
	{ L"USERA1",	L"User Defined A1",								L"",		L"",		0xA1, L"" },
	{ L"USERA2",	L"User Defined A2",								L"",		L"",		0xA2, L"" },
	{ L"USERA3",	L"User Defined A3",								L"",		L"",		0xA3, L"" },
	{ L"USERA4",	L"User Defined A4",								L"",		L"",		0xA4, L"" },
	{ L"USERA5",	L"User Defined A5",								L"",		L"",		0xA5, L"" },
	{ L"USERA6",	L"User Defined A6",								L"",		L"",		0xA6, L"" },
	{ L"USERA7",	L"User Defined A7",								L"",		L"",		0xA7, L"" },
	{ L"USERA8",	L"User Defined A8",								L"",		L"",		0xA8, L"" },
	{ L"USERA9",	L"User Defined A9",								L"",		L"",		0xA9, L"" },
	{ L"USERAA",	L"User Defined AA",								L"",		L"",		0xAA, L"" },
	{ L"USERAB",	L"User Defined AB",								L"",		L"",		0xAB, L"" },
	{ L"USERAC",	L"User Defined AC",								L"",		L"",		0xAC, L"" },
	{ L"USERAD",	L"User Defined AD",								L"",		L"",		0xAD, L"" },
	{ L"USERAE",	L"User Defined AE",								L"",		L"",		0xAE, L"" },
	{ L"USERAF",	L"User Defined AF",								L"",		L"",		0xAF, L"" },
	{ L"PUSHB",		L"PUSH Bytes",									L"",		L"",		0xB0, L"P" }, /* fix this */
	{ L"PUSHW",		L"PUSH Words",									L"",		L"", 		0xB8, L"P" },
	{ L"MDRP",		L"Move Direct Relative Point",					L"P",		L"", 		0xC0, L"M>RCc" },
	{ L"MIRP",		L"Move Indirect Relative Point",				L"PI",		L"", 		0xE0, L"M>RCc" },

	{ L"DLTP1",		L"DELTA Point 1, direct",						L"***",		L"",		0x15D, L"" },
	{ L"DLTP2",		L"DELTA Point 2, direct",						L"***",		L"",		0x171, L"" },
	{ L"DLTP3",		L"DELTA Point 3, direct",						L"***",		L"",		0x172, L"" },
	{ L"DLTC1",		L"DELTA Cvt 1, direct",							L"***",		L"",		0x173, L"" },
	{ L"DLTC2",		L"DELTA Cvt 2, direct",							L"***",		L"",		0x174, L"" },
	{ L"DLTC3",		L"DELTA Cvt 3, direct",							L"***",		L"",		0x175, L"" },
 	/* Jie 6-22-90 */
	/* #define FakeCode	0xAF*/
	#define FakeCode	0xFFFF	
	{ L"OFFSET",	L"Component character",					        L"V**",		 L"", 	FakeCode, L"R" },
	{ L"SOFFSET",	L"Component character",					        L"V******",	 L"", 	FakeCode, L"R" },
	{ L"ANCHOR",	L"Component character",					        L"V**",		 L"", 	FakeCode, L"" },
	{ L"SANCHOR",	L"Component character",					        L"V******",  L"", 	FakeCode, L"" },
	{ L"OVERLAP",	L"Component character",					        L"",  		L"", 	FakeCode, L"" },
	{ L"NONOVERLAP",L"Component character",					        L"",  		L"", 	FakeCode, L"" },
	{ L"USEMYMETRICS",L"Component character",					    L"",  		L"", 	FakeCode, L"" },
	{ L"SCALEDCOMPONENTOFFSET",L"Component character",				L"",  		L"", 	FakeCode, L"" },
	{ L"UNSCALEDCOMPONENTOFFSET",L"Component character",			L"",  		L"", 	FakeCode, L"" },
};

/* if we add/remove instructions, we should update tt_TOTALNUMBEROFINSTRUCTIONS in the .h file too */
#define TOTALNUMBEROFINSTRUCTIONS (sizeof(tt_instruction) / sizeof(tt_InstructionType))

const asm_BooleanTranslationType asm_booleanTranslation1[] = {
	{ L'A', L'X',	1 },
	{ L'A', L'Y',	0 },
	{ L'O', L'O',	1 },
	{ L'O', L'N',	0 },
	{ L'R', L'R',	1 },
	{ L'R', L'r',	0 },
	{ L'M', L'M',	1 },
	{ L'M', L'm',	0 },
	{ L'1', L'1',	1 },
	{ L'1', L'2',	0 },
	{ L'>', L'>',	1 },
	{ L'>', L'<',	0 },
	{ L'C', L'G',	0 },
	{ L'C', L'B',	0 },
	{ L'C', L'W',	0 },
	{ L'c', L'r',	0 },
	{ L'c', L'l',	1 },
	{ L'c', L'h',	2 },
	{ L'P', L'1',	0 },
	{ L'P', L'2',	1 },
	{ L'P', L'3',	2 },
	{ L'P', L'4',	3 },
	{ L'P', L'5',	4 },
	{ L'P', L'6',	5 },
	{ L'P', L'7',	6 },
	{ L'P', L'8',	7 }
};

#define NumberOfBooleanTranslations (sizeof(asm_booleanTranslation1) / sizeof(asm_BooleanTranslationType))

#define TOTALNUMBEROFSWITCH (sizeof(tt_CompilerSwitch) / sizeof(tt_CompilerSwitchType))



/**** label related code *****/
/**** moved and adapted from label.c */

#define  MAXJRPAIR   200			/* max. number of labels "#L100" or JR[]
								     * lines in one block (#BEGIN-#END 
								     */
#define MAXLABELLENGTH	22

typedef struct {
	wchar_t   label[MAXLABELLENGTH];			/* label  ["#L100"] */
	short     iPos;					/* instruction position from #BEGIN */
	short 	  *aPtr;				/* start ptr of argument storage */

    short     cArg;			
	wchar_t   *linePtr; /* pointer in the source to be able to display the location of an error */
}tt_jrWordType;


/*
 * labels
 * e.g
 *		#L100:
 */
typedef struct {
	short num;
	tt_jrWordType     *lab   [MAXJRPAIR];
}tt_LabelType;

/*
 * When #PUSHON
 *	   JR[],  #L100
 * or JROF[], #L100, 1
 */

typedef struct {
	short num;
	tt_jrWordType *jr    [MAXJRPAIR];
}tt_JRtype;

/*  
 *  One   #PUSH statement 
 *  eg.   #PUSH, 1,2, B1,  W2,  3,4
 */
 
#define   PUSH_ARG		256			  /* max. num of argument on one PUSH [6] */

typedef struct {
	wchar_t		label[MAXLABELLENGTH];			/* label  ["#L100"] */
	short		LocalIndex;					/* argument index from the beginning of the #PUSH */

	short	IsAByte;
	unsigned char 	  *aPtr;				/* ptr of argument storage */
	wchar_t      *linePtr;			/* pointer in the source to be able to display the location of an error */
}tt_psType;

typedef struct {
	short num;					/* number of total PUSH statements in a block */
	tt_psType *ps  [MAXJRPAIR];
}tt_PStype;

/* 
 * When #PUSHOFF
 * e.g.
 * 		JR[], ( B1 = #L100 )
 */
 
typedef struct {
	wchar_t      label[MAXLABELLENGTH];			/* Label  ["#L100"] */
	wchar_t      BWLabel[MAXLABELLENGTH];				/* BW word  ["B1"} */
	short        iPos;					/* instruction position from #BEGIN */
	wchar_t      *linePtr;  /* pointer in the source to be able to display the location of an error */
}tt_JrBWwordType;


typedef struct {
    short num;
    tt_JrBWwordType  *bw	[MAXJRPAIR];  
}tt_JrBWtype;


long TT_GetLineLength( wchar_t *p, wchar_t *endP);
long TT_GetLineLength( wchar_t *p, wchar_t *endP)
{
	long LineLength;

	LineLength = 0;
	while ( !(*p == L'\x0D') && !(*p == L'\x0A') && (p < endP)) // Allow both '\r' and '\n' to terminate lines.
	{
		LineLength++; p++;
	}
	return LineLength;
}

long TT_GetStringLength( wchar_t *p, wchar_t *endP);
long TT_GetStringLength( wchar_t *p, wchar_t *endP)
{
	long StringLength;

	StringLength = 0;
	while ( ( (*p >= L'A' && *p <= L'Z') || (*p >= L'a' && *p <= L'z') || (*p >= L'0' && *p <= L'9') ) && (p < endP))
	{
		StringLength++; p++;
	}
	return StringLength;
}

void TT_SavePushLabel(wchar_t * CurrentPtr, short numberofLocalArgs,long stringLenth,wchar_t *p,tt_PStype *PS, short * tt_error);
void TT_SavePushLabel(wchar_t * CurrentPtr, short numberofLocalArgs,long stringLenth,wchar_t *p,tt_PStype *PS, short * tt_error)
{
	short i, k;
	
	for ( k = PS->num-1; k >=0; k--) 
	{
		if (wcsncmp( PS->ps[k]->label, p, stringLenth) == 0 && (long)STRLENW(PS->ps[k]->label) == stringLenth )
		{
			*tt_error = tt_DuplicateLabel;
			return;
		}
	}


	k = PS->num;
	PS->ps[k] = ( tt_psType  *)  NewP (sizeof (tt_psType ) );
	if (PS->ps[k] == NULL)  {
		*tt_error = tt_NotEnoughMemory;
		return;
	}

	PS->ps[k]->aPtr = NULL; /* we don't know yet, depend on the optimisation of the PUSH */
	PS->ps[k]->linePtr = CurrentPtr; 

	PS->ps[k]->LocalIndex = numberofLocalArgs;		/* argument index from the beginning of the #PUSH */

	PS->ps[k]->IsAByte = false;
	if (p[0] == L'B') PS->ps[k]->IsAByte = true;

	for ( i = 0; i < stringLenth; i++) {			/* copy label */
		PS->ps[k]->label[i] = p[i];
	}
	PS->ps[k]->label[stringLenth] = '\0';

	PS->num ++;
	if ( PS->num >= MAXJRPAIR ) {
		*tt_error = tt_TooManyLabels;
	}
}


wchar_t *TT_ParseNumber( wchar_t *p, wchar_t *endP,short *Number, long * SelectionLength, short * error );
wchar_t *TT_ParseNumber( wchar_t *p, wchar_t *endP,short *Number, long * SelectionLength, short * error )
{
	short i, negatif;
	long tempNumber,maxNum;
	wchar_t *pNumStart;
	
	/* skip white space */
	while ( *p == L' ' && p < endP)
		p++;
		
	tempNumber = 0;
	negatif = false;
	i = 0;
	
	if ( *p == L'-'  && p < endP )
	{
		negatif = true;
		p++;
	}
		
	if ( *p == L'+'  && p < endP && !negatif)
	/* we accept + or - but don't want -+ together */
	{
		p++;
	}
	
	maxNum = negatif ? SHORTMAX : USHORTMAX; // allow full unicode range for glyph index B.St.

	pNumStart = p;

	if ( *p == L'0' &&  *(p+1) == L'x' && (p+2 < endP))
	{
	/* there is an hexadecimal number */
		p++; p++;
		while ( ((*p >= L'0' && *p <= L'9') || (*p >= L'A' && *p <= L'F') || (*p >= L'a' && *p <= L'f') ) && p < endP ) 
		{

			if ( *p >= L'0' && *p <= L'9' ) 
			{
				tempNumber = tempNumber * 16 + (long) *p - (long) L'0';
			} else if ( *p >= L'A' && *p <= L'F')
			{
				tempNumber = tempNumber * 16 + (long) *p - (long) L'A' + 10;
			} else if ( *p >= L'a' && *p <= L'f')
			{
				tempNumber = tempNumber * 16 + (long) *p - (long) L'a' + 10;
			}
			if (tempNumber > maxNum)
			{
				*error = tt_ParseOverflow;				
				*SelectionLength = (long)((ptrdiff_t)(p - pNumStart) + 1);
				return pNumStart;
			}
			p++;
			i++;
		}
	} else {
		while ( *p >= L'0' && *p <= L'9' && p < endP ) 
		{
			tempNumber = tempNumber * 10 + (short) *p - (short) L'0';
			if (tempNumber > maxNum)
			{
				*error = tt_ParseOverflow;				
				*SelectionLength = (long)((ptrdiff_t)(p - pNumStart) + 1);
				return pNumStart;
			}
			p++;
			i++;
		}
	}
	if (i == 0) *error = tt_UnableToParseArgument;
	
	*Number = (short)tempNumber;
	if (negatif) *Number = - (*Number);
	return p;
		
}

/*--------------------- Save to database ----------------*/

/*
 *   Parse *p = "#PUSH, 1,2, B1, W1, 3,4 \x0D"
 *   
 *   argStore   :   start ptr of argument storage
 *   *argIdex   :   return  total number of arguments [6] 
 *   if label begins with 'B', a  byte    is reserved
 *   if label begins with 'W', an integer is reserved 
 * 
 *   all the information is put  into *ps
 *   if  lableFlag = false;  memory *ps is freed
 */
wchar_t *TT_ParsePUSHandSave(tt_PStype *ps,wchar_t *CurrentPtr,wchar_t * EOLPtr,short *argStore,short *argIdex, long * SelectionLength, short * tt_error );
wchar_t *TT_ParsePUSHandSave(tt_PStype *ps,wchar_t *CurrentPtr,wchar_t * EOLPtr,short *argStore,short *argIdex, long * SelectionLength, short * tt_error )
{
	  


    (*argIdex) = 0;	

	while (CurrentPtr <= EOLPtr)
	{
			/* skip spaces */
			while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;

			if (CurrentPtr >= EOLPtr) break;
			
			/* look for the comma */
			if ( *CurrentPtr != L',' )
			{
				break; /* it could be a comment */
			}
			CurrentPtr = CurrentPtr +1;
			
			/* skip extra spaces */
			while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;

			if (CurrentPtr >= EOLPtr) break;

			if ( *CurrentPtr == L'B' || *CurrentPtr == L'W' )
			{
				long StringLength;
				if ( *CurrentPtr == L'B')      argStore[(*argIdex)] = 55;  /* reserve the space for the jump, one byte or one word */
				else if (*CurrentPtr == L'W' ) argStore[(*argIdex)] = 5555;

				StringLength = TT_GetStringLength (CurrentPtr, EOLPtr);
				if (StringLength < 2)
				{
					*tt_error = tt_VoidLabel;
					return (CurrentPtr);
				}
				
				if (StringLength >= MAXLABELLENGTH)
				{
					*tt_error = tt_LabelTooLong;
					return (CurrentPtr);
				}

				TT_SavePushLabel(CurrentPtr, *argIdex,StringLength,CurrentPtr,ps, tt_error);
				
				CurrentPtr = CurrentPtr + StringLength;
			} else {
				/* there should be a number */
				CurrentPtr = TT_ParseNumber(CurrentPtr, EOLPtr, &argStore[(*argIdex)], SelectionLength, tt_error);
				if (*tt_error != tt_NoError) return CurrentPtr;
			}
			(*argIdex) ++;
			if (*argIdex == PUSH_ARG)
			{
				*tt_error = tt_TooManyPushArgs;
				return (CurrentPtr-1);
			}
	}
	return CurrentPtr;


}


/* 
 * given label = "#L100"
 * return the index in the Label->lab[index]
 * return -1 on error
 */
static short TT_findLabelPos(tt_LabelType *Label,wchar_t *label, short * tt_error );
static short TT_findLabelPos(tt_LabelType *Label,wchar_t *label, short * tt_error )
{
	short i;
	
	for ( i = 0; i < Label->num; i++) {
		if ( ! wcscmp ( label, Label->lab[i]->label)   )  {
			return  Label->lab[i]->iPos;
		}
	}
	*tt_error = tt_LabelNotFound;
	return -1;
}

/* 
 *  repostion for  
 *            JR[], #L100     or JROF[], #L100, 1 
 *  when #PUSHON
 */
void TT_JRpushON_ReplaceLabel(tt_JRtype  *JR,tt_LabelType *Label,short *argStore, short * tt_error);
void TT_JRpushON_ReplaceLabel(tt_JRtype  *JR,tt_LabelType *Label,short *argStore, short * tt_error)
{
	short i, JRiPos,  labeliPos, delta, index;
	
	for ( i = 0; i < JR->num; i++) {
			 labeliPos = TT_findLabelPos( Label, JR->jr[i]->label, tt_error  );
			 JRiPos = JR->jr[i]->iPos;

			 delta = labeliPos - JRiPos;
			 /* if ( delta < 0  )  delta --; */

			 index = JR->jr[i]->cArg;
			 /* JR->jr[i]->aPtr[index] = delta; */
			 argStore[index] = delta; 
			 /*
			 printf(L"\n JRlabel=%s JRipos = %hd, labeliPos= %hd, delta = %hd",
			 			JR->jr[i]->label,	JRiPos, labeliPos, delta );
			 */
	}
}


/*
 * repostion the  JR[], ( B1 = #L100) type arguments
 */

void TT_JRpushOFF_ReplaceLabel(tt_JrBWtype  *JrBW,tt_PStype    *PS,tt_LabelType *Label, short * tt_error);
void TT_JRpushOFF_ReplaceLabel(tt_JrBWtype  *JrBW,tt_PStype    *PS,tt_LabelType *Label, short * tt_error)
{
	short i,  labeliPos, delta, index;
	
	for ( i = 0; i < PS->num; i++) {
		/* for each #PUSH Bn or Wn, look fot the corresponding JR */
		for ( index = 0; index < JrBW->num; index++) {
			if ( ! wcscmp( PS->ps[i]->label,JrBW->bw[index]->BWLabel) )  break;
		}
			
		labeliPos = TT_findLabelPos( Label, JrBW->bw[index]->label, tt_error  );
		delta = labeliPos - JrBW->bw[i]->iPos;
		
		if ( (*PS->ps[i]).IsAByte && (delta > 255 ))
		{
			*tt_error = tt_JumpTooBigForByte;
		}
		if ( (*PS->ps[i]).IsAByte && ( delta < 0  ))
		{
			*tt_error = tt_JumpNegativeForByte;
		}
		
		if ( (*PS->ps[i]).IsAByte )
		{
			*(*PS->ps[i]).aPtr = (unsigned char) delta;
		} else {
			short * sTempPtr;
			sTempPtr = (short *) (*PS->ps[i]).aPtr;
			*sTempPtr = SWAPW(delta); // B.St.
		}
			 
	}
}


wchar_t * TT_FindLabelError(tt_PStype    *PS, tt_JrBWtype  *JrBW, tt_JRtype  *JR, tt_LabelType *Label, wchar_t * CurrentPtr, long * SelectionLength, short * tt_error) ;
wchar_t * TT_FindLabelError(tt_PStype    *PS, tt_JrBWtype  *JrBW, tt_JRtype  *JR, tt_LabelType *Label, wchar_t * CurrentPtr, long * SelectionLength, short * tt_error) 
{				
	short i, k;
	
	/* look through the push on JR */
 	if ( JR->num != 0) 
 	{
		for ( k  = 0; k < JR->num; k++) {
			for ( i = 0; i < Label->num; i++) {
				if ( ! wcscmp( JR->jr[k]->label,Label->lab[i]->label) )  break;
			}
			if ( i >= Label->num ) {
				*tt_error = tt_LabelNotFound;
				*SelectionLength = (long)STRLENW(JR->jr[k]->label);
				return JR->jr[k]->linePtr;
			}
		}
	}

	/* look through the push off JR */
 	if ( JrBW->num != 0) 
 	{
		for ( k  = 0; k < JrBW->num; k++) {
			for ( i = 0; i < Label->num; i++) {
				if ( ! wcscmp( JrBW->bw[k]->label,Label->lab[i]->label) )  break;
			}
			if ( i >= Label->num ) {
				*tt_error = tt_LabelNotFound;
				*SelectionLength = (long)STRLENW(JrBW->bw[k]->label);
				return JrBW->bw[k]->linePtr;
			}

			/* look through the #PUSH,Bn,Wn for the corresponding label */
			for ( i = 0; i < PS->num; i++) {
				if ( ! wcscmp( JrBW->bw[k]->BWLabel,PS->ps[i]->label) )  break;
			}
			if ( i >= PS->num ) {
				*tt_error = tt_LabelNotFound;
				*SelectionLength = (long)STRLENW(JrBW->bw[k]->BWLabel);
				return JrBW->bw[k]->linePtr;
			}
		}
	}
	/* check if every #PUSH,Bn,Wn has a corresponding JR[], */
 	if ( PS->num != 0) 
 	{
		for ( k  = 0; k < PS->num; k++) {
			for ( i = 0; i < JrBW->num; i++) {
				if ( ! wcscmp( PS->ps[k]->label,JrBW->bw[i]->BWLabel) )  break;
			}
			if ( i >= JrBW->num ) {
				*tt_error = tt_LabelNotFound;
				*SelectionLength = (long)STRLENW(PS->ps[k]->label);
				return PS->ps[k]->linePtr;
			}
		}
	}
	return CurrentPtr;
}

/*
 * Save Label information into struct label_LabelType *Label.
 * input:
 *  char *p = #L100:"
 * numberofArgs			: number of arguments so far (from #BEGIN).
 * numberofInstructions : number of instructions so far (from #BEGIN).
 * stringLenth			: length of "#L100:"
 *
 * all the information is put into Label
 */

				
void TT_SaveLabel(short numberofArgs,short numberofInstructions,long stringLenth,wchar_t *p,tt_LabelType *Label, short * tt_error);
void TT_SaveLabel(short numberofArgs,short numberofInstructions,long stringLenth,wchar_t *p,tt_LabelType *Label, short * tt_error)
{
	short i, k;
	
	for ( k = Label->num-1; k >=0; k--) 
	{
		if (wcsncmp( Label->lab[k]->label, p, stringLenth) == 0  && (long)STRLENW(Label->lab[k]->label) == stringLenth )
		{
			*tt_error = tt_DuplicateLabel;
			return;
		}
	}


	k = Label->num;
	Label->lab[k] = ( tt_jrWordType  *)  NewP (sizeof (tt_jrWordType ) );
	if (Label->lab[k] == NULL)  {
		*tt_error = tt_NotEnoughMemory;
		return;
	}

	
	Label->lab[k]->linePtr = p;
	Label->lab[k]->iPos = numberofInstructions;

	for ( i = 0; i < stringLenth; i++) {			/* copy label */
		Label->lab[k]->label[i] = p[i];
	}
	Label->lab[k]->label[stringLenth] = '\0';

	Label->num ++;
	if ( Label->num >= MAXJRPAIR ) {
		*tt_error = tt_TooManyLabels;
	}
}

//void TT_SavePSLabel(short numberofArgs,short numberofInstructions,short stringLenth,char *p,tt_LabelType *Label, short * tt_error);

wchar_t * TT_SaveJR(short numberofArgs,short numberofInstructions,wchar_t * CurrentPtr, wchar_t *LabelPtr,long stringLenth, 
			wchar_t *BWLabelPtr,short BWstringLenth,tt_JRtype *JRList,tt_JrBWtype  *JrBW, short *aPtr,long * SelectionLength, short * tt_error);
wchar_t * TT_SaveJR(short numberofArgs,short numberofInstructions,wchar_t * CurrentPtr, wchar_t *LabelPtr,long stringLenth, 
			wchar_t *BWLabelPtr,short BWstringLenth,tt_JRtype *JRList,tt_JrBWtype  *JrBW, short *aPtr,long * SelectionLength, short * tt_error)
{
	short i, k;
	
	for ( k = JRList->num-1; k >=0; k--) 
	{
		if (wcsncmp( JRList->jr[k]->label, LabelPtr, stringLenth) == 0  && (long)STRLENW(JRList->jr[k]->label) == stringLenth  )
		{
			*tt_error = tt_DuplicateLabel;
			*SelectionLength = stringLenth;
			return LabelPtr;
		}
	}
	
	if (BWstringLenth != 0)
	{

		for ( k = JrBW->num-1; k >=0; k--) 
		{
			if (wcsncmp( JrBW->bw[k]->BWLabel, BWLabelPtr, BWstringLenth) == 0  && (long)STRLENW(JrBW->bw[k]->BWLabel) == stringLenth )
			{
				*tt_error = tt_DuplicateLabel;
				*SelectionLength = BWstringLenth;
				return LabelPtr;
			}
		}
			
		k = JrBW->num;
		JrBW->bw[k] = ( tt_JrBWwordType  *)  NewP (sizeof (tt_JrBWwordType ) );
		if (JrBW->bw[k] == NULL)  {
			*tt_error = tt_NotEnoughMemory;
			*SelectionLength = stringLenth;
			return LabelPtr;
		}
	
		JrBW->bw[k]->linePtr = LabelPtr;
		JrBW->bw[k]->iPos = numberofInstructions;

		for ( i = 0; i < stringLenth; i++) {			/* copy label */
			JrBW->bw[k]->label[i] = LabelPtr[i];
		}
		JrBW->bw[k]->label[stringLenth] = '\0';

		for ( i = 0; i < BWstringLenth; i++) {			/* copy label */
			JrBW->bw[k]->BWLabel[i] = BWLabelPtr[i];
		}
		JrBW->bw[k]->BWLabel[BWstringLenth] = '\0';

		JrBW->num ++;
		if ( JrBW->num >= MAXJRPAIR ) {
			*tt_error = tt_TooManyLabels;
			*SelectionLength = stringLenth;
			return LabelPtr;
		}

	} else {

		/* no Bn or Wn label */
		k = JRList->num;
		JRList->jr[k] = ( tt_jrWordType  *)  NewP (sizeof (tt_jrWordType ) );
		if (JRList->jr[k] == NULL)  {
			*tt_error = tt_NotEnoughMemory;
			*SelectionLength = stringLenth;
			return LabelPtr;
		}


		JRList->jr[k]->aPtr = aPtr;
		JRList->jr[k]->cArg  = numberofArgs; 

	
		JRList->jr[k]->linePtr = LabelPtr;
		JRList->jr[k]->iPos = numberofInstructions;

		for ( i = 0; i < stringLenth; i++) {			/* copy label */
			JRList->jr[k]->label[i] = LabelPtr[i];
		}
		JRList->jr[k]->label[stringLenth] = '\0';

		JRList->num ++;
		if ( JRList->num >= MAXJRPAIR ) {
			*tt_error = tt_TooManyLabels;
			*SelectionLength = stringLenth;
			return LabelPtr;
		}
	}
	return CurrentPtr;
}


void TT_FreeAllLabelMemory( tt_PStype    *PS, tt_JrBWtype  *JrBW, tt_LabelType *Label, tt_JRtype    *JR);
void TT_FreeAllLabelMemory( tt_PStype    *PS, tt_JrBWtype  *JrBW, tt_LabelType *Label, tt_JRtype    *JR)
{
	
	short k;	
		
	for ( k = Label->num-1; k >=0; k--)   DisposeP((void**)&Label->lab[k]);
	for ( k = JR->num-1; k >=0; k--)      DisposeP((void**)&JR->jr[k] );
	for ( k = JrBW->num-1; k >=0; k--)    DisposeP((void**)&JrBW->bw[k]);
	for ( k = PS->num-1; k >=0; k--)      DisposeP((void**)&PS->ps[k]);

   	 DisposeP((void**)&Label);
   	 DisposeP((void**)&JR );
   	 DisposeP((void**)&JrBW);
     DisposeP((void**)&PS);
}

/**** end of label related code ****/

short TT_ExpandArgs( const wchar_t *src,wchar_t * dest, short *loop , short * tt_error);
short TT_ExpandArgs( const wchar_t *src,wchar_t * dest, short *loop, short * tt_error )
// char *src, *dest;
// short *loop;
{
	wchar_t c, *base;
	short i;
	short usesLoop;
	
	usesLoop = false;
	base = dest;
	do {
		i = *loop;
		c = *src++;
		if ( c >= L'a' && c <= L'z' ) {
			usesLoop = true;
			c -= L'a' - L'A'; /* turn into uppercase */
			while ( i > 1 && c && ( dest-base <= ARGTYPEBUFFER_SIZE )) {
				i--;
				*dest++ = c;
			}
		}
		if ( dest-base > ARGTYPEBUFFER_SIZE ) {
			*tt_error = tt_SLOOPArgumentBufferTooSmall;
			return usesLoop;
		}
		
	} while ( (*dest++ = c) != 0);
	*loop = i;
	
	return usesLoop;
}

/* function to be called before TT_Compile to set the maximum values for the range check */
void TT_SetRangeCheck(short LastContNb, short LastPointNb, short LastZoneNb, short LastFunctionNb, short LastCvtNumber, short LastStorageIndex);
void TT_SetRangeCheck(short LastContNb, short LastPointNb, short LastElementNb, short LastFunctionNb, short LastCvtNumber, short LastStorageIndex)

{
	/* Point Number */
	asm_ppDescription1[0].highestValidValue = LastPointNb;
	
	/* Element Number */
	if ( LastElementNb == 0 ) {
		asm_ppDescription1[1].lowestValidValue = 1;
		asm_ppDescription1[1].highestValidValue = 1; 
	} else {
		asm_ppDescription1[1].lowestValidValue = 0;
		asm_ppDescription1[1].highestValidValue = LastElementNb;
	 }

	/* function number: */
	asm_ppDescription1[5].highestValidValue = LastFunctionNb;

	
	/*Index into control value table:*/
	asm_ppDescription1[6].highestValidValue = LastCvtNumber;
	
	/*Index into storage area*/
	asm_ppDescription1[7].highestValidValue = LastStorageIndex;
	 
	/*Number of Contours*/
	asm_ppDescription1[10].highestValidValue = LastContNb;
}


wchar_t * TT_ReadInstructionBooleans (wchar_t * CurrentPtr, wchar_t * EOLPtr, short InstructionIndex, unsigned short *InstructionCode, long * Selectionlength, short * tt_error);
wchar_t * TT_ReadInstructionBooleans (wchar_t * CurrentPtr, wchar_t * EOLPtr, short InstructionIndex, unsigned short *InstructionCode, long * Selectionlength, short * tt_error)
{
/* the instruction code may be changed according to the boolean flags */
	short booleanCount, booleanShift, NBOfBooleans, found,  k;
	wchar_t * tempP;
	if 	( *CurrentPtr != L'[' /* ] for balance */ || CurrentPtr >=  EOLPtr) 
	{ 
		*tt_error = tt_BooleanFlagsMissing;
		*Selectionlength = 1;
		return CurrentPtr;
	};
	tempP = CurrentPtr;
	CurrentPtr ++;
	NBOfBooleans = (short)STRLENW(tt_instruction[InstructionIndex].booleans) ;
	booleanShift = NBOfBooleans;

	for (booleanCount = 0;
		// for balance '[' 
				booleanCount < NBOfBooleans && *CurrentPtr != L']' && CurrentPtr < EOLPtr ; 
				booleanCount++ )
	{
		booleanShift--;
		for ( found = k = 0; k < NumberOfBooleanTranslations; k++ ) {
			if ( asm_booleanTranslation1[k].type == tt_instruction[InstructionIndex].booleans[booleanCount] &&
								 (asm_booleanTranslation1[k].code) == *CurrentPtr ) 
			{
				found = 1;
				*InstructionCode = *InstructionCode + (asm_booleanTranslation1[k].result << booleanShift);
			}
		}
		if (!found)
		{
			*tt_error = tt_UnrecognizedBoolean;
			*Selectionlength = 1;
			return CurrentPtr;
		}
			
		CurrentPtr++;
	}
			
	if (booleanCount != NBOfBooleans)
	{
		*tt_error = tt_WrongNumberOfBoolean;
		*Selectionlength = (short)(CurrentPtr-tempP)+1;
		return tempP;
	};
		
	if 	( (CurrentPtr >=  EOLPtr) || (*CurrentPtr == L',' ) ) 
	{ 
		*tt_error = tt_MissingClosingBracket;
		*Selectionlength = (short)(CurrentPtr-tempP);
		return tempP;
	};

	if 	( /* [ for balance */  *CurrentPtr != L']' ) 
	{ 
		*tt_error = tt_TooManyBooleans;
		*Selectionlength = (short)(CurrentPtr-tempP)+1;
		return tempP;
	};
	CurrentPtr ++;
	return CurrentPtr;
}


short TT_DeltaLevel( unsigned short opCode );
short TT_DeltaLevel( unsigned short opCode )
// register unsigned char opCode;
{
	short level;
	
	level = 0;
	if ( opCode == 0x5d || opCode == 0x73 || opCode == 0x15d || opCode == 0x173) {
		level = 0;
	} else if ( opCode == 0x71 || opCode == 0x74 || opCode == 0x171 || opCode == 0x174) {
		level = 1;
	} else if ( opCode == 0x72 || opCode == 0x75 || opCode == 0x172 || opCode == 0x175) {
		level = 2;
	}
	return level;
}

void TT_CompileDelta( tt_deltaPType dArr[], short count, unsigned short insCode, short args[], short *argCount);
void TT_CompileDelta(  tt_deltaPType dArr[], short deltaCount, unsigned short insCode, short args[], short *argCount )
{
	short i, tmp, sCount, valid;
	unsigned char argCode;
	
	valid = 1;
	sCount = 0;
	
	for ( i = 0; i < deltaCount; i++) {
	
		argCode = (unsigned char) dArr[i].relppem;
	
		/* claudebe, already checked in the parser 	
		if ( argCode < 0 || argCode > 15 ) {
			*tt_error = tt_DeltaOORangePpem;
			return;
		} */
		
		argCode <<= 4;
		
		/* in direct delta denominator == 1, otherwise we already tested that it is zero */
		
		tmp = dArr[i].num;

		tmp += 8;
		if ( tmp >= 8 ) tmp--;  /* there is no code for a null deplacement */
		
		/*  DELTASHIFT in the DELTAxx command is  constant 3, meaning delta from -8/8 to 8/8 by 1/8 steps 
			use direct delta DLTxx for other shift or base */
		
		if (tmp > 15) tmp = 15;
		if (tmp < 0) tmp = 0;
		
		argCode += tmp;
		args[ sCount++ ] = argCode;
		args[ sCount++ ] = dArr[i].point;
		
	}
	/* claudebe 11/9/95, raid #73, DELTA without argument in PUSHOFF mode shouldn't produce any argument */
	if (deltaCount != 0)
		args[ sCount++ ] = deltaCount;
	*argCount = sCount;
}

void TT_SortAndCombineDeltas(tt_deltaPType  dArr[], short *countPtr );
void TT_SortAndCombineDeltas(tt_deltaPType  dArr[], short *countPtr )
/* original Sampo code, except getting rid of the float and there is no need any more to test for zero div and allow delta of more than one pixel */
{
	short i, j, change;
	short count;
	
	count = *countPtr;
	/* Now sort the deltas */
	for ( change = true; change; ) {
		tt_deltaPType tmpDelta;
		
		change = false;
		for ( i = count-2; i >= 0; i-- ) {
			if ( (dArr[i].relppem > dArr[i+1].relppem) || ( dArr[i].relppem == dArr[i+1].relppem && dArr[i].point > dArr[i+1].point )  ) {
				TT_memSwap( (char *)&dArr[i], (char *)&dArr[i+1], (char *)&tmpDelta, sizeof( tt_deltaPType ) );
				change = true;
			}
		}
	}


	/* Combine deltas */
	for ( change = true; change; ) {
		change = false;
		for ( i = count-1; i >= 0; i-- ) {
			if (dArr[i].num == 0) {
			/* we should almost flag this as an error */
				for ( j = i; j < count-1; j++ ) {
					memcpy( (char *)&dArr[j], (char *)&dArr[j+1], sizeof( tt_deltaPType ) );
				}
				change = true;
				count--;
			} else if ( i < (count-1) && dArr[i].relppem == dArr[i+1].relppem && dArr[i].point == dArr[i+1].point && dArr[i].denum == dArr[i+1].denum ) {
				dArr[i].num += dArr[i+1].num;
				for ( j = i+1; j < count-1; j++ ) {
					memcpy( (char *)&dArr[j], (char *)&dArr[j+1], sizeof( tt_deltaPType ) );
				}
				change = true;
				count--;
			}
		}
	}
	*countPtr = count;
}

short TT_IsDelta( unsigned short opCode );
short TT_IsDelta( unsigned short opCode )
// register unsigned char opCode;
{
	return ( opCode == 0x5d || ( opCode >= 0x71 && opCode <= 0x75) ||  opCode == 0x15d ||  (opCode >= 0x171 && opCode <= 0x175) );
}

short TT_IsPushBW( unsigned short opCode );
short TT_IsPushBW( unsigned short opCode )
// register unsigned char opCode;
{
	return ( ( opCode >= 0xB0 && opCode <= 0xBF) ||  opCode == 0x40 ||  opCode == 0x41 );
}

short TT_IsPushB( unsigned short opCode );
short TT_IsPushB( unsigned short opCode )
// register unsigned char opCode;
{
	return ( ( opCode >= 0xB0 && opCode <= 0xB7) ||  opCode == 0x40  );
}

short TT_IsNPushBW( unsigned short opCode );
short TT_IsNPushBW( unsigned short opCode )
// register unsigned char opCode;
{
	return ( opCode == 0x40 ||  opCode == 0x41 );
}

short TT_IsDirectDelta( unsigned short opCode );
short TT_IsDirectDelta( unsigned short opCode )
// register unsigned char opCode;
{
	return ( opCode == 0x15d || ( opCode >= 0x171 && opCode <= 0x175 ) );
}

short TT_IsDeltaC( unsigned short opCode );
short TT_IsDeltaC( unsigned short opCode )
// register unsigned char opCode;
{
	return (  (opCode >= 0x73 && opCode <= 0x75) || (opCode >= 0x173 && opCode <= 0x175)  );
}

short TT_IsJR( unsigned short opCode );
short TT_IsJR( unsigned short opCode )
// register unsigned short opCode;
{
	return ( opCode == 0x1C || opCode == 0x78 || opCode == 0x79 );
}

short TT_IsBlockInstruction( unsigned short opCode );
short TT_IsBlockInstruction( unsigned short opCode )
// register unsigned char opCode;
{
	/* JR or IF, ELSE, FDEF , IDEF */
	return ( opCode == 0x1C || opCode == 0x78 || opCode == 0x79 || opCode == 0x2C || opCode == 0x58 || opCode == 0x89 || opCode == 0x1B );
}

short TT_IsIDEF_FDEFInstruction( unsigned short opCode );
short TT_IsIDEF_FDEFInstruction( unsigned short opCode )
// register unsigned char opCode;
{
	/*  FDEF or IDEF */
	return ( opCode == 0x2C || opCode == 0x89 );
}

wchar_t * TT_DecodeDeltaP (wchar_t * CurrentPtr, wchar_t * EOLPtr, wchar_t * EndPtr, short InstructionIndex, unsigned short InstructionCode, short *deltaCount, tt_deltaPType  dArr[], long * Selectionlength, short * tt_error);
wchar_t * TT_DecodeDeltaP (wchar_t * CurrentPtr, wchar_t * EOLPtr, wchar_t * EndPtr, short InstructionIndex, unsigned short InstructionCode, short *deltaCount, tt_deltaPType  dArr[], long * SelectionLength, short * tt_error)
{
	short tempNum;
	wchar_t * tempP;
	unsigned char argCode;
	*deltaCount = 0;
	
	/* opening bracket */
	if 	( *CurrentPtr != L'[' /* ] for balance */ || CurrentPtr >=  EOLPtr) 
	{ 
		*tt_error = tt_DeltaListMissing;
		*SelectionLength = 1;
		return CurrentPtr;
	};
	CurrentPtr++;

	/* skip spaces */
	while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
			CurrentPtr++;

	while (CurrentPtr == EOLPtr && CurrentPtr < EndPtr) {
		CurrentPtr++;
		/* the DELTA command is an exception, the only command that can be spread over several lines,
		  the previous compiler was allowing this and users were using this, so I need to be backwards compatible! */
		EOLPtr = CurrentPtr + TT_GetLineLength(CurrentPtr, EndPtr);
		/* skip spaces  */
		while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;
	}

	while ( /* [ for balance */ *CurrentPtr != L']'  && CurrentPtr <  EOLPtr) 
	{
		if (*deltaCount > MAXDELTA)
		{
			*tt_error = tt_TooManyDeltas;
			return CurrentPtr;
		};
		
		/* round bracket */
		if 	( *CurrentPtr != L'(' /* ) for balance */ || CurrentPtr >=  EOLPtr) 
		{ 
			*tt_error = tt_DeltaOpeningParenthesisMissing;
			*SelectionLength = 1;
			return CurrentPtr;
		};
		CurrentPtr++;
	
		tempP = CurrentPtr;
		CurrentPtr = TT_ParseNumber(CurrentPtr, EOLPtr, &tempNum, SelectionLength, tt_error);
	
		if (*tt_error != tt_NoError) return CurrentPtr;
		
		if (TT_IsDeltaC (InstructionCode))
		{
			if (tempNum < asm_ppDescription1[6].lowestValidValue || tempNum > asm_ppDescription1[6].highestValidValue) 
			{
				*tt_error = tt_CVTIndexOutOfRange;
				*SelectionLength = (long) (CurrentPtr - tempP);
				return tempP;
			}
		} else {
			if (tempNum < asm_ppDescription1[0].lowestValidValue || tempNum > asm_ppDescription1[0].highestValidValue) 
			{
				*tt_error = tt_PointNbOutOfRange;
				*SelectionLength = (long) (CurrentPtr - tempP);
				return tempP;
			}
		}
	
		dArr[*deltaCount].point = tempNum;

		/* skip spaces */
		while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;

		if 	( *CurrentPtr != L'@'  || CurrentPtr >=  EOLPtr) 
		{ 
			*tt_error = tt_DeltaAtSignMissing;
			*SelectionLength = 1;
			return CurrentPtr;
		};
		CurrentPtr++;
		
		tempP = CurrentPtr;
		CurrentPtr = TT_ParseNumber(CurrentPtr, EOLPtr, &tempNum, SelectionLength, tt_error);
	
		if (*tt_error != tt_NoError) return CurrentPtr;

		if (TT_IsDirectDelta(InstructionCode))
		{
			argCode = (unsigned char) tempNum;
		} else {
			argCode = (unsigned char)(tempNum - DELTABASE - 16 * TT_DeltaLevel( InstructionCode ));
		}
		
		if ( argCode < 0 || argCode > 15 ) {
			*tt_error = tt_PointSizeOutOfRange;
			*SelectionLength = (long) (CurrentPtr - tempP);
			return tempP;
		}
			
	
		dArr[*deltaCount].relppem = argCode;
	
		tempP = CurrentPtr;
		CurrentPtr = TT_ParseNumber(CurrentPtr, EOLPtr, &tempNum, SelectionLength, tt_error);

		if (*tt_error != tt_NoError) return CurrentPtr;

		dArr[*deltaCount].num = tempNum;
	
		/* skip spaces */
		while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;


		if (TT_IsDirectDelta(InstructionCode))
		{
			dArr[*deltaCount].denum = 1;
		} else {
			if 	( *CurrentPtr != L'/'  || CurrentPtr >=  EOLPtr) 
			{ 
				*tt_error = tt_DeltaDenominatorMissing;
				*SelectionLength = 1;
				return CurrentPtr;
			};
			CurrentPtr++;

			tempP = CurrentPtr;
			CurrentPtr = TT_ParseNumber(CurrentPtr, EOLPtr, &tempNum, SelectionLength, tt_error);

			if (*tt_error != tt_NoError) return CurrentPtr;
		
			if (tempNum != 8) /* ######## claudebe nobody should use a different denominator (DELTA command with fixed shift) */
			{
				*tt_error = tt_DeltaWrongDenominator;
				*SelectionLength = (long) (CurrentPtr - tempP);
				return tempP;
			};
	
			dArr[*deltaCount].denum = tempNum;
		}


		*deltaCount = *deltaCount + 1;
	
		/* skip spaces */
		while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;

		/* round bracket */
		if /* ( for balance */	( *CurrentPtr != L')'  || CurrentPtr >=  EOLPtr) 
		{ 
			*tt_error = tt_DeltaClosingParenthesisMissing;
			*SelectionLength = 1;
			return CurrentPtr;
		};
		CurrentPtr++;

		/* skip spaces */
		while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;
				
		while (CurrentPtr == EOLPtr && CurrentPtr < EndPtr) {
			CurrentPtr++;
		/* the DELTA command is an exception, the only command that can be spread over several lines,
		   the previous compiler was allowing this and users were using this, so I need to be backwards compatible! */
			EOLPtr = CurrentPtr + TT_GetLineLength(CurrentPtr, EndPtr);
			/* skip spaces  */
			while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
					CurrentPtr++;
		}
	}

	/* closing bracket */
	if 	( /* [ for balance */  *CurrentPtr != L']' || CurrentPtr >=  EOLPtr) 
	{ 
		*tt_error = tt_DeltaClosingBracketMissing;
		*SelectionLength = 1;
		return CurrentPtr;
	};
	CurrentPtr ++;

	/* Now sort the deltas */
	TT_SortAndCombineDeltas( dArr, deltaCount );
	return CurrentPtr;

}

wchar_t * TT_ReadInstructionParameters (wchar_t * CurrentPtr, wchar_t * EOLPtr, short InstructionIndex, unsigned char InstructionCode, 
				asm_PushAndPopDescriptionType asm_ppDescription[], short pushOn, wchar_t * ArgTypeBuffer, short  *argc,short *args,short  *argc2,wchar_t *args2,
				wchar_t ** LabelHandle, short * LabelLength, wchar_t ** BWLabelHandle, short * BWLabelLength, long * SelectionLength, short * MaxFunctionDefs, 
				/* offset in the instruction stream to the instruction corresponding to the cursor position, used for trace mode */
				short * BinaryOffset, 
				/* for the DovMan partial compilation feature that flash points referenced by the current command */
				tt_flashingPoints * flashingPoints,
				short * tt_error);
wchar_t * TT_ReadInstructionParameters (wchar_t * CurrentPtr, wchar_t * EOLPtr, short InstructionIndex, unsigned char InstructionCode, 
				asm_PushAndPopDescriptionType asm_ppDescription[], short pushOn, wchar_t * ArgTypeBuffer, short  *argc,short *args,short  *argc2,wchar_t *args2,
				wchar_t ** LabelHandle, short * LabelLength, wchar_t ** BWLabelHandle, short * BWLabelLength, long * SelectionLength, short * MaxFunctionDefs, 
				/* offset in the instruction stream to the instruction corresponding to the cursor position, used for trace mode */
				short * BinaryOffset, 
				/* for the DovMan partial compilation feature that flash points referenced by the current command */
				tt_flashingPoints * flashingPoints,
				short * tt_error)
{
	short	argindex, argNb;
	wchar_t * tempP = nullptr;
	*argc = 0;
	*argc2 = 0;
	argindex = 0;
	*LabelLength = 0;
	*BWLabelLength = 0;

	if (*BinaryOffset == -1)
	{
		flashingPoints->NumberOfFlashingPoint = 0;
	}
	
	if (pushOn || TT_IsPushBW(InstructionCode)) 
	{
		argNb = (short)STRLENW(ArgTypeBuffer);

		if ( InstructionCode == 0x2A || InstructionCode == 0x2B ) 
		{ /* function calls, we don't know what should be the number of arguments before processing */
			argNb = 255;
		};
		
		if (TT_IsPushBW(InstructionCode))
		{
			if (TT_IsNPushBW(InstructionCode))
			{
				argNb = 1;
				ArgTypeBuffer[0] = L'A';
				ArgTypeBuffer[1] = 0;
				
			} else {
				short k;
				if ( InstructionCode >= 0xB0 && InstructionCode <= 0xB7)
				{
					/* PUSHB[] */
					argNb = InstructionCode - 0xB0 + 1;
					for (k = 0; k < argNb; k++)
					{
						ArgTypeBuffer[k] = L'A';
					}
					ArgTypeBuffer[k] = 0;
					
				} else {
					/* PUSHW[] */
					argNb = InstructionCode - 0xB8 + 1;
					for (k = 0; k < argNb; k++)
					{
						ArgTypeBuffer[k] = L'H';
					}
					ArgTypeBuffer[k] = 0;
				}
			}
		}
		for (argindex=0;argindex<argNb;argindex++) {
			/* skip spaces */
			
			args2[argindex] = 0;

			while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;

			if (CurrentPtr >= EOLPtr) break;

			if ( *CurrentPtr != L',' )
			{
				break; /* it could be a comment */
			}
			CurrentPtr = CurrentPtr + 1;
			
			/* skip extra spaces */
			while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;

			if (CurrentPtr >= EOLPtr) break;

			if ( *CurrentPtr == L'*' )
			{
				if (TT_IsPushBW(InstructionCode))
				{
					*tt_error = tt_WildCardInPush;
					return (CurrentPtr);
				}
				/* there is a wildcard */
				args2[argindex] = L'*';
				*argc2 = *argc2 + 1;
				CurrentPtr++;
			} else if ( (argNb != 255) && ( ArgTypeBuffer[argindex] == L'L' ) && (*CurrentPtr == L'#')) 
			{
				long StringLength;
				CurrentPtr++;
				/* we are looking for a label in a JR instruction */
				StringLength = TT_GetStringLength (CurrentPtr, EOLPtr);
				if ( StringLength == 0 )
				{
					*tt_error = tt_VoidLabel;
					return (CurrentPtr);
				}

				if (StringLength >= MAXLABELLENGTH)
				{
					*tt_error = tt_LabelTooLong;
					return (CurrentPtr);
				}

				*LabelLength = (short)StringLength;
				*LabelHandle = CurrentPtr;

				args2[argindex] = L'L'; /* L for Label */
				*argc2 = *argc2 + 1;
				CurrentPtr = CurrentPtr + StringLength;
			} else if ( (argNb != 255) &&  ( ArgTypeBuffer[argindex] == L'F' ) && (InstructionCode == 0x2C )) 
			{
				CurrentPtr = TT_ParseNumber(CurrentPtr, EOLPtr, &args[argindex], SelectionLength, tt_error);
				if (*tt_error != tt_NoError) return CurrentPtr;
				/* function definition, we need to update MaxFunctionDefs */
				if (args[argindex] > *MaxFunctionDefs) *MaxFunctionDefs = args[argindex] ;
				*argc = *argc + 1;
			} else {
				short foundPP, PPindex;

				/* there should be a number */
				tempP = CurrentPtr;
				CurrentPtr = TT_ParseNumber(CurrentPtr, EOLPtr, &args[argindex],  SelectionLength, tt_error);
				if (*tt_error != tt_NoError) return CurrentPtr;

				if (argNb != 255) 
				{
					if ( ArgTypeBuffer[argindex] == L'P' && *BinaryOffset == -1 && flashingPoints->NumberOfFlashingPoint < tt_MAXFLASHINGPOINTS) 
					{
						flashingPoints->flashingPoints[flashingPoints->NumberOfFlashingPoint++] = args[argindex];
					}
					foundPP = 0;    /* Have not found it yet... */
					for (PPindex=0;(PPindex<NumberOfPPEntries && !foundPP ) ; PPindex++) {
						if (ArgTypeBuffer[argindex] == asm_ppDescription[PPindex].type) {
							foundPP=1;   /* Exit loop for speed's sake */
							if ((args[argindex] < asm_ppDescription[PPindex].lowestValidValue) || (args[argindex] > asm_ppDescription[PPindex].highestValidValue)) {
								*SelectionLength = (long) (CurrentPtr - tempP);
								CurrentPtr = tempP;
								if ( ArgTypeBuffer[argindex] == L'P' ) 
								{
									*tt_error = tt_PointNbOutOfRange;
								} else if ( ArgTypeBuffer[argindex] == L'I' ) 
								{
									*tt_error = tt_CVTIndexOutOfRange;
								} else if ( ArgTypeBuffer[argindex] == L'J' ) 
								{
									*tt_error = tt_StorageIndexOutOfRange;
								} else if ( ArgTypeBuffer[argindex] == L'C' ) 
								{	
									*tt_error = tt_ContourNbOutOfRange;
								} else {
									*tt_error = tt_ArgumentOutOfRange;
								}/*if */ 
							} /* if */
						} /*for */
					} /*for*/
					
					if ( ArgTypeBuffer[argindex] == L'A' && argindex == 0)	
					/* we are in a NPUSH and know now the amount of arguments */
					{
						short k;
						argNb = argNb + args[argindex];
						if ( InstructionCode == 0x40 )
						{
							/* NPUSHB[] */
							for (k = 1; k <= args[argindex]; k++)
							{
								ArgTypeBuffer[k] = L'A';
							}
							ArgTypeBuffer[k] = 0;
					
						} else {
							/* NPUSHW[] */
							for (k = 1; k <= args[argindex]; k++)
							{
								ArgTypeBuffer[k] = L'H';
							}
							ArgTypeBuffer[k] = 0;
						}
					}		
				} /*if*/

				*argc = *argc + 1;
				if (*tt_error != tt_NoError) return CurrentPtr;
			}

			
		}


		if ( InstructionCode == 0x2A || InstructionCode == 0x2B ) 
		{ /* function calls, we don't know waht should be the number of arguments before processing */
			/* we need to check the function number,
			     FUTURE : we may want to check the function arguments here */
			short foundPP, PPindex;
			
			if ((InstructionCode == 0x2B && argindex < 1) || (InstructionCode == 0x2A && argindex < 2) )
			{
				*tt_error = tt_MissingParameters;
				return CurrentPtr;
			}
			
			if (args2[argindex-1] != L'*')
			/* check the function number */
			{
				foundPP = 0;    /* Have not found it yet... */
				for (PPindex=0;(PPindex<NumberOfPPEntries && !foundPP ) ; PPindex++) {
					if (L'F' == asm_ppDescription[PPindex].type) {
						foundPP=1;   /* Exit loop for speed's sake */
						if ((args[argindex-1] < asm_ppDescription[PPindex].lowestValidValue) || (args[argindex-1] > asm_ppDescription[PPindex].highestValidValue)) {
							*tt_error = tt_FunctionNbOutOfRange;
							*SelectionLength = (long) (CurrentPtr - tempP);
							CurrentPtr = tempP;
						} /* if */
					} /* if */
				} /*for */
			}

		} else {
			if (argindex < (short)STRLENW(ArgTypeBuffer)) 
			{
				*tt_error = tt_MissingParameters;
				return CurrentPtr;
			}
		}

		return CurrentPtr;

	} else {

		if (TT_IsJR(InstructionCode)) 
		{
			long StringLength;
			/* in this case only jump command has parameters */

			while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;

			if ( *CurrentPtr++ != L',' || CurrentPtr >= EOLPtr )
			{
				*tt_error = tt_EmptyParameterList;
				return (CurrentPtr-1);
			}
			
			/* skip extra spaces */
			while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;

			if ( *CurrentPtr++ != L'(' || (CurrentPtr >= EOLPtr) )
			{
				*tt_error = tt_JRExpectingABracket;
				return (CurrentPtr-1);
			}

			/* skip extra spaces */
			while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;

			/* parse the Bn or Wn label */
			if ( (*CurrentPtr != L'B' && *CurrentPtr != L'W') || (CurrentPtr >= EOLPtr) )
			{
				*tt_error = tt_JRExpectingABWLabel;
				return (CurrentPtr);
			}
			
			StringLength = TT_GetStringLength (CurrentPtr, EOLPtr);
			if (StringLength < 2)
			{
				*tt_error = tt_VoidLabel;
				return (CurrentPtr);
			}
				
			if (StringLength >= MAXLABELLENGTH)
			{
				*tt_error = tt_LabelTooLong;
				return (CurrentPtr);
			}


			*BWLabelLength = (short)StringLength;
			*BWLabelHandle = CurrentPtr;

			CurrentPtr = CurrentPtr + StringLength;

			/* skip extra spaces */
			while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;

			if ( *CurrentPtr++ != L'=' || (CurrentPtr >= EOLPtr) )
			{
				*tt_error = tt_JRExpectingAEqual;
				return (CurrentPtr-1);
			}

			/* skip extra spaces */
			while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;

			if ( *CurrentPtr++ != L'#' || (CurrentPtr >= EOLPtr) )
			{
				*tt_error = tt_JRExpectingALabel;
				return (CurrentPtr-1);
			}


			StringLength = TT_GetStringLength (CurrentPtr, EOLPtr);
			if (StringLength >= MAXLABELLENGTH)
			{
				*tt_error = tt_LabelTooLong;
				*SelectionLength = StringLength;
				return (CurrentPtr);
			}

			if ( StringLength == 0 )
			{
				*tt_error = tt_VoidLabel;
				*SelectionLength = StringLength;
				return (CurrentPtr);
			}

			*LabelLength = (short)StringLength;
			*LabelHandle = CurrentPtr;

			CurrentPtr = CurrentPtr + StringLength;

			/* skip extra spaces */
			while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
				CurrentPtr++;

			if  ( *CurrentPtr != L')' || (CurrentPtr >= EOLPtr) )
			{
				*tt_error = tt_JRExpectingABracket;
				return (CurrentPtr-1);
			}
			CurrentPtr++;

		}
		return CurrentPtr;
	}
}

void TT_StoreArgumentsAndInstruction (unsigned char InstructionCode, short ** aHandle, unsigned char ** iHandle,
					short  argc, short *args, short  argc2,wchar_t *args2,  short * tt_error);

void TT_StoreArgumentsAndInstruction (unsigned char InstructionCode, short ** aHandle, unsigned char ** iHandle,  
					short  argc, short *args, short  argc2,wchar_t *args2,  short * tt_error)
{
	short TotalArg, j;
	
	TotalArg = argc + argc2;
	
	**iHandle = InstructionCode;
	*iHandle = *iHandle +1;

	if (TT_IsPushBW(InstructionCode))
	{
	/* PUSH arguments goes to the instruction stream */
		short index = 0;
		if (TT_IsNPushBW(InstructionCode))
		{
			/* number of bytes to push */
			**iHandle = (unsigned char)args[index++];
			*iHandle = *iHandle +1;

		}
		if (TT_IsPushB(InstructionCode))
		{
			/* arguments are bytes */
			for ( j = index; j < TotalArg; j++ ) {
				**iHandle = (unsigned char) args[index++];
				*iHandle = *iHandle +1;
			}
		} else {
			/* arguments are words */
			for ( j = index; j < TotalArg; j++ ) {
				short word = args[index++];
				*(short *) (*iHandle) = SWAPW(word);
				*iHandle = *iHandle +2;
			}
		}
		
	} else {
		for ( j = 0; j < TotalArg; j++ ) {
			short index;
			index = TotalArg - 1 - j;
			if ( index < 0 || index > 255 ) {
				* tt_error = tt_ArgumentIndexOutOfRange;
			}
					
			if ( args2[index] != L'*' ) 
			{ 
				// REVIEW: mjan - Do we need to think about byte order here?
				// B.St. It doesn't appear to matter, since we're incrementing aHandle by 1 byte at a time
				**aHandle = args[index]; /* Reverse since we are going to reverse everything later */
				*aHandle = *aHandle +1;
			}
		}
	}


}

 
/*
 *
 */
short TT_BytePush( short argStore[], short StartIndex, short numberofArgs, unsigned char *uCharP  , tt_PStype * PS);
short TT_BytePush(short  argStore[], short StartIndex, short  numberofArgs,unsigned char * uCharP ,tt_PStype * PS )
// short argStore[], numberofArgs;
// register unsigned char *uCharP;
{
	short i, k;
	short count;
	
	count = 0;
	if ( numberofArgs <= 8 ) {
		uCharP[count++] = 0xB0 + numberofArgs -1;/* PUSHB */
	} else {
		uCharP[count++] = 0x40; /* NPUSHB */
		uCharP[count++] = (unsigned char) numberofArgs;
	}
	if (PS != NULL)
	/* fill the iPos value for the Bn PUSH labels of the current PUSH */
	{
		for ( k = PS->num-1; k >=0; k--) 
		{
			if ((PS->ps[k]->aPtr == NULL) && (PS->ps[k]->LocalIndex >= StartIndex) &&  (PS->ps[k]->LocalIndex < StartIndex+numberofArgs) )
			{
				PS->ps[k]->aPtr = (unsigned char *) (uCharP + count + PS->ps[k]->LocalIndex - StartIndex);
			}
		}
	}
	for ( i = 0; i < numberofArgs; i++ ) {
		uCharP[count++] = (unsigned char) argStore[StartIndex+i];
	}
	return count;
}

/*
 *
 */
short TT_WordPush( short argStore[], short StartIndex, short numberofArgs, unsigned char *uCharP, tt_PStype * PS );
short TT_WordPush( short  argStore[], short StartIndex ,short  numberofArgs,unsigned char * uCharP, tt_PStype * PS )
// short argStore[], numberofArgs;
// register unsigned char *uCharP;
{
	short i,k;
	short count;
	short *shortP;

	count = 0;
	if ( numberofArgs <= 8 ) {
		uCharP[count++] = 0xB8 + numberofArgs -1;/* PUSHW */
	} else {
		uCharP[count++] = 0x41; /* NPUSHW */
		uCharP[count++] = (unsigned char) numberofArgs;
	}

	if (PS != NULL)
	/* fill the iPos value for the Wn PUSH labels of the current PUSH */
	{
		for ( k = PS->num-1; k >=0; k--) 
		{
			if ((PS->ps[k]->aPtr == 0) && (PS->ps[k]->LocalIndex >= StartIndex) &&  (PS->ps[k]->LocalIndex < StartIndex+numberofArgs) )
			{
				PS->ps[k]->aPtr = (unsigned char *)(uCharP + count + 2 * (PS->ps[k]->LocalIndex - StartIndex));
			}
		}
	}

	for ( i = 0; i < numberofArgs; i++ ) {
		shortP = (short *)&uCharP[count];
		*shortP = SWAPW(argStore[StartIndex+i]);
		count += 2;
	}
	return count;
}

short TT_ByteRunLength( short *args, short n );
/*
 *
 */
short TT_ByteRunLength(short * args,short  n )
//short *args, n;
{
	short len;
	
	for ( len = 0; len < n && *args >= 0 && *args <= 255; args++ ) {
		len++;
	}
	return len;
}

short TT_OptimizingPushArguments( unsigned char * BinaryOut,unsigned char * BinaryOutMaxPtr, short argStore[], short numberofArgs, tt_PStype * PS, short *tt_error);
short TT_OptimizingPushArguments( unsigned char * BinaryOut,unsigned char * BinaryOutMaxPtr, short argStore[], short numberofArgs, tt_PStype * PS, short *tt_error)
{
	short i, count, argument;
	short argCount, n, byteRun, runLength;
	short doBytePush, limit;
	
	
	count = 0;		
	
	for ( n = 0; numberofArgs > 0;  ) {
		argCount = numberofArgs > 255 ? 255 : numberofArgs;
		doBytePush = true;
		
		for ( runLength = 0, i = n; i < (n + argCount); i++ ) {
			argument = argStore[i];
			byteRun = TT_ByteRunLength( &argStore[i], (short) (n + argCount - i) );
			
			/* if we have a run of bytes of length 3 (2 if first or last) or more it is more
			   optimal in terms of space to push them as bytes */
			limit = 3;
			if ( (runLength == 0) || ((byteRun + i) >= (n + argCount)) ) {
				limit = 2;
			}
			if ( byteRun >= limit ) {
				if ( runLength > 0 ) {
					argCount = runLength;
					doBytePush = false;
					break; /*****/
				} else {
					argCount = byteRun;
					doBytePush = true;
					break; /*****/
				}
			}

			if ( argument > 255 || argument < 0 ) doBytePush = false;
			runLength++;
		}
		numberofArgs -= argCount;


		
		if ( doBytePush ) {
			if (BinaryOut + count + 2 + argCount > BinaryOutMaxPtr) 
			{
				*tt_error = tt_ProgramTooBig;
				return 0;
			}
			count += TT_BytePush( argStore, n, argCount, &BinaryOut[count], PS );
		} else {
			if (BinaryOut + count + 2 + 2 *argCount > BinaryOutMaxPtr) 
			{
				*tt_error = tt_ProgramTooBig;
				return 0;
			}
			count += TT_WordPush( argStore, n, argCount, &BinaryOut[count], PS );
		}
		n += argCount;
	}
	return count;
}


short TT_WriteOutBlock( unsigned char *BinaryOut,unsigned char *BinaryOutEndPtr, short * BinaryOffset, short AddOffset, short argStore[], 
		unsigned char insStore[], short numberofArgs, short numberofInstructions, short * tt_error);
short TT_WriteOutBlock( unsigned char *BinaryOut,unsigned char *BinaryOutEndPtr, short * BinaryOffset, short AddOffset, short argStore[], 
		unsigned char insStore[], short numberofArgs, short numberofInstructions, short * tt_error)
{
	short i, j, k;
	short count;
	
	/* Reverse arguments since we are pushing them */
	k = numberofArgs >> 1;
	for ( i = 0; i < k; i++ ) {
		j = argStore[i];
		argStore[i] = argStore[numberofArgs-1-i];
		argStore[numberofArgs-1-i] = j;
	}
	
	count = TT_OptimizingPushArguments(BinaryOut, BinaryOutEndPtr, argStore, numberofArgs, (tt_PStype *) NULL, tt_error);
	
	if (AddOffset && *BinaryOffset != -1)
	{
	 	*BinaryOffset = *BinaryOffset + count;
	}

	for ( i = 0; i < numberofInstructions; i++ ) {
		BinaryOut[ count++ ] = insStore[i];
	}
		
	return count;
}

void TT_ResetLastCommandCompilationStatus(tt_CompilationStatus *CompilationStatus);
void TT_ResetLastCommandCompilationStatus(tt_CompilationStatus *CompilationStatus)
{
	CompilationStatus->LastCommandWasAnIF 	= false; 
	CompilationStatus->LastCommandWasAnELSE 	= false;
	CompilationStatus->LastCommandWasAnFDEF 	= false; 
	CompilationStatus->LastCommandWasAnIDEF 	= false;
	CompilationStatus->LastCommandWasAnJUMP 	= false;
	CompilationStatus->LastCommandWasAnEND 	= false;
}

void TT_CheckAndUpdateCompilationStatus(short InstructionCode,tt_CompilationStatus *CompilationStatus,short * tt_error);
void TT_CheckAndUpdateCompilationStatus(short InstructionCode,tt_CompilationStatus *CompilationStatus,short * tt_error)
{

	if (CompilationStatus->WeAreInPushOnMode)
	{
		if (CompilationStatus->LastCommandWasAnIF || CompilationStatus->LastCommandWasAnELSE || CompilationStatus->LastCommandWasAnFDEF || 
				CompilationStatus->LastCommandWasAnIDEF || 	CompilationStatus->LastCommandWasAnJUMP)
		{
			*tt_error = tt_ExpectingaBEGIN;
			return;
		}
	}
	TT_ResetLastCommandCompilationStatus(CompilationStatus);
	
	switch ( InstructionCode ) {
		case 0x58: /* IF */
		{
			CompilationStatus->WeAreInsideAnIF = true;
			CompilationStatus->LastCommandWasAnIF = true;
			CompilationStatus->NumberOfEmbeddedIF ++;
			if (CompilationStatus->NumberOfEmbeddedIF > MAXIFRECURSION)
			{
				*tt_error = tt_TooManyEmbeddedIF;
				return;
			}
			CompilationStatus->ELSEStatus[CompilationStatus->NumberOfEmbeddedIF] = false;
			break;
		}
		case 0x59: /* EIF */
		{
			if ((!CompilationStatus->WeAreInsideAnIF) || (CompilationStatus->NumberOfEmbeddedIF == 0))
			{
				*tt_error = tt_EIFwithoutIF;
				return;
			}
			
			CompilationStatus->NumberOfEmbeddedIF --;
			if (CompilationStatus->NumberOfEmbeddedIF == 0) CompilationStatus->WeAreInsideAnIF = false;
			break;
		}
		case 0x1B: /* ELSE */
		{
			if ((!CompilationStatus->WeAreInsideAnIF) || (CompilationStatus->NumberOfEmbeddedIF == 0))
			{
				*tt_error = tt_ELSEwithoutIF;
				return;
			}
			if (CompilationStatus->ELSEStatus[CompilationStatus->NumberOfEmbeddedIF])
			{
				*tt_error = tt_ELSEwithinELSE;
				return;
			}
			CompilationStatus->ELSEStatus[CompilationStatus->NumberOfEmbeddedIF] = true;
			CompilationStatus->LastCommandWasAnELSE = true;
			break;
		}
		case 0x2C: /* FDEF */
		{
			if (CompilationStatus->WeAreInsideAnFDEF)
			{
				*tt_error = tt_FDEFInsideFDEF;
				return;
			}
			if (CompilationStatus->WeAreInsideAnIDEF)
			{
				*tt_error = tt_FDEFInsideIDEF;
				return;
			}
			CompilationStatus->WeAreInsideAnFDEF = true;
			CompilationStatus->LastCommandWasAnFDEF = true;
			break;
		}
		case 0x89: /* IDEF */
		{
			if (CompilationStatus->WeAreInsideAnIDEF)
			{
				*tt_error = tt_IDEFInsideIDEF;
				return;
			}
			if (CompilationStatus->WeAreInsideAnFDEF)
			{
				*tt_error = tt_IDEFInsideFDEF;
				return;
			}
			CompilationStatus->WeAreInsideAnFDEF = true;
			CompilationStatus->LastCommandWasAnFDEF = true;
			break;
		}
		case 0x2D: /* ENDF */
		{
			if (!CompilationStatus->WeAreInsideAnFDEF  && !CompilationStatus->WeAreInsideAnIDEF)
			{
				*tt_error = tt_ENDFwithoutFDEF;
				return;
			}
			
			CompilationStatus->WeAreInsideAnFDEF = false;
			CompilationStatus->WeAreInsideAnIDEF = false;
			break;
		}
	}

	CompilationStatus->LastCommandWasAnEND = false;

//typedef struct {
//	short	WeAreInsideAnIF;			
//	short	WeAreInsideTheELSEofAnIF;			
//	short	WeAreInsideAnFDEF;			
//	short	WeAreInsideAnIDEF;			
//	short	WeAreInPushOnMode;
//	short	WeAreInPrePushMode;
//	short	WeAreInsideGHBlock;
//	short	LastCommandWasAnIF; /* used to check for #BEGIN (beginning of a block) */
//	short	LastCommandWasAnELSE; /* used to check for #BEGIN (beginning of a block) */
//	short	LastCommandWasAnFDEF; /* used to check for #BEGIN (beginning of a block) */
//	short	LastCommandWasAnIDEF; /* used to check for #BEGIN (beginning of a block) */
//	short	LastCommandWasAnEND; /* to check if the ENDF, ELSE  or EIF is preceeded by #END (end of block) */
//} tt_CompilationStatus;

}

wchar_t *TT_SkipEmptyLines( wchar_t *p, wchar_t *endP,short *pLineCount );
wchar_t *TT_SkipEmptyLines( wchar_t *p, wchar_t *endP,short *pLineCount )
{
	wchar_t *myP;

	/* skip spaces and empty lines, keep track of the line number */
	for ( myP = 0; myP != p; ) {
		myP = p;
		while ( *p == L' ' && p < endP)
			p++;
		while ( (*p == L'\x0D' || *p == L'\x0A') && p < endP)
			{
			*pLineCount = *pLineCount+1; p++;
			}
	}
	return p;
}

wchar_t *TT_SkipCommentsAndEmptyLines( wchar_t *p, wchar_t *endP,short *lineCount, short * error );
wchar_t *TT_SkipCommentsAndEmptyLines( wchar_t *p, wchar_t *endP,short *pLineCount, short * error )
{
	wchar_t * tempP;
	*error = tt_NoError;
	p = TT_SkipEmptyLines( p, endP,pLineCount);

	while ( *p == L'/' && *(p+1) == L'*' && p < endP ) { /* skip C style comments */
		tempP = p;
		p++; p++;
		while ( !(*p == L'*' && *(p+1) == L'/') && p < endP ) {

			if ( *p == L'/' && *(p+1) == L'*' ) {
				*error = tt_EmbeddedComment;
				return p;
			}
			if ( *p == '\x0D' || *p == '\x0A')
				{
				*pLineCount = *pLineCount+1;
				}
			p++;

		}
		if (p == endP) {
			*error = tt_UnterminatedComment;
			return tempP; /* return the pointer to the beginning of the unterminated comment */
		}
		p++; p++;
		p = TT_SkipEmptyLines( p, endP,pLineCount);
	}
	return p;
}

wchar_t *TT_InnerCompile(
	/* source text, pointer to the begining, the end and the cursor position (to be able to trace until that line) */
		wchar_t *StartPtr, wchar_t * EndPtr, wchar_t * SelStartPtr, 
	/* pointer to the output buffer, it's maximal length and return the Binary length */
		unsigned char * BinaryOut, char * BinaryOutEndPtr, long * BinaryLength, 
	/* offset in the instruction stream to the instruction corresponding to the cursor position, used for trace mode */
		short * BinaryOffset, 
	/* length of the text to be selected in case of error */
		long * SelectionLength, 
	/* line number where the first error occur */
		short * ErrorLineNb,
	/* return : approximate stack need, higher function number */
		short * StackNeed, short * MaxFunctionDefs, 
	/* pass the value 1 to the recursion level, used internally to treat recursively embedded #BEGIN #END blocks */
		short RecursionLevel, 
		tt_CompilationStatus *CompilationStatus,
		/* for the DovMan partial compilation feature that flash points referenced by the current command */
		tt_flashingPoints * flashingPoints,
		ASMType asmType, /* asmGLYF, asmPREP, or asmFPGM */
	/* error code, return tt_NoError if no error */
		short * tt_error);

wchar_t *TT_InnerCompile(
	/* source text, pointer to the begining, the end and the cursor position (to be able to trace until that line) */
		wchar_t *StartPtr, wchar_t * EndPtr, wchar_t * SelStartPtr, 
	/* pointer to the output buffer, it's maximal length and return the Binary length */
		unsigned char * BinaryOut, char * BinaryOutEndPtr, long * BinaryLength, 
	/* offset in the instruction stream to the instruction corresponding to the cursor position, used for trace mode */
		short * BinaryOffset, 
	/* length of the text to be selected in case of error */
		long * SelectionLength, 
	/* line number where the first error occur */
		short * ErrorLineNb,
	/* return : approximate stack need, higher function number */
		short * StackNeed, short * MaxFunctionDefs, 
	/* pass the value 1 to the recursion level, used internally to treat recursively embedded #BEGIN #END blocks */
		short RecursionLevel, 
		tt_CompilationStatus *CompilationStatus,
		/* for the DovMan partial compilation feature that flash points referenced by the current command */
		tt_flashingPoints * flashingPoints,
		ASMType asmType, /* asmGLYF, asmPREP, or asmFPGM */
	/* error code, return tt_NoError if no error */
		short * tt_error)
{
	short	LineNb, LastLineCompiled;
	long	LineLength, SLoopLineLength;
	wchar_t 	*CurrentPtr, *SLoopPtr;
	short numberofArgs, numberofInstructions;
	short				*argStore, *aPtr;
	unsigned char	*insStore, *iPtr;
	short	NeedTwoPass = false; /* used to be MyCode */
	short 	ghtblock ;
	short 	loop;
	short	args[256], argc, argc2;
	wchar_t	args2[256];
	wchar_t	argTypeBuffer[ARGTYPEBUFFER_SIZE];
	tt_LabelType *	Label;
	tt_JRtype * JRList;
	tt_PStype    *PushLabels;
	tt_JrBWtype  *JrBW;
	short AddOffset = false;
	unsigned char * InnerBinaryOutMaxPtr,*InnerArgStoreMaxPtr;
							
	
	CompilationStatus->WeAreInPushOnMode 	= true; /* reset after a #BEGIN, to be backwards compatible */
	CompilationStatus->WeAreInPrePushMode 	= true;
	CompilationStatus->WeAreInsideGHBlock 	= false;
	CompilationStatus->LastCommandWasAnIF 	= false; 
	CompilationStatus->LastCommandWasAnELSE 	= false;
	CompilationStatus->LastCommandWasAnFDEF 	= false; 
	CompilationStatus->LastCommandWasAnIDEF 	= false;
	CompilationStatus->LastCommandWasAnJUMP 	= false;
	CompilationStatus->LastCommandWasAnEND 	= false;
			
	*tt_error = 0;
	*BinaryLength = 0;
	*SelectionLength = 0;
	*tt_error = tt_NoError;
	SLoopPtr = StartPtr;
	SLoopLineLength = 0;
	
	
	if (RecursionLevel == 1) 
	{
		*BinaryOffset = -1;
		AddOffset = true;
	}
	if (*BinaryOffset == -1) AddOffset = true;
	
	PushLabels = (tt_PStype *) NewP( sizeof(tt_PStype) );
	if ( PushLabels == NULL) {
		*tt_error = tt_NotEnoughMemory;
		*SelectionLength = 0;
		return StartPtr;
	}

	Label = (tt_LabelType *) NewP( sizeof(tt_LabelType) );
	if ( Label == NULL) {
		*tt_error = tt_NotEnoughMemory;
		*SelectionLength = 0;
		DisposeP((void**)&PushLabels);
		return StartPtr;
	}

	JRList = (tt_JRtype *) NewP( sizeof(tt_JRtype) );
	if ( JRList == NULL) {
		*tt_error = tt_NotEnoughMemory;
		*SelectionLength = 0;
		DisposeP((void**)&Label);
		DisposeP((void**)&PushLabels);
		return StartPtr;
	}
	
	JrBW = (tt_JrBWtype *) NewP( sizeof(tt_JrBWtype) );
	if ( JrBW == NULL) {
		*tt_error = tt_NotEnoughMemory;
		*SelectionLength = 0;
		DisposeP((void**)&Label);
		DisposeP((void**)&PushLabels);
		DisposeP((void**)&JRList);
		return StartPtr;
	}

	Label->num = 0;
	JRList->num = 0;
	PushLabels->num = 0;
	JrBW->num = 0;
	
	argStore = (short *)NewP( sizeof(short) * (MAXARGUMENTS) );
	if ( argStore == 0  ) {
		*tt_error = tt_NotEnoughMemory;
		*SelectionLength = 0;
		TT_FreeAllLabelMemory(PushLabels, JrBW, Label, JRList);
		return StartPtr;
	}
	insStore = (unsigned char *)NewP( sizeof(char) * (MAXINSTRUCTIONCOUNT) );
	if ( insStore == 0  ) {
		*tt_error = tt_NotEnoughMemory;
		DisposeP((void**)&argStore);
		TT_FreeAllLabelMemory(PushLabels, JrBW, Label, JRList);
		return StartPtr;
	}
	aPtr	= argStore; iPtr	= insStore; /* set up pointers */
	InnerBinaryOutMaxPtr = (unsigned char *)(insStore + MAXINSTRUCTIONCOUNT);
	InnerArgStoreMaxPtr = (unsigned char *)((unsigned char*)argStore + MAXARGUMENTS);
	
	CurrentPtr = StartPtr;
	LastLineCompiled = 0;
	ghtblock = false;
	loop = 1;
	
	for ( LineNb = 1; CurrentPtr <= EndPtr && *tt_error == tt_NoError; ) {
		short LineInc = 0;
		CurrentPtr = TT_SkipCommentsAndEmptyLines( CurrentPtr,EndPtr, &LineInc, tt_error );
		LineNb += LineInc;
		*ErrorLineNb += LineInc;

		if ( *tt_error != tt_NoError) 
		{
			DisposeP((void**)&argStore);
			DisposeP((void**)&insStore);
			TT_FreeAllLabelMemory(PushLabels, JrBW, Label, JRList);

			*SelectionLength = 2;
			return CurrentPtr;
		};

		if ( CurrentPtr == EndPtr) break;
		LineLength = TT_GetLineLength(CurrentPtr, EndPtr);
		
		if (LineNb == LastLineCompiled)
		{
			*tt_error = tt_TwoInstructionsInSameLine;
			*SelectionLength = LineLength;
			break;
		}
		LastLineCompiled = LineNb;
		
		if (*CurrentPtr == L'#') 
		/* label or compiler switch */
		{
			long StringLength;
			
			StringLength = TT_GetStringLength (CurrentPtr +1, EndPtr);
			if ((*(CurrentPtr + StringLength + 1) == L':') && (CurrentPtr + StringLength + 1 != EndPtr))
			/* this is a label */
			{
				if (CompilationStatus->WeAreInsideGHBlock)
				{
					CurrentPtr = CurrentPtr + LineLength;
				} else {
					numberofArgs = (short)(ptrdiff_t)(aPtr - argStore); 
					numberofInstructions = (short)(ptrdiff_t)(iPtr - insStore); 
				
					if (StringLength >= MAXLABELLENGTH)
					{
						*tt_error = tt_LabelTooLong;
						*SelectionLength = LineLength;
						break;
					}

					if ( StringLength == 0 )
					{
						*tt_error = tt_VoidLabel;
						*SelectionLength = LineLength;
						break;
					}

	    			TT_SaveLabel( numberofArgs, numberofInstructions, StringLength, CurrentPtr+1, Label, tt_error);

					if ( *tt_error != tt_NoError) 
					{
						*SelectionLength = LineLength;
						break;
					}
					CurrentPtr = CurrentPtr + StringLength+2; /* the # and the : */
				}
			} else {
			/* this is a compiler switch */
				short i, found, SwitchCode;
				for ( found = i = 0; i < TOTALNUMBEROFSWITCH; i++ ) {		
					if ( StringLength == (short) STRLENW( tt_CompilerSwitch[i].name ) &&
							wcsncmp( CurrentPtr+1, tt_CompilerSwitch[i].name, StringLength) == 0 ) {
			     
						found = true;
						SwitchCode = tt_CompilerSwitch[i].index;
					}
				};
				if (!found) {
					if (CompilationStatus->WeAreInsideGHBlock)
					{
						CurrentPtr = CurrentPtr + LineLength;
					} else {
						*tt_error = tt_UnknownSwitch;
						*SelectionLength = StringLength + 1;
					}
				} else
				{
					switch ( SwitchCode ) {
					case tt_Push_Switch:
					{
						wchar_t * EOLPtr;
						short mycount;

						if (CompilationStatus->WeAreInsideGHBlock)
						{
							CurrentPtr = CurrentPtr + LineLength;
						} else {
	    					short localArgStore[PUSH_ARG], localNumberofArgs;
							EOLPtr = CurrentPtr + LineLength;

							TT_ResetLastCommandCompilationStatus(CompilationStatus);
							CurrentPtr = CurrentPtr + StringLength+1;

							CurrentPtr = TT_ParsePUSHandSave( PushLabels, CurrentPtr, EOLPtr, localArgStore, &localNumberofArgs, SelectionLength,tt_error ); 

							mycount = TT_OptimizingPushArguments(iPtr, InnerBinaryOutMaxPtr, localArgStore, localNumberofArgs,PushLabels, tt_error );

							numberofArgs = (short)(ptrdiff_t)(aPtr - argStore); 		/* number of arguments */
							if (localNumberofArgs > numberofArgs)
								/* numberofArgs represent what will already be popped from the stack at this point */
								*StackNeed = *StackNeed + localNumberofArgs - numberofArgs;
							iPtr += mycount;
						}

						break;
					}
					case tt_PushOn_Switch:
						if (CompilationStatus->WeAreInsideGHBlock)
						{
							CurrentPtr = CurrentPtr + LineLength;
						} else {
/* should be a warning, too many errors on old fonts */
//							if (CompilationStatus->WeAreInPushOnMode)
//							{
//								*tt_error = tt_PUSHONwhenAlreadyOn;
//							}
	    					CompilationStatus->WeAreInPushOnMode = true;
							CurrentPtr = CurrentPtr + StringLength+1;
						}
						break;
					case tt_PushOff_Switch:
						if (CompilationStatus->WeAreInsideGHBlock)
						{
							CurrentPtr = CurrentPtr + LineLength;
						} else {
//							if (!CompilationStatus->WeAreInPushOnMode)
//							{
//								*tt_error = tt_PUSHOFFwhenAlreadyOff;
//							}
							if (CompilationStatus->WeAreInPushOnMode)
							{
								if (CompilationStatus->LastCommandWasAnIF || CompilationStatus->LastCommandWasAnELSE || CompilationStatus->LastCommandWasAnFDEF || 
										CompilationStatus->LastCommandWasAnIDEF || 	CompilationStatus->LastCommandWasAnJUMP)
								{
									*tt_error = tt_ExpectingaBEGIN;
								}
							}
	    					CompilationStatus->WeAreInPushOnMode = false;
							CurrentPtr = CurrentPtr + StringLength+1;
						}
						break;
					case tt_Begin_Switch:
						{
						if (CompilationStatus->WeAreInsideGHBlock)
						{
							CurrentPtr = CurrentPtr + LineLength;
						} else {
							long count;
							short subStackNeed, tempInsOffset;
							tt_CompilationStatus	SavedCompilationStatus;

							if (RecursionLevel == MAXBLOCKEMBEDDING)
							{
								*tt_error = tt_TooManyEnbeddedBlocks;
								*SelectionLength = StringLength + 1;
								break;
							}
							TT_ResetLastCommandCompilationStatus(CompilationStatus);
							CurrentPtr = CurrentPtr + StringLength+1;
							subStackNeed = 0;
						
							tempInsOffset = 0;
							if (*BinaryOffset == -1) tempInsOffset = (short)(ptrdiff_t)(iPtr - insStore);

							SavedCompilationStatus = *CompilationStatus;
							CurrentPtr = TT_InnerCompile(CurrentPtr, EndPtr,SelStartPtr, iPtr, (char *)InnerBinaryOutMaxPtr, &count, BinaryOffset, SelectionLength, ErrorLineNb,
												&subStackNeed, MaxFunctionDefs, RecursionLevel+1,CompilationStatus, flashingPoints, asmType, tt_error) ;

							/* restore the status that need to be restored */
							CompilationStatus->WeAreInPushOnMode = SavedCompilationStatus.WeAreInPushOnMode;

							if (*tt_error == tt_NoError)
							{
								if (CompilationStatus->NumberOfEmbeddedIF != SavedCompilationStatus.NumberOfEmbeddedIF)
								{
									*tt_error = tt_IFgoingAcrossBlocks;
									CurrentPtr = CurrentPtr - 4; /* go back before the #END */
									*SelectionLength = 4;
								}
								if (CompilationStatus->WeAreInsideAnFDEF != SavedCompilationStatus.WeAreInsideAnFDEF)
								{
									*tt_error = tt_FDEFgoingAcrossBlocks;
									CurrentPtr = CurrentPtr - 4; /* go back before the #END */
									*SelectionLength = 4;
								}
								if (CompilationStatus->WeAreInsideAnIDEF != SavedCompilationStatus.WeAreInsideAnIDEF)
								{
									*tt_error = tt_IDEFgoingAcrossBlocks;
									CurrentPtr = CurrentPtr - 4; /* go back before the #END */
									*SelectionLength = 4;
								}
							}


							if (*BinaryOffset != -1) *BinaryOffset = *BinaryOffset + tempInsOffset;

							numberofArgs = (short)(ptrdiff_t)(aPtr - argStore); 		/* number of arguments */
							if (subStackNeed > numberofArgs)
								/* numberofArgs represent what will already be popped from the stack at this point */
								*StackNeed = *StackNeed + subStackNeed - numberofArgs;

							iPtr += count;
						}
						break;
						}
					case tt_End_Switch:
						if (CompilationStatus->WeAreInsideGHBlock)
						{
							CurrentPtr = CurrentPtr + LineLength;
						} else {
							TT_ResetLastCommandCompilationStatus(CompilationStatus);
							if (RecursionLevel > 1) 
							{
								CurrentPtr = CurrentPtr + StringLength+1;
								/* write the code of the block before comming back */
								numberofArgs = (short)(ptrdiff_t)(aPtr - argStore); 		/* number of arguments */
								numberofInstructions = (short)(ptrdiff_t)(iPtr - insStore); /* number of instructions */


								if (NeedTwoPass)
								{
									CurrentPtr = TT_FindLabelError(PushLabels, JrBW, JRList, Label, CurrentPtr, SelectionLength, tt_error) ; 

									if (*tt_error != tt_NoError) 
									{
										DisposeP((void**)&argStore);
										DisposeP((void**)&insStore);
										TT_FreeAllLabelMemory(PushLabels, JrBW, Label, JRList);
										return CurrentPtr;
									};
   	 								TT_JRpushON_ReplaceLabel( JRList, Label, argStore, tt_error);
									TT_JRpushOFF_ReplaceLabel( JrBW, PushLabels, Label, tt_error);
								}

								if (*tt_error == tt_NoError) 
									*BinaryLength = TT_WriteOutBlock( BinaryOut, (unsigned char *)BinaryOutEndPtr, BinaryOffset, AddOffset, argStore, insStore, numberofArgs, numberofInstructions, tt_error);	

								*StackNeed = *StackNeed + numberofArgs;
	
								DisposeP((void**)&argStore);
								DisposeP((void**)&insStore);
								TT_FreeAllLabelMemory(PushLabels, JrBW, Label, JRList);
								return CurrentPtr;
							} else {
								*tt_error = tt_EndWithoutBegin;
								*SelectionLength = StringLength + 1;
							}
						}
						break;
					case tt_GHTBlockBegin_Switch:
						CompilationStatus->WeAreInsideGHBlock = true;
						CurrentPtr = CurrentPtr + LineLength;
						break;
					case tt_GHTBlockEnd_Switch:
						CompilationStatus->WeAreInsideGHBlock = false;
						CurrentPtr = CurrentPtr + LineLength;
						break;
					}
				}
			}
		} else {
			if (CompilationStatus->WeAreInsideGHBlock)
			{
				CurrentPtr = CurrentPtr + LineLength;
			} else {
				/* regular instruction */
				short InstructionIndex, found;
				unsigned short InstructionCode; /* unsigned short because of the fake code used to detect composite commands */
				long StringLength;
			
				StringLength = TT_GetStringLength (CurrentPtr, EndPtr);
			
				for ( found = InstructionIndex = 0; InstructionIndex < TOTALNUMBEROFINSTRUCTIONS; InstructionIndex++ ) {
					if ( StringLength == (short) STRLENW( tt_instruction[InstructionIndex].name ) &&
			     		wcsncmp(CurrentPtr, tt_instruction[InstructionIndex].name, StringLength) == 0 ) {
						found = true;
						InstructionCode = tt_instruction[InstructionIndex].baseCode;
						break;
					}
				}
				if (!found) {
					*SelectionLength = StringLength;
					*tt_error = tt_UnknownInstruction;
				} else {
					TT_CheckAndUpdateCompilationStatus(InstructionCode,CompilationStatus,tt_error);
					if ( *tt_error != tt_NoError) 
					{
						DisposeP((void**)&argStore);
						DisposeP((void**)&insStore);
						*SelectionLength = StringLength;
						TT_FreeAllLabelMemory(PushLabels, JrBW, Label, JRList);
						return CurrentPtr;
					};

					if (TT_IsDelta(InstructionCode))
					{
						wchar_t * EOLPtr, *LineBegin;
						short deltaCount;
						short i;
					
    					tt_deltaPType dArr[MAXDELTA];
						LineBegin = CurrentPtr;
						EOLPtr = CurrentPtr + LineLength;
						/* this is a delta instruction */
						CurrentPtr = TT_DecodeDeltaP( CurrentPtr+StringLength, EOLPtr, EndPtr, InstructionIndex, InstructionCode, &deltaCount, dArr, SelectionLength,tt_error); 

						if ((deltaCount == 0) && CompilationStatus->WeAreInPushOnMode && *tt_error == tt_NoError)
						{
							*tt_error = tt_DELTAWithoutArguments;
							*SelectionLength = LineLength;
							return LineBegin;
						}
						if ((deltaCount != 0) && !CompilationStatus->WeAreInPushOnMode && *tt_error == tt_NoError)
						{
							*tt_error = tt_DELTAWithArguments;
							*SelectionLength = LineLength;
							return LineBegin;
						}
						
						for ( i = 255; i >= 0; i-- ) {
							args2[i] = L' ';
						}
						argc2 = 0;
					
						TT_CompileDelta( dArr, deltaCount, (unsigned char) InstructionCode, args, &argc );
						if (CurrentPtr > SelStartPtr && *BinaryOffset == -1)
						{
							*BinaryOffset = (short)(ptrdiff_t)(iPtr - insStore);
						}
						InstructionCode = InstructionCode & 0xFF; /* for the diret delta */
						
						if (iPtr + 1 > InnerBinaryOutMaxPtr)
						{
							*tt_error = tt_ProgramTooBig;
							return CurrentPtr;
						}
						if ((char*)aPtr + argc + argc2 > (char*)InnerArgStoreMaxPtr)
						{
							*tt_error = tt_TooManyArguments;
							return CurrentPtr;
						}
						TT_StoreArgumentsAndInstruction((unsigned char) InstructionCode,  &aPtr, &iPtr, argc, args, argc2, args2, tt_error);
 
					} else if (InstructionCode == FakeCode) {
						/* this is a composite information */
						*tt_error = tt_CompositeCode;
						*SelectionLength = LineLength;
					} else {
						/* this is a regular instruction */
						wchar_t * LabelPtr, *BWLabelPtr;
						short LabelLength, BWLabelLength;
						wchar_t * EOLPtr, *LineBegin;
						LineBegin = CurrentPtr;
						EOLPtr = CurrentPtr + LineLength;
						CurrentPtr = TT_ReadInstructionBooleans (CurrentPtr+StringLength, EOLPtr, InstructionIndex, &InstructionCode, SelectionLength, tt_error);

						if (TT_IsPushBW(InstructionCode) && CompilationStatus->WeAreInPushOnMode && *tt_error == tt_NoError)
						{
							*tt_error = tt_PUSHBWInPushON;
							*SelectionLength = LineLength;
							return LineBegin;
						}
						if ( *tt_error != tt_NoError) 
						{
							DisposeP((void**)&argStore);
							DisposeP((void**)&insStore);
							TT_FreeAllLabelMemory(PushLabels, JrBW, Label, JRList);
							return CurrentPtr;
						};

						/* expand the argument list for a command affected by the SLOOP */
						if ( TT_ExpandArgs( tt_instruction[InstructionIndex].pops, argTypeBuffer, &loop, tt_error ) ) {
							loop = 1;
						}
						if ( *tt_error != tt_NoError) 
						{
							/* error should be SLOOP argument too big */
							DisposeP((void**)&argStore);
							DisposeP((void**)&insStore);
							TT_FreeAllLabelMemory(PushLabels, JrBW, Label, JRList);
							*SelectionLength = SLoopLineLength;
							return SLoopPtr;
						};

						CurrentPtr = TT_ReadInstructionParameters (CurrentPtr, EOLPtr, InstructionIndex, (unsigned char) InstructionCode, asm_ppDescription1, CompilationStatus->WeAreInPushOnMode, 
															argTypeBuffer, &argc, args, &argc2, args2, &LabelPtr, &LabelLength, &BWLabelPtr, &BWLabelLength, 
															SelectionLength, MaxFunctionDefs,BinaryOffset, flashingPoints,tt_error);				
						if ( *tt_error != tt_NoError) 
						{
							DisposeP((void**)&argStore);
							DisposeP((void**)&insStore);
							TT_FreeAllLabelMemory(PushLabels, JrBW, Label, JRList);
							return CurrentPtr;
						};

						if ( InstructionCode == asm_SLOOP ) {
							/* OK we got the arguments */
							if ( CompilationStatus->WeAreInPushOnMode ) {
								SLoopLineLength = LineLength;
								SLoopPtr = LineBegin;
								loop = args[0];
							} else {
								loop = 1;
							}
						}
					
						if (TT_IsJR(InstructionCode)) {
							numberofArgs = (short)(ptrdiff_t)(aPtr - argStore); /* number of arguments */
							numberofInstructions = (short)(ptrdiff_t)(iPtr - insStore); /* number of instructions */						
						
							if (LabelLength != 0 || BWLabelLength != 0) 
							{
								NeedTwoPass = true; /* we have a label to replace by the real value in the second pass */

	    						CurrentPtr = TT_SaveJR( numberofArgs+argc /* we need to count for the boolean if given as an argument, hence the + argc */
	    								, numberofInstructions, CurrentPtr, LabelPtr, LabelLength, BWLabelPtr, BWLabelLength, JRList, JrBW, argStore, SelectionLength , tt_error);
	    					}
	    				};
	    				

						if (InstructionCode == 0x8e /* INSTCTRL */ && asmType != asmPREP) 
						{
							*tt_error = tt_INSTCTRLnotInPreProgram;
							*SelectionLength = LineLength;
							return LineBegin;
						}

	    				if (TT_IsIDEF_FDEFInstruction(InstructionCode) && asmType == asmGLYF)
	    				{
	    					 *tt_error = tt_IDEF_FDEFinGlyphProgram;
							*SelectionLength = LineLength;
							return LineBegin;
	    				}
	    			
						if (CurrentPtr > SelStartPtr && *BinaryOffset == -1)
						{
							*BinaryOffset = (short)(ptrdiff_t)(iPtr - insStore);
						}

						if (TT_IsPushBW(InstructionCode))
						{
							if (iPtr + 1 + 2 * argc > InnerBinaryOutMaxPtr)
							{
								*tt_error = tt_ProgramTooBig;
								return CurrentPtr;
							}
						} else
						{
							if (iPtr + 1 > InnerBinaryOutMaxPtr)
							{
								*tt_error = tt_ProgramTooBig;
								return CurrentPtr;
							}
							if ((char*)aPtr + argc + argc2 > (char*)InnerArgStoreMaxPtr)
							{
								*tt_error = tt_TooManyArguments;
								return CurrentPtr;
							}
						}

						TT_StoreArgumentsAndInstruction((unsigned char)InstructionCode,  &aPtr, &iPtr, argc, args, argc2, args2, tt_error);
					}
				}
				
			}
		}
		
		
		
	}
	
	if (*tt_error != tt_NoError) 
	{
		DisposeP((void**)&argStore);
		DisposeP((void**)&insStore);
		TT_FreeAllLabelMemory(PushLabels, JrBW, Label, JRList);
		return CurrentPtr;
	};
	
	if (RecursionLevel > 1) 
	{
		*tt_error = tt_MissingEndBlock;
	} else {
		/* write the new code */
		numberofArgs = (short)(ptrdiff_t)(aPtr - argStore); 		/* number of arguments */
		numberofInstructions = (short)(ptrdiff_t)(iPtr - insStore); /* number of instructions */
		
		if (NeedTwoPass)
		{
			CurrentPtr = TT_FindLabelError(PushLabels, JrBW, JRList, Label, CurrentPtr, SelectionLength, tt_error) ; 

			if (*tt_error != tt_NoError) 
			{
				DisposeP((void**)&argStore);
				DisposeP((void**)&insStore);
				TT_FreeAllLabelMemory(PushLabels, JrBW, Label, JRList);
				return CurrentPtr;
			};
   	 		TT_JRpushON_ReplaceLabel( JRList, Label, argStore, tt_error);
			TT_JRpushOFF_ReplaceLabel( JrBW, PushLabels, Label, tt_error);
		}

		if (*tt_error == tt_NoError) 
			*BinaryLength = TT_WriteOutBlock( BinaryOut, (unsigned char *)BinaryOutEndPtr, BinaryOffset, AddOffset, argStore, insStore, numberofArgs, numberofInstructions, tt_error);
		*StackNeed = *StackNeed + numberofArgs;
	}	
	
	DisposeP((void**)&argStore);
	DisposeP((void**)&insStore);
	TT_FreeAllLabelMemory(PushLabels, JrBW, Label, JRList);
	return CurrentPtr;
}


wchar_t *TT_Compile(
	/* source text, pointer to the begining, the end and the cursor position (to be able to trace until that line) */
		wchar_t *StartPtr, wchar_t * EndPtr, wchar_t * SelStartPtr, 
	/* pointer to the output buffer, it's maximal length and return the length of the resulting binary */
		unsigned char * BinaryOut, short MaxBinaryLength, long * BinaryLength, 
	/* offset in the instruction stream to the instruction corresponding to the cursor position, used for trace mode */
		short * BinaryOffset, 
	/* length of the text to be selected in case of error */
		long * SelectionLength,
	/* line number where the first error occur */
		short * ErrorLineNb,
	/* return : approximate stack need, higher function number */
		short * StackNeed, short * MaxFunctionDefs, 

	/* for the DovMan partial compilation feature that flash points referenced by the current command */
		tt_flashingPoints * flashingPoints,

		ASMType asmType, /* asmGLYF, asmPREP, or asmFPGM */
	/* error code, return tt_NoError if no error */
		short * tt_error);
		
wchar_t *TT_Compile(wchar_t *StartPtr, wchar_t * EndPtr, wchar_t * SelStartPtr, unsigned char * BinaryOut, long MaxBinaryLength, 
		long * BinaryLength, short * BinaryOffset, long * SelectionLength, short * ErrorLineNb, 
		short * StackNeed, short * MaxFunctionDefs,
		/* for the DovMan partial compilation feature that flash points referenced by the current command */
		tt_flashingPoints * flashingPoints,
		ASMType asmType, short * tt_error)
{
	tt_CompilationStatus CompilationStatus; 
	wchar_t * Result;
	char * BinaryOutEndPtr;
	*ErrorLineNb = 0;
	
	BinaryOutEndPtr = (char *) (BinaryOut + MaxBinaryLength);

	CompilationStatus.WeAreInsideAnIF 	= false;			
	CompilationStatus.NumberOfEmbeddedIF 	= 0;			
	CompilationStatus.WeAreInsideAnFDEF 	= false;			
	CompilationStatus.WeAreInsideAnIDEF 	= false;			
	CompilationStatus.WeAreInPushOnMode 	= true;
	CompilationStatus.WeAreInPrePushMode 	= true;
	CompilationStatus.WeAreInsideGHBlock 	= false;
	CompilationStatus.LastCommandWasAnIF 	= false; 
	CompilationStatus.LastCommandWasAnELSE 	= false;
	CompilationStatus.LastCommandWasAnFDEF 	= false; 
	CompilationStatus.LastCommandWasAnIDEF 	= false;
	CompilationStatus.LastCommandWasAnJUMP 	= false;
	CompilationStatus.LastCommandWasAnEND 	= false;

	flashingPoints->NumberOfFlashingPoint = 0;
	
	Result = TT_InnerCompile(StartPtr, EndPtr, SelStartPtr, BinaryOut, BinaryOutEndPtr, 
		BinaryLength, BinaryOffset, SelectionLength, ErrorLineNb, 
		StackNeed, MaxFunctionDefs, 1, &CompilationStatus, flashingPoints, asmType, tt_error);
	
	if (*tt_error != tt_NoError) return Result; /* avoid to rewrite the error code by a junk code */
		
	if (CompilationStatus.NumberOfEmbeddedIF != 0)
	{
		*tt_error = tt_IFwithoutEIF;
	}
	if (CompilationStatus.WeAreInsideAnFDEF)
	{
		*tt_error = tt_FDEFwithoutENDF;
	}
	if (CompilationStatus.WeAreInsideAnIDEF)
	{
		*tt_error = tt_IDEFwithoutENDF;
	}
	
	return Result;
}

void TT_GetErrorString (short ErrorNb, wchar_t * ErrorString);
void TT_GetErrorString (short ErrorNb, wchar_t * ErrorString)
{

	switch (ErrorNb ) {
		case tt_NoError:
			swprintf( ErrorString, L"There is no Error");
			break;
		case tt_EmbeddedComment:
			swprintf( ErrorString, L"Nested comment");
			break;
		case tt_UnterminatedComment:
			swprintf( ErrorString, L"Unterminated comment");
			break;
		case tt_UnknownSwitch:
			swprintf( ErrorString, L"Unknown compiler switch");
			break;
		case tt_UnknownInstruction:
			swprintf( ErrorString, L"Unknown instruction");
			break;
		case tt_TwoInstructionsInSameLine:
			swprintf( ErrorString, L"End of line expected");
			break;
		case tt_BooleanFlagsMissing:
			swprintf( ErrorString, L"bool flags missing");
			break;
		case tt_WrongNumberOfBoolean:
			swprintf( ErrorString, L"Wrong number of boolean flags");
			break;
		case tt_TooManyBooleans:
			swprintf( ErrorString, L"Too many booleans");
			break;
		case tt_UnrecognizedBoolean:
			swprintf( ErrorString, L"Unrecognized boolean flag");
			break;
		case tt_MissingClosingBracket:
			swprintf( ErrorString, L"Missing closing bracket");
			break;
		case tt_SLOOPArgumentBufferTooSmall:
			swprintf( ErrorString, L"SLOOP number too big, the compiler cannot handle such a big number");
			break;
		case tt_EmptyParameterList:
			swprintf( ErrorString, L"Missing comma between parameters or empty parameter list");
			break;
		case tt_UnableToParseArgument:
			swprintf( ErrorString, L"Unable to parse argument");
			break;
		case tt_MissingParameters:
			swprintf( ErrorString, L"Missing parameters or missing comma between parameters ");
			break;
		case tt_PointNbOutOfRange:
			swprintf( ErrorString, L"Point number out of range");
			break;
		case tt_CVTIndexOutOfRange:
			swprintf( ErrorString, L"CVT index out of range");
			break;
		case tt_StorageIndexOutOfRange:
			swprintf( ErrorString, L"Storage number out of range");
			break;
		case tt_ContourNbOutOfRange:
			swprintf( ErrorString, L"Contour number out of range");
			break;
		case tt_FunctionNbOutOfRange:
			swprintf( ErrorString, L"Function number out of range");
			break;
		case tt_ArgumentOutOfRange:
			swprintf( ErrorString, L"Argument number out of range");
			break;
		case tt_ArgumentIndexOutOfRange:
			swprintf( ErrorString, L"Compiler Error! Argument index out of range");
			break;
		case tt_NotEnoughMemory:
			swprintf( ErrorString, L"Not enough memory");
			break;
		case tt_DeltaListMissing:
			swprintf( ErrorString, L"Delta, parameter list missing");
			break;
		case tt_DeltaOpeningParenthesisMissing:
			swprintf( ErrorString, L"Delta, opening parenthesis missing");
			break;
		case tt_DeltaClosingParenthesisMissing:
			swprintf( ErrorString, L"Delta, closing parenthesis missing");
			break;
		case tt_PointSizeOutOfRange:
			swprintf( ErrorString, L"Delta, point size out of range");
			break;
		case tt_DeltaDenominatorMissing:
			swprintf( ErrorString, L"Delta, denominator missing, format should be eg: (19 @12 3/8)");
			break;
		case tt_DeltaWrongDenominator:
			swprintf( ErrorString, L"Delta, wrong denominator, format should be eg: (19 @12 3/8)");
			break;
		case tt_DeltaAtSignMissing:
			swprintf( ErrorString, L"Delta, @ sign missing, format should be eg: (19 @12 3/8)");
			break;
		case tt_DeltaClosingBracketMissing:
			swprintf( ErrorString, L"Delta, closing bracket missing");
			break;
		case tt_TooManyDeltas:
			swprintf( ErrorString, L"Delta, too many deltas in the same line");
			break;
		case tt_DeltaOORangePpem:
			swprintf( ErrorString, L"Delta, out of range ppem for Delta");
			break;
		case tt_TooManyLabels:
			swprintf( ErrorString, L"Too many labels in the same block");
			break;
		case tt_LabelTooLong:
			swprintf( ErrorString, L"Label too long, limited to %hd character",(short) (MAXLABELLENGTH-1));
			break;
		case tt_DuplicateLabel:
			swprintf( ErrorString, L"Same label used twice");
			break;
		case tt_EndWithoutBegin:
			swprintf( ErrorString, L"#END without corresponding #BEGIN");
			break;
		case tt_MissingEndBlock:
			swprintf( ErrorString, L"End(s) of block, #END, missing");
			break;
		case tt_TooManyEnbeddedBlocks:
			swprintf( ErrorString, L"Too many levels of nested blocks, limit = %hd",(short) (MAXBLOCKEMBEDDING-1));
			break;
		case tt_CompositeCode:
			swprintf( ErrorString, L"Composite commands mixed into TrueType code");
			break;
		case tt_VoidLabel:
			swprintf( ErrorString, L"NULL label, must have at least one character");
			break;
		case tt_LabelNotFound:
			swprintf( ErrorString, L"Corresponding label not found");
			break;
		case tt_ExpectingAComma:
			swprintf( ErrorString, L"#PUSH argument list, missing arguments or missing comma");
			break;
		case tt_TooManyPushArgs:
			swprintf( ErrorString, L"#PUSH, too many arguments, limit = %hd", (short)(PUSH_ARG));
			break;
		case tt_ParseOverflow:
			swprintf( ErrorString, L"Number too large to be parsed, larger than 32,767");
			break;
		case tt_JRExpectingABracket:
			swprintf( ErrorString, L"JR instruction in PushOff mode, expecting an opening bracket");
			break;
		case tt_JRExpectingABWLabel:
			swprintf( ErrorString, L"JR instruction in PushOff mode, expecting an Bn or a Wn label");
			break;
		case tt_JRExpectingAEqual:
			swprintf( ErrorString, L"JR instruction in PushOff mode, expecting an equal between labels");
			break;
		case tt_JRExpectingALabel:
			swprintf( ErrorString, L"JR instruction in PushOff mode, expecting a #label");
			break;
		case tt_JumpTooBigForByte:
			swprintf( ErrorString, L"#PUSH, Bn : jump too far to be a byte");
			break;
		case tt_JumpNegativeForByte:
			swprintf( ErrorString, L"#PUSH, Bn : negative jump cannot be a byte, use Wn");
			break;
		case tt_EIFwithoutIF:
			swprintf( ErrorString, L"EIF without IF");
			break;
		case tt_ELSEwithoutIF:
			swprintf( ErrorString, L"ELSE without IF");
			break;
		case tt_ELSEwithinELSE:
			swprintf( ErrorString, L"expecting a EIF");
			break;
		case tt_TooManyEmbeddedIF:
			swprintf( ErrorString, L"too many embedded IF");
			break;
		case tt_ExpectingaBEGIN:
			swprintf( ErrorString, L"expecting a #BEGIN after IF[], ELSE[], FDEF[] or IDEF[] in push on mode");
			break;
		case tt_FDEFInsideFDEF:
			swprintf( ErrorString, L"FDEF found within FDEF - ENDF pair");
			break;
		case tt_FDEFInsideIDEF:
			swprintf( ErrorString, L"FDEF found within IDEF - ENDF pair");
			break;
		case tt_IDEFInsideFDEF:
			swprintf( ErrorString, L"IDEF found within FDEF - ENDF pair");
			break;
		case tt_IDEFInsideIDEF:
			swprintf( ErrorString, L"IDEF found within IDEF - ENDF pair");
			break;
		case tt_ENDFwithoutFDEF:
			swprintf( ErrorString, L"ENDF found without corresponding FDEF or IDEF");
			break;
		case tt_IFwithoutEIF:
			swprintf( ErrorString, L"IF without corresponding EIF");
			break;
		case tt_FDEFwithoutENDF:
			swprintf( ErrorString, L"FDEF without corresponding ENDF");
			break;
		case tt_IDEFwithoutENDF:
			swprintf( ErrorString, L"IDEF without corresponding ENDF");
			break;

		case tt_PUSHONwhenAlreadyOn:
			swprintf( ErrorString, L"#PUSHON when already in push on mode");
			break;
		case tt_PUSHOFFwhenAlreadyOff:
			swprintf( ErrorString, L"#PUSHOFF when already in push off mode");
			break;
		case tt_IFgoingAcrossBlocks:
			swprintf( ErrorString, L"IF statement going across block (#BEGIN #END) boundaries");
			break;
		case tt_FDEFgoingAcrossBlocks:
			swprintf( ErrorString, L"FDEF statement going across block (#BEGIN #END) boundaries");
			break;
		case tt_IDEFgoingAcrossBlocks:
			swprintf( ErrorString, L"IDEF statement going across block (#BEGIN #END) boundaries");
			break;

		case tt_IDEF_FDEFinGlyphProgram:
			swprintf( ErrorString, L"FDEF and IDEF can be called only from font program or the pre-program");
			break;

		case tt_INSTCTRLnotInPreProgram:
			swprintf( ErrorString, L"INSTCTRL[] can only be called from the pre-program");
			break;
		case tt_ProgramTooBig:
			swprintf( ErrorString, L"Program too big, if you really need such a big program, call product support");
			break;
		case tt_TooManyArguments:
			swprintf( ErrorString, L"Program too big (too many arguments), if you really need such a big program, call product support");
			break;
		case tt_DELTAWithoutArguments:
			swprintf( ErrorString, L"DELTA without argument in PUSHON mode");
			break;
		case tt_DELTAWithArguments:
			swprintf( ErrorString, L"DELTA with arguments in PUSHOFF mode");
			break;
		case tt_PUSHBWInPushON:
			swprintf( ErrorString, L"Illegal use of PUSHB or PUSHW in PUSHON mode, use #PUSH instead");
			break;
		case tt_WildCardInPush:
			swprintf( ErrorString, L"Illegal use * in a PUSH instruction");
			break;

		case tt_NotImplemented:
			swprintf( ErrorString, L"Not implemented");
			break;
			
		default :
			swprintf( ErrorString, L"Unknown error!");
			break;
	}

}


typedef struct {
	const wchar_t			*name;			/* Apple instruction name */
	const wchar_t			*description;	/* descriptive info */
	const wchar_t			*pops;			/* What the instruction pops   */
	const wchar_t			*booleans;		/* booleans */
} co_InstructionType;


typedef struct {
	wchar_t	type;
	wchar_t	Reserved1; /* for struct alignment */
	wchar_t	Reserved2;
	wchar_t	Reserved3;
	const wchar_t	*explanation;
	short     lowestValidValue;		/* 6-7-90 JTS Range Checking Adds */
	short		highestValidValue;		/* 6-7-90 JTS Range Checking Adds */
} co_ParameterDescriptionType;

/* lower case is reserved for loop variable dependent pops */
co_ParameterDescriptionType co_ppDescription[] = {
	{ L'G',L' ',L' ',L' ',	L"Glyph Index" ,0 ,(short)USHORTMAX}, // allow full unicode range for glyph index B.St.
	{ L'O',L' ',L' ',L' ',	L"Offset " ,SHORTMIN ,SHORTMAX},  
	{ L'P',L' ',L' ',L' ',	L"Point Number" ,0 ,SHORTMAX},
	{ L'M',L' ',L' ',L' ',	L"Matrix elements, can be either integer or real numbers" ,0,0}
};	

#define NumberOfParameterDescrEntries  4 

const co_InstructionType co_instruction[] = {
/****  Instruction name,			descriptive info,                   pops,   booleans *****/
	{ L"OFFSET",					L"Component character",				L"GOO",  	 L"R" },
	{ L"SOFFSET",					L"Component character",				L"GOOMMMM",  L"R" },
	{ L"ANCHOR",					L"Component character",				L"GPP",   	 L"" },
	{ L"SANCHOR",					L"Component character",				L"GPPMMMM",  L"" },
	{ L"OVERLAP",					L"Component character",				L"",  	 	 L"" },
	{ L"NONOVERLAP",				L"Component character",				L"",  	 	 L"" },
	{ L"USEMYMETRICS",				L"Component character",				L"",  	 	 L"" },
	{ L"SCALEDCOMPONENTOFFSET",		L"Component character",				L"",		 L"" },
	{ L"UNSCALEDCOMPONENTOFFSET",	L"Component character",				L"",		 L"" }
};

typedef struct {
	wchar_t type;
	wchar_t	code;
	char	result;
} co_BooleanTranslationType;

const co_BooleanTranslationType co_booleanTranslation[] = {
	{ L'R', L'R',	1 },
	{ L'R', L'r',	0 }
};

#define co_NumberOfBooleanTranslations (sizeof(co_booleanTranslation) / sizeof(co_BooleanTranslationType))


#define CONUMBEROFINSTRUCTIONS (sizeof(co_instruction) / sizeof(co_InstructionType))


wchar_t *CO_Parse2_14Number( wchar_t *p, wchar_t *endP,short *Number, short * error );
wchar_t *CO_Parse2_14Number( wchar_t *p, wchar_t *endP,short *Number, short * error )
{
	short i, negatif;
	long tempNumber;
	
	/* skip white space */
	while ( *p == L' ' && p < endP)
		p++;
		
	tempNumber = 0;
	negatif = false;
	i = 0;
	
	if ( *p == L'-'  && p < endP )
	{
		negatif = true;
		p++;
	}
		
	if ( *p == L'+'  && p < endP && !negatif)
	/* we accept + or - but don't want -+ together */
	{
		p++;
	}

	/* read the first number */
	if ( *p >= L'0' && *p <= L'9' && p < endP ) 
	{
		if  ( *p > L'2') {
			*error = tt_ParseOverflow;
			return p;
		}
		tempNumber = tempNumber * 10 + (short) *p - (short) '0';
		p++;
		i++;
	} else {
		*error = tt_UnableToParseArgument;
		return p;
	}


	if ( *p >= L'0' && *p <= L'9' && p < endP ) 
	{
		*error = tt_ParseOverflow;
		return p;
	}

	if ( *p == L'.' && p < endP ) 
	{
		p++;
		/* read the fractional part */
		while ( *p >= L'0' && *p <= L'9' && p < endP ) 
		{
			if (i < 5) /* ignore the remainding digits */
			{
				tempNumber = tempNumber * 10 + (short) *p - (short) '0';
				i++;
			}
			p++;
		}
	}

	while (i < 5)
	{
		tempNumber = tempNumber * 10;
		i++;
	}
	
	if (i == 0) *error = tt_UnableToParseArgument;
	


	tempNumber = tempNumber * 0x4000 / 10000;
	if (negatif) tempNumber = - tempNumber;

	if (tempNumber > SHORTMAX)
	{
		tempNumber = SHORTMAX;
	}

	if (tempNumber < SHORTMIN)
	{
		tempNumber = SHORTMIN;
	}
	
	*Number = (short)tempNumber;
	return p;
		
}

wchar_t * CO_ReadInstructionParameters (wchar_t * CurrentPtr, wchar_t * EOLPtr, short InstructionIndex, 
				co_ParameterDescriptionType co_ppDescription[], short  *argc,short *args,
				long * SelectionLength,  short * co_error);
wchar_t * CO_ReadInstructionParameters (wchar_t * CurrentPtr, wchar_t * EOLPtr, short InstructionIndex, 
				co_ParameterDescriptionType co_ppDescription[], short  *argc,short *args,
				long * SelectionLength,  short * co_error)
{
	short	argindex, argNb;
	wchar_t * tempP;
	const wchar_t * argTypeBuffer = co_instruction[InstructionIndex].pops;
	*argc = 0;
	argindex = 0;
	bool inRange;
	
	argNb = (short)STRLENW(co_instruction[InstructionIndex].pops);

	for (argindex=0;argindex<argNb;argindex++) {
		/* skip spaces */
		

		while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
			CurrentPtr++;

		if (CurrentPtr >= EOLPtr) break;

		if ( *CurrentPtr++ != L',' )
		{
			*co_error = co_EmptyParameterList;
			return (CurrentPtr-1);
		}
		
		/* skip extra spaces */
		while ( *CurrentPtr == L' ' && CurrentPtr <= EOLPtr)
			CurrentPtr++;

		if (CurrentPtr >= EOLPtr) break;

		if ( co_instruction[InstructionIndex].pops[argindex] == L'M' ) 
		{
			/* there should be a 2.14 number */
			tempP = CurrentPtr;
			CurrentPtr = CO_Parse2_14Number(CurrentPtr, EOLPtr, &args[argindex], co_error);
			*argc = *argc + 1;
			if (*co_error != co_NoError) return CurrentPtr;
		} else {
			short foundPP, PPindex;

			/* there should be a number */
			tempP = CurrentPtr;
			CurrentPtr = TT_ParseNumber(CurrentPtr, EOLPtr, &args[argindex], SelectionLength, co_error);
			if (*co_error != co_NoError) return CurrentPtr;

			foundPP = 0;    /* Have not found it yet... */
			for (PPindex=0;(PPindex<NumberOfParameterDescrEntries && !foundPP ) ; PPindex++) {
				if (co_instruction[InstructionIndex].pops[argindex] == co_ppDescription[PPindex].type) {
					foundPP=1;   /* Exit loop for speed's sake */
					if (co_instruction[InstructionIndex].pops[argindex] == L'G')
						inRange = (unsigned short)co_ppDescription[PPindex].lowestValidValue <= (unsigned short)args[argindex] && (unsigned short)args[argindex] <= (unsigned short)co_ppDescription[PPindex].highestValidValue; // allow full unicode range for glyph index B.St.
					else
						inRange = co_ppDescription[PPindex].lowestValidValue <= args[argindex] && args[argindex] <= co_ppDescription[PPindex].highestValidValue;
					
					if (!inRange) {
						*SelectionLength = (long) (CurrentPtr - tempP);
						CurrentPtr = tempP;
						if ( co_instruction[InstructionIndex].pops[argindex] == L'P' ) {
							*co_error = co_PointNbOutOfRange;
						} else if ( co_instruction[InstructionIndex].pops[argindex] == L'G' ) {
							*co_error = co_GlyphIndexOutOfRange; // <----- should be in range 0..USHORTMAX
						} else {
							*co_error = co_ArgumentOutOfRange;
						}/*if */ 
					} /* if */
				} /*if*/
			} /*for*/

			*argc = *argc + 1;
			if (*co_error != co_NoError) return CurrentPtr;
		}

		
	}


		if (argindex < (short)STRLENW(co_instruction[InstructionIndex].pops)) 
		{
			*co_error = co_MissingParameters;
			return CurrentPtr;
		}

	return CurrentPtr;

} // CO_ReadInstructionParameters


wchar_t * CO_ReadInstructionBooleans (wchar_t * CurrentPtr, wchar_t * EOLPtr, short InstructionIndex, short * RoundingCode, long * Selectionlength, short * co_error);
wchar_t * CO_ReadInstructionBooleans (wchar_t * CurrentPtr, wchar_t * EOLPtr, short InstructionIndex, short * RoundingCode, long * Selectionlength, short * co_error)
{
	short booleanCount, booleanShift, NBOfBooleans, found,  k;
	wchar_t * tempP;
	if 	( *CurrentPtr != L'[' /* ] for balance */ || CurrentPtr >=  EOLPtr) 
	{ 
		*co_error = co_BooleanFlagsMissing;
		*Selectionlength = 1;
		return CurrentPtr;
	};
	tempP = CurrentPtr;
	CurrentPtr ++;
	NBOfBooleans = (short)STRLENW(co_instruction[InstructionIndex].booleans) ;
	booleanShift = NBOfBooleans;

	for (booleanCount = 0;
		// for balance L'[' 
				booleanCount < NBOfBooleans && *CurrentPtr != L']' && CurrentPtr < EOLPtr ; 
				booleanCount++ )
	{
		booleanShift--;
		for ( found = k = 0; k < co_NumberOfBooleanTranslations; k++ ) {
			if ( co_booleanTranslation[k].type == co_instruction[InstructionIndex].booleans[booleanCount] &&
								 (co_booleanTranslation[k].code) == *CurrentPtr ) 
			{
				found = 1;
				*RoundingCode = co_booleanTranslation[k].result;
			}
		}
		if (!found)
		{
			*co_error = co_UnrecognizedBoolean;
			*Selectionlength = 1;
			return CurrentPtr;
		}
			
		CurrentPtr++;
	}
			
	if (booleanCount != NBOfBooleans)
	{
		*co_error = co_WrongNumberOfBoolean;
		*Selectionlength = (short)(CurrentPtr-tempP)+1;
		return tempP;
	};
		
	if 	( (CurrentPtr >=  EOLPtr) || (*CurrentPtr == L',' ) ) 
	{ 
		*co_error = co_MissingClosingBracket;
		*Selectionlength = (short)(CurrentPtr-tempP);
		return tempP;
	};

	if 	( /* [ for balance */  *CurrentPtr != L']' ) 
	{ 
		*co_error = co_TooManyBooleans;
		*Selectionlength = (short)(CurrentPtr-tempP)+1;
		return tempP;
	};
	CurrentPtr ++;
	return CurrentPtr;
}

/*
 * returns true on error compile the composite information
 */

wchar_t *CO_Compile(TrueTypeFont * font, TrueTypeGlyph * glyph, wchar_t *StartPtr, wchar_t * EndPtr, short *numCompositeContours, short *numCompositePoints, long * SelectionLength, short * co_error);
wchar_t *CO_Compile(TrueTypeFont * font, TrueTypeGlyph * glyph, wchar_t *StartPtr, wchar_t * EndPtr, short *numCompositeContours, short *numCompositePoints, long * SelectionLength, short * co_error) {
//	B.St.'s DISCLAIMER: So far, I've barely done a minimum to make this hideous piece of code somewhat understandable.
//	The way this appears to work is by looking at a pair of composite instructions (prevInstrIndex and currInstrIndex)
//	in a window that is being slided along the code. This appears to be used to determine, whether such instructions
//	as USEMYMETRICS or OVERLAP and NONOVERLAP (the latter two being obsolete, as far as I understand the TT manual),
//	are properly followed by further composite instructions...
	short	LineNb,LastLineCompiled,RoundingCode,currInstrIndex,prevInstrIndex,args[256],argc;
	bool currInstrIsCompInstr,prevInstrIsCompInstr;
	long	LineLength,StringLength;
	wchar_t 	*CurrentPtr,*EOLPtr;
	TTCompositeProfile compositeProfile;
	sfnt_glyphbbox Newbbox; // for composite glyph
	
	Newbbox.xmin = SHRT_MAX;
	Newbbox.ymin = SHRT_MAX;
	Newbbox.xmax = SHRT_MIN;
	Newbbox.ymax = SHRT_MIN;
	
	*SelectionLength = 0;
	*co_error = co_NoError;
	prevInstrIsCompInstr = false;

    glyph->componentSize = 0;
    compositeProfile.GlobalUSEMYMETRICS	   = 0;
	compositeProfile.GlobalSCALEDCOMPONENTOFFSET = 0;
	compositeProfile.GlobalUNSCALEDCOMPONENTOFFSET = 0;
	compositeProfile.GlobalNON_OVERLAPPING = 1;
	compositeProfile.GlobalMORE_COMPONENTS = 1;
	compositeProfile.nextExitOffset   = false;
	
	compositeProfile.numberOfCompositeContours = 0;
	compositeProfile.numberOfCompositePoints   = 0;
	compositeProfile.numberOfCompositeElements = 0;

	compositeProfile.anchorPrevPoints  = -1; 
	
	CurrentPtr = StartPtr;
	LastLineCompiled = 0;
	
	for ( LineNb = 1; CurrentPtr <= EndPtr && *co_error == co_NoError; ) {
		CurrentPtr = TT_SkipCommentsAndEmptyLines( CurrentPtr,EndPtr, &LineNb, co_error );
		if (*co_error != co_NoError) { *SelectionLength = 2; goto failure; }

		if (CurrentPtr == EndPtr) break;

		LineLength = TT_GetLineLength(CurrentPtr, EndPtr);
		
		if (LineNb == LastLineCompiled) { *co_error = co_TwoInstructionsInSameLine; *SelectionLength = LineLength; goto failure; }
		LastLineCompiled = LineNb;
			
		// regular instruction
		StringLength = TT_GetStringLength (CurrentPtr, EndPtr);
		currInstrIndex = 0;
		while (currInstrIndex < CONUMBEROFINSTRUCTIONS && !(StringLength == (long)STRLENW(co_instruction[currInstrIndex].name) && wcsncmp(CurrentPtr, co_instruction[currInstrIndex].name, StringLength) == 0)) currInstrIndex++;
		currInstrIsCompInstr = currInstrIndex < CONUMBEROFINSTRUCTIONS;
		
		if (prevInstrIsCompInstr) {
			font->UpdateCompositeProfile(glyph,&compositeProfile,currInstrIsCompInstr ? CO_CompInstrFollow : CO_StdInstrFollow, RoundingCode, prevInstrIndex, args, argc, &Newbbox, co_error);
		}

		if (!currInstrIsCompInstr) goto success; // because it is assumed to be the first TrueType instruction

		EOLPtr = CurrentPtr + LineLength;
		prevInstrIndex = currInstrIndex;
		CurrentPtr = CO_ReadInstructionBooleans (CurrentPtr+StringLength, EOLPtr, currInstrIndex, &RoundingCode, SelectionLength, co_error);
		if (*co_error != co_NoError) goto failure;
		
		CurrentPtr = CO_ReadInstructionParameters (CurrentPtr, EOLPtr, currInstrIndex, co_ppDescription, &argc, args, SelectionLength, co_error);
		if (*co_error != co_NoError) goto failure;

		prevInstrIsCompInstr = true;
	}
	
	if (prevInstrIsCompInstr) {
		font->UpdateCompositeProfile(glyph,&compositeProfile,CurrentPtr >= EndPtr ? CO_NothingFollows : CO_StdInstrFollow, RoundingCode, currInstrIndex, args, argc,&Newbbox,co_error);
		if (*co_error != co_NoError) goto failure;
	}

success:
    if (Newbbox.xmin != SHRT_MAX) { // update the bounding box of a composite glyph
    	glyph->xmin = Newbbox.xmin;
    	glyph->ymin = Newbbox.ymin;
    	glyph->xmax = Newbbox.xmax;
    	glyph->ymax = Newbbox.ymax;
	}
	*numCompositeContours = compositeProfile.numberOfCompositeContours;
	*numCompositePoints	  = compositeProfile.numberOfCompositePoints;
	return CurrentPtr;

failure:
	glyph->componentSize  = 0;
	*numCompositeContours = 0;
	*numCompositePoints	  = 0;
	return CurrentPtr;
} // CO_Compile

	
void CO_GetErrorString (short ErrorNb, wchar_t * ErrorString);
void CO_GetErrorString (short ErrorNb, wchar_t * ErrorString)
{

	switch (ErrorNb ) {
		case co_NoError:
			swprintf( ErrorString, L"There is no Error");
			break;
		case tt_EmbeddedComment:
			swprintf( ErrorString, L"Embedded comment");
			break;
		case tt_UnterminatedComment:
			swprintf( ErrorString, L"Unterminated comment");
			break;
		case co_TwoInstructionsInSameLine:
			swprintf( ErrorString, L"End of line expected");
			break;
		case co_BooleanFlagsMissing:
			swprintf( ErrorString, L"bool flags missing");
			break;
		case co_WrongNumberOfBoolean:
			swprintf( ErrorString, L"Wrong number of boolean flags");
			break;
		case co_TooManyBooleans:
			swprintf( ErrorString, L"Too many booleans");
			break;
		case co_UnrecognizedBoolean:
			swprintf( ErrorString, L"Unrecognized boolean flag");
			break;
		case co_MissingClosingBracket:
			swprintf( ErrorString, L"Missing closing bracket");
			break;
		case co_EmptyParameterList:
			swprintf( ErrorString, L"Missing comma between parameters or empty parameter list");
			break;
		case co_MissingParameters:
			swprintf( ErrorString, L"Missing parameters");
			break;
		case co_PointNbOutOfRange:
			swprintf( ErrorString, L"Point number out of range");
			break;
		case co_GlyphIndexOutOfRange:
			swprintf( ErrorString, L"Glyph index out of range");
			break;
		case co_ArgumentOutOfRange:
			swprintf( ErrorString, L"Argument out of range");
			break;
		case tt_ParseOverflow:
			swprintf( ErrorString, L"Number too big to be parsed, bigger than MaxShort");
			break;
		case co_AnchorArgExceedMax:
			swprintf( ErrorString, L"Anchor argument exceed maximum value");
			break;
		case co_AnchorNothingAbove:
			swprintf( ErrorString, L"Composite, no instruction in the line above");
			break;
		case co_2_14Overflow:
			swprintf( ErrorString, L"Composite, number too big for 2.14 float");
			break;
		case co_ComponentSizeOverflow:
			swprintf( ErrorString, L"Composite, too many components");
			break;

		case co_OverlapLastInstruction:
			swprintf( ErrorString, L"Composite, OVERLAP cannot be the last composite command");
			break;
		case co_NonOverlapLastInstruction:
			swprintf( ErrorString, L"Composite, NONOVERLAP cannot be the last composite command");
			break;
		case co_UseMymetricsLastInstruction:
			swprintf( ErrorString, L"Composite, USEMYMETRICS cannot be the last composite command");
			break;
		case co_ScaledComponentOffsetLastInstruction:
			swprintf( ErrorString, L"Composite, SCALEDCOMPONENTOFFSET cannot be the last composite command");
			break;
		case co_UnscaledComponentOffsetLastInstruction:
			swprintf( ErrorString, L"Composite, UNSCALEDCOMPONENTOFFSET cannot be the last composite command");
			break;
		case co_ScaledComponentOffsetAlreadySet:
			swprintf( ErrorString, L"Composite, UNSCALEDCOMPONENTOFFSET (Microsoft compatible) cannot be the used when SCALEDCOMPONENTOFFSET (Apple compatible) is alread used. Only one can be used.");
			break;
		case co_UnscaledComponentOffsetAlreadySet:
			swprintf( ErrorString, L"Composite, SCALEDCOMPONENTOFFSET (Apple compatible) cannot be the used when UNSCALEDCOMPONENTOFFSET (Microsoft compatible) is alread used. Only one can be used.");
			break;
		case co_ComponentChangeOnVariationFont:
			swprintf(ErrorString, L"Composite definition has changed or is not present");
			break; 

		case co_NotImplemented:
			swprintf( ErrorString, L"Not implemented");
			break;
		default :
			swprintf( ErrorString, L"Unknown error!");
			break;
	}

}

bool DisassemComponent(TrueTypeGlyph *glyph, TextBuffer *src, wchar_t errMsg[]) {
	short i,flags,glyphIndex,arg1,arg2,xscale,yscale,scale01,scale10;
	wchar_t c,buf[maxLineSize];
	
	i = 0;
	do {
		flags = glyph->componentData[i++];
		flags = SWAPW(flags);
		glyphIndex = glyph->componentData[i++];
		glyphIndex = SWAPW(glyphIndex);
		if (flags & ARG_1_AND_2_ARE_WORDS) {
			arg1 = glyph->componentData[i++];
			arg1 = SWAPW(arg1);
			arg2 = glyph->componentData[i++];
			arg2 = SWAPW(arg2);
		} else if (flags & ARGS_ARE_XY_VALUES ) { // signed bytes
			signed char *sByte = (signed char *)&glyph->componentData[i];
			arg1 = sByte[0];
			arg2 = sByte[1];
			i++;
		} else { // unsigned bytes, do we ever have those? 
			unsigned char *uByte = (unsigned char *)&glyph->componentData[i];
			arg1 = uByte[0];
			arg2 = uByte[1];
			i++;
		}

		if (flags & WE_HAVE_A_TWO_BY_TWO) {
			xscale	= glyph->componentData[i++];
			xscale  = (signed short)SWAPW(xscale);
			scale01 = glyph->componentData[i++];
			scale01 = (signed short)SWAPW(scale01);
			scale10 = glyph->componentData[i++];
			scale10 = (signed short)SWAPW(scale10);
			yscale	= glyph->componentData[i++];
			yscale  = (signed short)SWAPW(yscale);
		} else if (flags & WE_HAVE_A_SCALE) {
			short word = glyph->componentData[i++];
			xscale	= yscale = SWAPW(word);			
			scale01	= scale10 = 0;
			flags	= (flags & ~WE_HAVE_A_SCALE) | WE_HAVE_A_TWO_BY_TWO;
		} else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
			xscale	= glyph->componentData[i++];
			xscale  = (signed short)SWAPW(xscale);
			yscale	= glyph->componentData[i++];
			yscale  = (signed short)SWAPW(yscale);
			scale01	= scale10 = 0;
			flags	= (flags & ~WE_HAVE_AN_X_AND_Y_SCALE) | WE_HAVE_A_TWO_BY_TWO;
 		} 
		
		if (i > glyph->componentSize) {
			swprintf(errMsg,L"DisassemComponent: Component data size %hd does not match parsed %hd size", glyph->componentSize,i);
			return false;
		}

		if (flags & USE_MY_METRICS) src->Append(L"USEMYMETRICS[]\r"); 
		else if (!(flags & NON_OVERLAPPING)) src->Append(L"OVERLAP[]\r");
		// If both UNSCALED_COMPONENT_OFFSET and SCALED_COMPONENT_OFFSET are set we will only emit the UNSCALEDCOMPONENTOFFSET for Microsoft compatibility. These are mutually exclusive flags.
		if (flags & UNSCALED_COMPONENT_OFFSET) src->Append(L"UNSCALEDCOMPONENTOFFSET[]\r"); else if (flags & SCALED_COMPONENT_OFFSET) src->Append(L"SCALEDCOMPONENTOFFSET[]\r"); 
		c = flags & ROUND_XY_TO_GRID ? L'R' : L'r';
	 	
		if (flags & WE_HAVE_A_TWO_BY_TWO) {
			if (flags & ARGS_ARE_XY_VALUES) swprintf(buf,L"SOFFSET[%c",c); else swprintf(buf,L"SANCHOR[");
			swprintf(&buf[STRLENW(buf)],L"], %hu, %hd, %hd, %.4f, %.4f, %.4f, %.4f\r",
				glyphIndex,arg1,arg2,
			 	(double)xscale /16384.0 + 0.000005, (double)scale01/16384.0 + 0.000005, 
			 	(double)scale10/16384.0 + 0.000005, (double)yscale /16384.0 + 0.000005);	
		} else {
	 		if (flags & ARGS_ARE_XY_VALUES) swprintf(buf,L"OFFSET[%c",c); else swprintf(buf,L"ANCHOR[");
			swprintf(&buf[STRLENW(buf)],L"], %hu, %hd, %hd\r",glyphIndex,arg1,arg2);	
		}
		src->Append(buf);
	} while (flags & MORE_COMPONENTS);
	
	return true;
} // DisassemComponent

bool TTAssemble(ASMType asmType, TextBuffer* src, TrueTypeFont* font, TrueTypeGlyph* glyph,
	long maxBinLen, unsigned char* bin, long* actBinLen, bool variationCompositeGuard, long* errPos, long* errLen, wchar_t errMsg[]) {

	wchar_t* startPtr, * endPtr, * SelStartPtr, * tempPtr;
	short BinaryOffset, CompileError = co_NoError, StackNeed, MaxFunctionDefs, ErrorLineNb, componentSize, numCompositeContours, numCompositePoints, maxContourNumber, maxPointNumber;
	long srcLen, highestCvtNum;
	sfnt_maxProfileTable profile;
	short componentData[MAXCOMPONENTSIZE];

	tt_flashingPoints MyflashingPoints;

	profile = font->GetProfile();
	highestCvtNum = font->TheCvt()->HighestCvtNum();

	*actBinLen = 0; *errPos = -1; *errLen = 0;

	if (asmType == asmGLYF && glyph->composite && src->TheLength() == 0 && !DisassemComponent(glyph, src, errMsg)) return false;

	srcLen = src->TheLength();
	startPtr = (wchar_t*)NewP((srcLen + 1L) * sizeof(wchar_t)); // if lengths are zero, we are guaranteed a pointer if + 1L

	SelStartPtr = startPtr;
	size_t len; 
	src->GetText(&len, startPtr);
	tempPtr = startPtr;
	endPtr = startPtr + srcLen;

	switch (asmType) {
	case asmFPGM:
		TT_SetRangeCheck(0, 0, profile.maxElements - 1, 0, (short)highestCvtNum, profile.maxStorage - 1);
		break;
	case asmPREP:
		/* twilight point may be used in the pre-program for italic slope,... */
		TT_SetRangeCheck(0, profile.maxTwilightPoints, profile.maxElements - 1, profile.maxFunctionDefs - 1, (short)highestCvtNum, profile.maxStorage - 1);
		break;
	case asmGLYF:
		/* compile first the composite information */
		componentSize = glyph->componentSize;
		memcpy(componentData, glyph->componentData, glyph->componentSize * sizeof(short));
		tempPtr = CO_Compile(font, glyph, tempPtr, endPtr, &numCompositeContours, &numCompositePoints, errLen, &CompileError);
		if (CompileError == co_NoError)
		{
			// If a variation font check to make sure new component size and data matches pre CO_Compile() state. 
			if (font->IsVariationFont() && variationCompositeGuard && (glyph->componentSize > 0 || componentSize > 0))
			{
				const CheckCompositeResult result = CheckCompositeVariationCompatible(componentData, componentSize * sizeof(short), glyph->componentData, glyph->componentSize * sizeof(short));
				if (result == CheckCompositeResult::Fail)
				{
					// VTT should not change definition of glyphs in a variation font. 
					CompileError = co_ComponentChangeOnVariationFont;
					// Revert
					glyph->componentSize = componentSize;
					memcpy(glyph->componentData, componentData, componentSize * sizeof(short));

				}
				else if (result == CheckCompositeResult::Tolerance)
				{
					// VTT should not change definition of glyphs in a variation font. 
					// Revert but no error
					glyph->componentSize = componentSize;
					memcpy(glyph->componentData, componentData, componentSize * sizeof(short));
				}
			}
		}
		if (CompileError == co_NoError) {
			//if (componentSize == 0 && glyph->componentSize > 0) { // we've just created a new composite
			if(glyph->componentSize > 0) {
				maxContourNumber = numCompositeContours; maxPointNumber = numCompositePoints - 1 + PHANTOMPOINTS; // hence pick up-to-date profile data for range ends
			}
			else {
				maxContourNumber = (short)glyph->numContoursInGlyph - 1; maxPointNumber = glyph->endPoint[maxContourNumber] + PHANTOMPOINTS; // else glyph data should still be up-to-date
			}
			TT_SetRangeCheck(maxContourNumber, maxPointNumber, profile.maxElements - 1, profile.maxFunctionDefs - 1, (short)highestCvtNum, profile.maxStorage - 1);
		}
		else {
			*errPos = (long)(tempPtr - startPtr);
			if (*errLen < 1) *errLen = 1; // it is easy to find a selection than a cursor <--- and difficult to get it right in the first place???
			CO_GetErrorString(CompileError, errMsg);
		}
		break;
	}

	if (CompileError == co_NoError) {

		StackNeed = 0;
		MaxFunctionDefs = 0;

		tempPtr = TT_Compile(tempPtr, endPtr, SelStartPtr, bin, maxBinLen, actBinLen,
			&BinaryOffset, errLen, &ErrorLineNb, &StackNeed, &MaxFunctionDefs, &MyflashingPoints, asmType, &CompileError);

		if (CompileError == tt_NoError) {
			font->UpdateAssemblerProfile(asmType, MaxFunctionDefs + 1, StackNeed, (short)(*actBinLen));
		}
		else {
			*errPos = (long)(tempPtr - startPtr);
			if (*errLen < 1) *errLen = 1; // it is easy to find a selection than a cursor <--- and difficult to get it right in the first place???
			TT_GetErrorString(CompileError, errMsg);

			*actBinLen = 0;
			if (asmType == asmGLYF) glyph->componentSize = 0;
		}
	}

	font->UpdateGlyphProfile(glyph);
	DisposeP((void**)&startPtr);

	return CompileError == tt_NoError;

}