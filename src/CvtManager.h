// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef CvtManager_dot_h
#define CvtManager_dot_h

typedef enum { anyGroup = 0, otherCase, upperCase, lowerCase, figureCase, nonLatinCase, reservedCase2, reservedCase3 } CharGroup; 


// reserved character groups because of the way the char group table is encoded, cf. TTFont.cpp, charGroupToIntInFile and intInFileToCharGroup

#define firstCharGroup anyGroup
#define lastCharGroup reservedCase3
#define numCharGroups (CharGroup)(lastCharGroup - firstCharGroup + 1)

typedef enum { linkAnyColor, linkBlack, linkGrey, linkWhite } LinkColor;
#define firstLinkColor linkAnyColor
#define lastLinkColor linkWhite
#define numLinkColors (LinkColor)(lastLinkColor - firstLinkColor + 1)

typedef enum { linkAnyDir, linkX, linkY, linkDiag } LinkDirection;
#define firstLinkDirection linkAnyDir
#define lastLinkDirection linkDiag
#define numLinkDirections (LinkDirection)(lastLinkDirection - firstLinkDirection + 1)

typedef enum {cvtAnyCategory = 0, cvtDistance, cvtStroke, cvtRound,
			  cvtLsb, cvtRsb, cvtBlackBody, cvtSerifThin, cvtSerifHeight, cvtSerifExt, cvtSerifCurveHeight, cvtSerifOther, cvtSerifBottom,
			  cvtSquareHeight, cvtRoundHeight, cvtItalicRun, cvtItalicRise,
			  cvtBendRadius, cvtBranchBendRadius, cvtScoopDepth, cvtNewStraightStroke, cvtNewDiagStroke, cvtNewAnyCategory} CvtCategory;
#define firstCvtCategory cvtAnyCategory
#define lastCvtCategory cvtNewAnyCategory
#define numUsedCvtCategories (CvtCategory)(cvtRound - firstCvtCategory + 1)
#define numCvtCategories (CvtCategory)(lastCvtCategory - firstCvtCategory + 1)

#define	maxCvtNum 0x1000L // get past FontLab's Else

#define invalidCvtNum (-1)

#define cvtAttributeStrgLen 64

class ControlValueTable {
public:
	ControlValueTable(void) {}
	virtual ~ControlValueTable(void) {}
	virtual bool Compile(TextBuffer *source, TextBuffer *prepText, bool legacyCompile, int32_t *errPos, int32_t *errLen, wchar_t errMsg[], size_t errMsgLen) = 0;
	virtual bool IsControlProgramFormat(void) = 0;
	virtual bool LinearAdvanceWidths(void) = 0;
	virtual int32_t LowestCvtNum(void) = 0;
	virtual int32_t HighestCvtNum(void) = 0;
	virtual int32_t LowestCvtIdx(void) = 0;
	virtual int32_t HighestCvtIdx(void) = 0;
	virtual int32_t CvtNumOf(int32_t idx) = 0;
	virtual int32_t CvtIdxOf(int32_t num) = 0; 
	virtual bool CvtNumExists(int32_t cvtNum) = 0;
	virtual bool GetCvtValue(int32_t cvtNum, short *cvtValue) = 0;
	virtual bool CvtAttributesExist(int32_t cvtNum) = 0;  // entered a cvt "comment"?
	virtual bool GetCvtAttributes(int32_t cvtNum, CharGroup *charGroup, LinkColor *linkColor, LinkDirection *linkDirection, CvtCategory *cvtCategory, bool *relative) = 0;
	virtual bool GetAttributeStrings(int32_t cvtNum, wchar_t charGroup[], wchar_t linkColor[], wchar_t linkDirection[], wchar_t cvtCategory[], wchar_t relative[], size_t commonStrSize) = 0;
	virtual int32_t NumCharGroups(void) = 0;
	virtual bool GetCharGroupString(CharGroup charGroup, wchar_t string[]) = 0;
	virtual bool GetSpacingText(CharGroup charGroup, wchar_t spacingText[]) = 0;
	virtual int32_t GetBestCvtMatch(CharGroup charGroup, LinkColor linkColor, LinkDirection linkDirection, CvtCategory cvtCategory, int32_t distance) = 0; // returns invalidCvtNum if no match
	virtual void PutCvtBinary(int32_t size, unsigned char data[]) = 0;
	virtual void GetCvtBinary(int32_t *size, unsigned char data[]) = 0;
	virtual int32_t GetCvtBinarySize(void) = 0;
	virtual bool DumpControlValueTable(TextBuffer *text) = 0;
	virtual bool CompileCharGroup(File *from, short platformID, unsigned char toCharGroupOfCharCode[], wchar_t errMsg[], size_t errMsgLen) = 0;
private:
};

ControlValueTable *NewControlValueTable(void);
short PackCvtHexAttribute(CharGroup charGroup, LinkColor linkColor, LinkDirection linkDirection, CvtCategory cvtCategory);
void UnpackCvtHexAttribute(short hexAttribute, CharGroup *charGroup, LinkColor *linkColor, LinkDirection *linkDirection, CvtCategory *cvtCategory);
void UnpackCvtHexAttributeStrings(short hexAttribute, wchar_t charGroup[], wchar_t linkColor[], wchar_t linkDirection[], wchar_t cvtCategory[]);

#endif // CvtManager_dot_h

