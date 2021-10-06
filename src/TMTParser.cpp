/*****

	TMTParser.c - New TypeMan Talk Compiler - Parser

	
	Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

*****/
#define _CRT_SECURE_NO_DEPRECATE 
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <string.h> // wcslen
#include <stdio.h> // printf

#include "pch.h"
#include "List.h"
#include "TTEngine.h"
#include "TTGenerator.h"
#include "TMTParser.h"

#define idents 		illegal
#define compLogics	2L
#define phases		4L
#define serifs		5L
#define idLen  		32L
#define maxFPs 		10L
#define maxAngle	18000L /* half a full circle at 100 units per degree */
#define shortMax 	0x7fffL
#define special		0
#define lexical		1
#define	syntactical	2
#define contextual	3
#define chLookAhead 2

#define WhiteSpace(self) (L'\x0' < self->ch && self->ch <= L' ')
#define InitComment(self) (self->ch == L'/' && self->ch2 == L'*')
#define TermComment(self) (self->ch == L'*' && self->ch2 == L'/')
#define	Numeric(ch) (L'0' <= (ch) && (ch) <= L'9')
#define Alpha(ch) ((L'A' <= (ch) && (ch) <= L'Z') || (L'a' <= (ch) && (ch) <= L'z'))
#define Command(self) (align <= (self)->sym && (self)->sym <= illegal)
#define InitFlag(self) (leftDir <= (self)->sym && (self)->sym <= postRound)
#define InitParam(self) (illegal <= (self)->sym && (self)->sym <= leftDir) // leftDir included because it doubles up as lessThan in the minDistGeneral parameter
#define Separator(self) (period <= (self)->sym && (self)->sym <= semiColon)
#define InterpolateCmd(cmd) ((cmd) == xInterpolate || (cmd) == xInterpolate0 || (cmd) == xInterpolate1 || (cmd) == xIPAnchor || (cmd) == yInterpolate || (cmd) == yInterpolate0 || (cmd) == yInterpolate1 || (cmd) == yIPAnchor)
#define maxGenerators 3

void TMTParser::Parse(bool *changedSrc, long *errPos, long *errLen, wchar_t errMsg[]) { /* abstract */ }
#if _DEBUG
void TMTParser::RemoveAltCodePath(bool *changedSrc, long *errPos, long *errLen, wchar_t error[]) { /* abstract */ }
#endif
void TMTParser::InitTMTParser(TextBuffer *talkText, TrueTypeFont *font, TrueTypeGlyph *glyph, bool legacyCompile, short generators, TTGenerator *gen[]) { /* abstract */ }
void TMTParser::TermTMTParser(void) { /* abstract */ }
TMTParser::TMTParser(void) { /* abstract */ }
TMTParser::~TMTParser(void) { /* abstract */ }

/* note: this and all following enumeration typedefs should be assumed to be ordered */
typedef enum { align = 0, asM, autoHinter, 
			   beginCodePath, // COM
			   call, compilerLogic, cvtAllocation, cvtLogic,
			   dAlign, diagEndCtrl, diagonalMT, diagSerifs, dStroke,
			   endCodePath, // COM
			   fixDStrokes, glyphStrokeAngle, grabHereInX, height, InLine, intersect, iStroke, mainStrokeAngle, processXSymmetrically, processYSymmetrically, quit,
			   resIIPDDist, resIIPDLink, // COM
			   resXAnchor, resXDDist, resXDist, resXDLink, resXIPAnchor, resXIPDDDist, resXIPDDist, resXIPDDLink, resXIPDist, resXIPDLink, resXIPLink, resXLink, // COM
			   resYAnchor, resYDDist, resYDist, resYDLink, resYIPAnchor, resYIPDDDist, resYIPDDist, resYIPDDLink, resYIPDist, resYIPDLink, resYIPLink, resYLink, // COM
			   scoop, serif, setItalicStrokeAngle, setItalicStrokePhase, smooth, stroke, tail, tweakMetrics, vacuFormLimit, vacuFormRound,
			   //xAlign, xAnchor, xBDelta, xDelta, xDiagonal, xDist, xDoubleGrid, xDownToGrid, xGDelta, xHalfGrid, xInterpolate, xInterpolate0, xInterpolate1, xIPAnchor, xLink, xMove, xNoRound, xRound, xShift, xSmooth, xStem, xStroke, xUpToGrid,
			   xAlign, xAnchor, xBDelta, xCDelta, xDelta, xDiagonal, xDist, xDoubleGrid, xDownToGrid, xGDelta, xHalfGrid, xInterpolate, xInterpolate0, xInterpolate1, xIPAnchor, xLink, xMove, xNoRound, xRound, xShift, xSmooth, xStem, xStroke, xUpToGrid, // MODIFY GREGH
			   //yAlign, yAnchor, yBDelta, yDelta, yDiagonal, yDist, yDoubleGrid, yDownToGrid, yGDelta, yHalfGrid, yInterpolate, yInterpolate0, yInterpolate1, yIPAnchor, yLink, yMove, yNoRound, yRound, yShift, ySmooth, yStem, yStroke, yUpToGrid,
			   yAlign, yAnchor, yBDelta, yCDelta, yDelta, yDiagonal, yDist, yDoubleGrid, yDownToGrid, yGDelta, yHalfGrid, yInterpolate, yInterpolate0, yInterpolate1, yIPAnchor, yLink, yMove, yNoRound, yRound, yShift, ySmooth, yStem, yStroke, yUpToGrid, // MODIFY GREGH
			   illegal,
			   leftParen, plus, minus, natural, rational, literal, aT, atLeast,
			   upDir, // COM
			   leftDir, /* InitParam (including above illegal) */
			   rightDir, italAngle, adjItalAngle, 
			   optStroke, optStrokeLeftBias, optStrokeRightBias, // COM
			   postRound, /* leftDir through postRound: Flags() */
			   rightParen, timeS,
			   period, ellipsis,
			   colon, // COM
			   percent,
			   comma, semiColon, /* period through semiColon: Separator() */
			   eot
			 } Symbol;

typedef enum { voidParam = 0,
			   anyN, knotN, 
			   knotNttvOpt, knotNttvOptXY, // COM
			   cvtN, compLogicN, cvtLogicN, phaseN, angle100N, colorN, serifN, curveN, radiusN,
			   rationalN, posRationalN,
			   ppemSize, ppemN, rangeOfPpemN,
			   rangeOfPpemNcolorOpt,
			   anyS,
			   minDistFlagOnly, minDistGeneral,
			   dirFlag, angleFlag, 
			   strokeFlag, // COM
			   postRoundFlag} ParamType;

//	#define knotNttvOpt knotN // !COM
//	#define knotNttvOptXY knotN // !COM

typedef enum { mand, opt, optR} Presence;

typedef struct {
	ParamType type;
	Presence pres;
} FormParam;

typedef struct {
	ParamType type;
	long numValue;
	wchar_t *litValue; // pointer to parser's litValue for memory efficiency, since we don't have more than 1 string parameter per TMT command
	short minDists;
	long jumpPpemSize[maxMinDist],pixelSize[maxMinDist];
	bool deltaPpemSize[maxPpemSize]; // here we have possibly more than one rangeOfPpemN parameter, but could implement bit vectors...
	DeltaColor deltaColor; // alwaysDelta, blackDelta, greyDelta, ..., same for the entire bit vector deltaPpemSize above
	bool hasTtvOverride;
	TTVectorDesc ttvOverride; // for diagDir and perpDiagDir
} ActParam;

class Height : public ListElem {
public:
	Height(void);
	virtual ~Height(void);
	short of;
	short cvtOverride;
};

class Partner : public ListElem {
public:
	Partner(void);
	virtual ~Partner(void);
	LinkDirection direction;
	CvtCategory category;
	short of,with;
	short cvtOverride;
};

Height::Height(void) {
	// nix, but have to have at least a (pair of) method(s) or else the compiler complains...
} // Height::Height

Height::~Height(void) {
	// nix, but have to have at least a (pair of) method(s) or else the compiler complains...
} // Height::~Height

Partner::Partner(void) {
	// nix, but have to have at least a (pair of) method(s) or else the compiler complains...
} // Partner::Partner

Partner::~Partner(void) {
	// nix, but have to have at least a (pair of) method(s) or else the compiler complains...
} // Partner::~Partner

class TMTSourceParser : public TMTParser {
public:
	virtual void Parse(bool *changedSrc, long *errPos, long *errLen, wchar_t errMsg[]);
#if _DEBUG
	virtual void RemoveAltCodePath(bool *changedSrc, long *errPos, long *errLen, wchar_t error[]);
#endif
	virtual void InitTMTParser(TextBuffer *talkText, TrueTypeFont *font, TrueTypeGlyph *glyph, bool legacyCompile, short generators, TTGenerator *gen[]);
	virtual void TermTMTParser(void);
	TMTSourceParser(void);
	virtual ~TMTSourceParser(void);
private:
	long errPos,symLen;
	wchar_t errMsg[maxLineSize];
	TextBuffer *talkText;
	TrueTypeFont *font;
	TrueTypeGlyph *glyph;
	LinearListStruct *heights,*partners;
	short knots;
	double tanStraightAngle;
	bool italicStrokePhase,italicStrokeAngle,mainStrokeAngle,glyphStrokeAngle;
	short generators;
	TTGenerator *gen[maxGenerators];
	bool changedSrc;
	long pos,prevPos,prevPrevPos; // prevPos = position previous to starting the scanning of current symbol
	wchar_t ch,ch2; // 2-char look-ahead
	Symbol sym;  // 1-symbol look-ahead
	long numValue;
	bool legacyCompile; 
	wchar_t litValue[maxAsmSize];
	
	short actParams;
	ActParam actParam[maxParams];
	long paramPos[maxParams + 1]; // +1 needed to avoid out of bounds error if at max parameters
	
	bool MakeProjFreeVector(bool haveFlag, long flagValue, bool y, ActParam *parent, ActParam child[], long children, ProjFreeVector *projFreeVector, wchar_t errMsg[]);
	
	virtual void Dispatch(Symbol cmd, short params, ActParam param[], wchar_t errMsg[]);
	
	Height *TheHeight(short at);
	void RegisterHeight(short at, short cvt);
	Partner *ThePartner(bool y, short from, short to);
	void RegisterPartner(short from, short to, bool y, bool round, short cvt);
	
	/***** Scanner *****/
	virtual void GetCh(void);
	virtual void SkipComment(void);
	virtual void SkipWhiteSpace(bool includingComments);
	virtual void GetNumber(void);
	virtual void GetIdent(void);
	virtual void GetLiteral(void);
	virtual void GetSym(void);
	
	void Delete(long pos, long len);
	void Insert(long pos, const wchar_t strg[]);
	void ReplAtCurrPos(short origLen, const wchar_t repl[]);
	
	/***** Parser *****/
	virtual void XFormToNewSyntax(void);
	virtual void Flag(ActParam *actParam);
	virtual void Parameter(ActParam *actParam);
	virtual void MatchParameter(FormParam *formParams, short *formParamNum, ParamType *actParamType);
	virtual void ValidateParameter(ActParam *actParam);
	virtual void Expression(ActParam *actParam);
	virtual void Term(ActParam *actParam);
	virtual void Factor(ActParam *actParam);
	virtual void MinDist(ActParam *actParam);
	virtual void Range(ActParam *actParam);
	virtual void PpemRange(ActParam *actParam);
	
	/***** Common *****/
	/*virtual void Error(short kind, short num);*/
	virtual void ErrorMsg(short kind, const wchar_t errMsg[]);
};

typedef struct {
	wchar_t name[idLen];
	short minMatch;
	FormParam param[maxFPs];
} CommandDesc;

CommandDesc tmtCmd[idents] = {
	{L"Align",					 2, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, optR}}},
	{L"ASM",						 2, {{anyS, mand}}},
	{L"AutoHinter",				 2, {{anyS, mand}}},
	{L"BeginCodePath",			 1, {{anyN, mand}}},
	{L"Call",					 2, {{anyN, mand}, {anyN, optR}}},
	{L"CompilerLogic",			 2, {{compLogicN, mand}}},
	{L"CvtAllocation",			 4, {{anyS, mand}}},
	{L"CvtLogic",				 4, {{cvtLogicN, mand}}},
	{L"DAlign",					 2, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, optR}, {ppemSize, opt}}},
	{L"DiagEndCtrl",				 5, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {anyN, mand}}},
	{L"Diagonal",				 5, {{dirFlag, mand}, {dirFlag, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {cvtN, opt}, {ppemSize, opt}}},
	{L"DiagSerifs",				 5, {{knotN, mand}, {knotN, mand}}},
	{L"DStroke",					 2, {{dirFlag, mand}, {dirFlag, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {cvtN, opt}}},
	{L"EndCodePath",				 1},
	{L"FixDStrokes",				 1},
	{L"GlyphStrokeAngle",		 2, {{cvtN, mand}, {cvtN, mand}}},
	{L"GrabHereInX",				 2, {{knotN, mand}, {knotN, mand}}},
	{L"Height",					 1, {{knotN, mand}, {cvtN, mand}}},
	{L"Inline",					 3, {{anyS, mand}}},
	{L"Intersect",				 3, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {ppemSize, opt}, {ppemSize, opt}}},
	{L"IStroke",					 2, {{dirFlag, mand}, {dirFlag, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {cvtN, mand}, {cvtN, mand}, {phaseN, mand}, {cvtN, opt}}},
	{L"MainStrokeAngle",			 1, {{angle100N, mand}}},
	{L"ProcessXSymmetrically",	 8},
	{L"ProcessYSymmetrically",	 8},
	{L"Quit",					 1},

	{L"ResIIPDDist",				 8, {{knotN, mand}, {knotN/*ttvOpt*/, mand}, {knotN/*ttvOpt*/, mand}, {knotN/*ttvOpt*/, mand}, {knotN/*ttvOpt*/, mand}, {knotN, mand}}}, // for the time being, we'll assume that parents and children have been constrained (moved) in y...
	{L"ResIIPDLink",				 8, {{knotN, mand}, {knotN/*ttvOpt*/, mand}, {knotN/*ttvOpt*/, mand}, {cvtN, mand}, {knotN/*ttvOpt*/, mand}, {knotN/*ttvOpt*/, mand}, {cvtN, mand}, {knotN, mand}}}, // ... before calling ResIIPDDist/Link, hence we don't allow fv overrides

	{L"ResXAnchor",				 5, {{angleFlag, opt}, {knotNttvOpt, mand}, {cvtN, opt}}}, // see also TMTSourceParser::Dispatch wherein the mandatory knot is considered a child point hence the ttvOpt override designates an alternate fv (unless angleFlag is used)
	{L"ResXDDist",				 6, {{knotN, mand}, {knotNttvOptXY, mand}, {knotN, mand}, {knotNttvOptXY, mand}}}, // only children get to override fv, including x to y and v.v. though
	{L"ResXDist",				 6, {{angleFlag, opt}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {minDistFlagOnly, opt}}},
	{L"ResXDLink",				 6, {{knotN, mand}, {knotNttvOptXY, mand}, {cvtN, mand}, {knotN, mand}, {knotNttvOptXY, mand}, {cvtN, mand}}}, // only children get to override fv, including x to y and v.v. though
	{L"ResXIPAnchor",			 7, {{angleFlag, opt}, {postRoundFlag, opt}, {knotN/*ttvOpt*/, mand}, {knotN/*ttvOpt*/, mand}, {knotN, mand}}}, // for the time being, we only allow one child, hence last parameter is a parent, which does not have to override the pv again
	{L"ResXIPDDDist",			 9, {{angleFlag, opt}, {knotN, mand}, {knotN, mand}, {knotNttvOptXY, mand}, {knotN, mand}, {knotNttvOptXY, mand}, {knotN, mand}}}, // only children get to override fv, including x to y and v.v. though
	{L"ResXIPDDist",				 9, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}}}, // for lc 'm' could use a TDist version, so think about generalizing to a MDist version
	{L"ResXIPDDLink",			 9, {{angleFlag, opt}, {knotN, mand}, {knotN, mand}, {knotNttvOptXY, mand}, {cvtN, mand}, {knotN, mand}, {knotNttvOptXY, mand}, {cvtN, mand}, {knotN, mand}}}, // only children get to override fv, including x to y and v.v. though
	{L"ResXIPDist",				 8, {{angleFlag, opt}, {strokeFlag, opt}, {knotN, mand}, {knotN/*ttvOpt*/, mand}, {knotN/*ttvOpt*/, mand}, {knotN, mand}}}, // currently we don't allow to override the fv of the parent (!) nor the child...
	{L"ResXIPDLink",				 8, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {cvtN, mand}, {knotN, mand}, {knotN, mand}, {cvtN, mand}, {knotN, mand}}}, // for lc 'm' could use a TLink version, so think about generalizing to a MLink version
	{L"ResXIPLink",				 7, {{angleFlag, opt}, {strokeFlag, opt}, {knotN, mand}, {knotN/*ttvOpt*/, mand}, {knotN/*ttvOpt*/, mand}, {cvtN, mand}, {knotN, mand}}}, // ...generalizing this could become a parameter-passing nightmare
	{L"ResXLink",				 5, {{angleFlag, opt}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {cvtN, mand}, {minDistFlagOnly, opt}}},

	{L"ResYAnchor",				 5, {{angleFlag, opt}, {knotNttvOpt, mand}, {cvtN, opt}}}, // see also TMTSourceParser::Dispatch wherein the mandatory knot is considered a child point hence the ttvOpt override designates an alternate fv (unless angleFlag is used)
	{L"ResYDDist",				 6, {{knotN, mand}, {knotNttvOptXY, mand}, {knotN, mand}, {knotNttvOptXY, mand}}}, // only children get to override fv, including x to y and v.v. though
	{L"ResYDist",				 6, {{angleFlag, opt}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {minDistFlagOnly, opt}}},
	{L"ResYDLink",				 6, {{knotN, mand}, {knotNttvOptXY, mand}, {cvtN, mand}, {knotN, mand}, {knotNttvOptXY, mand}, {cvtN, mand}}}, // only children get to override fv, including x to y and v.v. though
	{L"ResYIPAnchor",			 7, {{angleFlag, opt}, {knotN/*ttvOpt*/, mand}, {knotN/*ttvOpt*/, mand}, {knotN, mand}}}, // for the time being, we only allow one child, hence last parameter is a parent, which does not have to override the pv again
	{L"ResYIPDDDist",			 9, {{knotN, mand}, {knotN, mand}, {knotNttvOptXY, mand}, {knotN, mand}, {knotNttvOptXY, mand}, {knotN, mand}}}, // only children get to override fv, including x to y and v.v. though
	{L"ResYIPDDist",				 9, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}}}, // for lc 'm' could use a TDist version, so think about generalizing to a MDist version
	{L"ResYIPDDLink",			 9, {{knotN, mand}, {knotN, mand}, {knotNttvOptXY, mand}, {cvtN, mand}, {knotN, mand}, {knotNttvOptXY, mand}, {cvtN, mand}, {knotN, mand}}}, // only children get to override fv, including x to y and v.v. though
	{L"ResYIPDist",				 8, {{angleFlag, opt}, {strokeFlag, opt}, {knotN, mand}, {knotN/*ttvOpt*/, mand}, {knotN/*ttvOpt*/, mand}, {knotN, mand}}}, // currently we don't allow to override the fv of the parent (!) nor the child...
	{L"ResYIPDLink",				 8, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {cvtN, mand}, {knotN, mand}, {knotN, mand}, {cvtN, mand}, {knotN, mand}}}, // for lc 'm' could use a TLink version, so think about generalizing to a MLink version
	{L"ResYIPLink",				 7, {{angleFlag, opt}, {strokeFlag, opt}, {knotN, mand}, {knotN/*ttvOpt*/, mand}, {knotN/*ttvOpt*/, mand}, {cvtN, mand}, {knotN, mand}}}, // ...generalizing this could become a parameter-passing nightmare
	{L"ResYLink",				 5, {{angleFlag, opt}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {cvtN, mand}, {minDistFlagOnly, opt}}},

	{L"Scoop",					 2, {{knotN, mand}, {knotN, mand}, {knotN, mand}}},
	{L"Serif",					 3, {{dirFlag, mand}, {serifN, mand}, {knotN, mand}}}, // context dependent number of knots adjusted on-the-fly in AdjustFPs
	{L"SetItalicStrokeAngle",	16},
	{L"SetItalicStrokePhase",	16},
	{L"Smooth",					 2, {{angleFlag, opt}}},  // !COM {L"Smooth",					 2},
	{L"Stroke",					 2, {{dirFlag, mand}, {dirFlag, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {cvtN, opt}, {ppemSize, opt}}},
	{L"Tail",					 2, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}}},
	{L"TweakMetrics",			 2},
	{L"VacuFormLimit",			 9, {{ppemSize, mand}}},
	{L"VacuFormRound",			 9, {{dirFlag, mand}, {dirFlag, mand}, {curveN, mand}, {radiusN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}}},
	
	{L"XAlign",					 3, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, optR}, {ppemSize, opt}}},
	{L"XAnchor",				 3, {{angleFlag, opt}, {knotNttvOpt, mand}}},
	{L"XBDelta",				 2, {{knotN, mand}, {rangeOfPpemNcolorOpt, mand}, {rangeOfPpemNcolorOpt, optR}}},
	{L"XCDelta",				 2, {{knotN, mand}, {rangeOfPpemNcolorOpt, mand}, {rangeOfPpemNcolorOpt, optR}}},
	{L"XDelta",					 3, {{knotN, mand}, {rangeOfPpemNcolorOpt, mand}, {rangeOfPpemNcolorOpt, optR}}},
	{L"XDiagonal",				 4, {{dirFlag, mand}, {dirFlag, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {cvtN, opt}, {ppemSize, opt}}},
	{L"XDist",					 4, {{angleFlag, opt}, {postRoundFlag, opt}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {minDistGeneral, opt}}},
	{L"XDoubleGrid",			 4, {{knotN, mand}, {knotN, optR}}},
	{L"XDownToGrid",			 4, {{knotN, mand}, {knotN, optR}}},
	{L"XGDelta",				 2, {{knotN, mand}, {rangeOfPpemNcolorOpt, mand}, {rangeOfPpemNcolorOpt, optR}}},
	{L"XHalfGrid",				 2, {{knotN, mand}, {knotN, optR}}},
	{L"XInterpolate",			 3, {{angleFlag, opt}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {knotNttvOpt, optR}}}, // last actual parameter set to knotN in TMTSourceParser::Parse...
	{L"XInterpolate0",			13},
	{L"XInterpolate1",			13},
	{L"XIPAnchor",				 4, {{angleFlag, opt}, {postRoundFlag, opt}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {knotNttvOpt, optR}}}, // last actual parameter set to knotN in TMTSourceParser::Parse...
	{L"XLink",					 2, {{angleFlag, opt}, {postRoundFlag, opt}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {cvtN, opt}, {minDistGeneral, opt}}},
	{L"XMove",					 2, {{rationalN, mand}, {knotN, mand}, {knotN, optR}}},
	{L"XNoRound",				 2, {{knotN, mand}, {knotN, optR}}},
	{L"XRound",					 2, {{knotN, mand}, {knotN, mand}, {cvtN, opt}}},
	{L"XShift",					 3, {{angleFlag, opt}, {knotN, mand}, {knotNttvOpt, mand}, {knotNttvOpt, optR}}}, // shift never respects the pv
	{L"XSmooth",				 3, {{angleFlag, opt}}},  // !COM {L"XSmooth",					 3},
	{L"XStem",					 4, {{knotN, mand}, {knotN, mand}, {cvtN, opt}}},
	{L"XStroke",				 4, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {cvtN, opt}}},
	{L"XUpToGrid",				 2, {{knotN, mand}, {knotN, optR}}},
	
	{L"YAlign",					 3, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, optR}, {ppemSize, opt}}},
	{L"YAnchor",				 3, {{angleFlag, opt}, {knotNttvOpt, mand}, {cvtN, opt}}},
	{L"YBDelta",				 2, {{knotN, mand}, {rangeOfPpemNcolorOpt, mand}, {rangeOfPpemNcolorOpt, optR}}},
	{L"YCDelta",				 2, {{knotN, mand}, {rangeOfPpemNcolorOpt, mand}, {rangeOfPpemNcolorOpt, optR}}}, // ADD GREGH
	{L"YDelta",					 3, {{knotN, mand}, {rangeOfPpemNcolorOpt, mand}, {rangeOfPpemNcolorOpt, optR}}},
	{L"YDiagonal",				 4, {{dirFlag, mand}, {dirFlag, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {cvtN, opt}, {ppemSize, opt}}},
	{L"YDist",					 4, {{angleFlag, opt}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {minDistGeneral, opt},}},
	{L"YDoubleGrid",			 4, {{knotN, mand}, {knotN, optR}}},
	{L"YDownToGrid",			 4, {{knotN, mand}, {knotN, optR}}},
	{L"YGDelta",				 2, {{knotN, mand}, {rangeOfPpemNcolorOpt, mand}, {rangeOfPpemNcolorOpt, optR}}},
	{L"YHalfGrid",				 2, {{knotN, mand}, {knotN, optR}}},
	{L"YInterpolate",			 3, {{angleFlag, opt}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {knotNttvOpt, optR}}}, // last actual parameter set to knotN in TMTSourceParser::Parse...
	{L"YInterpolate0",			13},
	{L"YInterpolate1",			13},
	{L"YIPAnchor",				 4, {{angleFlag, opt}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {knotNttvOpt, optR}}}, // last actual parameter set to knotN in TMTSourceParser::Parse...
	{L"YLink",					 2, {{angleFlag, opt}, {knotNttvOpt, mand}, {knotNttvOpt, mand}, {cvtN, opt}, {minDistGeneral, opt}}},
	{L"YMove",					 2, {{rationalN, mand}, {knotN, mand}, {knotN, optR}}},
	{L"YNoRound",				 2, {{knotN, mand}, {knotN, optR}}},
	{L"YRound",					 2, {{knotN, mand}, {knotN, mand}, {cvtN, opt}}},
	{L"YShift",					 3, {{angleFlag, opt}, {knotN, mand}, {knotNttvOpt, mand}, {knotNttvOpt, optR}}}, // shift never respects the pv
	{L"YSmooth",				 3, {{angleFlag, opt}}}, // !COM {L"YSmooth",					 3},
	{L"YStem",					 4, {{knotN, mand}, {knotN, mand}, {cvtN, opt}}},
	{L"YStroke",					 4, {{knotN, mand}, {knotN, mand}, {knotN, mand}, {knotN, mand}, {cvtN, opt}}},
	{L"YUpToGrid",				 2, {{knotN, mand}, {knotN, optR}}},
};

FormParam minDistParam[maxFPs]      = {{posRationalN, mand}, {ppemSize, mand}, {posRationalN, mand}}; // auxiliary parameter lists
FormParam rangeOfPpemNParam[maxFPs] = {{rationalN, mand}, {ppemN, mand}, {ppemN, mand}};			  // for recursively calling MatchParameter
FormParam deltaColorParam[maxFPs]	= {{colorN, mand}};
FormParam knotNttvOptParams[maxFPs]	= {{knotN, mand}, {knotN, mand}, {knotN, mand}};

void AdjustFPs(short serifType, FormParam *formParams);
void AdjustFPs(short serifType, FormParam *formParams) {
	/* SERIF < (type, knot0, knot1, ... knotn-1)
	   param 0     1,     2,     3,     param-1 */
	short params,i;
	
	switch (serifType) {
		case 0: params = 9; break;
		case 1:
		case 2:
		case 3: params = 6; break;
		case 4: params = 8; break;
	}
	for (i = 3; i < params; i++) formParams[i] = formParams[2];
	for (i = params; i < maxFPs-1; i++) formParams[i] = formParams[maxFPs-1];
} /* AdjustFPs */

void TMTSourceParser::Parse(bool *changedSrc, long *errPos, long *errLen, wchar_t errMsg[]) {
	Symbol cmd;
	ActParam aParam;
	short formParamNum;
	FormParam formParams[maxFPs];
	long cmdStart,actParam;

	this->GetSym();
	while (this->errPos < 0 && Command(this)) {
		cmdStart = this->prevPos;
		cmd = this->sym;
		if (cmd == illegal) this->ErrorMsg(syntactical,L"unknown VTT Talk command");
		if (cmd == xAnchor || cmd == xInterpolate || cmd == xIPAnchor || cmd == xLink ||
			cmd == yAnchor || cmd == yInterpolate || cmd == yIPAnchor || cmd == yLink) this->XFormToNewSyntax();
		this->GetSym();
		/*****
		if (cmd == ::mainStrokeAngle || cmd == ::glyphStrokeAngle || cmd == setItalicStrokeAngle || cmd == setItalicStrokePhase) {
			if (cmd == ::mainStrokeAngle && this->mainStrokeAngle || cmd == ::glyphStrokeAngle && this->glyphStrokeAngle || cmd == setItalicStrokeAngle && this->italicStrokeAngle || cmd == setItalicStrokePhase && this->italicStrokePhase) {
				swprintf(errMsg,L"%s already used in this glyph",tmtCmd[cmd].name); this->ErrorMsg(contextual,errMsg);
			}
			if (cmd == ::mainStrokeAngle && this->glyphStrokeAngle || cmd == ::glyphStrokeAngle && this->mainStrokeAngle) {
				swprintf(errMsg,L"Cannot use both %s and %s in the same glyph",tmtCmd[::mainStrokeAngle].name,tmtCmd[::glyphStrokeAngle].name); this->ErrorMsg(contextual,errMsg);
			}
			if (cmd == ::mainStrokeAngle) this->mainStrokeAngle = true;
			else if (cmd == ::glyphStrokeAngle) this->glyphStrokeAngle = true;
			else if (cmd == setItalicStrokeAngle) this->italicStrokeAngle = true;
			else if (cmd == setItalicStrokePhase) this->italicStrokePhase = true;
		}
		*****/
	//	formParamNum = 0; formParams = &tmtCmd[cmd].param[0];
		for (formParamNum = 0; formParamNum < maxFPs; formParams[formParamNum] = tmtCmd[cmd].param[formParamNum], formParamNum++);
		formParamNum = 0;
		this->actParams = 0;
		while (InitFlag(this)) {
			this->paramPos[this->actParams] = this->prevPrevPos; 
			this->Flag(&aParam); this->MatchParameter(formParams,&formParamNum,&aParam.type); this->ValidateParameter(&aParam);
			if (this->actParams < maxParams)
				this->actParam[this->actParams++] = aParam;
			else {
				swprintf(errMsg,L"too many actual parameters (cannot have more than %li parameters)",maxParams);
				this->ErrorMsg(contextual,errMsg);
			}
		}
		if (this->sym == leftParen) this->GetSym(); else this->ErrorMsg(syntactical,L"( expected");
		if (InitParam(this)) {
			this->paramPos[this->actParams] = this->prevPrevPos;
			this->Parameter(&aParam);
			this->MatchParameter(formParams,&formParamNum,&aParam.type);
			this->ValidateParameter(&aParam);
			if (this->actParams < maxParams)
				this->actParam[this->actParams++] = aParam;
			else {
				swprintf(errMsg,L"too many actual parameters (cannot have more than %li parameters)",maxParams);
				this->ErrorMsg(contextual,errMsg);
			}
			if (cmd == serif) AdjustFPs((short)(aParam.numValue/one6),formParams); // make the remaining formal parameters compatible with the actual serif type...
			while (Separator(this) || InitParam(this)) {
				if (this->sym != comma) this->ErrorMsg(syntactical,L", expected");
				if (Separator(this)) this->GetSym();
				this->paramPos[this->actParams] = this->prevPrevPos; 
				this->Parameter(&aParam);
				if (!this->legacyCompile && InterpolateCmd(cmd) && !Separator(this) && !InitParam(this)) formParams[formParamNum].type = knotN; // drop ttvOpt of other parent
				this->MatchParameter(formParams,&formParamNum,&aParam.type);
				this->ValidateParameter(&aParam);
				if (this->actParams < maxParams)
					this->actParam[this->actParams++] = aParam;
				else {
					swprintf(errMsg,L"too many actual parameters (cannot have more than %li parameters)",maxParams);
					this->ErrorMsg(contextual,errMsg);
				}
			}
		}
		while (formParamNum < maxFPs && opt <= formParams[formParamNum].pres && formParams[formParamNum].pres <= optR) formParamNum++;
		if (formParamNum < maxFPs && formParams[formParamNum].type != voidParam) this->ErrorMsg(contextual,L"not enough parameters");
		if (this->sym == rightParen) this->GetSym(); else this->ErrorMsg(syntactical,L") expected");
		for (actParam = this->actParams; actParam < maxParams; actParam++) 
            this->paramPos[actParam] = this->prevPrevPos;
		if (this->generators > 0 && this->errPos < 0) {
			this->prevPrevPos = cmdStart;
			this->Dispatch(cmd,this->actParams,this->actParam,errMsg);
			if (errMsg[0]) this->ErrorMsg(contextual,errMsg);
		}
		if (cmd == quit) this->ErrorMsg(syntactical,L""); // generate dummy error to drop out of loop the same way as errors
	}
	if (this->sym != eot) this->ErrorMsg(syntactical,L"VTT Talk command expected");
	*changedSrc = this->changedSrc;
	*errPos = this->errPos;
	*errLen = this->symLen;
	STRCPYW(errMsg,this->errMsg);
} /* TMTSourceParser::Parse */

#if _DEBUG
void TMTSourceParser::RemoveAltCodePath(bool *changedSrc, long *errPos, long *errLen, wchar_t error[]) {
	long beginPos,endPos;
	
	beginPos = endPos = -1; *changedSrc = false;
	this->GetSym();
	while (this->sym != eot && this->errPos < 0 && !(beginPos >= 0 && endPos >= 0)) {
		if (beginPos < 0 && this->sym == beginCodePath) {
			this->GetSym(); this->GetSym(); this->GetSym(); this->SkipWhiteSpace(false);
			beginPos = this->pos - chLookAhead;
		}
		if (endPos < 0 && this->sym == endCodePath) {
			endPos = this->prevPos;
		}
		this->GetSym();
	}
	if (beginPos >= 0 && endPos >= 0) {
		this->talkText->Delete(endPos,this->talkText->TheLength());
		this->talkText->Delete(0,beginPos);
		*changedSrc = true;
	}
	*errPos = this->errPos;
	*errLen = this->symLen;
} // TMTSourceParser::RemoveAltCodePath
#endif


TMTSourceParser::TMTSourceParser(void) {
	this->tanStraightAngle = tan(Rad(STRAIGHTANGLEFUDGE));
	this->italicStrokePhase = this->italicStrokeAngle = this->mainStrokeAngle = this->glyphStrokeAngle = false;
	this->heights = new LinearListStruct;
	this->partners = new LinearListStruct;
	this->legacyCompile = false; 
} // TMTSourceParser::TMTSourceParser

TMTSourceParser::~TMTSourceParser(void) {
	if (this->partners) delete this->partners;
	if (this->heights) delete this->heights;
} // TMTSourceParser::~TMTSourceParser

Height *TMTSourceParser::TheHeight(short at) {
	Height *height;
	
	height = NULL;
	if (this->heights)
		for (height = (Height*)this->heights->first; height && height->of != at; height = (Height*)height->next);
	return height;
} // TMTSourceParser::TheHeight

void TMTSourceParser::RegisterHeight(short at, short cvt) {
	Height *height;
	bool found;
	
	height = this->TheHeight(at);
	found = height != NULL;
	if (!found) height = new Height; // else overwrite like GUI would
	if (height) {
		height->of = at;
		height->cvtOverride = cvt;
		if (!found) this->heights->InsertAtEnd(height);
	}
} // TMTSourceParser::RegisterHeight

Partner *TMTSourceParser::ThePartner(bool y, short from, short to) {
	Partner *partner;
	
	partner = NULL;
	if (this->partners)
		for (partner = (Partner*)this->partners->first; partner && !((partner->of == from && partner->with == to) || (partner->of == to && partner->with == from)); partner = (Partner*)partner->next);
	return partner;
} // TMTSourceParser::ThePartner

void TMTSourceParser::RegisterPartner(short from, short to, bool y, bool round, short cvt) {
	Partner *partner;
	bool found;
	
	partner = this->ThePartner(y,from,to);
	found = partner != NULL;
	if (!found) partner = new Partner; // else overwrite like GUI would
	if (partner) {
		partner->direction = y ? linkY : linkX;
		partner->category = round ? cvtRound : cvtStroke;
		partner->of = from; partner->with = to;
		partner->cvtOverride = cvt;
		if (!found) this->partners->InsertAtEnd(partner);
	}
} // TMTSourceParser::RegisterPartner

void TMTSourceParser::InitTMTParser(TextBuffer *talkText, TrueTypeFont *font, TrueTypeGlyph *glyph, bool legacyCompile, short generators, TTGenerator *gen[]) {
	short i;
	
	this->errPos = -1; this->symLen = 0;
	this->errMsg[0] = L'\0';
	this->talkText = talkText;
	this->font = font;
	this->glyph = glyph;
	this->knots = glyph->numContoursInGlyph > 0 ? glyph->endPoint[glyph->numContoursInGlyph-1] + 1 : 0;
	this->knots += PHANTOMPOINTS;
	this->generators = generators;
	for (i = 0; i < generators; i++) this->gen[i] = gen[i];
	this->changedSrc = false;
	this->pos = this->prevPos = this->prevPrevPos = 0;
	this->ch2 = 0; // silence BC
	this->legacyCompile = legacyCompile; 
	this->GetCh();
	this->GetCh();
} /* TMTSourceParser::InitTMTParser */

void TMTSourceParser::TermTMTParser(void) {
	
} /* TMTSourceParser::TermTMTParser */

bool TMTSourceParser::MakeProjFreeVector(bool haveFlag, long flagValue, bool y, ActParam *parent, ActParam child[], long children, ProjFreeVector *projFreeVector, wchar_t errMsg[]) {
	TTVDirection flagToDir[numTTVDirections] = {xRomanDir, yRomanDir, xItalDir, yItalDir, xAdjItalDir, yAdjItalDir, diagDir, perpDiagDir};
	long i,idx = 2*(haveFlag ? 1 + flagValue : 0) + (long)y;
	bool pvOverrideError = false,fvOverrideError = false;

	projFreeVector->pv.dir = flagToDir[idx%numTTVDirections];
	projFreeVector->pv.from = projFreeVector->pv.to = illegalKnotNum;
	for (i = 0; i < children; i++) projFreeVector->fv[i] = projFreeVector->pv;
	if (!this->legacyCompile)
	{

		if (parent != NULL && parent->hasTtvOverride) { // NULL for Anchors, they have no parents
			projFreeVector->pv = parent->ttvOverride;
			if (projFreeVector->pv.from == illegalKnotNum) projFreeVector->pv.from = (short) (parent->numValue / one6);
			if (!y && haveFlag) pvOverrideError = true;
		}
		for (i = 0; i < children && !pvOverrideError && !fvOverrideError; i++) {
			if (child[i].hasTtvOverride) {
				projFreeVector->fv[i] = child[i].ttvOverride;
				if (projFreeVector->fv[i].from == illegalKnotNum) projFreeVector->fv[i].from = (short) (child[i].numValue / one6);
				if (y && haveFlag) fvOverrideError = true;
			}
		}
		if (pvOverrideError || fvOverrideError)
			swprintf(errMsg, L"cannot override %s direction when using the italic or adjusted italic angle / or //", pvOverrideError ? L"projection" : L"freedom");
	}
	return !(pvOverrideError || fvOverrideError);
} // TMTSourceParser::MakeProjFreeVector

void TMTSourceParser::Dispatch(Symbol cmd, short params, ActParam param[], wchar_t errMsg[]) {
	short i,j;
	
	errMsg[0] = L'\0';
	switch (cmd) {
		case align:
		case xAlign:
		case yAlign:
		case dAlign: {
			short ppem = param[params-1].type == ppemSize ? (short)(param[params-1].numValue/one6) : -1,
				  children = params - (2 + (ppem >= 0)), // 2 parents...
				  parent0 = (short)(param[0].numValue/one6),parent1 = (short)(param[children+1].numValue/one6),child[maxParams];
			FVOverride fvOverride = cmd == align ? fvOldMethod : (cmd == xAlign ? fvForceX : (cmd == yAlign ? fvForceY : fvStandard));

			for (i = 0; i < children; i++) child[i] = (short)(param[1+i].numValue/one6);
			for (i = 0; i < this->generators; i++) this->gen[i]->Align(fvOverride,parent0,children,child,parent1,ppem,errMsg);
			break;
		}
		case asM:
		case InLine:
			for (i = 0; i < this->generators; i++) this->gen[i]->Asm(cmd == InLine,param[0].litValue,errMsg);
			break;
		case autoHinter:
			break;
		case call: {
			short anyNum[maxParams];
			
			for (i = 0; i < params; i++) anyNum[i] = (short)(param[i].numValue/one6);
			for (i = 0; i < this->generators; i++) this->gen[i]->Call(params-1,anyNum,anyNum[params-1]);
			break;
		}
		case dStroke: {
			bool leftStationary[2];
			short cvt = (short)((params <= 6) ? illegalCvtNum : param[6].numValue/one6);
			short actualCvt;
			short knot[4];
			wchar_t buf[32];
			
			for (i = 0; i < 2; i++) leftStationary[i] = param[i].numValue > 0;
			for (i = 0; i < 4; i++) knot[i] = (short)(param[2+i].numValue/one6);
			for (i = 0; i < this->generators; i++) {
				this->gen[i]->DStroke(leftStationary,knot,cvt,&actualCvt,errMsg);
				if (actualCvt != cvt) {
					swprintf(buf,L",%hi",actualCvt); this->Insert(this->paramPos[6],buf);
				}
			}
			break;
		}
		case fixDStrokes:
			for (i = 0; i < this->generators; i++) this->gen[i]->FixDStrokes();
			break;
		case grabHereInX:
			for (i = 0; i < this->generators; i++) this->gen[i]->GrabHereInX((short)(param[0].numValue/one6),(short)(param[1].numValue/one6),errMsg);
			break;
		case intersect: {
			short ppem0 = params > 5 ? (short)(param[5].numValue/one6) : noPpemLimit,ppem1 = params > 6 ? (short)(param[6].numValue/one6) : noPpemLimit;

			for (i = 0; i < this->generators; i++) this->gen[i]->Intersect((short)(param[0].numValue/one6),(short)(param[1].numValue/one6),(short)(param[2].numValue/one6),(short)(param[3].numValue/one6),(short)(param[4].numValue/one6),ppem0,ppem1,errMsg);
			break;
		}
		case iStroke: {
			bool leftStationary[2];
			short knot[4],height[2],phase,actualCvt;
			short cvt = (short)((params <= 9) ? illegalCvtNum : param[9].numValue/one6);
			wchar_t buf[32];
			
			for (i = 0; i < 2; i++) leftStationary[i] = param[i].numValue > 0;
			for (i = 0; i < 4; i++) knot[i] = (short)(param[2+i].numValue/one6);
			for (i = 0; i < 2; i++) height[i] = (short)(param[6+i].numValue/one6);
			phase = (short)(param[8].numValue/one6);
			for (i = 0; i < this->generators; i++) {
				this->gen[i]->IStroke(leftStationary,knot,height,phase,cvt,&actualCvt,errMsg);
				if (actualCvt != cvt) {
					swprintf(buf,L",%hi",actualCvt); this->Insert(this->paramPos[9],buf);
				}
			}
			break;
		}
		case ::mainStrokeAngle:
			for (i = 0; i < this->generators; i++) this->gen[i]->MainStrokeAngle((short)(param[0].numValue/one6),errMsg);
			break;
		case ::glyphStrokeAngle:
			for (i = 0; i < this->generators; i++) this->gen[i]->GlyphStrokeAngle((short)(param[0].numValue/one6),(short)(param[1].numValue/one6),errMsg);
			break;
		case quit:
			for (i = 0; i < this->generators; i++) this->gen[i]->Quit();
			break;
		case scoop:
			for (i = 0; i < this->generators; i++) this->gen[i]->Scoop((short)(param[0].numValue/one6),(short)(param[1].numValue/one6),(short)(param[2].numValue/one6),errMsg);
			break;
		case serif: {
			bool forward = param[0].numValue > 0;
			short type = (short)(param[1].numValue/one6),knots,knot[7];
			
			knots = params-2; // direction flag and type
			for (i = 0; i < knots; i++) knot[i] = (short)(param[2+i].numValue/one6);
			for (i = 0; i < this->generators; i++) this->gen[i]->Serif(forward,type,knots,knot,errMsg);
			break;
		}
		case setItalicStrokeAngle:
			for (i = 0; i < this->generators; i++) this->gen[i]->SetItalicStroke(false,errMsg);
			break;
		case setItalicStrokePhase:
			for (i = 0; i < this->generators; i++) this->gen[i]->SetItalicStroke(true,errMsg);
			break;
		case smooth:
		case xSmooth:
		case ySmooth: {
			short y = cmd == ySmooth ? 1 : (cmd == xSmooth ? 0 : -1);
			short italicFlag = -1; 
			if (!this->legacyCompile)
			{
				bool haveFlag = params > 0 && param[0].type == angleFlag;
				italicFlag = haveFlag ? (short) param[0].numValue : -1;
			}			
			for (i = 0; i < this->generators; i++) this->gen[i]->Smooth(y,italicFlag);
			break;
		}
		case diagonalMT:
		case xDiagonal:
		case yDiagonal:
		case stroke:
		case xStroke:
		case yStroke: {
			bool leftStationary[2],nearVert,nearHorz;
			short base = cmd != xStroke && cmd != yStroke ? 2 : 0,optionalBase = base + 4, knot[4];
			short cvt = (short)((params <= optionalBase || param[optionalBase].type == ppemSize) ? illegalCvtNum : param[optionalBase].numValue/one6);
			short ppem = (short)((params <= optionalBase || param[params-1].type != ppemSize) ? -1 : param[params-1].numValue/one6);
			FVOverride fvOverride = cmd == stroke ? fvOldMethod : (cmd == xDiagonal || cmd == xStroke ? fvForceX : (cmd == yDiagonal || cmd == yStroke ? fvForceY : fvStandard));
			short actualCvt;
			Vector leftEdge,rightEdge;
			wchar_t buf[32];
			
			for (i = 0; i < 4; i++) knot[i] = (short)((short)(param[base+i].numValue)/one6);
			leftEdge.x  = this->glyph->x[knot[2]] - this->glyph->x[knot[0]]; leftEdge.x  = Abs(leftEdge.x);
			leftEdge.y  = this->glyph->y[knot[2]] - this->glyph->y[knot[0]]; leftEdge.y  = Abs(leftEdge.y);
			rightEdge.x = this->glyph->x[knot[3]] - this->glyph->x[knot[1]]; rightEdge.x = Abs(rightEdge.x);
			rightEdge.y = this->glyph->y[knot[3]] - this->glyph->y[knot[1]]; rightEdge.y = Abs(rightEdge.y);
			nearVert = leftEdge.x <= leftEdge.y*this->tanStraightAngle && rightEdge.x <= rightEdge.y*this->tanStraightAngle;
			nearHorz = leftEdge.y <= leftEdge.x*this->tanStraightAngle && rightEdge.y <= rightEdge.x*this->tanStraightAngle;
			if (cmd == xStroke && !nearVert)
				swprintf(errMsg,L"cannot accept XSTROKE (edges differ from vertical axis by %f degrees or more)",(double)STRAIGHTANGLEFUDGE);
			else if (cmd == yStroke && !nearHorz)
				swprintf(errMsg,L"cannot accept YSTROKE (edges differ from horizontal axis by %f degrees or more)",(double)STRAIGHTANGLEFUDGE);
			else if ((cmd == stroke || cmd == xStroke || cmd == yStroke) && (nearHorz || nearVert)) { // either nearHorz or nearVert, hence "informative" command
				for (i = 0; i < 4; i += 2) for (j = 0; j < 4; j += 2) this->RegisterPartner(knot[i],knot[j+1],nearHorz,false,cvt);
			} else { // diagonalMT, xDiagonal, yDiagonal, or general stroke "action" command
				for (i = 0; i < 2; i++) leftStationary[i] = param[i].numValue > 0;
				for (i = 0; i < this->generators; i++) {
					this->gen[i]->Stroke(fvOverride,leftStationary,knot,cvt,ppem,&actualCvt,errMsg);
					if (actualCvt != cvt) {
						swprintf(buf,L",%hi",actualCvt); this->Insert(this->paramPos[6],buf);
					}
				}
			}
			break;
		}
		case xStem:
		case yStem:
		case xRound:
		case yRound: {
			short knot[2],cvt;
			
			cvt = params > 2 ? (short)param[2].numValue/one6 : illegalCvtNum;
			for (i = 0; i < 2; i++) knot[i] = (short)(param[i].numValue/one6);
			
			this->RegisterPartner(knot[0],knot[1],cmd == yStem || cmd == yRound,cmd == xRound || cmd == yRound,cvt);
			break;
		}
		case vacuFormLimit:
			for (i = 0; i < this->generators; i++) this->gen[i]->VacuFormLimit((short)(param[0].numValue/one6));
			break;
		case vacuFormRound: {
			short knot[4];
			bool forward[2];
			
			for (i = 0; i < 2; i++) forward[i] = param[i].numValue > 0;
			for (i = 0; i < 4; i++) knot[i] = (short)(param[4+i].numValue/one6);
			for (i = 0; i < this->generators; i++) this->gen[i]->VacuFormRound((short)(param[2].numValue/one6),(short)(param[3].numValue/one6),forward,knot,errMsg);
			break;
		}
		case height:
			this->RegisterHeight((short)(param[0].numValue/one6),(short)(param[1].numValue/one6));
			break;
		case xAnchor:
		case yAnchor: {
			bool y = cmd == yAnchor,haveFlag = param[0].type == angleFlag;
			ActParam *knotParam = &param[haveFlag];
			short knot = (short)(knotParam->numValue/one6);
			short cvt = (short)((params <= haveFlag + 1) ? illegalCvtNum : param[haveFlag + 1].numValue/one6);
			Height *height = y ? this->TheHeight(knot) : NULL;
			short cvtHint = height ? height->cvtOverride : illegalCvtNum;
			ProjFreeVector projFreeVector;
			
			if (cvt >= 0 && cvtHint >= 0)
				swprintf(errMsg,L"cannot override a cvt number specified by a HEIGHT command");
			else {
				if (cvt < 0) cvt = cvtHint; // no cvt override => try previously specified cvt
				// since MDAP, MIAP don't ever use the dual projection vector, we'll define the knot to be a child, hence it gets to override the fv
				// this may change once I implement overrides for italic and adjusted italic angles, and this may turn out to be somewhat tricky:
				// if we measure perpendicular to the italic angle, and move in x, then subsequent instructions on the same knot must move along the italic angle, and measure in y
				// in other words, XAnchor/(knot) overrides the pv, while YAnchor/(knot) overrides the fv.
				if (this->MakeProjFreeVector(haveFlag,param[0].numValue,y,NULL,knotParam,1,&projFreeVector,errMsg)) {
					for (i = 0; i < this->generators; i++) this->gen[i]->Anchor(cmd == yAnchor,&projFreeVector,knot,cvt,true,errMsg);
				}
			}
			break;
		}
		case xLink:
		case yLink:
		case xDist:
		case yDist: {
			bool y = cmd == yLink || cmd == yDist,
					dist = cmd == xDist || cmd == yDist,
					haveFlag = param[0].type == angleFlag,
					havePostRound = param[haveFlag].type == postRoundFlag;
			short base = haveFlag+havePostRound+2;
			bool haveCvt = params > base && param[base].type == cvtN,
					haveMinDist = params > base+haveCvt && param[base+haveCvt].type == minDistGeneral,relative;
			ActParam *parentParam = &param[haveFlag+havePostRound],*childParam = &param[haveFlag+havePostRound+1];
			short parent = (short)(parentParam->numValue/one6),
				  child = (short)(childParam->numValue/one6),
				  cvt = haveCvt ? (short)(param[base].numValue/one6) : illegalCvtNum,actualCvt = cvt,
				  minDists = haveMinDist ? param[base+haveCvt].minDists : -1,
				  jumpPpemSize[maxMinDist];
			long *jSize = minDists >= 0 ? &param[base+haveCvt].jumpPpemSize[0] : NULL;
			Partner *partner = this->ThePartner(y,parent,child);
			CvtCategory cvtCategory = dist ? cvtAnyCategory : (partner ? partner->category : cvtDistance);
			short cvtHint = partner ? partner->cvtOverride : illegalCvtNum;
			short lsb = this->knots - PHANTOMPOINTS,rsb = lsb + 1;
			CharGroup charGroup;
			LinkColor linkColor;
			LinkDirection linkDirection;
			wchar_t buf[32];
			ProjFreeVector projFreeVector;

			if (cvt >= 0 && cvtHint >= 0)
				swprintf(errMsg,L"cannot override a cvt number specified by an X|YSTROKE, an X|YSTEM, or an X|YROUND command");
			else if (dist && (cvtHint >= 0 || cvtCategory != cvtAnyCategory))
				swprintf(errMsg,L"cannot use a X|YDIST command preceeded by X|YSTROKE, X|YSTEM, or X|YROUND");
			else if (havePostRound && !haveFlag)
				swprintf(errMsg,L"cannot use $ (post round flag) without using / (italic angle) or // (adjusted italic angle)");
			else {
				if (!dist) {
					if (cvt < 0) cvt = cvtHint; // no cvt override => try previously specified cvt
					if (cvt >= 0) {
						this->font->TheCvt()->GetCvtAttributes(cvt,&charGroup,&linkColor,&linkDirection,&cvtCategory,&relative); // fetch actual cvt category to encourage its use in GUI...
						if (cvtCategory == cvtAnyCategory) cvtCategory = cvtDistance; // but default to cvtDistance if cvtAnyCategory to allow bi-directionality with GUI input of cvtAnyCategory as cvtDistance...
					} else {
						if (parent == lsb || child == lsb) cvtCategory = cvtLsb;
						else if (parent == rsb || child == rsb) cvtCategory = cvtRsb;
					//	links lsb to rsb and v.v. not allowed by TTGenerator
					}
				}

				if (this->MakeProjFreeVector(haveFlag,param[0].numValue,y,parentParam,childParam,1,&projFreeVector,errMsg)) {
					for (i = 0; i < minDists; i++) jumpPpemSize[i] = (short)(jSize[i]/one6);
					for (i = 0; i < this->generators; i++) {
						this->gen[i]->Link(y,dist,&projFreeVector,havePostRound,parent,child,cvtCategory,cvt,minDists,jumpPpemSize,param[base+haveCvt].pixelSize,&actualCvt,errMsg);
						if (actualCvt != cvt) {
							swprintf(buf,L",%hi",actualCvt); this->Insert(this->paramPos[base],buf);
						}
					}
				}
			}
			break;
		}
		case xMove:
		case yMove: {
			F26Dot6 amount = param[0].numValue;
			short knots = params-1,knot[maxParams];
			
			for (i = 0; i < knots; i++) knot[i] = (short)(param[1+i].numValue/one6);
			for (i = 0; i < this->generators; i++) this->gen[i]->Move(cmd == yMove,amount,knots,knot,errMsg);
			break;
		}
		case xBDelta:
		case xCDelta: 
		case xDelta:
		case xGDelta:
		case yBDelta:
		case yDelta:
		case yCDelta: 
		case yGDelta: {
			short knot = (short)(param[0].numValue/one6),j;			
			DeltaColor cmdColor = cmd == xDelta || cmd == yDelta ? alwaysDelta : (cmd == xBDelta || cmd == yBDelta ? blackDelta : (cmd == xGDelta || cmd == yGDelta ? greyDelta : ctNatVerRGBIAWBLYDelta)),paramColor; 

			
			for (j = 1; j < params; j++) {
				paramColor = param[j].deltaColor;
				if (cmdColor != alwaysDelta && paramColor != alwaysDelta){					
					swprintf(errMsg,L"cannot override delta color specified by an X|YBDELTA or an X|YGDELTA or an X|YCDELTA command"); 
                }else{
					if (paramColor == alwaysDelta) paramColor = cmdColor;
					for (i = 0; i < this->generators; i++)
						this->gen[i]->Delta(cmd >= yBDelta,paramColor,knot,param[j].numValue,param[j].deltaPpemSize,errMsg);
				}
			}
			break;
		}
		case xHalfGrid:
		case yHalfGrid:
		case xDoubleGrid:
		case yDoubleGrid:
		case xDownToGrid:
		case yDownToGrid:
		case xUpToGrid:
		case yUpToGrid:
		case xNoRound:
		case yNoRound: {
			short knot[maxParams];
			Rounding r;
			
			for (i = 0; i < params; i++) knot[i] = (short)(param[i].numValue/one6);
			switch (cmd) {
				case xHalfGrid:
				case yHalfGrid:	  r = rthg; break;
				case xDoubleGrid:
				case yDoubleGrid: r = rtdg; break;
				case xDownToGrid:
				case yDownToGrid: r = rdtg; break;
				case xUpToGrid:
				case yUpToGrid:	  r = rutg; break;
				case xNoRound:
				case yNoRound:	  r = roff; break;
				default: break;
			}
			for (i = 0; i < this->generators; i++) this->gen[i]->SetRounding(cmd == yHalfGrid || cmd == yDoubleGrid || cmd == yDownToGrid || cmd == yUpToGrid || cmd == yNoRound,r,params,knot);
			break;
		}
		case xInterpolate: // xInterpolate0, xInterpolate1 mapped to xInterpolate
		case yInterpolate: // yInterpolate0, yInterpolate1 mapped to yInterpolate
		case xIPAnchor:
		case yIPAnchor: {
			bool y = cmd == yInterpolate || cmd == yIPAnchor,
				    haveFlag = param[0].type == angleFlag,
					havePostRound = param[haveFlag].type == postRoundFlag;
			ActParam *parent0Param = &param[haveFlag+havePostRound], *childParam = &param[haveFlag+havePostRound+1];
			short parent0 = (short)(parent0Param->numValue/one6),parent1 = (short)(param[params-1].numValue/one6),children,child[maxParams];
			ProjFreeVector projFreeVector;
			
			if (havePostRound && !haveFlag)
				swprintf(errMsg,L"cannot use $ (post round flag) without using / (italic angle) or // (adjusted italic angle)");
			else {
				children = params-2-havePostRound-haveFlag; // 2 parents...
				if (this->MakeProjFreeVector(haveFlag,param[0].numValue,y,parent0Param,childParam,children,&projFreeVector,errMsg)) {
					for (i = 0; i < children; i++) child[i] = (short)(childParam[i].numValue/one6);
					for (i = 0; i < this->generators; i++) this->gen[i]->Interpolate(y,&projFreeVector,havePostRound,parent0,children,child,parent1,cmd == xIPAnchor || cmd == yIPAnchor,errMsg);
				}
			}
			break;
		}
		case xShift:
		case yShift: {
			bool y = cmd == yShift,
				    haveFlag = param[0].type == angleFlag;
			ActParam *parentParam = &param[haveFlag], *childParam = &param[haveFlag+1];
			short parent = (short)(param[haveFlag].numValue/one6),children,child[maxParams];
			ProjFreeVector projFreeVector;
			
			children = params-1-haveFlag;
			if (this->MakeProjFreeVector(haveFlag,param[0].numValue,y,parentParam,childParam,children,&projFreeVector,errMsg)) {
				for (i = 0; i < children; i++) child[i] = (short)(childParam[i].numValue/one6);
				for (i = 0; i < this->generators; i++) this->gen[i]->Shift(y,&projFreeVector,parent,children,child,errMsg);
			}
			break;
		}
		// new rendering environment specific (Res) commands
		case beginCodePath: 
			if (!this->legacyCompile)
			{
				short fpgmBias = (short)(param[0].numValue/one6);

				for (i = 0; i < this->generators; i++) this->gen[i]->BeginCodePath(fpgmBias,errMsg);				
			}
			break; 
		case endCodePath: 
			if (!this->legacyCompile)
			{
				for (i = 0; i < this->generators; i++) this->gen[i]->EndCodePath(errMsg);				
			}
			break; 
		case resXAnchor:
		case resYAnchor: 
			if (!this->legacyCompile)
			{			 
				bool y = cmd == resYAnchor,
						haveFlag = param[0].type == angleFlag;
				short optParamOffs = haveFlag+1;
				bool haveCvt = params > optParamOffs && param[optParamOffs].type == cvtN;
				ActParam *childParam = &param[haveFlag];
				short child = (short)(childParam->numValue/one6),
					  cvt = haveCvt ? (short)(param[optParamOffs].numValue/one6) : illegalCvtNum;
				ProjFreeVector projFreeVector;

				if (this->MakeProjFreeVector(haveFlag,param[0].numValue,y,NULL,childParam,1,&projFreeVector,errMsg)) {
					for (i = 0; i < this->generators; i++) this->gen[i]->ResAnchor(cmd == resYAnchor,&projFreeVector,child,cvt,errMsg);
				}			
			}
			break; 
		case resXIPAnchor:
		case resYIPAnchor:
			if (!this->legacyCompile)
			{
				bool y = cmd == resYIPAnchor,
						haveFlag = param[0].type == angleFlag,
						havePostRound = param[haveFlag].type == postRoundFlag;
				ActParam *parent0Param = &param[haveFlag+havePostRound], *childParam = &param[haveFlag+havePostRound+1], *parent1Param = &param[haveFlag+havePostRound+2];
				short parent0 = (short)(parent0Param->numValue/one6),child = (short)(childParam->numValue/one6),parent1 = (short)(param[params-1].numValue/one6);
				ProjFreeVector projFreeVector;

				if (havePostRound && !haveFlag)
					swprintf(errMsg,L"cannot use $ (post round flag) without using / (italic angle) or // (adjusted italic angle)");
				else {
					if (this->MakeProjFreeVector(haveFlag,param[0].numValue,y,parent0Param,childParam,1,&projFreeVector,errMsg)) {
						for (i = 0; i < this->generators; i++) this->gen[i]->ResIPAnchor(y,&projFreeVector,havePostRound,parent0,child,parent1,errMsg);
					}
				}				
			}
			break; 
		case resXDist:
		case resYDist:
		case resXLink:
		case resYLink: 
			if (!this->legacyCompile)
			{
				bool y = cmd == resYDist || cmd == resYLink,
						dist = cmd == resXDist || cmd == resYDist,
						haveFlag = param[0].type == angleFlag;
				short optParamOffs = haveFlag+2;
				bool haveCvt = params > optParamOffs && param[optParamOffs].type == cvtN,
						haveMinDistFlag = params > optParamOffs+haveCvt && param[optParamOffs+haveCvt].type == minDistFlagOnly;
				short minDists = haveMinDistFlag ? param[optParamOffs+haveCvt].minDists : -1;
				ActParam *parentParam = &param[haveFlag],*childParam = &param[haveFlag+1];
				short parent = (short)(parentParam->numValue/one6),
					  child = (short)(childParam->numValue/one6),
					  cvt = haveCvt ? (short)(param[optParamOffs].numValue/one6) : illegalCvtNum;
				ProjFreeVector projFreeVector;

				if (this->MakeProjFreeVector(haveFlag,param[0].numValue,y,parentParam,childParam,1,&projFreeVector,errMsg)) {
					for (i = 0; i < this->generators; i++) {
						this->gen[i]->ResLink(y,dist,&projFreeVector,parent,child,cvt,minDists,errMsg);
					}
				}
			}
			break; 
		case resXIPDist:
		case resYIPDist:
		case resXIPLink:
		case resYIPLink: 
			if (!this->legacyCompile)
			{
				bool y = cmd == resYIPDist || cmd == resYIPLink,
						dist = cmd == resXIPDist || cmd == resYIPDist,
						haveAngleFlag = param[0].type == angleFlag,
						haveStrokeFlag = param[haveAngleFlag].type == strokeFlag;
				ActParam *grandParent0Param = &param[haveAngleFlag+haveStrokeFlag],
						 *parentParam       = &param[haveAngleFlag+haveStrokeFlag+1],
						 *childParam        = &param[haveAngleFlag+haveStrokeFlag+2],
						 *cvtParam  = !dist ? &param[haveAngleFlag+haveStrokeFlag+3] : NULL,
						 *grandParent1Param = &param[haveAngleFlag+haveStrokeFlag+!dist+3];
				short strokeFlag   = haveStrokeFlag ? (short)param[haveAngleFlag].numValue : 0,
					  grandParent0 = (short)(grandParent0Param->numValue/one6),
					  parent       = (short)(parentParam->numValue/one6),
					  child        = (short)(childParam->numValue/one6),
					  cvt  = !dist ? (short)(cvtParam->numValue/one6) : illegalCvtNum,
					  grandParent1 = (short)(grandParent1Param->numValue/one6);
				ProjFreeVector projFreeVector;

				if (this->MakeProjFreeVector(haveAngleFlag,param[0].numValue,y,grandParent0Param,parentParam,2,&projFreeVector,errMsg)) { // 2 children (?)
					for (i = 0; i < this->generators; i++) {
						this->gen[i]->ResIPLink(y,dist,&projFreeVector,strokeFlag,grandParent0,parent,child,cvt,grandParent1,errMsg);
					}
				}
			}
			break; 
		case resXIPDDist:
		case resYIPDDist:
		case resXIPDLink:
		case resYIPDLink:
			if (!this->legacyCompile)		
			{
				bool y = cmd == resYIPDDist || cmd == resYIPDLink,
						dist = cmd == resXIPDDist || cmd == resYIPDDist,
						haveAngleFlag = false, // so far, TT fn not implemented orthogonally enough
						haveStrokeFlag = false; // so far, TT fn not implemented orthogonally enough
				short cvt0ParamOffs = haveAngleFlag+haveStrokeFlag+3,
					  cvt1ParamOffs = haveAngleFlag+haveStrokeFlag+6;
				ActParam *grandParent0Param = &param[haveAngleFlag+haveStrokeFlag],
						 *parent0Param      = &param[haveAngleFlag+haveStrokeFlag+1],
						 *child0Param       = &param[haveAngleFlag+haveStrokeFlag+2],
						 *cvt0Param = !dist ? &param[haveAngleFlag+haveStrokeFlag+3] : NULL,
						 *parent1Param      = &param[haveAngleFlag+haveStrokeFlag+!dist+3],
						 *child1Param       = &param[haveAngleFlag+haveStrokeFlag+!dist+4],
						 *cvt1Param = !dist ? &param[haveAngleFlag+haveStrokeFlag+!dist+5] : NULL,
						 *grandParent1Param = &param[haveAngleFlag+haveStrokeFlag+!dist+!dist+5];
				short strokeFlag   = haveStrokeFlag ? (short)param[haveAngleFlag].numValue : 0,
					  grandParent0 = (short)(grandParent0Param->numValue/one6),
					  parent0      = (short)(parent0Param->numValue/one6),
					  child0       = (short)(child0Param->numValue/one6),
					  cvt0 = !dist ? (short)(cvt0Param->numValue/one6) : illegalCvtNum,
					  parent1      = (short)(parent1Param->numValue/one6),
					  child1       = (short)(child1Param->numValue/one6),
					  cvt1 = !dist ? (short)(cvt1Param->numValue/one6) : illegalCvtNum,
					  grandParent1 = (short)(grandParent1Param->numValue/one6);
				ProjFreeVector projFreeVector;

				if (this->MakeProjFreeVector(haveAngleFlag,param[0].numValue,y,grandParent0Param,parent0Param,4,&projFreeVector,errMsg)) { // 4 children (?)
					for (i = 0; i < this->generators; i++) {
						this->gen[i]->ResIPDLink(y,dist,&projFreeVector,strokeFlag,grandParent0,parent0,child0,cvt0,parent1,child1,cvt1,grandParent1,errMsg);
					}
				}
			}
			break; 
		case resXIPDDDist:
		case resYIPDDDist:
		case resXIPDDLink:
		case resYIPDDLink: 
			if (!this->legacyCompile)
			{
				bool y = cmd == resYIPDDDist || cmd == resYIPDDLink,
						dist = cmd == resXIPDDDist || cmd == resYIPDDDist,
						haveAngleFlag = param[0].type == angleFlag,
						haveStrokeFlag = false; // so far, TT fn not implemented orthogonally enough
				short cvt0ParamOffs = haveAngleFlag+haveStrokeFlag+3,
					  cvt1ParamOffs = haveAngleFlag+haveStrokeFlag+6;
				ActParam *grandParent0Param = &param[haveAngleFlag+haveStrokeFlag],
						 *parent0Param      = &param[haveAngleFlag+haveStrokeFlag+1],
						 *child0Param       = &param[haveAngleFlag+haveStrokeFlag+2],
						 *cvt0Param = !dist ? &param[haveAngleFlag+haveStrokeFlag+3] : NULL,
						 *parent1Param      = &param[haveAngleFlag+haveStrokeFlag+!dist+3],
						 *child1Param       = &param[haveAngleFlag+haveStrokeFlag+!dist+4],
						 *cvt1Param = !dist ? &param[haveAngleFlag+haveStrokeFlag+!dist+5] : NULL,
						 *grandParent1Param = &param[haveAngleFlag+haveStrokeFlag+!dist+!dist+5];
				short strokeFlag   = haveStrokeFlag ? (short)param[haveAngleFlag].numValue : 0,
					  grandParent0 = (short)(grandParent0Param->numValue/one6),
					  parent0      = (short)(parent0Param->numValue/one6),
					  child0       = (short)(child0Param->numValue/one6),
					  cvt0 = !dist ? (short)(cvt0Param->numValue/one6) : illegalCvtNum,
					  parent1      = (short)(parent1Param->numValue/one6),
					  child1       = (short)(child1Param->numValue/one6),
					  cvt1 = !dist ? (short)(cvt1Param->numValue/one6) : illegalCvtNum,
					  grandParent1 = (short)(grandParent1Param->numValue/one6);
				ProjFreeVector projFreeVector0,projFreeVector1;

				if (this->MakeProjFreeVector(haveAngleFlag,param[0].numValue,y,parent0Param,child0Param,1,&projFreeVector0,errMsg) && 
					this->MakeProjFreeVector(haveAngleFlag,param[0].numValue,y,parent1Param,child1Param,1,&projFreeVector1,errMsg)) {
					// pv not allowed to override, only fv, hence copy into single projFreeVector parameter
					projFreeVector0.fv[1] = projFreeVector1.fv[0];
					for (i = 0; i < this->generators; i++) {
						this->gen[i]->ResIPDDLink(y,dist,&projFreeVector0,strokeFlag,grandParent0,parent0,child0,cvt0,parent1,child1,cvt1,grandParent1,errMsg);
					}
				}
			}
			break;
		case resXDDist:
		case resYDDist:
		case resXDLink:
		case resYDLink: 
			if (!this->legacyCompile)		
			{
				bool y = cmd == resYDDist || cmd == resYDLink,
						dist = cmd == resXDDist || cmd == resYDDist;
				short cvtParamOffs0 = 2;
				bool haveCvt0 = !dist && params > cvtParamOffs0 && param[cvtParamOffs0].type == cvtN;
				short cvtParamOffs1 = 2 + haveCvt0 + 2;
				bool haveCvt1 = !dist && params > cvtParamOffs1 && param[cvtParamOffs1].type == cvtN,
						haveMinDist = params > cvtParamOffs1+haveCvt1 && param[cvtParamOffs1+haveCvt1].type == minDistGeneral;
				ActParam *parent0Param = &param[0],*child0Param = &param[1],*parent1Param = &param[2+haveCvt0],*child1Param = &param[3+haveCvt0];
				short parent0 = (short)(parent0Param->numValue/one6),
					  child0 = (short)(child0Param->numValue/one6),
					  cvt0 = haveCvt0 ? (short)(param[cvtParamOffs0].numValue/one6) : illegalCvtNum,
					  parent1 = (short)(parent1Param->numValue/one6),
					  child1 = (short)(child1Param->numValue/one6),
					  cvt1 = haveCvt1 ? (short)(param[cvtParamOffs1].numValue/one6) : cvt0;
				ProjFreeVector projFreeVector0,projFreeVector1;
			
				if (this->MakeProjFreeVector(false,0,y,parent0Param,child0Param,1,&projFreeVector0,errMsg) && 
					this->MakeProjFreeVector(false,0,y,parent1Param,child1Param,1,&projFreeVector1,errMsg)) {
					// pv not allowed to override, only fv, hence copy into single projFreeVector parameter
					projFreeVector0.fv[1] = projFreeVector1.fv[0];
					for (i = 0; i < this->generators; i++) {
						this->gen[i]->ResDDLink(y,dist,&projFreeVector0,parent0,child0,cvt0,parent1,child1,cvt1,errMsg);
					}
				}
			}
			break; 
		case resIIPDDist:
		case resIIPDLink: 
			if (!this->legacyCompile)
			{
				bool dist = cmd == resIIPDDist;
				short optParamOffs = cmd == resIIPDDist ? 6 : 8;

				short cvtParamOffs0 = 3;
				bool haveCvt0 = !dist && params > cvtParamOffs0 && param[cvtParamOffs0].type == cvtN;
				short cvtParamOffs1 = 3 + haveCvt0 + 2;
				bool haveCvt1 = !dist && params > cvtParamOffs1 && param[cvtParamOffs1].type == cvtN,
						haveMinDist = params > cvtParamOffs1+haveCvt1 && param[cvtParamOffs1+haveCvt1].type == minDistGeneral;
				ActParam *grandParent0Param = &param[0],*parent0Param = &param[1],*child0Param = &param[2],*parent1Param = &param[3+haveCvt0],*child1Param = &param[4+haveCvt0],*grandParent1Param = &param[5+haveCvt0+haveCvt1];
				short grandParent0 = (short)(grandParent0Param->numValue/one6),
					  parent0 = (short)(parent0Param->numValue/one6),
					  child0 = (short)(child0Param->numValue/one6),
					  cvt0 = haveCvt0 ? (short)(param[cvtParamOffs0].numValue/one6) : illegalCvtNum,
					  parent1 = (short)(parent1Param->numValue/one6),
					  child1 = (short)(child1Param->numValue/one6),
					  grandParent1 = (short)(grandParent1Param->numValue/one6),
					  cvt1 = haveCvt1 ? (short)(param[cvtParamOffs1].numValue/one6) : cvt0;
				ProjFreeVector projFreeVector0,projFreeVector1;
			
				if (this->MakeProjFreeVector(true,0,false,parent0Param,child0Param,1,&projFreeVector0,errMsg) &&
					this->MakeProjFreeVector(true,0,false,parent1Param,child1Param,1,&projFreeVector1,errMsg)) {
					// pv not allowed to override, only fv, hence copy into single projFreeVector parameter
					projFreeVector0.fv[1] = projFreeVector1.fv[0];
					for (i = 0; i < this->generators; i++) {
						this->gen[i]->ResIIPDLink(dist,&projFreeVector0,grandParent0,parent0,child0,cvt0,parent1,child1,cvt1,grandParent1,errMsg);
					}
				}
			}
			break; 
		case compilerLogic:
		case cvtAllocation:
		case cvtLogic:
		case diagEndCtrl:
		case diagSerifs:
		case processXSymmetrically:
		case processYSymmetrically:
		case tail:
		case tweakMetrics:
		default:
			swprintf(errMsg,L"Sorry, this command is no longer supported");
			break;
	}
} /* TMTSourceParser::Dispatch */

void TMTSourceParser::XFormToNewSyntax(void) {
/* this is a bit of a botched job, but I'd rather have the standard parameter checking mechanism do all the serious work */
	long savePos,flagPos,parmPos;
	wchar_t old[32],neu[64];
	short s,d,l;
	
	savePos = this->pos;
	
	while (this->ch && this->ch != L'(' && this->ch != L'[') this->GetCh(); // scan for start of parameter list
	flagPos = this->pos-chLookAhead;
	this->GetCh();
	while (this->ch && this->ch != L')' && this->ch != L']' && this->ch != L'"') { // find opening quote
		if (this->ch == L',') parmPos = this->pos-chLookAhead;
		this->GetCh();
	}
	l = 0;
	if (this->ch == L'"') {
		this->GetCh();
		while (this->ch && this->ch != L'"' && l < 31) { old[l++] = this->ch; this->GetCh(); } // find closing quote
		old[l] = '\0'; this->GetCh();
	}
	
	if (l > 0) {
		this->talkText->Delete(parmPos,this->pos-chLookAhead);
		
		d = s = 0;
		while (s < l) {
			if (old[s] == L'/') { // italic angle
				neu[d++] = old[s]; 
				old[s++] = L' ';
			} else if (old[s] == 0xAF) { // adjusted italic angle
				neu[d++] = L'/';
				neu[d++] = L'/';
				old[s++] = L' ';
			} else if (old[s] == 0xA8) { // post round
				neu[d++] = L'$';
				old[s++] = L' ';
			} else {
				s++;
			}
		}
		if (d > 0) {
			neu[d] = '\0';
			this->talkText->Insert(flagPos,neu);
			parmPos += d;
		}
		
		d = s = 0;
		while (s < l-2) {
			if ((old[s] == L'c' || old[s] == L'C') && (old[s+1] == L'v' || old[s+1] == L'V') && (old[s+2] == L't' || old[s+2] == L'T')) { // "cvt123"
				neu[d++] = L',';
				old[s++] = L' '; old[s++] = L' '; old[s++] = L' ';
				while (s < l && ((L'0' <= old[s] && old[s] <= L'9') || old[s] == L'.' || old[s] == L' ')) {
					if (old[s] != L' ') neu[d++] = old[s];
					old[s++] = L' ';
				}
			} else {
				s++;
			}
		}
		s = 0;
		while (s < l) {
			if (old[s] == L'<' || old[s] == 0xB3) { // "<" or "≥12" or "≥(12,@2,24)"
				neu[d++] = L',';
				if (old[s] == L'<')
					neu[d++] = old[s];
				else {
					neu[d++] = L'>';
					neu[d++] = L'=';
				}
				old[s++] = L' ';
				if (s < l && old[s] == L'(') { // "≥(12,@2,24)"
					neu[d++] = old[s]; old[s++] = L' '; // '('
					while (s < l-1 && old[s] != L',') { neu[d++] = old[s]; old[s++] = L' '; }
					neu[d++] = old[s]; old[s++] = L' '; // ','
					neu[d++] = L'@';
					while (s < l-1 && old[s] != L')') { neu[d++] = old[s]; old[s++] = L' '; }
					neu[d++] = old[s]; old[s++] = L' '; // ')'
				} else { // "≥12"
					while (s < l && ((L'0' <= old[s] && old[s] <= L'9') || old[s] == L'.')) { neu[d++] = old[s]; old[s++] = L' '; }
				}
			} else {
				s++;
			}
		}
		if (d > 0) {
			neu[d] = '\0';
			this->talkText->Insert(parmPos,neu);
			parmPos += d;
		}
		
		d = s = 0;
		neu[d++] = L',';
		neu[d++] = L'"';
		while (s < l) {
			if (old[s] != L' ') neu[d++] = old[s];
			s++;
		}
		neu[d++] = L'"';
		if (d > 3) {
			neu[d] = '\0';
			this->talkText->Insert(parmPos,neu);
			parmPos += d;
		}
		this->changedSrc = true;
	}
	
	this->pos = savePos-chLookAhead;
	this->GetCh(); this->GetCh();
} /* TMTSourceParser::XFormToNewSyntax */

/*****/
void TMTSourceParser::Flag(ActParam *actParam) {
	long paramStart;
	
	paramStart = this->prevPos;
	switch (this->sym) {
		case leftDir:
		case rightDir:
			actParam->type = dirFlag; actParam->numValue = (long)this->sym - (long)leftDir;
			this->GetSym();
			break;
		case italAngle:
		case adjItalAngle:
			actParam->type = angleFlag; actParam->numValue = (long)this->sym - (long)italAngle;
			this->GetSym();
			break;
		case optStroke:
		case optStrokeLeftBias:
		case optStrokeRightBias:
			if (!this->legacyCompile)
			{
				actParam->type = strokeFlag; actParam->numValue = (long)this->sym - (long) optStroke + 1;
				this->GetSym();
			}
			break;
		case postRound:
			actParam->type = postRoundFlag;
			this->GetSym();
			break;
		default:
			break;
	}
	this->prevPrevPos = paramStart;
} // TMTSourceParser::Flag

/*****
void TMTSourceParser::Flag(ActParam *actParam) {
	long paramStart;
#ifdef VTT_PRO_SP_YAA_AUTO
	bool doubleSlash;
	ActParam fvPoint0,fvPoint1;
	short subParams;
#endif
	
	paramStart = this->prevPos;
	switch (this->sym) {
		case leftDir:
		case rightDir:
			actParam->type = dirFlag; actParam->numValue = (long)this->sym - (long)leftDir;
			this->GetSym();
			break;
		case italAngle:
		case adjItalAngle:
#ifdef VTT_PRO_SP_YAA_AUTO
			doubleSlash = this->sym == adjItalAngle;
#endif
			actParam->type = angleFlag;
			actParam->numValue = (long)this->sym - (long)italAngle;
			this->GetSym();
#ifdef VTT_PRO_SP_YAA_AUTO
			if (InitParam(this)) {
				if (!doubleSlash) this->ErrorMsg(syntactical,L"Cannot use / to delimit freedom vector direction (use // instead)");
				subParams = 0;
				this->Parameter(&fvPoint0); this->MatchParameter(fvPointsParam,&subParams,&fvPoint0.type); this->ValidateParameter(&fvPoint0);
				if (Separator(this) || InitParam(this)) {
					if (this->sym != comma) this->ErrorMsg(syntactical,L", expected");
					if (Separator(this)) this->GetSym();
					this->Parameter(&fvPoint1); this->MatchParameter(fvPointsParam,&subParams,&fvPoint1.type); this->ValidateParameter(&fvPoint1);
				} else {
					fvPoint1 = fvPoint0;
				}
				if (this->sym == adjItalAngle) this->GetSym(); else this->ErrorMsg(syntactical,L"// expected");
				actParam->fvPoint0 = (short)(fvPoint0.numValue/one6);
				actParam->fvPoint1 = (short)(fvPoint1.numValue/one6);
			} else {
				actParam->fvPoint0 = actParam->fvPoint1 = illegalKnotNum;
			}
#endif
			break;
		case postRound:
			actParam->type = postRoundFlag;
			this->GetSym();
			break;
	}
	this->prevPrevPos = paramStart;
} // TMTSourceParser::Flag
*****/

void TMTSourceParser::Parameter(ActParam *actParam) {
	long paramStart,localParamStart;
	short subParams;
	ActParam colorParam;
	Symbol ttvSym;
	ParamType paramType;
	long numValue,firstLocalParamStart;
	bool gotKnot[2];
	
	paramStart = this->prevPos;
	if (leftParen <= this->sym && this->sym <= rational) {
		this->Expression(actParam);
		this->prevPrevPos = paramStart; // can't recursively call this->Parameter() here...
		actParam->hasTtvOverride = false;
		actParam->ttvOverride.dir  = xRomanDir; // say
		actParam->ttvOverride.from = illegalKnotNum;
		actParam->ttvOverride.to   = illegalKnotNum;

		if (this->sym == aT) {
			subParams = 0;
			if (actParam->type == anyN) actParam->type = rationalN; // for now
			this->MatchParameter(rangeOfPpemNParam,&subParams,&actParam->type);
			this->ValidateParameter(actParam);
			this->GetSym();
			this->PpemRange(actParam);
			actParam->deltaColor = alwaysDelta;
			if (this->sym == percent) { // optional delta color sub-parameter
				this->GetSym();
				this->Parameter(&colorParam);
				if (colorParam.type == anyN) colorParam.type = colorN;
				subParams = 0;
				this->MatchParameter(deltaColorParam,&subParams,&colorParam.type);
				this->ValidateParameter(&colorParam);
				actParam->deltaColor = DeltaColorOfByte((unsigned char)(colorParam.numValue/one6));
				actParam->type = rangeOfPpemNcolorOpt; // by now
			}
		} else if (!this->legacyCompile && (this->sym == colon || this->sym == rightDir || this->sym == upDir)) {
			// the following are all legal:
			//
			// knot                   no ttv override
			// knot >                 ttv in x-direction
			// knot ^                 ttv in y-direction
			// knot > knot1           ttv on line from knot to knot1
			// knot ^ knot1           ttv perpendicular to line from knot to knot1
			// knot : knot0 > knot1   ttv on line from knot0 to knot1
			// knot : knot0 ^ knot1   ttv perpendicular to line from knot to knot1
			//
			// (no italic or adjusted italic angle yet,
			//  this would probably complement the ^ and > by / or // ...
			//  or use \ and \\ for perpendicular to the italic angle and don't require ^ and > ???)
			//
			// at this point we have parsed knot, now we're about to parse the rest

			actParam->hasTtvOverride = true;

			subParams = 0;
			paramType = actParam->type;
			if (paramType == anyN) actParam->type = knotN; // for now
			this->MatchParameter(knotNttvOptParams,&subParams,&actParam->type); this->ValidateParameter(actParam);
			numValue = actParam->numValue;
			
			gotKnot[0] = false;
			
			if (this->sym == colon) {
				this->GetSym();

				firstLocalParamStart = localParamStart = this->prevPos;
				this->Expression(actParam);
				this->prevPrevPos = localParamStart;
				this->MatchParameter(knotNttvOptParams,&subParams,&actParam->type); this->ValidateParameter(actParam);
				if (actParam->type == knotN) actParam->ttvOverride.from = (short)(actParam->numValue/one6);
				
				gotKnot[0] = true;
			}

			if (this->sym != rightDir && this->sym != upDir)
				this->ErrorMsg(syntactical,L"> or ^ expected");
			else {
				ttvSym = this->sym;
				this->GetSym();
			}

			gotKnot[1] = false;
			
			if (this->sym != rightParen && !Separator(this)) {

				localParamStart = this->prevPos;
				this->Expression(actParam);
				this->prevPrevPos = localParamStart;
				this->MatchParameter(knotNttvOptParams,&subParams,&actParam->type); this->ValidateParameter(actParam);
				if (actParam->type == knotN) actParam->ttvOverride.to = (short)(actParam->numValue/one6);
				
				gotKnot[1] = true;
			}

			if (gotKnot[0] && !gotKnot[1]) {
				this->prevPrevPos = firstLocalParamStart;
				this->ErrorMsg(contextual,L"illegal freedom or projection vector (second knot expected)");
			} else if (gotKnot[0] && gotKnot[1] && actParam->ttvOverride.from == actParam->ttvOverride.to) {
				this->prevPrevPos = firstLocalParamStart;
				this->ErrorMsg(contextual,L"illegal freedom or projection vector (knots must differ)");
			} 
			
			// no italic or adjusted italic angle yet			
			actParam->ttvOverride.dir = gotKnot[1] ? (ttvSym == upDir ? perpDiagDir : diagDir) : (ttvSym == upDir ? yRomanDir : xRomanDir);
			
			if (paramType == anyN) actParam->type = gotKnot[0] || gotKnot[1] ? knotNttvOpt : knotNttvOptXY; // reset
			actParam->numValue = numValue;
		}
	} else if (this->sym == aT) {
		this->GetSym();
		localParamStart = this->prevPos;
		this->Expression(actParam);
		this->prevPrevPos = localParamStart; // can't recursively call this->Parameter() here...
		if (actParam->type != anyN) {
			this->ErrorMsg(contextual,L"ppem size expected (can be an integer only)");
			actParam->numValue = one6;
		}
		actParam->type = ppemSize;
	} else if (this->sym == literal) {
		actParam->type = anyS; actParam->litValue = this->litValue;
		this->GetSym();
	} else if (this->sym == atLeast || this->sym == leftDir) {
		this->MinDist(actParam);
	} else {
		this->ErrorMsg(syntactical,L"parameter starts with illegal symbol (+, -, @, <, >=, number, or \x22string\x22 expected)"); actParam->type = voidParam; actParam->numValue = 0;
	}
	this->prevPrevPos = paramStart;
} /* TMTSourceParser::Parameter */

bool Match(ParamType formParamType, ParamType actParamType);
bool Match(ParamType formParamType, ParamType actParamType) {
	return (actParamType == formParamType ||
			(actParamType == anyN && anyN <= formParamType && formParamType <= posRationalN) ||
		//	actParamType == knotNttvOpt && knotNttvOpt <= formParamType && formParamType <= k
			(knotNttvOpt <= actParamType && actParamType <= knotNttvOptXY && knotNttvOpt <= formParamType && formParamType <= knotNttvOptXY) ||
			(actParamType == rangeOfPpemN && formParamType == rangeOfPpemNcolorOpt) ||
			(actParamType == posRationalN && rationalN <= formParamType && formParamType <= posRationalN) ||
			(actParamType == minDistFlagOnly && formParamType == minDistGeneral));
} /* Match */

void TMTSourceParser::MatchParameter(FormParam *formParams, short *formParamNum, ParamType *actParamType) {
	short tentative;
	wchar_t errMsg[maxLineSize];
	ParamType expected;
	
	tentative = *formParamNum;
	while (tentative < maxFPs && !Match(formParams[tentative].type,*actParamType) && opt <= formParams[tentative].pres && formParams[tentative].pres <= optR) tentative++;
	if (*formParamNum < tentative && tentative < maxFPs && Match(formParams[tentative].type,*actParamType)) *formParamNum = tentative; // skip optional parameters
	if (*formParamNum < maxFPs && formParams[*formParamNum].type != voidParam) {
		if (Match(formParams[*formParamNum].type,*actParamType))
			*actParamType = formParams[*formParamNum].type;
		else {
			expected = *formParamNum < maxFPs ? formParams[*formParamNum].type : voidParam;
			switch (expected) {
				case anyN:			
				case knotN:
				case knotNttvOpt:
				case knotNttvOptXY:
				case cvtN:			
				case compLogicN:	
				case cvtLogicN:		
				case phaseN:		
				case angle100N:	
				case colorN:
				case serifN:		
				case curveN:		
				case radiusN:       swprintf(errMsg,L"integer number expected (example: 1)"); break;
				case rationalN:		swprintf(errMsg,L"rational number expected (example: 1/8 or -1.5)"); break;
				case posRationalN:	swprintf(errMsg,L"positive rational number expected (example: 1/8 or 1.5)"); break;
				case ppemSize:		swprintf(errMsg,L"ppem size expected (example: @12)"); break;
				case ppemN:			swprintf(errMsg,L"ppem number expected (example: 12)"); break;
				case rangeOfPpemNcolorOpt:
				case rangeOfPpemN:	  swprintf(errMsg,L"ppem range expected (example: @8..13;21)"); break;
				case anyS:			  swprintf(errMsg,L"quoted string expected (example: \x22V1.11\x22 or %cCALL[], 9\x22)",'\x22'); break; // %c or else compiler won't accept "escape sequence"...
				case minDistFlagOnly: swprintf(errMsg,L"minimum distance flag expected (example: < or >= only)"); break;
				case minDistGeneral:  swprintf(errMsg,L"minimum distance expected (example: < or >= or >=1.5 or >=(1.5,@12,2.5) )"); break;
				case dirFlag:		  swprintf(errMsg,L"direction flag expected (example: either < or >)"); break;
				case angleFlag:		  swprintf(errMsg,L"angle flag expected (example: either / or //)"); break;
				case postRoundFlag:	  swprintf(errMsg,L"post round flag expected (example: $)"); break;
				default:			  swprintf(errMsg,L"actual parameter does not match"); break;
			}
			this->ErrorMsg(contextual,errMsg);
		}
		if (mand <= formParams[*formParamNum].pres && formParams[*formParamNum].pres <= opt) (*formParamNum)++; // use next parameter next time;
	} else {
		this->ErrorMsg(contextual,L"too many parameters");
	}	
} /* TMTSourceParser::MatchParameter */

void TMTSourceParser::ValidateParameter(ActParam *actParam) {
	/* test if actParam->type is in range of phaseN or angle100N or whatever it is by now */
	wchar_t errMsg[maxLineSize];
	
	switch (actParam->type) { // which by now is the formal parameter type
		case voidParam:
			break;
		case anyN:
			break;
		case knotNttvOpt:
		case knotN: {
			long knot = actParam->numValue/one6;
			
			if (knot < 0 || knot >= this->knots) {
				swprintf(errMsg,L"illegal knot number (can be in range 0 through %hi only)",this->knots-1); this->ErrorMsg(contextual,errMsg);
				actParam->numValue = 0;
			}
		//	for knotNttvOpt, ttvKnot[0], and ttvKnot[1] already validated against being in range
		//	what's left to do is to verify (if we can) whether this yields an almost perpendicular case.
		//	probably can't do at this level, because we don't know the value of the projection vector.
		//	would need to know tt->PVDir() but this may not be valid if compiling into graphical representation
			break;
		}
		case cvtN: {
			long cvt = actParam->numValue/one6;
			
			if (!this->font->TheCvt()->CvtNumExists(cvt)) {
				this->ErrorMsg(contextual,L"illegal cvt number (must be defined in the control value table)");
				actParam->numValue = 0;
			}
			break;
		}
		case compLogicN:
			break;
		case cvtLogicN:
			break;
		case phaseN:
			if (actParam->numValue < 0 || actParam->numValue >= phases*one6) {
				swprintf(errMsg,L"illegal phase type (can be in range 0 through %li only)",phases-1); this->ErrorMsg(contextual,errMsg);
				actParam->numValue = 0;
			}
			break;
		case angle100N:
			if (actParam->numValue < 0 || actParam->numValue > maxAngle*one6) {
				swprintf(errMsg,L"illegal angle x100 (can be in range 0 through %li only)",maxAngle); this->ErrorMsg(contextual,errMsg);
				actParam->numValue = 0;
			}
			break;
		case colorN:
			if (DeltaColorOfByte((unsigned char)(actParam->numValue/one6)) == illegalDelta) {
				swprintf(errMsg,L"illegal delta color flag (can be %hs only)",AllDeltaColorBytes()); this->ErrorMsg(contextual,errMsg);
				actParam->numValue = 0;
			}
			break;
		case serifN:
			if (actParam->numValue < 0 || actParam->numValue >= serifs*one6) {
				swprintf(errMsg,L"illegal serif type (can be in range 0 through %li only)",serifs-1); this->ErrorMsg(contextual,errMsg);
				actParam->numValue = 0;
			}
			break;
		case curveN:
			break;
		case radiusN:
			break;
		case rationalN:
		case posRationalN:
			if ((actParam->type == posRationalN && actParam->numValue < 0) || actParam->numValue < -maxPixelValue || actParam->numValue > maxPixelValue) {
				swprintf(errMsg,L"illegal pixel size (can be in range %li through %li only)",actParam->type == posRationalN ? 0 : -maxPixelValue/one6,maxPixelValue/one6); this->ErrorMsg(contextual,errMsg);
				actParam->numValue = one6;
			}
			if (actParam->numValue == 0) {
				this->ErrorMsg(contextual,L"pixel size cannot be 0");
				actParam->numValue = one6;
			}
			break;
		case ppemSize:
		case ppemN:
			if (actParam->numValue < one6 || actParam->numValue >= maxPpemSize*one6) {
				swprintf(errMsg,L"illegal ppem number (can be in range 1 through %li only)",maxPpemSize-1); this->ErrorMsg(contextual,errMsg);
				actParam->numValue = one6;
			}
			break;
		case rangeOfPpemN:
			/* this->numValue already validated in PpemRange to be able to report errors at correct source position, likewise this->deltaPpemSize */
			break;
		case anyS:
			break;
		case minDistGeneral:
			/* this->jumpPpemSize[maxMinDist] and this->pixelSize[maxMinDist] already validated in MinDist for same reason as case deltaRangeN: */
			break;
		case dirFlag:
			break;
		case angleFlag:
			break;
		case postRoundFlag:
			break;
		default:
			break;
	}
} /* TMTSourceParser::ValidateParameter */

void TMTSourceParser::Expression(ActParam *actParam) {
	Symbol sign,op;
	ActParam actParam2;
	wchar_t errMsg[maxLineSize];
	
	sign = plus;
	if (this->sym == plus || this->sym == minus) {
		sign = this->sym; this->GetSym();
	}
	this->Term(actParam);
	if (sign == minus) actParam->numValue = -actParam->numValue;
	while (this->sym == plus || this->sym == minus) {
		op = this->sym; this->GetSym();
		this->Term(&actParam2);
		if (op == plus) actParam->numValue += actParam2.numValue; else actParam->numValue -= actParam2.numValue; // assuming we have not more than 32 - 17 - 1 binary places
		if (Abs(actParam->numValue) >= (shortMax+1)*one6) {
			if (op == plus)
				swprintf(errMsg,L"result of addition too large (cannot be %li or above)",shortMax+1);
			else
				swprintf(errMsg,L"result of subtraction too large (cannot be -%li or below)",shortMax+1);
			this->ErrorMsg(contextual,errMsg);
		}
		actParam->type = Max(actParam->type,actParam2.type);
	}
	if (actParam->type == rationalN && actParam->numValue >= 0) actParam->type = posRationalN;
} /* TMTSourceParser::Expression */

void TMTSourceParser::Term(ActParam *actParam) {
	Symbol op;
	ActParam actParam2;
	wchar_t errMsg[maxLineSize];
	
	this->Factor(actParam);
	while (this->sym == timeS || this->sym == italAngle) {
		op = this->sym; this->GetSym();
		this->Factor(&actParam2);
		if (op == timeS) {
			if ((double)Abs(actParam->numValue)*(double)Abs(actParam2.numValue) < (double)((shortMax+1)*one6*one6))
				actParam->numValue = (actParam->numValue*actParam2.numValue + half6)/one6;
			else {
				swprintf(errMsg,L"result of multiplication too large (cannot be %li or larger in magnitude)",shortMax+1); this->ErrorMsg(contextual,errMsg);
			}
		} else { // op == italAngle, i.e. divide
			if (actParam2.numValue != 0 && (double)Abs(actParam->numValue) < (double)(shortMax+1)*(double)Abs(actParam2.numValue)) {
				if (actParam->type == anyN && actParam2.type == anyN && actParam->numValue%actParam2.numValue != 0) actParam->type = rationalN;
				actParam->numValue = (2*actParam->numValue*one6 + actParam2.numValue)/(2*actParam2.numValue);
			} else {
				swprintf(errMsg,L"result of division too large (cannot be %li or larger in magnitude)",shortMax+1); this->ErrorMsg(contextual,errMsg);
			}
		}
		actParam->type = Max(actParam->type,actParam2.type);
	}
} /* TMTSourceParser::Term */

void TMTSourceParser::Factor (ActParam *actParam) {
	if (this->sym == natural || this->sym == rational) {
		actParam->type = this->sym == natural ? anyN : rationalN;
		actParam->numValue = this->numValue; this->GetSym();
	} else if (this->sym == leftParen) {
		this->GetSym();
		this->Expression(actParam);
		if (this->sym == rightParen) this->GetSym(); else this->ErrorMsg(syntactical,L") expected");
	} else {
		this->ErrorMsg(syntactical,L"factor starts with illegal symbol (number or ( expected)");
		actParam->type = voidParam;
	}
} /* TMTSourceParser::Factor */

void TMTSourceParser::MinDist(ActParam *actParam) {
	Symbol op;
	bool haveLeftParen,gotMinDist;
	short subParams;
	ActParam ppem,pixel;
	wchar_t errMsg[maxLineSize];
	
	op = this->sym;
	actParam->type = minDistFlagOnly;
	actParam->minDists = 0;
	this->GetSym();
	if (op == atLeast) {
		haveLeftParen = false; 
		if (this->sym == leftParen) { haveLeftParen = true; this->GetSym(); }
		gotMinDist = InitParam(this);
		subParams = 0;
		if (gotMinDist) { this->Parameter(&pixel); this->MatchParameter(minDistParam,&subParams,&pixel.type); this->ValidateParameter(&pixel); } 
		else { pixel.type = anyN; pixel.numValue = one6; }
		actParam->jumpPpemSize[actParam->minDists] = one6; // say...
		actParam->pixelSize[actParam->minDists] = pixel.numValue;
		actParam->minDists++;
		if (haveLeftParen && gotMinDist) {
			while (Separator(this) || InitParam(this)) {
				if (this->sym != comma) this->ErrorMsg(syntactical,L", expected");
				if (Separator(this)) this->GetSym();
				this->Parameter(&ppem); this->MatchParameter(minDistParam,&subParams,&ppem.type); this->ValidateParameter(&ppem);
				if (ppem.numValue <= actParam->jumpPpemSize[actParam->minDists-1]) {
					this->ErrorMsg(contextual,L"this ppem size should be larger than the previous one");
					ppem.numValue = actParam->jumpPpemSize[actParam->minDists-1] + one6;
				}
				if (Separator(this) || InitParam(this)) {
					if (this->sym != comma) this->ErrorMsg(syntactical,L", expected");
					if (Separator(this)) this->GetSym();
					this->Parameter(&pixel); this->MatchParameter(minDistParam,&subParams,&pixel.type); this->ValidateParameter(&pixel);
					if (pixel.numValue <= actParam->pixelSize[actParam->minDists-1]) {
						this->ErrorMsg(contextual,L"this pixel size should be larger than the previous one");
						pixel.numValue = actParam->pixelSize[actParam->minDists-1] + one6;
					}
				} else {
					this->ErrorMsg(syntactical,L", (followed by another pixel size) expected");
				}
				if (actParam->minDists < maxMinDist) {
					actParam->jumpPpemSize[actParam->minDists] = ppem.numValue;
					actParam->pixelSize[actParam->minDists] = pixel.numValue;
					actParam->minDists++;
				} else {
					swprintf(errMsg,L"too many minimum distances (cannot have more than %li)",maxMinDist); this->ErrorMsg(contextual,errMsg);
				}
			}
		}
		if (haveLeftParen) {
			if (this->sym == rightParen) this->GetSym(); else this->ErrorMsg(syntactical,L") expected");
		}
		if (gotMinDist || actParam->minDists > 1 || actParam->pixelSize[0] != one6 || actParam->jumpPpemSize[0] != one6) {
			actParam->type = minDistGeneral;
		}
	}
} /* TMTSourceParser::MinDist */

void TMTSourceParser::Range(ActParam *actParam) {
	ActParam lowParam,highParam;
	short subParams,low,high,i;
	wchar_t errMsg[maxLineSize];

	subParams = 1; // skip pixel number
	this->Parameter(&lowParam);
	if (lowParam.type == anyN) lowParam.type = ppemN;
	this->MatchParameter(rangeOfPpemNParam,&subParams,&lowParam.type);
	this->ValidateParameter(&lowParam);
	low = high = (short)(lowParam.numValue/one6);
	if (this->sym == ellipsis) {
		this->GetSym();
		this->Parameter(&highParam);
		if (highParam.type == anyN) highParam.type = ppemN;
		this->MatchParameter(rangeOfPpemNParam,&subParams,&highParam.type);
		this->ValidateParameter(&highParam);
		high = (short)(highParam.numValue/one6);
		if (low > high) { this->ErrorMsg(contextual,L"low end of ppem range cannot be above high end"); high = low; }
	}
	for (i = low; i <= high; i++) {
		if (!actParam->deltaPpemSize[i])
			actParam->deltaPpemSize[i] = true;
		else {
			swprintf(errMsg,L"ppem size %hi occurs more than once",i);
			this->ErrorMsg(contextual,errMsg);
		}
	}
} /* TMTSourceParser::Range */

void TMTSourceParser::PpemRange(ActParam *actParam) {
	/* e.g. XDelta(13, 3/8 @ 12..18; 20; 24, 2/8 @ 19; 21..23) */
	short i;

	actParam->type = rangeOfPpemN; // by now
	for (i = 0; i < maxPpemSize; i++) actParam->deltaPpemSize[i] = false;
	this->Range(actParam);
	while (this->sym == semiColon || InitParam(this)) {
		if (this->sym == semiColon) this->GetSym(); else this->ErrorMsg(syntactical,L"; expected");
		this->Range(actParam);
	}
} /* TMTSourceParser::PpemRange */

void TMTSourceParser::GetCh(void) {
	this->ch = this->ch2;
	this->ch2 = this->talkText->GetCh(this->pos);
	this->pos++;
} /* TMTSourceParser::GetCh */

void TMTSourceParser::SkipComment(void) {
	long startPos;
	
	startPos = this->pos-chLookAhead;
	this->GetCh(); this->GetCh();
	while (this->ch && !TermComment(this)) {
		if (InitComment(this)) this->SkipComment(); else this->GetCh();
	}
	if (this->ch) {
		this->GetCh(); this->GetCh();
	} else {
		this->prevPos = startPos;
		this->ErrorMsg(special,L"comment opened but not closed");
	}
} /* TMTSourceParser::SkipComment */

void TMTSourceParser::SkipWhiteSpace(bool includingComments) {
	while (WhiteSpace(this) || (includingComments && InitComment(this))) {
		if (WhiteSpace(this)) this->GetCh();
		if (includingComments && InitComment(this)) this->SkipComment();
	}
} /* TMTSourceParser::Skip */

void TMTSourceParser::GetNumber(void) {
	bool overflow;
	long digit,decPlcs,pwrOf10;
	wchar_t errMsg[maxLineSize];
	
	this->sym = natural;
	overflow = false;
	this->numValue = 0;
	while (Numeric(this->ch) || Alpha(this->ch) || this->ch == L'_') {
		if (Numeric(this->ch)) {
			digit = (long)this->ch - (long)'0';
			if (this->numValue <= (shortMax - digit)/10)
				this->numValue = 10*this->numValue + digit;
			else
				overflow = true;
		} else
			this->ErrorMsg(lexical,L"illegal character in number (can be digits 0 through 9 only)");
		this->GetCh();
	}
	this->numValue *= one6;
	if (this->ch == L'.' && this->ch2 != L'.') { // permit ppem ranges such as 12..18
		this->GetCh();
		this->sym = rational;
		decPlcs = 0; pwrOf10 = 1;
		while (Numeric(this->ch) || Alpha(this->ch) || this->ch == L'_') {
			if (Numeric(this->ch)) {
				digit = (long)this->ch - (long)'0';
				if (decPlcs <= (1000000L - digit)/10) { // 1/64 = 0.015625
					decPlcs = 10*decPlcs + digit; pwrOf10 *= 10L;
				} else
					overflow = true;
			} else
				this->ErrorMsg(lexical,L"illegal character in number (can be digits 0 through 9 only)");
			this->GetCh();
		}
		this->numValue += (decPlcs*one6 + pwrOf10/2)/pwrOf10;
	}
	if (overflow) {
		swprintf(errMsg,L"number too large (cannot be %li or larger in magnitude)",shortMax+1); this->ErrorMsg(syntactical,errMsg);
	}
} /* TMTSourceParser::GetNumber */

Symbol Search(wchar_t *entry, short left, short right, short *matching);
Symbol Search(wchar_t *entry, short left, short right, short *matching) {
	short mid,diff,minMatch;
	wchar_t *id,*en;
	
	while (left <= right) {
		mid = (left + right)/2;
		id = tmtCmd[mid].name; minMatch = tmtCmd[mid].minMatch; *matching = 0; en = entry;
		while (*matching < minMatch && !(diff = Cap(*id) - Cap(*en))) {(*matching)++; id++; en++;}
		if (diff > 0) right = mid - 1;
		else if (diff < 0) left = mid + 1;
		else return (Symbol)mid; // found
	}
	return illegal; // not found
} /* Search */

void TMTSourceParser::GetIdent(void) {
	wchar_t id[idLen],*replId;
	short i,matching,origLen,replLen;
	bool textReplace;
	
	i = origLen = 0;
	while (Alpha(this->ch) || Numeric(this->ch) || this->ch == L'_') {
		if (this->ch != L'_' && i < idLen-1) id[i++] = this->ch;
		origLen++; this->GetCh();
	}
	id[i] = '\0';
	this->sym = Search(id,0,idents-1,&matching);
	if (this->sym != illegal) {
		replId = tmtCmd[this->sym].name;
		replLen = (short)STRLENW(replId);
		for (matching = 0; matching < replLen && id[matching] == replId[matching]; matching++);
		textReplace = matching < replLen || i > replLen; // original contains extra characters
		if (this->sym == xInterpolate0 || this->sym == xInterpolate1) { this->sym = xInterpolate; replLen = (short)STRLENW(tmtCmd[this->sym].name); textReplace = true; } else
		if (this->sym == yInterpolate0 || this->sym == yInterpolate1) { this->sym = yInterpolate; replLen = (short)STRLENW(tmtCmd[this->sym].name); textReplace = true; }
		if (textReplace) this->ReplAtCurrPos(origLen,tmtCmd[this->sym].name);
	}
} /* TMTSourceParser::GetIdent */

void TMTSourceParser::GetLiteral(void) {
	bool overflow;
	short i;
	wchar_t errMsg[maxLineSize];
	
	this->sym = literal;
	this->GetCh();
	overflow = false;
	i = 0;
	while (this->ch && this->ch != L'"') {
		if (i < maxAsmSize-1)
			this->litValue[i++] = this->ch;
		else
			overflow = true;
		this->GetCh();
	}
	this->litValue[i++] = '\0';
	
	if (!this->ch) this->ErrorMsg(special,L"string quoted but not unquoted");
	this->GetCh();
	if (overflow) {
		swprintf(errMsg,L"string too long (cannot be longer than %li characters)",maxAsmSize-1); this->ErrorMsg(syntactical,errMsg);
	}
}

void TMTSourceParser::GetSym(void) {
	this->SkipWhiteSpace(true);
	this->prevPrevPos = this->prevPos; this->prevPos = this->pos-chLookAhead;
	if (Numeric(this->ch)) {
		this->GetNumber();
	} else if (Alpha(this->ch)) {
		this->GetIdent();
	} else {
		switch (this->ch) {
			case L'(':	this->sym = leftParen;	this->GetCh(); break;
			case L'[':	this->sym = leftParen;	this->GetCh(); this->ReplAtCurrPos(1,L"("); break;
			case L')':	this->sym = rightParen;	this->GetCh(); break;
			case L']':	this->sym = rightParen; this->GetCh(); this->ReplAtCurrPos(1,L")"); break;

			case 0xB3:	this->sym = atLeast;	this->GetCh(); this->ReplAtCurrPos(1,L">="); break; // replace Mac special char
			case L'+':	this->sym = plus;		this->GetCh(); break;
			case L'-':	this->sym = minus;		this->GetCh(); break;
			case L'*':	this->sym = timeS;		this->GetCh(); break;
			case L':':	this->sym = colon;		this->GetCh(); break;
			case L'%':	this->sym = percent;    this->GetCh(); break;
			case L',':	this->sym = comma;		this->GetCh(); break;
			case L';':	this->sym = semiColon;	this->GetCh(); break;
			case L'@':	this->sym = aT;			this->GetCh(); break;
			case L'<':
				this->GetCh();
				if (this->ch == L'|') {
					this->sym = optStrokeLeftBias;
					this->GetCh();
					this->ReplAtCurrPos(2,L"|<");
				} else {
					this->sym = leftDir;
				}
				break;
			case L'^':	this->sym = upDir;		this->GetCh(); break;
			case L'>':
				this->GetCh();
				if (this->ch == L'=') {
					this->sym = atLeast;
					this->GetCh(); 
				} else if (this->ch == L'|') {
					this->sym = optStrokeRightBias;
					this->GetCh(); 
				} else {
					this->sym = rightDir;
				}
				break;
			case L'/':
				this->GetCh();
				if (this->ch == L'/') {
					this->sym = adjItalAngle;
					this->GetCh();
				} else {
					this->sym = italAngle;
				}
				break;
			case L'|':
				this->GetCh();
				if (this->ch == L'|') {
					this->sym = optStroke;
					this->GetCh();
				} else if (this->ch == L'<') {
					this->sym = optStrokeLeftBias;
					this->GetCh();
				} else if (this->ch == L'>') {
					this->sym = optStrokeRightBias;
					this->GetCh();
					this->ReplAtCurrPos(2,L">|");
				} else {
					this->sym = illegal;
				}
				break;
			case 0xAF:	this->sym = adjItalAngle; this->GetCh(); this->ReplAtCurrPos(1,L"//"); break; // replace Mac special char
			case L'$':	this->sym = postRound;	  this->GetCh(); break;
			case 0xA8:	this->sym = postRound;	  this->GetCh(); this->ReplAtCurrPos(1,L"$"); break; // replace Mac special char
			case L'"':	this->GetLiteral();	break;
			case L'.':
				this->GetCh();
				if (this->ch == L'.') {
					this->sym = ellipsis;
					this->GetCh();
				} else {
					this->sym = period;
				}
				break;
			case '\x0':	this->sym = eot; break;
			default:
				this->ErrorMsg(lexical,L"unknown character");
				break;
		}
	}
} /* TMTSourceParser::GetSym */

void TMTSourceParser::Delete(long pos, long len) {
	if (len > 0) {
		this->talkText->Delete(pos,pos + len);
		if (this->pos > pos + len) this->pos -= len;
		else if (this->pos > pos) this->pos = pos;
		this->changedSrc = true;
	}
} /* TMTSourceParser::Delete */

void TMTSourceParser::Insert(long pos, const wchar_t strg[]) {
	long len;
	
	len = (long)STRLENW(strg);
	if (len > 0) {
		this->talkText->Insert(pos,strg);
		if (this->pos > pos) this->pos += len;
		this->changedSrc = true;
	}
} /* TMTSourceParser::Insert */

void TMTSourceParser::ReplAtCurrPos(short origLen, const wchar_t repl[]) {
	long pos;
	
	pos = this->pos-chLookAhead-origLen;
	this->Delete(pos,origLen);
	this->Insert(pos,repl);
} /* TMTSourceParser::ReplAtCurrPos */

void TMTSourceParser::ErrorMsg(short kind, const wchar_t errMsg[]) {
	if (this->errPos < 0) { // no error reported yet
		this->ch = this->ch2 = L'\x0';
		this->sym = eot;
		if (errMsg[0] == L'\0')
			this->errPos = 0; // dummy error to QUIT() compilation...
		else {
			switch (kind) {
				case special:	  this->errPos = this->pos; this->symLen = this->errPos - this->prevPos; break;
				case lexical:	  this->errPos = this->pos-chLookAhead+1; this->symLen = 1; break;
				case syntactical: this->errPos = this->pos-chLookAhead; this->symLen = this->errPos - this->prevPos; break;
				case contextual:  this->errPos = this->prevPos; 		this->symLen = this->errPos - this->prevPrevPos; break;
			}
			STRCPYW(this->errMsg,errMsg);
		}
	}
} /* TMTSourceParser::ErrorMsg */

TMTParser *NewTMTSourceParser(void) {
	return new TMTSourceParser;
}

bool TMTCompile(TextBuffer *talkText, TrueTypeFont *font, TrueTypeGlyph *glyph, long glyphIndex, TextBuffer *glyfText, bool legacyCompile, long *errPos, long *errLen, wchar_t errMsg[]) {
	TTEngine *ttengine; // should think of keeping these around somewhere w/o having to allocate over and over again...
	short generators,i;
	TTGenerator *ttgenerator[3];
	TMTParser *tmtparser;
	bool memError,changedSrc;
	
	if (glyph->componentSize > 0) return true; // don't touch composites...
	
	tmtparser = NewTMTSourceParser(); memError = !tmtparser;
	generators = 0;
	if (glyfText) {
		ttgenerator[generators] = NewTTSourceGenerator();
		if (!ttgenerator[generators] ) memError = true; else generators++;
	}
/*****
	if (grafWindow) {
		ttgenerator[generators] = NewTTGlyphStrGenerator();
		if (!ttgenerator[generators] ) memError = true; else generators++;
	}
*****/
	if (glyfText) {
		ttengine = NewTTSourceEngine();
		memError = memError || !ttengine;
	} else
		ttengine = NULL;
	if (ttengine && !memError) ttengine->InitTTEngine(legacyCompile, &memError);
	for (i = 0; i < generators && !memError; i++) ttgenerator[i]->InitTTGenerator(font,glyph,glyphIndex,ttengine,legacyCompile,&memError);
	if (!memError) {
		tmtparser->InitTMTParser(talkText,font,glyph,legacyCompile,generators,ttgenerator);
		tmtparser->Parse(&changedSrc,errPos,errLen,errMsg);
		tmtparser->TermTMTParser();
		for (i = 0; i < generators; i++) ttgenerator[i]->TermTTGenerator();
		if (ttengine) ttengine->TermTTEngine(*errPos < 0 ? glyfText : NULL,&memError);
	}
	if (ttengine) delete ttengine;
	for (i = 0; i < generators; i++) delete ttgenerator[i];
	if (tmtparser) delete tmtparser;
	
	if (memError) swprintf(errMsg,L"Insufficient memory for compilation");
	else if (*errPos > 0) *errPos -= *errLen;
	return !memError && *errPos + *errLen <= 0;
} /* TMTCompile */

#if _DEBUG
bool TMTRemoveAltCodePath(TextBuffer *talkText, TrueTypeFont *font, TrueTypeGlyph *glyph, long *errPos, long *errLen, wchar_t errMsg[]) {
	TMTParser *tmtparser;
	bool memError,changedSrc;
	
	if (glyph->componentSize > 0) return true; // don't touch composites...
	
	tmtparser = NewTMTSourceParser(); memError = !tmtparser;
	if (!memError) {
		tmtparser->InitTMTParser(talkText,font,glyph,false, 0,NULL);
		tmtparser->RemoveAltCodePath(&changedSrc,errPos,errLen,errMsg);
		tmtparser->TermTMTParser();
	}
	if (tmtparser) delete tmtparser;
	
	if (memError) swprintf(errMsg,L"Insufficient memory for compilation");
	else if (*errPos > 0) *errPos -= *errLen;
	return !memError && *errPos + *errLen <= 0;
} // TMTRemoveAltCodePath
#endif


/*	the flickering at the end of the compilation comes from a flag (edit_saveOnCharChange)
	that forces every change to be saved (with the ominous double fs_DoFile...) and subse-
	quently reloaded, which in turn causes an update event for all visible windows (at least
	the main-, tmt-, and tt-window), and apparently, doing a TEInsert causes an update of
	the respective window as well, hence the tt-window gets updated twice (once before the
	save).
		For the future, might think of making such things more consistent: remember what
	has changed, or that something has changed, and save changes only upon request (alerting
	the uncautious user of unsaved data upon close). Should poll the users what they want,
	explaining the pros (speed, flickering) and cons (computer failure induced loss of data).
	
	looking at all the /one6 statements, it might turn out to be more sensible to come back
	with these values the way they have to be, such as in ValidateParam.
	
	Another question raised (by Claude): wouldn't it be possible to check, whether an in-
	formative command does preceed the action command(s) it is supposed to influence. I think
	with the GUI to come, and its data structure, this will come for free, so I'd rather not
	duplicate work now.
	
	An idea in this area is to get rid of having to use X|YRound, X|YStroke, but instead make
	the compiler/code generator more intelligent: if there is no cvt override, and if there
	are no informative commands upon generating code for a Link, then try to obtain the fea-
	ture category (cvtRound, cvtStroke, cvtDistance) automatically. Should be fairly simple for
	rounds, may have to extract some code from the auto-hinter for determining the strokes.
	
	Seems like the values for the overshoots are actually relative to the caps line (etc.),
	ending up doing much like what I do: actualHeight = capsLine + overshoot
	
	Why are there different values for the cvt_cut_in <27, 27-68, >68 ppem???
	
	In any case the X|YStroke had probably better be called X|YStem and expect 2 knots only,
	rather than 4, much like the X|YRound, or an even number of parameters?

*/