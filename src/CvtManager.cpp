// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#define _CRT_SECURE_NO_DEPRECATE 
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <stdio.h> // swprintf
#include <string.h> // wcscpy, memcpy
#include "pch.h"
#include "List.h"
#include "CvtManager.h"
#include "TTEngine.h" // maxPpemSize & al.

//	#define DEBUGCVT
 
/***** from cluster.h, for backwards compatibility with existing "high-level" control value tables (cvt "comments")

 1 1 1 1 1 1 
 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
+---+-----+---+-----+-----------+
| ? |Group| C | Dir |  Feature  |
+---+-----+---+-----+-----------+

14 bits used => use 14th bit to indicate a defined cvt entry

*****/

/* group 3 bits */
#define C_ANYGROUP	0
#define C_UC 		1
#define C_LC 		2
#define C_FIG	 	3
#define C_OTHER	 	4
//#define C_RESERVED1	5
#define C_NONLATIN	5	// @@TM::LIGHT
#define C_RESERVED2	6
#define C_RESERVED3	7
#define groupPos	(colorPos + colorBits)
#define groupBits	3
#define groupMask	(~((unsigned int)(-1) << groupBits))

/* color 2 bits */
#define C_GREY	 	0
#define C_BLACK 	1
#define C_WHITE 	2
#define C_ANYCOLOR	3
#define colorPos	(dirPos + dirBits)
#define colorBits	2
#define colorMask	(~((unsigned int)(-1) << colorBits))

/* direction 3 bits */
#define C_XDIR	 	1
#define C_YDIR	 	2
#define C_DIAG		3
#define C_ANYDIR	4
#define dirPos		(catPos + catBits)
#define dirBits		3
#define dirMask		(~((unsigned int)(-1) << dirBits))

/* features 6 bits */
#define C_DUMMY					 0
#define C_STROKE 				 1
#define C_ROUND 			 	 2
#define C_DISTANCE 				 3
#define C_BEND_RADIUS 			 4
#define C_LSB		 			 5
#define C_RSB					 6
#define C_SERIF_THIN			 7
#define C_SERIF_HEIGHT			 8
#define C_SERIF_EXT				 9
#define C_SERIF_CRVHGT			10 /* for triangular serifs */
#define C_SERIF_OTHER			11 /* for triangular serifs */
#define C_BLACKBODYWIDTH 		12
#define C_SERIFBOTTOM 			13
#define C_BRANCH_BEND_RADIUS 	14 /* Type 2 bends */
#define C_SCOOP_DEPTH			15
#define C_NEW_STRAIGHT			16
#define C_NEW_DIAGONAL			17
#define C_NEW_ANYDIR			18
#define C_SQUARE_HEIGHT			19 // new cvt category for absolute (square) heights
#define C_ROUND_HEIGHT			20 // new cvt category for relative (round, accent) heights
#define C_ITALIC_RUN			21 // new cvt category, too
#define C_ITALIC_RISE			22 // new cvt category, too
#define catPos					0
#define catBits					6
#define catMask					(~((unsigned int)(-1) << catBits))

// @@TM::LIGHT - begin
const short charGroupToInt[numCharGroups] = {C_ANYGROUP, C_OTHER, C_UC, C_LC, C_FIG, C_NONLATIN, C_RESERVED2, C_RESERVED3};
const wchar_t charGroupToStrg[numCharGroups][cvtAttributeStrgLen] = {L"AnyGroup", L"Other", L"UpperCase", L"LowerCase", L"Figure", L"Reserved1", L"Reserved2", L"Reserved3"};
const wchar_t charGroupToSpacingText[numCharGroups][cvtAttributeStrgLen] = {L"   ", L"HH HOHO OO   ", L"HH HOHO OO   ", L"nn nono oo   ", L"11 1010 00   ", L"   ", L"   ", L"   "};
const CharGroup intToCharGroup[numCharGroups] = {anyGroup, upperCase, lowerCase, figureCase, otherCase, nonLatinCase, reservedCase2, reservedCase3};
// @@TM::LIGHT - end


const short linkColorToInt[numLinkColors] = {C_ANYCOLOR, C_BLACK, C_GREY, C_WHITE};
const wchar_t linkColorToStrg[numLinkColors][cvtAttributeStrgLen] = {L"AnyColor", L"Black", L"Grey", L"White"};
const LinkColor intToLinkColor[numLinkColors] = {linkGrey, linkBlack, linkWhite, linkAnyColor};

const short linkDirectionToInt[numLinkDirections] = {C_ANYDIR, C_XDIR, C_YDIR, C_DIAG};
const wchar_t linkDirectionToStrg[numLinkDirections][cvtAttributeStrgLen] = {L"AnyDirection", L"X", L"Y", L"Diag"};
const LinkDirection intToLinkDirection[1 + numLinkDirections] = {linkAnyDir, linkX, linkY, linkDiag, linkAnyDir};

const short cvtCategoryToInt[numCvtCategories] = {C_DUMMY, C_DISTANCE, C_STROKE, C_ROUND, 
												  C_LSB, C_RSB, C_BLACKBODYWIDTH, C_SERIF_THIN, C_SERIF_HEIGHT, C_SERIF_EXT, C_SERIF_CRVHGT, C_SERIF_OTHER, C_SERIFBOTTOM,
												  C_SQUARE_HEIGHT,C_ROUND_HEIGHT,C_ITALIC_RUN,C_ITALIC_RISE,
												  C_BEND_RADIUS, C_BRANCH_BEND_RADIUS, C_SCOOP_DEPTH, C_NEW_STRAIGHT, C_NEW_DIAGONAL, C_NEW_ANYDIR};
const wchar_t cvtCategoryToStrg[numCvtCategories][cvtAttributeStrgLen] = {L"AnyCategory", L"Distance", L"StraightStroke", L"RoundStroke",
																	   L"LSB", L"RSB", L"BlackBodyWidth", L"SerifThin", L"SerifHeight", L"SerifExt", L"SerifCurveHeight", L"SerifOther", L"SerifBottom",
																	   L"SquareHeight", L"RoundHeight", L"ItalicRun", L"ItalicRise",
																	   L"BendRadius", L"BranchBendRadius", L"ScoopDepth", L"NewStraight", L"NewDiagonal", L"NewAnyFeature"};
const CvtCategory intToCvtCategory[numCvtCategories] = {cvtAnyCategory, cvtStroke, cvtRound, cvtDistance,
														cvtBendRadius, cvtLsb, cvtRsb, cvtSerifThin, cvtSerifHeight, cvtSerifExt, cvtSerifCurveHeight, cvtSerifOther, cvtBlackBody,
														cvtSerifBottom, cvtBranchBendRadius, cvtScoopDepth, cvtNewStraightStroke, cvtNewDiagStroke, cvtNewAnyCategory, cvtSquareHeight, cvtRoundHeight, cvtItalicRun, cvtItalicRise};

#define DebugS(s) (ParentView::ActiveApplication()->ApplicationError(s))

#define lookahead 2 // #chars

typedef enum {group = 0, color, direction, category,									   // attribute declaration and settings first (color and direction are only dummies),
			  fpgmBias, 
			  instructionsOn, dropOutCtrlOff, scanCtrl, scanType, cvtCutIn, 
			  clearTypeCtrl, linearAdvanceWidths, // they are reserved identifiers,
			  cvtDelta, bCvtDelta, gCvtDelta,											   // so are these
			  asM, ident,																   // user defined identifier next, acts as sentinel in case searched identifier is not reserved
			  natural, hexadecimal, rational, literal,
			  period, comma, ellipsis, colon, semiColon, percent, at,
			  leftParen, plus, minus, times, divide, rightParen, equals, relatesTo, eot} Symbol; // followed by all the other symbols (ordered, don't re-order)

#define numKeyWords (int)ident
#define maxKeyWordLen 32
const wchar_t keyWord[numKeyWords][maxKeyWordLen] = {L"GROUP", L"COLOR", L"DIRECTION", L"CATEGORY",
												  L"FPgmBias",
												  L"InstructionsOn", L"DropOutCtrlOff", L"ScanCtrl", L"ScanType", L"CvtCutIn",
												  L"ClearTypeCtrl", L"LinearAdvanceWidths",
												  L"Delta", L"BDelta", L"GDelta", L"ASM" }; // cf. above

#define firstSetting fpgmBias
#define lastSetting linearAdvanceWidths
#define numSettings (lastSetting - firstSetting + 1)

typedef struct Scanner { // make it a static class, neither want to dynamically allocate nor subclass it
public:
	Symbol sym;
	int value;
	wchar_t literal[maxAsmSize];
	bool Init(TextBuffer*source, File *file, wchar_t errMsg[]);
	void Term(int *errPos, int *errLen);
	bool GetSym(void);
	void ReplaceIdent(const wchar_t capIdent[]);
	void ErrUnGetSym(void);
private:
	void GetCh(void);
	bool SkipComment(void);
	bool Skip(void);
	bool GetNum(void);
	bool GetIdent(void);
	bool GetLiteral(void);
	TextBuffer*source; // for replacing properly capitalized text
	File *file; // when compiling character group
	int pos,len; // into and of text
	wchar_t *text;
	wchar_t ch,ch2; // 2-char look-ahead
	int prevSymPos,prevSymEnd,symPos; // symPos >= 0 => error
	wchar_t *errMsg;
} Scanner;

#define numSubAttributes (int)instructionsOn
#define maxSubAttributes 0x100 // e.g. max # character groups
#define subAttributeBits 8
#define subAttributeMask (~((unsigned int)(-1) << subAttributeBits))

class Attribute {
public:
	Attribute(void);
	virtual ~Attribute(void);
	static bool InsertByName(Attribute **tree, bool predefined, const wchar_t name[], const wchar_t spacingText[], Symbol subAttribute, int value, wchar_t errMsg[]);
	static bool SearchByName(Attribute *tree, wchar_t name[], wchar_t actualName[], Symbol *subAttribute, int *value, wchar_t errMsg[]);
	static bool InsertByValue(Attribute **tree, Symbol subAttribute, int value, wchar_t name[], wchar_t spacingText[], wchar_t errMsg[]);
	static bool SearchByValue(Attribute *tree, Symbol subAttribute, int value, wchar_t name[], wchar_t spacingText[], wchar_t errMsg[]);
	static bool SortByValue(Attribute **to, Attribute *from, wchar_t errMsg[]);
#ifdef DEBUGCVT
	static void Dump(Attribute *tree);
#endif
private:
	Attribute *left,*right; // binary tree
	wchar_t name[cvtAttributeStrgLen],spacingText[cvtAttributeStrgLen];
	bool predefined;
	Symbol subAttribute; // group..category
	int value;
};

Attribute::Attribute(void) {
	this->left = this->right = NULL;
	this->predefined = false;
	this->name[0] = L'\0';
	this->spacingText[0] = L'\0';
	this->subAttribute = eot;
	this->value = -1;
} // Attribute::Attribute

Attribute::~Attribute(void) {
	if (this->left) delete this->left;
	if (this->right) delete this->right;
} // Attribute::~Attribute

void AssignString(wchar_t d[], const wchar_t s[], int n) {
	int i;

	for (i = 0; i < n && *s; i++) *d++ = *s++;
	if (i < n) *d = L'\0';
} // AssignString

int CompareString(wchar_t a[], wchar_t b[], int n) {
	int i;

	for (i = 0; i < n && *a && *b && *a == *b; a++, b++, i++);
	return i == n ? 0 : *a - *b;
} // CompareString

#define QCap(ch) (((ch) & 0xffdf)) // we know at this point that we compare alpha-numeric strings, hence it's enough to clear bit 5

int CompareCapString(const wchar_t a[], const wchar_t b[], int n) {
	int i;

	for (i = 0; i < n && *a && *b && QCap(*a) == QCap(*b); a++, b++, i++);
	return i == n ? 0 : QCap(*a) - QCap(*b);
} // CompareCapString

bool Attribute::InsertByName(Attribute **tree, bool predefined, const wchar_t name[], const wchar_t spacingText[], Symbol subAttribute, int value, wchar_t errMsg[]) {
	int cmp;

	if (!(*tree)) {
		*tree = new Attribute;
		if (!(*tree)) { swprintf(errMsg,L"Insufficient memory to define attribute \x22%s\x22",name); return false; }
		(*tree)->predefined = predefined;
		AssignString((*tree)->name,name,cvtAttributeStrgLen);
		if (spacingText) AssignString((*tree)->spacingText,spacingText,cvtAttributeStrgLen);
		(*tree)->subAttribute = subAttribute;
		(*tree)->value = value;
		return true;
	} else {
		cmp = CompareCapString(name,(*tree)->name,cvtAttributeStrgLen);
		if (!cmp) { swprintf(errMsg,L"Attribute \x22%s\x22 %sdefined",(*tree)->name,(*tree)->predefined ? L"is pre-" : L"already "); return false; }
		return Attribute::InsertByName(cmp < 0 ? &(*tree)->left : &(*tree)->right,predefined,name,spacingText,subAttribute,value,errMsg);
	}
} // Attribute::InsertByName

bool Attribute::SearchByName(Attribute *tree, wchar_t name[], wchar_t actualName[], Symbol *subAttribute, int *value, wchar_t errMsg[]) {
	int cmp;

	while (tree) {
		cmp = CompareCapString(name,tree->name,cvtAttributeStrgLen);
		if (!cmp) {
			if (actualName) AssignString(actualName,tree->name,cvtAttributeStrgLen);
			*subAttribute = tree->subAttribute;
			*value = tree->value;
			return true; // found
		}
		tree = cmp < 0 ? tree->left : tree->right;
	}
	swprintf(errMsg,L"Attribute \x22%s\x22 not defined",name); return false;
} // Attribute::SearchByName

#define PackKey(subAttribute,value) ((int)(subAttribute) << subAttributeBits | (value))

bool Attribute::InsertByValue(Attribute **tree, Symbol subAttribute, int value, wchar_t name[], wchar_t spacingText[], wchar_t errMsg[]) {
	int key,thisKey;

	if (!(*tree)) {
		*tree = new Attribute;
		if (!(*tree)) { swprintf(errMsg,L"Insufficient memory to insert attribute \x22%s\x22",name); return false; }
		AssignString((*tree)->name,name,cvtAttributeStrgLen);
		AssignString((*tree)->spacingText,spacingText,cvtAttributeStrgLen);
		(*tree)->subAttribute = subAttribute;
		(*tree)->value = value;
		return true;
	} else {
		key = PackKey(subAttribute,value); thisKey = PackKey((*tree)->subAttribute,(*tree)->value);
		if (key == thisKey) { swprintf(errMsg,L"Attribute \x22%s\x22 already inserted",name); return false; } // not expected by now, though
		return Attribute::InsertByValue(key < thisKey ? &(*tree)->left : &(*tree)->right,subAttribute,value,name,spacingText,errMsg);
	}
} // Attribute::InsertByValue

bool Attribute::SearchByValue(Attribute *tree, Symbol subAttribute, int value, wchar_t name[], wchar_t spacingText[], wchar_t errMsg[]) {
	int key,thisKey;

	while (tree) {
		key = PackKey(subAttribute,value); thisKey = PackKey(tree->subAttribute,tree->value);
		if (key == thisKey) {
			if (name) AssignString(name,tree->name,cvtAttributeStrgLen);
			if (spacingText) AssignString(spacingText,tree->spacingText,cvtAttributeStrgLen);
			return true; // found
		}
		tree = key < thisKey ? tree->left : tree->right;
	}
	swprintf(errMsg,L"Attribute \x22%s\x22 not defined",name); return false;
} // Attribute::SearchByValue

bool Attribute::SortByValue(Attribute **to, Attribute *from, wchar_t errMsg[]) {
//	alternatively, to be a tad more memory efficient, could traverse source tree in prefix, inserting allocated nodes at the end of each visit,
//	making sure to have two legal trees at any intermediate stage, for the purpose of standard de-allocation in case of any error.
	if (from) {
		if (!Attribute::SortByValue(to,from->left,errMsg)) return false;
		if (!Attribute::SortByValue(to,from->right,errMsg)) return false;
		if (!Attribute::InsertByValue(to,from->subAttribute,from->value,from->name,from->spacingText,errMsg)) return false;
	}
	return true; // by now
} // Attribute::SortByValue

#ifdef DEBUGCVT
void Attribute::Dump(Attribute *tree) {
	wchar_t out[maxLineSize];
	
	if (tree) {
		Attribute::Dump(tree->left);
		switch (tree->subAttribute) {
			case group:		swprintf(out,L"%-10s %-32s %-32s %10li\r",keyWord[tree->subAttribute],tree->name,tree->spacingText,tree->value); break;
			case color:
			case direction:
			case category:	swprintf(out,L"%-10s %-32s %10li\r",keyWord[tree->subAttribute],tree->name,tree->value); break;
			default:		swprintf(out,L"*INVALID* %-32s %10li\r",tree->name,tree->value); break;
		}
		DebugS(out);
		Attribute::Dump(tree->right);
	}
} // Attribute::Dump
#endif

// flags' values
#define cvtDefined 1
#define attributeDefined 2
#define relativeValue 4

typedef struct {
//	bool defined,attributeDefined;
//	bool relative;
	short value;
	unsigned short flags;
	unsigned int attribute;
	short breakPpemSize;
	short parent;
//	LinearListStruct *delta;
} ControlValue; // 16 bytes (?)

#define cvtReservedFrom 40
#define cvtReservedTo   64

#define maxCvtCutIns 4L

typedef struct {
	short instructionsOnFromPpemSize,instructionsOnToPpemSize;
	short dropOutCtrlOffPpemSize;
	short scanCtrlFlags,scanTypeFlags;
	short clearTypeCtrlFlag;
	short linearAdvanceWidthsFlag;
	short fpgmBiasNum;
	short numCvtCutIns;
	short cvtCutInPpemSize[maxCvtCutIns];
	F26Dot6 cvtCutInValue[maxCvtCutIns];
	bool defined[numSettings];
} Settings;

void DefaultSettings(Settings *settings) {
	int i;

	settings->instructionsOnFromPpemSize = 8;
	settings->instructionsOnToPpemSize = 2047;
	settings->dropOutCtrlOffPpemSize = 255; // always on
	settings->scanCtrlFlags = 0x100 | settings->dropOutCtrlOffPpemSize;
	settings->scanTypeFlags = 5; // smart drop out control
	settings->clearTypeCtrlFlag = 0; // not optimized for ClearType
	settings->linearAdvanceWidthsFlag = 0; // don't allow fractional ppem sizes
	settings->fpgmBiasNum = 0;
	settings->numCvtCutIns = 3;
	settings->cvtCutInPpemSize[0] = 1;
	settings->cvtCutInPpemSize[1] = 29;
	settings->cvtCutInPpemSize[2] = 128;
	settings->cvtCutInValue[0] = 4*one6;
	settings->cvtCutInValue[1] = 3*one6/2;
	settings->cvtCutInValue[2] = 0;
	for (i = 0; i < numSettings; i++) settings->defined[i] = false; // got defaults only
} // DefaultSettings
	
typedef enum { voidParam = 0, naturalN, rationalN, ppemN, rangeOfPpemN, multipleRangesOfPpemN, deltaAtRangeOfPpemN, anyS} ParamType;

typedef struct {
	ParamType type;
	int value;
	wchar_t *literal; // pointer to scanner's literal for memory efficiency, since we don't have more than 1 string parameter per TMT command
	int lowPpemSize,highPpemSize; // in-out for ppemSize related params
	bool deltaPpemSize[maxPpemSize]; // here we have possibly more than one rangeOfPpemN parameter, but could implement bit vectors...
	DeltaColor deltaColor; // alwaysDelta, blackDelta, greyDelta, ..., same for the entire bit vector deltaPpemSize above
} ActParam;

typedef struct {
	unsigned int attribute;
	unsigned short value; // cvt values are biased by 0x8000
	short num;
} CvtKey;

class PrivateControlValueTable : public ControlValueTable {
public:
	PrivateControlValueTable(void);
	virtual ~PrivateControlValueTable(void);
	virtual bool Compile(TextBuffer*source, TextBuffer*prepText, bool legacyCompile, int *errPos, int *errLen, wchar_t errMsg[]);
	virtual bool IsControlProgramFormat(void);
	virtual bool LinearAdvanceWidths(void);
	virtual int LowestCvtNum(void);
	virtual int HighestCvtNum(void);
	virtual int LowestCvtIdx(void);
	virtual int HighestCvtIdx(void);
	virtual int CvtNumOf(int idx);
	virtual int CvtIdxOf(int num);
	virtual bool CvtNumExists(int cvtNum);
	virtual bool GetCvtValue(int cvtNum, short *cvtValue);
	virtual bool CvtAttributesExist(int cvtNum); // entered a cvt "comment"?
	virtual bool GetCvtAttributes(int cvtNum, CharGroup *charGroup, LinkColor *linkColor, LinkDirection *linkDirection, CvtCategory *cvtCategory, bool *relative);
	virtual int NumCharGroups(void);
	virtual bool GetAttributeStrings(int cvtNum, wchar_t charGroup[], wchar_t linkColor[], wchar_t linkDirection[], wchar_t cvtCategory[], wchar_t relative[]);
	virtual bool GetCharGroupString(CharGroup group, wchar_t string[]);
	virtual bool GetSpacingText(CharGroup group, wchar_t spacingText[]);
	virtual int GetBestCvtMatch(CharGroup charGroup, LinkColor linkColor, LinkDirection linkDirection, CvtCategory cvtCategory, int distance); // returns illegalCvtNum if no match
	virtual void PutCvtBinary(int size, unsigned char data[]);
	virtual void GetCvtBinary(int *size, unsigned char data[]);
	virtual int GetCvtBinarySize(void);
	virtual unsigned int PackAttribute(CharGroup charGroup, LinkColor linkColor, LinkDirection linkDirection, CvtCategory cvtCategory);
	virtual void UnpackAttribute(unsigned int attribute, CharGroup *charGroup, LinkColor *linkColor, LinkDirection *linkDirection, CvtCategory *cvtCategory);
	virtual void UnpackAttributeStrings(unsigned int attribute, wchar_t charGroup[], wchar_t linkColor[], wchar_t linkDirection[], wchar_t cvtCategory[]);
	virtual bool DumpControlValueTable(TextBuffer *text);
	virtual bool CompileCharGroup(File *from, short platformID, unsigned char toCharGroupOfCharCode[], wchar_t errMsg[]);
private:
	Scanner scanner;
	TTEngine *tt;
	Attribute *attributes,*tempAttributes;
	bool oldSyntax,newSyntax;
	bool legacyCompile; 
	wchar_t *errMsg;
	bool cvtDataValid,cvtDataSorted;
	int lowestCvtNum,highestCvtNum;
	int lowestCvtIdx,highestCvtIdx;
	int newNumCharGroups;
	Settings cpgmSettings,tempSettings;
	ControlValue *cpgmData,*tempData;
	CvtKey cvtKeyOfIdx[1 + maxCvtNum + 1]; // cvt key sorted by cvtAttribute and cvtValue, with sentinels at either end
	short cvtIdxOfNum[maxCvtNum]; // inverse table of above, cvtIdx[cvtNum[idx]] = idx
	bool AttributeDeclaration(int firstAvailSubAttributeValue[]);
	bool SettingsDeclaration(void);
	bool CvtDeclaration(unsigned int *attribute);
	bool AttributeAssociation(unsigned int *attribute);
	bool ValueAssociation(unsigned int attribute, int *cvtNum, ControlValue **cvt);
	bool InheritanceRelation(int cvtNum, ControlValue *cvt);
	bool DeltaDeclaration(int cvtNum, ControlValue *cvt);
	bool InlineSttmt(void);
	bool Parameter(ActParam *actParam);
	bool Expression(ActParam *actParam);
	bool Term(ActParam *actParam);
	bool Factor(ActParam *actParam);
	bool PixelAtPpemRange(DeltaColor cmdColor, ActParam *actParam, DeltaColor *paramColor);
	bool PpemRange(ActParam *actParam);
	bool Range(ActParam *actParam);
	void AssertSortedCvt(void);
	void SortCvtKeys(int low, int high);
};

ControlValue *NewCvtData(void) {
	int cvtNum;
	ControlValue *cvt,*cvtData;
	
	cvtData = (ControlValue *)NewP(((int)maxCvtNum)*((int)sizeof(ControlValue))); // all these parens appear to be necessary to convice the vc compiler that the operation should be carried out in int...
	if (!cvtData) return NULL;
	for (cvt = cvtData, cvtNum = 0; cvtNum < maxCvtNum; cvt++, cvtNum++) {
		cvt->flags = 0;
		cvt->value = 0;
		cvt->attribute = 0;
		cvt->breakPpemSize = 0;
		cvt->parent = illegalCvtNum;
	//	cvt->delta = NULL;
	}
	return cvtData;
} // NewCvtData

void DisposeCvtData(ControlValue **cvtData) {
//	int cvtNum;
//	ControlValue *cvt;
	
	if (!(*cvtData)) return;
//	for (cvt = *cvtData, cvtNum = 0; cvtNum < maxCvtNum; cvt++, cvtNum++) {
//		if (cvt->delta) ListStruct::Delete((ListStruct **)&cvt->delta);
//	}
	DisposeP((void **)cvtData);
} // DisposeCvtData

PrivateControlValueTable::PrivateControlValueTable(void) {
	this->legacyCompile = false; 
	this->tt = NULL;
	this->cvtDataValid = this->cvtDataSorted = false;
	this->lowestCvtNum  = maxCvtNum;
	this->highestCvtNum = -1;
	this->attributes = this->tempAttributes = NULL;
	DefaultSettings(&this->cpgmSettings);
	this->cpgmData = NewCvtData();
	this->tempData = NULL;
} // PrivateControlValueTable::PrivateControlValueTable

PrivateControlValueTable::~PrivateControlValueTable(void) {
	if (this->cpgmData) DisposeCvtData(&this->cpgmData);
	if (this->attributes) delete this->attributes;
//	CvtDelta::Flush();
} // PrivateControlValueTable::~PrivateControlValueTable

#define WhiteSpace(scanner) (L'\0' < scanner->ch && scanner->ch <= L' ' /* && scanner->ch != '\r' */)
#define InitComment(scanner) (scanner->ch == L'/' && scanner->ch2 == L'*')
#define TermComment(scanner) (scanner->ch == L'*' && scanner->ch2 == L'/')
#define	Numeric(ch) (L'0' <= (ch) && (ch) <= L'9')
#define Alpha(ch) ((L'A' <= (ch) && (ch) <= L'Z') || (L'a' <= (ch) && (ch) <= L'z'))
#define shortMax  32767L
#define hShortMax 65535L
#define shortMin -32768L

void Scanner::GetCh(void) {
	this->ch = this->ch2;
	this->ch2 = this->text && this->pos < this->len ? this->text[this->pos] : L'\0';
//	this->ch2 = Cap(this->ch2); // don't make it case insensitive for now
	this->pos++;
} // Scanner::GetCh

bool Scanner::SkipComment(void) {
	int commentPos = this->pos;
	
	this->GetCh(); this->GetCh();
	while (this->ch && !TermComment(this)) {
		if (InitComment(this)) this->SkipComment(); else this->GetCh();
	}
	if (this->ch) { // TermComment
		this->GetCh(); this->GetCh();
	} else {
		this->symPos = commentPos; swprintf(this->errMsg,L"Comment opened but not closed"); return false;
	}
	return true;
} // Scanner::SkipComment

bool Scanner::Skip(void) {
	while (WhiteSpace(this) || InitComment(this)) {
		if (this->ch <= L' ')
			this->GetCh();
		else if (!this->SkipComment())
			return false;
	}
	return true;
} // Scanner::Skip

bool Scanner::Init(TextBuffer*source, File *file, wchar_t errMsg[]) {
	int i; 
	size_t textLen;

	this->prevSymPos = this->prevSymEnd = this->symPos = -1;
	this->source = source;
	this->file = file;
	this->errMsg = errMsg; // copy pointer
	this->pos = 0;	
	if (this->source) 
	{
		this->len = source->TheLength();
		this->text = (wchar_t *)NewP((this->len + 1) * sizeof(wchar_t)); // '\0'...
		if (!this->text) return false;

		source->GetText(&textLen,this->text); 
	}
	else 
	{
		file->ReadUnicode(&this->len, &this->text); 		
	}
	this->text[this->len] = L'\0';
	this->ch2 = 0; // silence BC
	for (i = 0; i < lookahead; i++) this->GetCh();
	return this->GetSym();
} // Scanner::Init

void Scanner::Term(int *errPos, int *errLen) {
	*errPos = this->symPos - lookahead;
	*errLen = this->pos - this->symPos;
	if (this->text) DisposeP((void **)(&this->text));
} // Scanner::Term

bool Scanner::GetNum(void) {
	int digit,decPlcs,pwrOf10;
	
	this->value = 0;
	if (this->ch == L'0' && Cap(this->ch2) == L'X') {
		this->GetCh(); this->GetCh(); this->ch = Cap(this->ch);
		while (Numeric(this->ch) || (L'A' <= this->ch && this->ch <= L'F')) {
			digit = this->ch <= L'9' ? (int)this->ch - (int)'0' : (int)this->ch - (int)'A' + 10;
			if (this->value*16 + digit > hShortMax) { swprintf(this->errMsg,L"Hexadecimal number too large"); return false; }
			this->value = this->value*16 + digit;
			this->GetCh(); this->ch = Cap(this->ch);
		}
		this->sym = hexadecimal;
	} else {
		while (Numeric(this->ch)) {
			digit = (int)this->ch - (int)'0';
			if (this->value*10 + digit > shortMax) { swprintf(this->errMsg,L"Number too large"); return false; }
			this->value = this->value*10 + digit;
			this->GetCh();
		}
		this->sym = natural; // so far
		if (this->ch == '.' && this->ch2 != '.') { // distinguish from ellipsis
			this->GetCh();
			decPlcs = 0; pwrOf10 = 1;
			while (Numeric(this->ch)) {
				digit = (int)this->ch - (int)'0';
				if (decPlcs*10 * digit > 1000000L) { swprintf(this->errMsg,L"Too many decimal places"); return false; } // 1/64 = 0.015625
				decPlcs = 10*decPlcs + digit; pwrOf10 *= 10L;
				this->GetCh();
			}
			if (pwrOf10 > 1) {
				this->value = this->value*one6 + (decPlcs*one6 + pwrOf10/2)/pwrOf10;
				this->sym = rational; // by now
			}
		}
	}
	return true; // by now
} // Scanner::GetNum

bool Scanner::GetIdent(void) {
	int i;
	
	i = 0;
	while (Alpha(this->ch) || Numeric(this->ch)) {
		if (i >= cvtAttributeStrgLen) 
		{ 
			swprintf(this->errMsg,L"Identifier too int (cannot have more than %li characters)",(int)cvtAttributeStrgLen); return false; 
		}
		this->literal[i++] = this->ch;
		this->GetCh();
	}
	if (i < cvtAttributeStrgLen) this->literal[i] = L'\0';
	if (this->source) {
		for (i = 0; i < numKeyWords && CompareCapString(this->literal,(wchar_t *)keyWord[i],cvtAttributeStrgLen); i++); // linear search, there are only a few keywords
		this->sym = (Symbol)i;
		if (this->sym == color || this->sym == direction) this->sym = ident; // don't allow user to define these, so far at least...
		if (this->sym < ident && CompareString(this->literal,(wchar_t *)keyWord[this->sym],cvtAttributeStrgLen)) this->ReplaceIdent((wchar_t *)keyWord[this->sym]);
	} else { // compiling character group, there are no reserved words...
		this->sym = ident;
	}
	return true; // by now
} // Scanner::GetIdent

bool Scanner::GetLiteral(void) {
	int i;
	
	this->GetCh();
	i = 0;
	while (this->ch && this->ch != L'"') {
		if (i >= maxAsmSize-1) { swprintf(this->errMsg,L"String too int (cannot be longer than %li characters)",maxAsmSize-1); return true; }
		this->literal[i++] = this->ch;
		this->GetCh();
	}
	this->literal[i++] = L'\0';
	if (!this->ch) { swprintf(this->errMsg,L"\x22 expected"); return false; }
	this->GetCh();
	this->sym = ::literal;
	return true; // by now
} // Scanner::GetLiteral

bool Scanner::GetSym(void) {
	this->sym = eot;
	this->prevSymEnd = this->pos;
	if (!this->Skip()) return false;
	this->prevSymPos = this->symPos; this->symPos = this->pos;
	if (Numeric(this->ch)) {
		return this->GetNum();
	} else if (Alpha(this->ch)) {
		return this->GetIdent();
	} else {
		switch (this->ch) {
			case L'"' : return this->GetLiteral();
			case L'.' : this->GetCh(); if (this->ch == L'.') { this->GetCh(); this->sym = ellipsis; } else this->sym = period; break;
			case L',' : this->GetCh(); this->sym = comma; break;
			case L':' : this->GetCh(); this->sym = colon; break;
			case L';' : this->GetCh(); this->sym = semiColon; break;
			case L'%' : this->GetCh(); this->sym = percent; break;
			case L'@' : this->GetCh(); this->sym = at; break;
			case L'(' : this->GetCh(); this->sym = leftParen; break;
			case L'[' : this->GetCh(); this->sym = leftParen; this->ReplaceIdent(L"("); break;
			case L')' : this->GetCh(); this->sym = rightParen; break;
			case L']' : this->GetCh(); this->sym = rightParen; this->ReplaceIdent(L")"); break;
			case L'+' : this->GetCh(); this->sym = plus; break;
			case L'-' : this->GetCh(); this->sym = minus; break;
			case L'*' : this->GetCh(); this->sym = times; break;
			case L'/' : this->GetCh(); this->sym = divide; break;
			case L'=' : this->GetCh(); this->sym = equals; break;
			case L'~' : this->GetCh(); this->sym = relatesTo; break;
		//	case L'\r': this->GetCh(); this->sym = eol; break;
			case L'\0': this->sym = eot; break;
			default  : this->GetCh(); swprintf(this->errMsg,L"Illegal character in control value table"); return false; break;
		}
	}
	return true;
} // Scanner::GetSym

void Scanner::ReplaceIdent(const wchar_t capIdent[]) {
	int beg,end;

	beg = this->symPos - lookahead;
	end = beg + (int)STRLENW(capIdent);
	this->source->Delete(beg,end);
	this->source->Insert(beg,capIdent);
} // Scanner::ReplaceIdent

void Scanner::ErrUnGetSym(void) {
	this->pos = this->prevSymEnd; this->symPos = this->prevSymPos; // after semantical/contextual error, we're one symbol ahead, hence retract for correct error high-lighting
} // Scanner::ErrUnGetSym

bool AssertNatural(ActParam *actParam, int low, int high, const wchar_t name[], wchar_t errMsg[]) {
	if (actParam->type != naturalN) { swprintf(errMsg,L"%s expected (must be an integer in range %li through %li)",name,low,high); return false; }
	actParam->value >>= places6;
	if (actParam->value < low || high < actParam->value) { swprintf(errMsg,L"%s out of range (must be in range %li through %li)",name,low,high); return false; }
	return true; // by now
} // AssertNatural

bool AssertPixelAmount(ActParam *actParam, F26Dot6 low, F26Dot6 high, const wchar_t name[], wchar_t errMsg[]) {
	if (actParam->type == naturalN) actParam->type = rationalN;
	if (actParam->type != rationalN) { swprintf(errMsg,L"%s expected (must be a pixel amount in range %8.6f through %8.6f)",name,(double)low/one6,(double)high/one6); return false; }
	if (actParam->value < low || high < actParam->value) { swprintf(errMsg,L"%s expected (must be in range %8.6f through %8.6f)",name,(double)low/one6,(double)high/one6); return false; }
	return true; // by now
} // AssertPixelAmount

bool PrivateControlValueTable::AttributeDeclaration(int firstAvailSubAttributeValue[]) {
	Symbol sym;
	wchar_t name[cvtAttributeStrgLen],spacingText[cvtAttributeStrgLen];

	this->newSyntax = true;
	sym = this->scanner.sym;
	if (!this->scanner.GetSym()) return false;
	if (this->scanner.sym != ident) { swprintf(errMsg,L"%s name expected",keyWord[sym]); return false; }
	if (firstAvailSubAttributeValue[sym] >= maxSubAttributes) { swprintf(errMsg,L"%s name exceeds capacity (cannot have more than %li)",keyWord[sym],maxSubAttributes); return false; }
	AssignString(name,this->scanner.literal,cvtAttributeStrgLen);
	if (!this->scanner.GetSym()) return false;
	spacingText[0] = L'\0';
	if (sym == group && this->scanner.sym == literal) {
		AssignString(spacingText,this->scanner.literal,cvtAttributeStrgLen);
		if (!this->scanner.GetSym()) return false;
	}
	if (!Attribute::InsertByName(&this->tempAttributes,false,name,spacingText,sym,firstAvailSubAttributeValue[sym],errMsg)) { this->scanner.ErrUnGetSym(); return false; }
	firstAvailSubAttributeValue[sym]++;
	return true; // by now

} // PrivateControlValueTable::AttributeDeclaration

bool PrivateControlValueTable::SettingsDeclaration(void) {
	Symbol sym;
	ActParam instrOnParam,dropOffParam,scanCtrlParam,scanTypeParam,cvtCutInPixelSizeParam,cvtCutInPpemSizeParam,clearTypeCtrlParam,linearAdvanceWidthsParam,fpgmBiasParam;
	wchar_t comment[maxLineSize];

	this->newSyntax = true;
	sym = this->scanner.sym;
	if (this->tempSettings.defined[sym-firstSetting]) { swprintf(this->errMsg,L"%s already defined",keyWord[sym]); return false; }
	if (!this->scanner.GetSym()) return false;

	if (this->legacyCompile || sym != fpgmBias) {
	swprintf(comment,L"/* %s */",keyWord[sym]); this->tt->Emit(comment);
	}

	switch (sym) {
		case instructionsOn:
			instrOnParam.lowPpemSize = 0; instrOnParam.highPpemSize = shortMax;
			if (!this->Parameter(&instrOnParam)) return false;
			if (instrOnParam.type != rangeOfPpemN) { swprintf(this->errMsg,L"Range of ppem sizes at which instructions are on expected (Example: @8..2047 to activate instructions in range 8 through 2047 ppem)"); this->scanner.ErrUnGetSym(); return false; }
			this->tempSettings.instructionsOnFromPpemSize = (short)instrOnParam.value;
			this->tempSettings.instructionsOnToPpemSize = (short)instrOnParam.lowPpemSize;
			this->tt->INSTCTRL(this->tempSettings.instructionsOnFromPpemSize,this->tempSettings.instructionsOnToPpemSize);
			this->tempSettings.defined[sym - firstSetting] = true;
			break;
		case dropOutCtrlOff:
			if (this->tempSettings.defined[scanCtrl-firstSetting] || this->tempSettings.defined[scanType-firstSetting]) { swprintf(this->errMsg,L"Cannot use %s together with %s or %s",keyWord[sym],keyWord[scanCtrl],keyWord[scanType]); this->scanner.ErrUnGetSym(); return false; }
			dropOffParam.lowPpemSize = -1; dropOffParam.highPpemSize = maxPpemSize-1; // lowest permissible ppem size - 1
			if (!this->Parameter(&dropOffParam)) return false;
			if (dropOffParam.type != ppemN) { swprintf(this->errMsg,L"Drop-out control turn-off ppem size expected (must be an integer in range @%li through @%li)" BRK L"Drop-out control turn-off ppem size specifies the ppem size at and above which drop-out control is no longer turned on.",1,dropOffParam.highPpemSize); this->scanner.ErrUnGetSym(); return false; }
			this->tempSettings.dropOutCtrlOffPpemSize = (short)dropOffParam.value;
			this->tempSettings.scanCtrlFlags = (this->tempSettings.scanCtrlFlags & 0xff00) | this->tempSettings.dropOutCtrlOffPpemSize;
			this->tt->SCANCTRL(this->tempSettings.scanCtrlFlags);
			this->tt->SCANTYPE(this->tempSettings.scanTypeFlags);
			this->tempSettings.defined[sym - firstSetting] = true;
			break;
		case scanCtrl:
			if (this->tempSettings.defined[dropOutCtrlOff-firstSetting]) { swprintf(this->errMsg,L"Cannot use %s together with %s",keyWord[sym],keyWord[dropOutCtrlOff]); this->scanner.ErrUnGetSym(); return false; }
			if (this->scanner.sym != equals) { swprintf(this->errMsg,L"= expected"); return false; }
			if (!this->scanner.GetSym()) return false;
			if (!this->Parameter(&scanCtrlParam)) return false;
			if (!AssertNatural(&scanCtrlParam,0,16383,L"Value for scan control",this->errMsg)) { this->scanner.ErrUnGetSym(); return false; } // bits 14 and 15 reserved for future use
			this->tempSettings.scanCtrlFlags = (short)scanCtrlParam.value;
			this->tt->SCANCTRL(this->tempSettings.scanCtrlFlags);
			this->tempSettings.defined[sym - firstSetting] = true;
			break;
		case scanType:
			if (this->tempSettings.defined[dropOutCtrlOff-firstSetting]) { swprintf(this->errMsg,L"Cannot use %s together with %s",keyWord[sym],keyWord[dropOutCtrlOff]); return false; }
			if (this->scanner.sym != equals) { swprintf(this->errMsg,L"= expected"); return false; }
			if (!this->scanner.GetSym()) return false;
			if (!this->Parameter(&scanTypeParam)) return false;
			if (!AssertNatural(&scanTypeParam,1,6,L"Value for scan type",this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
			this->tempSettings.scanTypeFlags = (short)scanTypeParam.value;
			this->tt->SCANTYPE(this->tempSettings.scanTypeFlags);
			this->tempSettings.defined[sym - firstSetting] = true;
			break;
		case cvtCutIn:
			if (this->scanner.sym != equals) { swprintf(this->errMsg,L"= expected"); return false; }
			if (!this->scanner.GetSym()) return false;
			if (!this->Parameter(&cvtCutInPixelSizeParam)) return false;
			if (!AssertPixelAmount(&cvtCutInPixelSizeParam,0,maxPixelValue,L"Cut-in pixel amount",this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
			this->tempSettings.cvtCutInValue[0] = cvtCutInPixelSizeParam.value;
			cvtCutInPpemSizeParam.lowPpemSize = 0; cvtCutInPpemSizeParam.highPpemSize = maxPpemSize-1;
			this->tempSettings.numCvtCutIns = 1;
			while (this->scanner.sym == comma) {
				if (this->tempSettings.numCvtCutIns >= maxCvtCutIns) { swprintf(this->errMsg,L"Too many cvt cut-ins (cannot have more than %li)",maxCvtCutIns); return false; }
				if (!this->scanner.GetSym()) return false;
				if (!this->Parameter(&cvtCutInPixelSizeParam)) return false;
				if (!AssertPixelAmount(&cvtCutInPixelSizeParam,0,this->tempSettings.cvtCutInValue[this->tempSettings.numCvtCutIns-1]-1,L"Cut-in pixel amount",this->errMsg)) { this->scanner.ErrUnGetSym(); return false; } // allow at most 1/64 less than preceding cut in 
				if (!this->Parameter(&cvtCutInPpemSizeParam)) return false;
				this->tempSettings.cvtCutInValue[this->tempSettings.numCvtCutIns] = cvtCutInPixelSizeParam.value;
				this->tempSettings.cvtCutInPpemSize[this->tempSettings.numCvtCutIns] = (short)cvtCutInPpemSizeParam.value;
				this->tempSettings.numCvtCutIns++;
			}
			this->tt->AssertFreeProjVector(yRomanDir); // so far, this may become aspect-ratio dependent, or such like...
			this->tt->SCVTCI(this->tempSettings.numCvtCutIns,this->tempSettings.cvtCutInPpemSize,this->tempSettings.cvtCutInValue);
			this->tempSettings.defined[sym - firstSetting] = true;
			break;		
		default:
			break;
	}

	if (!this->legacyCompile)
	{
		switch (sym)
		{
		case clearTypeCtrl:
			if (this->scanner.sym != equals) { swprintf(this->errMsg, L"= expected"); return false; }
			if (!this->scanner.GetSym()) return false;
			if (!this->Parameter(&clearTypeCtrlParam)) return false;
			if (!AssertNatural(&clearTypeCtrlParam, 0, 1, L"Value for ClearTypeCtrl", this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
			this->tempSettings.clearTypeCtrlFlag = (short) clearTypeCtrlParam.value;
			this->tt->SetClearTypeCtrl(this->tempSettings.clearTypeCtrlFlag);
			this->tempSettings.defined[sym - firstSetting] = true;
			break;
		case linearAdvanceWidths:
			if (this->scanner.sym != equals) { swprintf(this->errMsg, L"= expected"); return false; }
			if (!this->scanner.GetSym()) return false;
			if (!this->Parameter(&linearAdvanceWidthsParam)) return false;
			if (!AssertNatural(&linearAdvanceWidthsParam, 0, 1, L"Value for linearAdvanceWidths", this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
			this->tempSettings.linearAdvanceWidthsFlag = (short) linearAdvanceWidthsParam.value;
			//	client to inquire flag after CVT compilation and to call font->UpdateAdvanceWidthFlag
			this->tempSettings.defined[sym - firstSetting] = true;
			break;
		case fpgmBias:
			if (this->scanner.sym != equals) { swprintf(this->errMsg, L"= expected"); return false; }
			if (!this->scanner.GetSym()) return false;
			if (!this->Parameter(&fpgmBiasParam)) return false;
			if (!AssertNatural(&fpgmBiasParam, 0, 32767, L"Value for FPgmBias", this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
			this->tempSettings.fpgmBiasNum = (short) fpgmBiasParam.value;
			this->tt->SetFunctionNumberBias(this->tempSettings.fpgmBiasNum);
			this->tempSettings.defined[sym - firstSetting] = true;
			break;
		default:
			break;
		}
	}
	
	return true; // by now
} // PrivateControlValueTable::SettingsDeclaration

bool PrivateControlValueTable::CvtDeclaration(unsigned int *attribute) {
	int cvtNum;
	ControlValue *cvt;

	if (!(this->AttributeAssociation(attribute) && // this->InlineSttmt() && 
		  this->ValueAssociation(*attribute,&cvtNum,&cvt) && this->InlineSttmt() && 
		  this->InheritanceRelation(cvtNum,cvt) && this->InlineSttmt() && 
		  this->DeltaDeclaration(cvtNum,cvt) && this->InlineSttmt())) return false;
	cvt->flags |= cvtDefined; // at the very end only, or else we would allow referencing self as a parent...
	return true; // by now
} // PrivateControlValueTable::CvtDeclaration

bool PrivateControlValueTable::AttributeAssociation(unsigned int *attribute) {
	CharGroup currGroup;
	LinkColor currColor;
	LinkDirection currDirection;
	CvtCategory currCategory;
	Symbol subAttribute;
	int value;
	wchar_t actualName[cvtAttributeStrgLen];

	PrivateControlValueTable::UnpackAttribute(*attribute,&currGroup,&currColor,&currDirection,&currCategory);
	while (this->scanner.sym == ident) {
		this->newSyntax = true;
		if (!Attribute::SearchByName(this->tempAttributes,this->scanner.literal,actualName,&subAttribute,&value,this->errMsg)) return false;
		if (CompareString(this->scanner.literal,actualName,cvtAttributeStrgLen)) this->scanner.ReplaceIdent(actualName);
		switch (subAttribute) {
			case group:		currGroup = (CharGroup)value; break;
			case color:		currColor = (LinkColor)value; break;
			case direction: currDirection = (LinkDirection)value; break;
			case category:	currCategory = (CvtCategory)value; break;
			default: break;
		}
		if (!this->scanner.GetSym()) return false;
		if (!this->InlineSttmt()) return false;
	}
	*attribute = PrivateControlValueTable::PackAttribute(currGroup,currColor,currDirection,currCategory);
	return true; // by now
} // PrivateControlValueTable::AttributeAssociation

bool PrivateControlValueTable::ValueAssociation(unsigned int attribute, int *cvtNum, ControlValue **cvt) {
	ActParam cvtNumParam,cvtValueParam;
	CharGroup charGroup;
	LinkColor linkColor;
	LinkDirection linkDirection;
	CvtCategory cvtCategory;

	if (!this->Parameter(&cvtNumParam)) return false;
	if (!AssertNatural(&cvtNumParam,0,maxCvtNum-1,L"Cvt number",this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
	*cvtNum = cvtNumParam.value;
//	we're currently not testing this as there may be users with different fpgms and we should really un-hardwire the remaining hard-wired cvts
//	if (cvtReservedFrom <= *cvtNum && *cvtNum <= cvtReservedTo) { swprintf(this->errMsg,L"Cvt numbers in range %li through %li are reserved",(int)cvtReservedFrom,(int)cvtReservedTo); this->scanner.ErrUnGetSym(); return false; }
	*cvt = &this->tempData[cvtNumParam.value];
	if ((*cvt)->flags & cvtDefined) { swprintf(this->errMsg,L"cvt number already defined"); this->scanner.ErrUnGetSym(); return false; }
	if (this->scanner.sym != colon) { swprintf(this->errMsg,L"':' expected"); return false; }
	if (!this->scanner.GetSym()) return false;
	if (!this->Parameter(&cvtValueParam)) return false;
	if (cvtValueParam.type != naturalN) { swprintf(this->errMsg,L"Cvt value expected (must be an integer specifying font design units)"); this->scanner.ErrUnGetSym(); return false; }
	cvtValueParam.value >>= places6;
	(*cvt)->value = (short)cvtValueParam.value;
	if (this->scanner.sym == hexadecimal) {
		if (this->newSyntax) { swprintf(this->errMsg,L"Cannot mix cvt formats (hexadecimal attributes are used in the old cvt format only)"); return false; }
		this->oldSyntax = true;
		UnpackCvtHexAttribute((short)this->scanner.value,&charGroup,&linkColor,&linkDirection,&cvtCategory);
		(*cvt)->attribute = PrivateControlValueTable::PackAttribute(charGroup,linkColor,linkDirection,cvtCategory);
		(*cvt)->flags |= attributeDefined;
		if (!this->scanner.GetSym()) return false;
	} else if (this->newSyntax) {
		(*cvt)->attribute = attribute;
		(*cvt)->flags |= attributeDefined;
	}
//	(*cvt)->flags |= PostSizeToViewTitle; not yet
	return true; // by now
} // PrivateControlValueTable::ValueAssociation

bool PrivateControlValueTable::InheritanceRelation(int cvtNum, ControlValue *cvt) {
	bool relative;
	ActParam parentCvtNumParam,ppemValueParam;
	ControlValue *parentCvt;
	CharGroup charGroup;
	LinkColor linkColor;
	LinkDirection linkDirection;
	CvtCategory cvtCategory;

	if (this->scanner.sym == equals || this->scanner.sym == relatesTo) {
		relative = this->scanner.sym == relatesTo;
		this->newSyntax = true;
		if (!this->scanner.GetSym()) return false;
		if (!this->Parameter(&parentCvtNumParam)) return false;
		if (!AssertNatural(&parentCvtNumParam,0,maxCvtNum-1,L"Parent cvt number",this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
		parentCvt = &this->tempData[parentCvtNumParam.value];
		if (!parentCvt->flags & cvtDefined) { swprintf(this->errMsg,L"Parent cvt not defined (must be completely defined prior to tying child cvts to it)"); this->scanner.ErrUnGetSym(); return false; }
//		if (this->scanner.sym != at) { swprintf(this->errMsg,L"'@' expected"); return false; }
//		if (!this->scanner.GetSym()) return false;
		if (!this->legacyCompile) {
			ppemValueParam.lowPpemSize = 0; // lowest permissible ppem size - 1
		} else {
			ppemValueParam.lowPpemSize = parentCvt->breakPpemSize; // lowest permissible ppem size - 1
		}

		ppemValueParam.highPpemSize = maxPpemSize-1;
		if (!this->Parameter(&ppemValueParam)) return false;
		if (ppemValueParam.type != ppemN) { swprintf(this->errMsg,L"Break ppem size expected (must be an integer in range @%li through @%li)" BRK L"The break ppem size specifies the ppem size at which this child cvt is no longer tied to its parent.",parentCvt->breakPpemSize+1,ppemValueParam.highPpemSize); this->scanner.ErrUnGetSym(); return false; }
		if (this->legacyCompile){
			if (ppemValueParam.value <= parentCvt->breakPpemSize || ppemValueParam.highPpemSize < ppemValueParam.value) { swprintf(this->errMsg, L"Break ppem size out of range (must be in range @%li through @%li)" BRK L"The break ppem size must be above the break ppem size of the parent of this child cvt)", parentCvt->breakPpemSize + 1, ppemValueParam.highPpemSize); this->scanner.ErrUnGetSym(); return false; }
		}

		cvt->parent = (short)parentCvtNumParam.value;
		cvt->breakPpemSize = (short)ppemValueParam.value;
		if (relative) cvt->flags |= relativeValue;
		this->UnpackAttribute(cvt->attribute,&charGroup,&linkColor,&linkDirection,&cvtCategory);
		this->tt->AssertFreeProjVector(linkDirection == linkX ? xRomanDir : yRomanDir);
		this->tt->CvtRegularization(relative,(short)cvtNum,cvt->breakPpemSize,cvt->parent);
		cvt->attribute = this->PackAttribute(charGroup,linkColor,linkDirection,cvtCategory);
	}
	return true; // by now
} // PrivateControlValueTable::InheritanceRelation

bool PrivateControlValueTable::DeltaDeclaration(int cvtNum, ControlValue *cvt) {
	ActParam pixelAtPpemRangeParam;
	bool colorDeltaDone[numDeltaColors];
	DeltaColor cmdColor,paramColor;
	int i;
	CharGroup charGroup;
	LinkColor linkColor;
	LinkDirection linkDirection;
	CvtCategory cvtCategory;
	
	for (i = 0; i < numDeltaColors; i++) colorDeltaDone[i] = false;
	while (cvtDelta <= this->scanner.sym && this->scanner.sym <= gCvtDelta) {
		this->newSyntax = true;
		cmdColor = (DeltaColor)(this->scanner.sym-cvtDelta);
		if (colorDeltaDone[cmdColor]) { swprintf(this->errMsg,L"Cannot have more than one %s command per control value" BRK \
										 L"Please combine them to a single %s command. Example: %s(1 @18..20;22, -1 @ 24..25)",
										  keyWord[this->scanner.sym],keyWord[this->scanner.sym],keyWord[this->scanner.sym]); return false; }
		colorDeltaDone[cmdColor] = true;
		if (!this->scanner.GetSym()) return false;
		if (this->scanner.sym != leftParen) { swprintf(this->errMsg,L"( expected"); return false; }
		if (!this->scanner.GetSym()) return false;
		pixelAtPpemRangeParam.lowPpemSize = cvt->breakPpemSize-1; // lowest permissible ppem size - 1
		pixelAtPpemRangeParam.highPpemSize = maxPpemSize-1;
		if (!this->PixelAtPpemRange(cmdColor,&pixelAtPpemRangeParam,&paramColor)) return false;
//		if (!AppendDeltas(cmdColor,&pixelAtPpemRangeParam,cvt,this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
		this->UnpackAttribute(cvt->attribute,&charGroup,&linkColor,&linkDirection,&cvtCategory);
		this->tt->AssertFreeProjVector(linkDirection == linkX ? xRomanDir : yRomanDir);
		this->tt->DLT(true,paramColor,(short)cvtNum,pixelAtPpemRangeParam.value,pixelAtPpemRangeParam.deltaPpemSize);
		while (this->scanner.sym == comma) {
			if (!this->scanner.GetSym()) return false;
			for (i = maxPpemSize-1; i > 0 && !pixelAtPpemRangeParam.deltaPpemSize[i]; i--);
			pixelAtPpemRangeParam.lowPpemSize = i; // lowest permissible ppem size - 1
			if (!this->PixelAtPpemRange(cmdColor,&pixelAtPpemRangeParam,&paramColor)) return false;
//			if (!AppendDeltas(cmdColor,&pixelAtPpemRangeParam,cvt,this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
			this->tt->DLT(true,paramColor,(short)cvtNum,pixelAtPpemRangeParam.value,pixelAtPpemRangeParam.deltaPpemSize);
		}
		if (this->scanner.sym != rightParen) { swprintf(this->errMsg,L") expected"); return false; }
		if (!this->scanner.GetSym()) return false;
	}
	return true; // by now
} // PrivateControlValueTable::DeltaDeclaration

bool PrivateControlValueTable::InlineSttmt(void) {
	ActParam asmCodeParam;
	
	while (this->scanner.sym == asM) {
		if (!this->scanner.GetSym()) return false;
		if (this->scanner.sym != leftParen) { swprintf(this->errMsg,L"( expected"); return false; }
		if (!this->scanner.GetSym()) return false;
		if (!this->Parameter(&asmCodeParam)) return false;
		if (asmCodeParam.type != anyS) { swprintf(this->errMsg,L"Actual TrueType code expected (Example: \x22/* Comment */\x22)"); this->scanner.ErrUnGetSym(); return false; }
		this->tt->Emit(this->scanner.literal);	
		if (this->scanner.sym != rightParen) { swprintf(this->errMsg,L") expected"); return false; }
		if (!this->scanner.GetSym()) return false;
	}
	return true; // by now
} // PrivateControlValueTable::InlineSttmt

bool PrivateControlValueTable::Parameter(ActParam *actParam) {
	if ((natural <= this->scanner.sym && this->scanner.sym <= rational) || (leftParen <= this->scanner.sym && this->scanner.sym <= minus)) {
		if (!this->Expression(actParam)) return false;
	} else if (this->scanner.sym == at) {
		if (!this->scanner.GetSym()) return false;
		if (!this->PpemRange(actParam)) return false;
	} else if (this->scanner.sym == literal) {
		actParam->type = anyS; actParam->literal = this->scanner.literal;
		if (!this->scanner.GetSym()) return false;
	} else {
		swprintf(this->errMsg,L"parameter starts with illegal symbol (+, -, @, number, or \x22string\x22 expected)"); return false;
	}
	return true; // by now
} // PrivateControlValueTable::Parameter

bool ValidBinaryOperation(ActParam *a, ActParam *b, Symbol op, wchar_t errMsg[]) {
	wchar_t opName[4][10] = {L"add",L"subtract",L"multiply",L"divide"};
	
	if (a->type < naturalN || rationalN < a->type || b->type < naturalN || rationalN < b->type) {
		swprintf(errMsg,L"cannot %s these operands",opName[op-plus]); return false;
	}
	a->type = Max(a->type,b->type);
	if (op == divide && a->type == naturalN && b->type == naturalN && b->value != 0 && a->value % b->value != 0) a->type = rationalN;
	switch (op) {
	case plus:
	case minus:
		if (op == plus) a->value += b->value; else a->value -= b->value;
		if (a->value < shortMin << places6) {
			swprintf(errMsg,L"result of subtraction too small (cannot be below %li)",shortMin); return false;
		} else if (shortMax << places6 < a->value) {
			swprintf(errMsg,L"result of addition too large (cannot be above %li)",shortMax); return false;
		}
		break;
	case times:
		if ((double)Abs(a->value)*(double)Abs(b->value) < (double)((shortMax + 1) << (places6 + places6))) {
			a->value = (a->value*b->value + half6) >> places6;
		} else {
			swprintf(errMsg,L"result of multiplication too large (cannot be %li or larger in magnitude)",shortMax+1); return false;
		}
		break;
	case divide:
		if (b->value != 0 && (double)Abs(a->value) < (double)(shortMax+1)*(double)Abs(b->value)) {
			if (a->type == naturalN && b->type == naturalN && a->value%b->value != 0) a->type = rationalN;
			a->value = ((a->value << (places6 + 1)) + b->value)/(b->value << 1);
		} else {
			swprintf(errMsg,L"result of division too large (cannot be %li or larger in magnitude)",shortMax+1); return false;
		}
		break;
	default:
		break;
	}
	return true; // by now
} // ValidBinaryOperation

bool PrivateControlValueTable::Expression(ActParam *actParam) {
	Symbol sign,op;
	ActParam actParam2;
	
	sign = plus;
	if (this->scanner.sym == plus || this->scanner.sym == minus) {
		sign = this->scanner.sym;
		if (!this->scanner.GetSym()) return false;
	}
	if (!this->Term(actParam)) return false;
	if (sign == minus) actParam->value = -actParam->value;
	while (this->scanner.sym == plus || this->scanner.sym == minus) {
		op = this->scanner.sym;
		if (!this->scanner.GetSym()) return false;
		if (!this->Term(&actParam2)) return false;
		if (!ValidBinaryOperation(actParam,&actParam2,op,this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
	}
	return true; // by now
} // PrivateControlValueTable::Expression

bool PrivateControlValueTable::Term(ActParam *actParam) {
	Symbol op;
	ActParam actParam2;
	
	if (!this->Factor(actParam)) return false;
	while (this->scanner.sym == times || this->scanner.sym == divide) {
		op = this->scanner.sym;
		if (!this->scanner.GetSym()) return false;
		if (!this->Factor(&actParam2)) return false;
		if (!ValidBinaryOperation(actParam,&actParam2,op,this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
	}
	return true; // by now
} // PrivateControlValueTable::Term

bool PrivateControlValueTable::Factor(ActParam *actParam) {
	if (natural <= this->scanner.sym && this->scanner.sym <= rational) {
		actParam->type = this->scanner.sym == natural ? naturalN : rationalN;
		actParam->value = this->scanner.value;
		if (actParam->type == naturalN) actParam->value <<= places6; // standardize for arithmetic
		if (!this->scanner.GetSym()) return false;
	} else if (this->scanner.sym == leftParen) {
		if (!this->scanner.GetSym()) return false;
		if (!this->Expression(actParam)) return false;
		if (this->scanner.sym != rightParen) { swprintf(this->errMsg,L") expected"); return false; }
		if (!this->scanner.GetSym()) return false;
	} else {
		swprintf(this->errMsg,L"factor starts with illegal symbol (number or ( expected)"); return false;
	}
	return true; // by now
} // PrivateControlValueTable::Factor

bool PrivateControlValueTable::PixelAtPpemRange(DeltaColor cmdColor, ActParam *actParam, DeltaColor *paramColor) {
	int pixelAmount;
	ActParam colorParam;

	if (!this->Parameter(actParam)) return false;
	if (!AssertPixelAmount(actParam,-maxPixelValue,maxPixelValue,L"Delta amount",this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
	pixelAmount = actParam->value;
	if (pixelAmount == 0) { swprintf(this->errMsg,L"Delta amount cannot be 0"); this->scanner.ErrUnGetSym(); return false; }
	if (!this->Parameter(actParam)) return false;
	if (actParam->type < ppemN || multipleRangesOfPpemN < actParam->type) { swprintf(this->errMsg,L"Ppem size(s) expected (Example 10..12;14 for ppem sizes 10 through 12 and ppem size 14)"); this->scanner.ErrUnGetSym(); return false; }
	*paramColor = cmdColor;
	actParam->deltaColor = alwaysDelta;
	if (this->scanner.sym == percent) { // optional delta color sub-parameter
		this->scanner.GetSym();
		this->Parameter(&colorParam);
		if (colorParam.type != naturalN || DeltaColorOfByte((unsigned char)(colorParam.value/one6)) == illegalDelta) {
			swprintf(this->errMsg,L"illegal delta color flag (can be %hs only)",AllDeltaColorBytes());
			this->scanner.ErrUnGetSym(); return false;
		}
		actParam->deltaColor = DeltaColorOfByte((unsigned char)(colorParam.value/one6));
		if (cmdColor != alwaysDelta && actParam->deltaColor != alwaysDelta) {
			swprintf(this->errMsg,L"cannot override delta color specified by a BDELTA or a GDELTA command");
			this->scanner.ErrUnGetSym(); return false;
		}
		*paramColor = actParam->deltaColor;
	}
	actParam->value = pixelAmount; // restore
	actParam->type = deltaAtRangeOfPpemN;
	return true; // by now
} // PrivateControlValueTable::PixelAtPpemRange
	
bool PrivateControlValueTable::PpemRange(ActParam *actParam) {
	int i;
	
	actParam->type = ppemN; // for now
	for (i = 0; i < maxPpemSize; i++) actParam->deltaPpemSize[i] = false;
	if (!this->Range(actParam)) return false;
	while (this->scanner.sym == semiColon) {
		if (!this->scanner.GetSym()) return false;
		if (!this->Range(actParam)) return false;
		actParam->type = multipleRangesOfPpemN; // by now
	}
	for (i = 0; i < maxPpemSize && !actParam->deltaPpemSize[i]; i++);
	actParam->value = i; // assign lowest for single ppem size (such as break ppem size)
	return true; // by now
} // PrivateControlValueTable::PpemRange

bool PrivateControlValueTable::Range(ActParam *actParam) {
	ActParam lowParam,highParam;
	int low,high,i;

	if (!this->Expression(&lowParam)) return false;
	if (!AssertNatural(&lowParam,actParam->lowPpemSize+1,actParam->highPpemSize,L"Ppem size",this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
	actParam->lowPpemSize = low = high = lowParam.value;
	if (this->scanner.sym == ellipsis) {
		if (!this->scanner.GetSym()) return false;
		if (!this->Expression(&highParam)) return false;
		if (!AssertNatural(&highParam,actParam->lowPpemSize+1,actParam->highPpemSize,L"Ppem size",this->errMsg)) { this->scanner.ErrUnGetSym(); return false; }
		actParam->lowPpemSize = high = highParam.value;
		actParam->type = rangeOfPpemN; // by now
	}
	for (i = low; i <= Min(maxPpemSize-1,high); i++) actParam->deltaPpemSize[i] = true;
	return true; // by now
} // PrivateControlValueTable::Range

bool PrivateControlValueTable::Compile(TextBuffer*source, TextBuffer*prepText, bool legacyCompile, int *errPos, int *errLen, wchar_t errMsg[]) {
	int i,firstAvailSubAttributeValue[numSubAttributes];
	unsigned int currAttribute;
	Attribute *sortedAttributes;
	bool memError;
	wchar_t comment[maxLineSize],dateTime[32];

	this->legacyCompile = legacyCompile;
	this->tempAttributes = NULL;
	this->oldSyntax = this->newSyntax = false; // don't know either way at this point
	this->errMsg = errMsg; // copy pointer
	this->cvtDataValid = this->cvtDataSorted = false;
	DefaultSettings(&this->tempSettings);
	this->tempData = NewCvtData();
	this->tt = NewTTSourceEngine();
	if (this->tt) this->tt->InitTTEngine(legacyCompile, &memError);
	if (!this->cpgmData || !this->tempData || !this->tt || memError) 
	{ 
		swprintf(this->errMsg,L"Not enough memory to compile control program"); 
		goto error; 
	}

	for (i = firstCharGroup; i <= lastCharGroup; i++)
		if (!Attribute::InsertByName(&this->tempAttributes,true,(wchar_t *)charGroupToStrg[i],(wchar_t *)charGroupToSpacingText[i],group,i,this->errMsg)) goto error;
	firstAvailSubAttributeValue[group] = i;
	for (i = firstLinkColor; i <= lastLinkColor; i++)
		if (!Attribute::InsertByName(&this->tempAttributes,true,(wchar_t *)linkColorToStrg[i],L"",color,i,this->errMsg)) goto error;
	firstAvailSubAttributeValue[color] = i;
	for (i = firstLinkDirection; i <= lastLinkDirection; i++)
		if (!Attribute::InsertByName(&this->tempAttributes,true,(wchar_t *)linkDirectionToStrg[i],L"",direction,i,this->errMsg)) goto error;
	firstAvailSubAttributeValue[direction] = i;
	for (i = firstCvtCategory; i <= lastCvtCategory; i++)
		if (!Attribute::InsertByName(&this->tempAttributes,true,(wchar_t *)cvtCategoryToStrg[i],L"",category,i,this->errMsg)) goto error;
	firstAvailSubAttributeValue[category] = i;
	
	
	this->tt->Emit(L"/* auto-generated pre-program */");
	DateTimeStrg(dateTime);
	swprintf(comment,L"/* VTT %s compiler %s */",VTTVersionString,dateTime); 
	this->tt->Emit(comment);
	this->tt->Emit(L"");

	if (!this->scanner.Init(source,NULL,errMsg)) goto error;
	if (!this->InlineSttmt()) goto error;
	while (this->scanner.sym == category || this->scanner.sym == group) {
		if (!this->AttributeDeclaration(firstAvailSubAttributeValue)) goto error;
		if (!this->InlineSttmt()) goto error;
	}
		
	// need to do fpgmBias BEFORE mandatory settings, but mandatory settings before all other settings
	if (!this->legacyCompile && this->scanner.sym == fpgmBias) {
		if (!this->SettingsDeclaration()) goto error;
		if (!this->InlineSttmt()) goto error;
	}

	// mandatory settings
	if (this->legacyCompile){
		this->tt->SetAspectRatioFlag(); // not needed
	}else {
		this->tt->SetGreyScalingFlag();
	}
	
	// compile settings
	while (firstSetting <= this->scanner.sym && this->scanner.sym <= lastSetting) {
		if (!this->SettingsDeclaration()) goto error;
		if (!this->InlineSttmt()) goto error;
	}

	// provide defaults for settings if necessary
	if (!this->tempSettings.defined[instructionsOn-firstSetting]) {
		swprintf(comment,L"/* %s (default) */",keyWord[instructionsOn]); this->tt->Emit(comment);
		this->tt->INSTCTRL(this->tempSettings.instructionsOnFromPpemSize,this->tempSettings.instructionsOnToPpemSize);
	}
	if (!this->tempSettings.defined[dropOutCtrlOff-firstSetting] && !this->tempSettings.defined[scanCtrl-firstSetting]) {
		swprintf(comment,L"/* %s (default) */",keyWord[scanCtrl]); this->tt->Emit(comment);
		this->tt->SCANCTRL(this->tempSettings.scanCtrlFlags);
	}
	if (!this->tempSettings.defined[dropOutCtrlOff-firstSetting] && !this->tempSettings.defined[scanType-firstSetting]) {
		swprintf(comment,L"/* %s (default) */",keyWord[scanType]); this->tt->Emit(comment);
		this->tt->SCANTYPE(this->tempSettings.scanTypeFlags);
	}
	if (!this->tempSettings.defined[cvtCutIn-firstSetting]) {
		swprintf(comment,L"/* %s (default) */",keyWord[cvtCutIn]); this->tt->Emit(comment);
		this->tt->AssertFreeProjVector(yRomanDir); // so far, this may become aspect-ration dependent, or such like...
		this->tt->SCVTCI(this->tempSettings.numCvtCutIns,this->tempSettings.cvtCutInPpemSize,this->tempSettings.cvtCutInValue);
	}
	
	currAttribute = PrivateControlValueTable::PackAttribute(anyGroup,linkAnyColor,linkAnyDir,cvtAnyCategory);
	while (this->scanner.sym != eot) {
	//	if (this->scanner.sym != eol) {
		if (!this->CvtDeclaration(&currAttribute)) goto error;
	//	}
	//
	//	if (this->scanner.sym == eol) {
	//		if (!this->scanner.GetSym()) goto error;
	//	} else if (this->scanner.sym != eot) {
	//		swprintf(errMsg,L"extra text on line"); goto error;
	//	}
	//
		}
	this->scanner.Term(errPos,errLen);
	this->tt->ResetPrepState();
	this->tt->TermTTEngine(this->newSyntax ? prepText : NULL,&memError);
	if (memError) 
	{ 
		swprintf(this->errMsg,L"Not enough memory to compile control program"); 
		goto error; 
	}

#ifdef DEBUGCVT
	DebugS(L"LIST OF DEFINED ATTRIBUTES SORTED BY NAME\r");
	DebugS(L"\r");
	Attribute::Dump(this->tempAttributes);
	DebugS(L"\r");
#endif
	sortedAttributes = NULL;
	if (!Attribute::SortByValue(&sortedAttributes,this->tempAttributes,this->errMsg)) { if (sortedAttributes) delete sortedAttributes; goto error; }
	if (this->tempAttributes) delete this->tempAttributes;
	this->tempAttributes = sortedAttributes;
#ifdef DEBUGCVT
	DebugS(L"LIST OF DEFINED ATTRIBUTES SORTED BY VALUE\r");
	DebugS(L"\r");
	Attribute::Dump(this->tempAttributes);
	DebugS(L"\r");
#endif
	
	this->cpgmSettings = this->tempSettings;
	this->cvtDataValid = true;
	for (this->lowestCvtNum = 0; this->lowestCvtNum < maxCvtNum && !(this->tempData[this->lowestCvtNum].flags & cvtDefined); this->lowestCvtNum++);
	for (this->highestCvtNum = maxCvtNum-1; this->highestCvtNum >= 0 && !(this->tempData[this->highestCvtNum].flags & cvtDefined); this->highestCvtNum--);

	this->newNumCharGroups = firstAvailSubAttributeValue[group];
	DisposeCvtData(&this->cpgmData);
	this->cpgmData = this->tempData;
	this->tempData = NULL;
	if (this->attributes) delete this->attributes;
	this->attributes = this->tempAttributes;
	this->tempAttributes = NULL;
#ifdef DEBUGCVT
	if (!this->newSyntax) {
		DumpOldSyntaxInNewFormat(this);
	}
#endif
	delete this->tt; this->tt = NULL;
	return true;
error:
	this->scanner.Term(errPos,errLen);
	DisposeCvtData(&this->tempData);
	if (this->tempAttributes) { delete this->tempAttributes; this->tempAttributes = NULL; }
	if (this->tt) { this->tt->TermTTEngine(NULL,&memError); delete this->tt; this->tt = NULL; }
	return false;
} // PrivateControlValueTable::Compile

bool PrivateControlValueTable::IsControlProgramFormat(void) {
	return this->newSyntax;
} // PrivateControlValueTable::IsControlProgramFormat

bool PrivateControlValueTable::LinearAdvanceWidths(void) {
	if (!this->legacyCompile)
	{
		return this->cpgmSettings.linearAdvanceWidthsFlag > 0;
	}
	else
	{
		return false;
	}
} // PrivateControlValueTable::LinearAdvanceWidths


int PrivateControlValueTable::LowestCvtNum(void) {
	return this->cvtDataValid ? this->lowestCvtNum : maxCvtNum;
} // PrivateControlValueTable::LowestCvtNum

int PrivateControlValueTable::HighestCvtNum(void) {
	return this->cvtDataValid ? this->highestCvtNum : -1;
} // PrivateControlValueTable::HighestCvtNum

int PrivateControlValueTable::LowestCvtIdx(void) {
	this->AssertSortedCvt();
	return this->cvtDataSorted ? this->lowestCvtIdx : maxCvtNum;
} // PrivateControlValueTable::LowestCvtIdx

int PrivateControlValueTable::HighestCvtIdx(void) {
	this->AssertSortedCvt();
	return this->cvtDataSorted ? this->highestCvtIdx : -1;
} // PrivateControlValueTable::HighestCvtIdx

int PrivateControlValueTable::CvtNumOf(int idx) {
	this->AssertSortedCvt();
	return this->cvtDataSorted && this->lowestCvtIdx <= idx && idx <= this->highestCvtIdx ? this->cvtKeyOfIdx[idx].num : illegalCvtNum;
} // PrivateControlValueTable::CvtNumOf

int PrivateControlValueTable::CvtIdxOf(int num) {
	this->AssertSortedCvt();
	return this->cvtDataSorted && this->lowestCvtNum <= num && num <= this->highestCvtNum ? this->cvtIdxOfNum[num] : -1;
} // PrivateControlValueTable::CvtIdxOf

#define Exists(this,cvtNum) (this->cvtDataValid && 0 <= cvtNum && cvtNum <= this->highestCvtNum && this->cpgmData[cvtNum].flags & cvtDefined)
#define AttribExists(this,cvtNum) (Exists(this,cvtNum) && this->cpgmData[cvtNum].flags & attributeDefined)

bool PrivateControlValueTable::CvtNumExists(int cvtNum) {
	return Exists(this,cvtNum);
} // PrivateControlValueTable::CvtNumExists

bool PrivateControlValueTable::GetCvtValue(int cvtNum, short *cvtValue) {
	if (!Exists(this,cvtNum)) return false;
	*cvtValue = this->cpgmData[cvtNum].value;
	return true;
} // PrivateControlValueTable::GetCvtValue

bool PrivateControlValueTable::CvtAttributesExist(int cvtNum) {
	return AttribExists(this,cvtNum);
} // PrivateControlValueTable::CvtAttributesExist

bool PrivateControlValueTable::GetCvtAttributes(int cvtNum, CharGroup *charGroup, LinkColor *linkColor, LinkDirection *linkDirection, CvtCategory *cvtCategory, bool *relative) {
	if (!AttribExists(this,cvtNum)) return false;
	PrivateControlValueTable::UnpackAttribute(this->cpgmData[cvtNum].attribute,charGroup,linkColor,linkDirection,cvtCategory);
	*relative = (this->cpgmData[cvtNum].flags & relativeValue) != 0;
	return true;
} // PrivateControlValueTable::GetCvtAttributes

int PrivateControlValueTable::NumCharGroups(void) {
	return this->cvtDataValid ? this->newNumCharGroups : 0;
} // PrivateControlValueTable::NumCharGroups

bool PrivateControlValueTable::GetAttributeStrings(int cvtNum, wchar_t charGroup[], wchar_t linkColor[], wchar_t linkDirection[], wchar_t cvtCategory[], wchar_t relative[]) {
	charGroup[0] = linkColor[0] = linkDirection[0] = cvtCategory[0] = relative[0] = L'\0';
	if (!AttribExists(this,cvtNum)) return false;
	PrivateControlValueTable::UnpackAttributeStrings(this->cpgmData[cvtNum].attribute,charGroup,linkColor,linkDirection,cvtCategory);
	swprintf(relative,L"%s",this->cpgmData[cvtNum].flags & relativeValue ? L"relative" : L"absolute");
	return true;
} // PrivateControlValueTable::GetAttributeStrings

bool PrivateControlValueTable::GetCharGroupString(CharGroup charGroup, wchar_t string[]) {
	wchar_t errMsg[maxLineSize];
	
	string[0] = L'\0';
	if (!this->cvtDataValid || charGroup < 0 || this->newNumCharGroups <= charGroup) return false;
	return Attribute::SearchByValue(this->attributes,group,charGroup,string,NULL,errMsg);
} // PrivateControlValueTable::GetCharGroupString

bool PrivateControlValueTable::GetSpacingText(CharGroup charGroup, wchar_t spacingText[]) {
	wchar_t errMsg[maxLineSize];

	spacingText[0] = L'\0';
	if (!this->cvtDataValid || group < 0 || this->newNumCharGroups <= group) return false;
	return Attribute::SearchByValue(this->attributes,group,charGroup,NULL,spacingText,errMsg);
} // PrivateControlValueTable::GetSpacingText

#define CompareKey(a,b) ((a).attribute < (b).attribute ? -1 : ((a).attribute > (b).attribute ? +1 : (int)(a).value - (int)(b).value))

int PrivateControlValueTable::GetBestCvtMatch(CharGroup charGroup, LinkColor linkColor, LinkDirection linkDirection, CvtCategory cvtCategory, int distance) {
	int attempt,firstAttempt,low,high,mid,cmp,lowCvtNum,highCvtNum;
	unsigned int lowAttr,highAttr,lowValue,highValue,catMask4cvtDummy;
	CvtKey cvtKey,targetKey;

	if (!this->cvtDataValid || cvtCategory == cvtAnyCategory) return illegalCvtNum;
	
	if (cvtCategory == -1) {
		catMask4cvtDummy = ~subAttributeMask; // since cvtCategory occupies lowest 8 bits...
		firstAttempt = 1;
		cvtCategory = cvtAnyCategory;
	} else {
		catMask4cvtDummy = 0xffffffff;
		firstAttempt = 0;
	}
	
	this->AssertSortedCvt();

	targetKey.value	= (unsigned short)(distance + 0x8000);
	for (attempt = firstAttempt; attempt <= 2; attempt++) {
		switch (attempt) {
			case 0: break;
			case 1: cvtCategory = cvtAnyCategory; break;
			case 2: linkDirection = linkAnyDir; break;
		}
		targetKey.attribute = PrivateControlValueTable::PackAttribute(charGroup,linkColor,linkDirection,cvtCategory) & catMask4cvtDummy;
		
		low = this->lowestCvtIdx; high = this->highestCvtIdx;
		do { // binary search
			mid = (low + high) >> 1;
			cvtKey = this->cvtKeyOfIdx[mid];
			cvtKey.attribute &= catMask4cvtDummy;
			cmp = CompareKey(targetKey,cvtKey);
			if (!cmp) return this->cvtKeyOfIdx[mid].num; // found
			if (cmp > 0) low = mid + 1; else high = mid - 1;
		} while (low <= high);
		// since by now low > high => lowValue > highValue
		lowCvtNum  = this->cvtKeyOfIdx[low].num;
		highCvtNum = this->cvtKeyOfIdx[high].num;
		lowAttr	   = this->cvtKeyOfIdx[low].attribute  & catMask4cvtDummy;
		highAttr   = this->cvtKeyOfIdx[high].attribute & catMask4cvtDummy;
		lowValue   = this->cvtKeyOfIdx[low].value;
		highValue  = this->cvtKeyOfIdx[high].value;
		if (lowAttr == targetKey.attribute && highAttr == targetKey.attribute) return lowValue - targetKey.value < targetKey.value - highValue ? lowCvtNum : highCvtNum;
		else if (lowAttr == targetKey.attribute) return lowCvtNum;
		else if (highAttr == targetKey.attribute) return highCvtNum;
	//	else there is no cvt in this linkDirection and cvtCategory
	}
	return illegalCvtNum; // by now
} // PrivateControlValueTable::GetBestCvtMatch

void PrivateControlValueTable::PutCvtBinary(int size, unsigned char data[]) {
	short *cvtData = (short *)data;
	int i;
	
	this->lowestCvtNum = 0; // we don't know any better at this point, plus there are no valid attributes...
	this->highestCvtNum = (size >> 1) - 1;
	for (i = 0; i <= this->highestCvtNum; i++, cvtData++) this->cpgmData[i].value = SWAPW(*cvtData);
} // PrivateControlValueTable::PutCvtBinary

void PrivateControlValueTable::GetCvtBinary(int *size, unsigned char data[]) {
	short *cvtData = (short *)data;
	int i;

	*size = (this->highestCvtNum + 1) << 1;
	for (i = 0; i <= this->highestCvtNum; i++, cvtData++) *cvtData = SWAPW(this->cpgmData[i].value);
} // PrivateControlValueTable::GetCvtBinary

int PrivateControlValueTable::GetCvtBinarySize(void) {
	return (this->highestCvtNum + 1) << 1;
} // PrivateControlValueTable::GetCvtBinarySize

unsigned int PrivateControlValueTable::PackAttribute(CharGroup charGroup, LinkColor linkColor, LinkDirection linkDirection, CvtCategory cvtCategory) {
	return ((((((unsigned int)charGroup << subAttributeBits) + (unsigned int)linkColor) << subAttributeBits) + (unsigned int)linkDirection) << subAttributeBits) + (unsigned int)cvtCategory;
} // PrivateControlValueTable::PackAttribute

void PrivateControlValueTable::UnpackAttribute(unsigned int attribute, CharGroup *charGroup, LinkColor *linkColor, LinkDirection *linkDirection, CvtCategory *cvtCategory) {
	*cvtCategory   = (CvtCategory)  (attribute & subAttributeMask); attribute >>= subAttributeBits;
	*linkDirection = (LinkDirection)(attribute & subAttributeMask); attribute >>= subAttributeBits;
	*linkColor     = (LinkColor)    (attribute & subAttributeMask); attribute >>= subAttributeBits;
	*charGroup     = (CharGroup)    (attribute & subAttributeMask);
} // PrivateControlValueTable::UnpackAttribute

void PrivateControlValueTable::UnpackAttributeStrings(unsigned int attribute, wchar_t charGroup[], wchar_t linkColor[], wchar_t linkDirection[], wchar_t cvtCategory[]) {
	CharGroup theGroup;
	LinkColor theColor;
	LinkDirection theDirection;
	CvtCategory theCategory;
	wchar_t errMsg[maxLineSize];

	PrivateControlValueTable::UnpackAttribute(attribute,&theGroup,&theColor,&theDirection,&theCategory);
	if (!Attribute::SearchByValue(this->attributes,group,    theGroup,    charGroup,    NULL,errMsg)) charGroup[0]     = L'\0';
	if (!Attribute::SearchByValue(this->attributes,color,    theColor,    linkColor,    NULL,errMsg)) linkColor[0]     = L'\0';
	if (!Attribute::SearchByValue(this->attributes,direction,theDirection,linkDirection,NULL,errMsg)) linkDirection[0] = L'\0';
	if (!Attribute::SearchByValue(this->attributes,category, theCategory, cvtCategory,  NULL,errMsg)) cvtCategory[0]   = L'\0';
} // PrivateControlValueTable::UnpackAttributeStrings

void PrivateControlValueTable::AssertSortedCvt(void) {
	int cvtNum,cvtIdx,actCvtValue;
	ControlValue *cvt;
	unsigned int attribute;
	CharGroup aGroup;
	LinkColor aColor;
	LinkDirection aDir;
	CvtCategory aCat;
	
	if (!this->cvtDataValid) return; // nothing to do
	
	if (this->cvtDataSorted) return; // we're done
		
	// produce compacted and sorted array of cvtAttribute|cvtValue keys aint with cvtNumber data
	cvtIdx = 0;
	this->cvtKeyOfIdx[cvtIdx].attribute = 0;
	this->cvtKeyOfIdx[cvtIdx].value		= 0; // low sentinel
	this->cvtKeyOfIdx[cvtIdx].num		= illegalCvtNum; // cvtNum irrelevant, just silence BC
	cvtIdx++;
	for (cvtNum = 0; cvtNum <= this->highestCvtNum; cvtNum++) {
		cvt = &this->cpgmData[cvtNum];
		if (cvt->flags & cvtDefined) { // skip the gaps
			attribute = cvt->attribute;
			PrivateControlValueTable::UnpackAttribute(attribute,&aGroup,&aColor,&aDir,&aCat);
			if (aCat == cvtRoundHeight) aCat = cvtSquareHeight; // map them all to square until we determine features automatically
			actCvtValue = cvt->value;
			if (cvt->flags & relativeValue) { // make it an absolute height for the purpose of finding the best match
				actCvtValue += this->cpgmData[cvt->parent].value;
				attribute = PrivateControlValueTable::PackAttribute(aGroup,aColor,aDir,aCat);
			}
			this->cvtKeyOfIdx[cvtIdx].attribute = attribute;
			this->cvtKeyOfIdx[cvtIdx].value		= (unsigned short)(actCvtValue + 0x8000); // bias cvt values
			this->cvtKeyOfIdx[cvtIdx].num		= (short)cvtNum;
			cvtIdx++;
		}
	}
	this->cvtKeyOfIdx[cvtIdx].attribute = 0xffffffff;
	this->cvtKeyOfIdx[cvtIdx].value		= 0xffff; // high sentinel
	this->cvtKeyOfIdx[cvtIdx].num		= illegalCvtNum; // cvtNum irrelevant, just silence BC
	cvtIdx++;
	this->lowestCvtIdx = 1;
	this->highestCvtIdx = cvtIdx-2;
	this->SortCvtKeys(0,cvtIdx-1);
	for (cvtNum = this->lowestCvtNum; cvtNum <= this->highestCvtNum; cvtNum++) this->cvtIdxOfNum[cvtNum] = illegalCvtNum; // init possibly sparsely populated cvt
	for (cvtIdx = this->lowestCvtIdx; cvtIdx <= this->highestCvtIdx; cvtIdx++) this->cvtIdxOfNum[this->cvtKeyOfIdx[cvtIdx].num] = (short)cvtIdx; // setup bijectivity
	this->cvtDataSorted = true;
} // PrivateControlValueTable::AssertSortedCvt

void PrivateControlValueTable::SortCvtKeys(int low, int high) { // quicksort
	int i,j;
	CvtKey midKey,auxKey;
	
	if (low < high) {
		i = low; j = high;
		midKey = this->cvtKeyOfIdx[(i + j) >> 1];
		do {
			while (CompareKey(this->cvtKeyOfIdx[i],midKey) < 0) i++;
			while (CompareKey(midKey,this->cvtKeyOfIdx[j]) < 0) j--;
			if (i <= j) { auxKey = this->cvtKeyOfIdx[i]; this->cvtKeyOfIdx[i] = this->cvtKeyOfIdx[j]; this->cvtKeyOfIdx[j] = auxKey; i++; j--; }
		} while (i <= j);
		this->SortCvtKeys(low,j);
		this->SortCvtKeys(i,high);
	}	
} // PrivateControlValueTable::SortCvtKeys

/*****
; Auto CharacterGroup.dat for the font  UNICOURC.TTF 2.1 Beta2
; Numbered columns represent:
; Microsoft Unicode
;   Mac code
;     Apple Unicode
;       Dummy code (a count)
;         Character type - YOU MUST CORRECT THIS
;           and PostScript Name

* 0    0  0    0    <-Minimum
* FFEE FF FFEE FFEE <-Maximum

0020    20    0020      4    OTHER        space
0021    21    0021      5    OTHER        exclam
0022    22    0022      6    OTHER        quotedbl
0023    23    0023      7    OTHER        numbersign
0024    24    0024      8    FIGURE       dollar
0391    **    0391    384    UPPERCASE    Alpha
*****/

/*****
#define maxColumns 6
#define cglookahead 1
#define maxColumnSize cvtAttributeStrgLen
#define cgBufSize 0x400
#define eolCh '\r'
*****/

#define unknownUnicode 0xffff

bool PrivateControlValueTable::DumpControlValueTable(TextBuffer *text) {
	int cvtNum,pos;
	bool newFormat;
	wchar_t dump[maxLineSize],groupStrg[32],colorStrg[32],directionStrg[32],categoryStrg[32],relativeStrg[32];

	newFormat = this->IsControlProgramFormat();
	for (cvtNum = this->LowestCvtNum(); cvtNum <= this->HighestCvtNum(); cvtNum++) {
		short cvtValue; 
		if (this->GetCvtValue(cvtNum,&cvtValue)) {
			pos = swprintf(dump,L"%4li: %6i",cvtNum,cvtValue);
			if (this->CvtAttributesExist(cvtNum)) {
				this->GetAttributeStrings(cvtNum,groupStrg,colorStrg,directionStrg,categoryStrg,relativeStrg);
				pos += swprintf(&dump[pos],L" /* %s %s %s %s (%s) */",groupStrg,colorStrg,directionStrg,categoryStrg,relativeStrg);
			}
			text->AppendLine(dump);
		}
	}
	return true; // by now
} // PrivateControlValueTable::DumpControlValueTable

bool PrivateControlValueTable::CompileCharGroup(File *from, short platformID, unsigned char toCharGroupOfCharCode[], wchar_t errMsg[]) {
	int aGroup,errPos,errLen,row,col,theCol,code[4],platToCol[5] = {2, 1, 2 /* bug??? */, 0, 3}; // plat_Unicode, plat_Macintosh, plat_ISO, plat_MS, unknown case 4
	wchar_t data[2][cvtAttributeStrgLen];
	Scanner scanner;
	Attribute *groups;
	Symbol subAttribute;

	if (platformID < 0 || 4 < platformID) platformID = plat_MS;
	theCol = platToCol[platformID];
	
	row = col = 0;
	groups = NULL;
	for (aGroup = 0; aGroup < this->newNumCharGroups && Attribute::SearchByValue(this->attributes,group,aGroup,data[0],NULL,errMsg) && Attribute::InsertByName(&groups,false,data[0],NULL,group,aGroup,errMsg); aGroup++);
	if (aGroup < this->newNumCharGroups) goto error;
	if (!scanner.Init(NULL,from,errMsg)) goto error;
	while (scanner.sym != eot) {
		col = 0;
		while (col < 4 && (scanner.sym == (col < 3 ? hexadecimal : natural) || scanner.sym == times)) {
			code[col] = scanner.sym == times ? unknownUnicode : scanner.value;
			col++;
			if (!scanner.GetSym()) goto error;
		}
		if (col < 4) { swprintf(errMsg,L"%s number expected",col < 3 ? L"hexadecimal" : L"decimal"); goto error; }
		while (col < 6 && scanner.sym == ident) {
			AssignString(data[col-4],scanner.literal,cvtAttributeStrgLen);
			col++;
			if (!scanner.GetSym()) goto error;
		}
		if (col < 6) { swprintf(errMsg,L"%s expected",col < 5 ? L"character group" : L"postscript name"); goto error; }
		if (!Attribute::SearchByName(groups,data[0],NULL,&subAttribute,&aGroup,errMsg) || subAttribute != group) goto error;
		if (code[theCol] != unknownUnicode) toCharGroupOfCharCode[code[theCol]] = (unsigned char)aGroup;
		row++;
	}
	scanner.Term(&errPos,&errLen);
	if (groups) delete groups;
	return true;
error:
	swprintf(&errMsg[STRLENW(errMsg)],L" on line %li, column %li",row,col);
	scanner.Term(&errPos,&errLen);
	if (groups) delete groups;
	return false;
} // PrivateControlValueTable::CompileCharGroup


ControlValueTable *NewControlValueTable(void) {
	return new PrivateControlValueTable;
} // NewControlValueTable

short PackCvtHexAttribute(CharGroup charGroup, LinkColor linkColor, LinkDirection linkDirection, CvtCategory cvtCategory) {
	return charGroupToInt[charGroup] << groupPos | linkColorToInt[linkColor] << colorPos | linkDirectionToInt[linkDirection] << dirPos | cvtCategoryToInt[cvtCategory] << catPos;
} // PackCvtHexAttribute

void UnpackCvtHexAttribute(short hexAttribute, CharGroup *charGroup, LinkColor *linkColor, LinkDirection *linkDirection, CvtCategory *cvtCategory) {
	*cvtCategory   = intToCvtCategory[	hexAttribute >> catPos	 & catMask];
	*linkDirection = intToLinkDirection[hexAttribute >> dirPos   & dirMask];
	*linkColor	   = intToLinkColor[	hexAttribute >> colorPos & colorMask];
	*charGroup	   = intToCharGroup[	hexAttribute >> groupPos & groupMask];
} // UnpackCvtHexAttribute

void UnpackCvtHexAttributeStrings(short hexAttribute, wchar_t charGroup[], wchar_t linkColor[], wchar_t linkDirection[], wchar_t cvtCategory[]) {
	CharGroup group;
	LinkColor color;
	LinkDirection direction;
	CvtCategory category;
	
	UnpackCvtHexAttribute(hexAttribute,&group,&color,&direction,&category);
	STRCPYW(charGroup,charGroupToStrg[group]);
	STRCPYW(linkColor,linkColorToStrg[color]);
	STRCPYW(linkDirection,linkDirectionToStrg[direction]);
	STRCPYW(cvtCategory,cvtCategoryToStrg[category]);
} // UnpackCvtHexAttributeStrings
