/*****

	TTEngine.c - New TypeMan Talk Compiler - TrueType Engine

	Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.
 
*****/
#define _CRT_SECURE_NO_DEPRECATE 
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <string.h> // wcslen
#include <stdio.h> // printf
#include "pch.h"
#include "CvtManager.h" // LinkColor
#include "TTEngine.h"

#define MAX_HEIGHTS	37

#define minBufLen 1024L // should be > 0 and preferrably a power of 2

/***** THE FOLLOWING #defines SHOULD BE COORDINATED WITH AutomaticConstraintGenerator!!!!! *****/

#define cvtCutInStorage				3

#define setTTVtoXItalDirFn			 9
#define setTTVtoYItalDirFn			10
#define setTTVtoXAdjItalDirFn		16
#define setTTVtoYAdjItalDirFn		40
#define oldDiagMirpFn				20
#define deltaCvtPpemRangeFn			70
#define deltaCvtSinglePpemFn		71
#define deltaKnotPpemRangeFn		72
#define deltaKnotSinglePpemFn		73
#define colorDeltaCvtPpemRangeFn	74
#define colorDeltaCvtSinglePpemFn	75
#define colorDeltaKnotPpemRangeFn	76
#define colorDeltaKnotSinglePpemFn	77
#define offsetCvtFn					(deltaCvtPpemRangeFn - deltaKnotPpemRangeFn)
#define threshold2UseDeltaFn 4
#define defaultDeltaBase			9 // ppem
#define defaultDeltaShift			3 // 1/8 pixel
#define roundDownToGridBelowPpemFn	82
#define aspectRatioFn				83
#define greyScalingFn				84
#define doubleHeightFn				85
#define italicRiseRunFn				88
#define singleWeightFn				31
	//	78 through 100 all used by AutomaticConstraintGenerator
	//	resActDistFn00			   101
	//	resActDistFn01			   102
	//	resActDistFn10			   103
	//	resActDistFn11			   104
#define resMIRPFn00				   105
	//	resMIRPFn01				   106
	//	resMIRPFn10				   107
	//	resMIRPFn11				   108
	//	symDistFn				   109
	//	ipMedianOffsetFn		   110
	//	ipMedianFn				   111
	//	resAdjStrokePhaseFn		   112
	//	deltaPhaseOptStrokePosFn   113
#define resMIAPFn				   114
#define resIPMDAPFn				   115
#define resIPMIRPFn				   116
#define resIPDMIRPFn			   117
	//	reversePvFn				   118
	//	condAlignFn				   119
#define resDDMIRP0Fn			   120
#define resDDMIRP1Fn			   121
#define resDDMIRP2Fn			   122
#define resDDMIRP3Fn			   123
	//	whichSideOfEdgeFn		   124
#define resIPDDMIRPFn			   125
#define resIPDDMIRPGlue0Fn         126
#define resIPDDMIRPGlue1Fn         127
#define resIPDDMIRPGlue2Fn         128
#define resIPDDMIRPGlue3Fn         129
	//	epXtoBaseItalicFn		   130
#define resIIPDMIRPFn			   131
	//	regularizeLineFn		   132
#define iIUPLoopFn				   133
#define iIUPSimpleFn			   134
#define iIUPBoundaryFn			   135


DeltaColor DeltaColorOfByte(unsigned char byte) {
	static DeltaColor deltaColorOfByte[0xE0] = {
		/*   0 */ blackDelta,
		/*   1 */ greyDelta,
		/*   2 */ ctNatVerRGBIAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*   6 */ ctComVerRGBIAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  10 */ ctNatHorRGBIAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  14 */ ctComHorRGBIAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  18 */ ctNatVerBGRIAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  22 */ ctComVerBGRIAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  26 */ ctNatHorBGRIAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  30 */ ctComHorBGRIAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
		/*  66 */ ctNatVerRGBFAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  70 */ ctComVerRGBFAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  74 */ ctNatHorRGBFAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  78 */ ctComHorRGBFAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  82 */ ctNatVerBGRFAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  86 */ ctComVerBGRFAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  90 */ ctNatHorBGRFAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
		/*  94 */ ctComHorBGRFAWBLYDelta, illegalDelta, illegalDelta, illegalDelta,
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
		/* 130 */ ctNatVerRGBIAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 134 */ ctComVerRGBIAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 138 */ ctNatHorRGBIAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 142 */ ctComHorRGBIAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 146 */ ctNatVerBGRIAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 150 */ ctComVerBGRIAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 154 */ ctNatHorBGRIAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 158 */ ctComHorBGRIAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
				  illegalDelta, illegalDelta, illegalDelta, illegalDelta, 
		/* 194 */ ctNatVerRGBFAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 198 */ ctComVerRGBFAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 202 */ ctNatHorRGBFAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 206 */ ctComHorRGBFAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 210 */ ctNatVerBGRFAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 214 */ ctComVerBGRFAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 218 */ ctNatHorBGRFAWYAADelta, illegalDelta, illegalDelta, illegalDelta,
		/* 222 */ ctComHorBGRFAWYAADelta, illegalDelta
	};

	return 0 <= byte && byte < 0xE0 ? deltaColorOfByte[byte] : illegalDelta;
} // DeltaColorOfByte

DeltaColor DeltaColorOfOptions(bool grayScale, bool clearType, bool clearTypeCompWidth, /* bool clearTypeVertRGB, */ bool clearTypeBGR, bool clearTypeFract, bool clearTypeYAA, bool clearTypeGray) {
	unsigned char byte;

	// we'll assume that the combination of options is valid (the UI enforces this)
	byte = 0;
	if (grayScale)          byte +=    1;
	if (clearType)          byte +=    2;
	if (clearTypeCompWidth) byte +=    4;
/*	if (clearTypeVertRGB)   byte +=    8; */
	if (clearTypeBGR)       byte += 0x10;
	if (clearTypeFract)     byte += 0x40;
	if (clearTypeYAA)       byte += 0x80;
	return DeltaColorOfByte(byte);
} // DeltaColorOfOptions

unsigned char ByteOfDeltaColor(DeltaColor color) {
	static unsigned char byteOfDeltaColor[numDeltaColors] = {
		0x5f, 0, 1, 
			  2,   6,  10,  14,  18,  22,  26,  30,  66,  70,  74,  78,  82,  86,  90,  94, 
			130, 134, 138, 142, 146, 150, 154, 158, 194, 198, 202, 206, 210, 214, 218, 222,
			0xdf };
	
	return 0 <= color && color < numDeltaColors ? byteOfDeltaColor[color] : 0xdf /* ??? */;
} // ByteOfDeltaColor

char *AllDeltaColorBytes(void) {
	static char allBytes[0x100] = "0, 1, 2, 6, 10, 14, 18, 22, 26, 30, 66, 70, 74, 78, 82, 86, 90, 94, 130, 134, 138, 142, 146, 150, 154, 158, 194, 198, 202, 206, 210, 214, 218, or 222";

	return &allBytes[0];
} // AllDeltaColorBytes

void TTEngine::Emit(const wchar_t text[]) { /* abstract */ }
void TTEngine::AssertFreeProjVector(TTVDirection dir) { /* abstract */ }
void TTEngine::AssertTTVonLine(TTVector ttv, short parent0, short parent1, Vector P0, Vector P1, bool rot) { /* abstract */ }
void TTEngine::AssertFVonCA(bool y) { /* abstract */ }
void TTEngine::AssertPVonCA(bool y) { /* abstract */ }
void TTEngine::AssertFVonPV(void) { /* abstract */ }
TTVDirection TTEngine::FVDir(void) { /* abstract */ return xRomanDir; }
TTVDirection TTEngine::PVDir(void) { /* abstract */ return xRomanDir; }
void TTEngine::AssertRefPoint(short rp, short knot) { /* abstract */ }
void TTEngine::AssertRefPointPair(short rp0, short rp1, short knot0, short knot1) { /* abstract */ }
short TTEngine::AssertEitherRefPointOnKnot(short rp0, short rp1, short knot) { /* abstract */ return 0; }
short TTEngine::AssertEitherKnotOnRefPoint(short knot0, short knot1, short rp) { /* abstract */ return 0; }
void TTEngine::AssertMinDist(short minDists, short jumpPpemSize[], F26Dot6 pixelSize[]) { /* abstract */ }
void TTEngine::AssertAutoFlip(bool on) { /* abstract */ }
void TTEngine::AssertRounding(Rounding round) { /* abstract */ }
void TTEngine::AssertRoundingBelowPpem(Rounding round, short ppem) { /* abstract */ }
void TTEngine::AssertSuperRounding(short period, short phase, short thresHold) { /* abstract */ }
void TTEngine::RoundDownToGridBelowPpem(short ppem) { /* abstract */ }
void TTEngine::IfPpemBelow(short ppem) { /* abstract */ }
void TTEngine::Else(void) { /* abstract */ }
void TTEngine::End(bool invalidateRefPoints) { /* abstract */ }
void TTEngine::MDAP(bool round, short knot) { /* abstract */ }
void TTEngine::MIAP(bool round, short knot, short cvt) { /* abstract */ }
void TTEngine::MDRP(bool minDist, bool round, short color, short knot) { /* abstract */ }
void TTEngine::MIRP(bool minDist, bool round, short color, short knot, short cvt, bool negative) { /* abstract */ }
void TTEngine::DMIRP(short knot, short cvt, short pvFrom, short pvTo) { /* abstract */ }
void TTEngine::ALIGNRP(short knot) { /* abstract */ }
void TTEngine::IP(short knots, short knot[]) { /* abstract */ }
void TTEngine::SHP(short rp, short knots, short knot[]) { /* abstract */ }
void TTEngine::SHPIX(short knots, short knot[], F26Dot6 amount) { /* abstract */ }
void TTEngine::SLOOP(short count) { /* abstract */ }
void TTEngine::ISECT(short intersection, short line0start, short line0end, short line1start, short line1end) { /* abstract */ }
void TTEngine::IUP(bool y) { /* abstract */ }
void TTEngine::IPRange(bool y, short parent0, short parent1, short start, short end) { /* abstract */ }
void TTEngine::DLT(bool cvt, DeltaColor color, short knot, F26Dot6 amount, bool ppemSize[]) { /* abstract */ }
void TTEngine::CALL24(short leftCvt, short rightCvt) { /* abstract */ }
void TTEngine::CALL3456(short type, short knot3, short cvt3, short knot2, short cvt2, short knot1, short cvt1) { /* abstract */ }
void TTEngine::CALL64(short parent, short child, short cvt, bool half, bool flip) { /* abstract */ }
void TTEngine::CALL656(bool crissCrossLinks, short knot0, short knot1, short knot2, short knot3, short cvt, short storage, bool xLinks, bool flip) { /* abstract */ }
void TTEngine::CALL678(bool back, short knot, short sameSide, short cvt, short storage) { /* abstract */ }
void TTEngine::CALL012345(short type, short knot0, short knot1, short knot2, short cvt) { /* abstract */ }
void TTEngine::CALL6(short knots, short knot[], short targetKnot) { /* abstract */ }
void TTEngine::CALL378(short type, short targetKnot) { /* abstract */ }
void TTEngine::CALL88(short riseCvt, short runCvt) { /* abstract */ }
void TTEngine::ResMIAP(short child, short cvt) { /* abstract */ }
void TTEngine::ResIPMDAP(TTVDirection pvP, bool postRoundFlag, short parent0, short child, short parent1) { /* abstract */ }
void TTEngine::ResMIRP(short parent, short child, short cvt, bool useMinDist) { /* abstract */ }
void TTEngine::ResIPMIRP(TTVDirection pvGP, short strokeOptimizationFlag, short grandParent0, short parent, short child, short cvt, short grandParent1) { /* abstract */ }
void TTEngine::ResDDMIRP(short parent0, short child0, TTVectorDesc fv0, short cvt0, short parent1, short child1, TTVectorDesc fv1, short cvt1) { /* abstract */ }
void TTEngine::ResIPDMIRP(TTVDirection pvGP, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1) { /* abstract */ }
void TTEngine::ResIPDDMIRP(TTVDirection pvGP, short grandParent0, short parent0, short child0, TTVectorDesc fv0, short cvt0, short parent1, short child1, TTVectorDesc fv1, short cvt1, short grandParent1) { /* abstract */ }
void TTEngine::ResIIPDMIRP(short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1) { /* abstract */ }
void TTEngine::CALL(short actParams, short anyNum[], short functNum) { /* abstract */ }
void TTEngine::ASM(bool inLine, wchar_t text[]) { /* abstract */ }
void TTEngine::INSTCTRL(short fromPpem, short toPpem) { /* abstract */ }
void TTEngine::SCANCTRL(short ctrl) { /* abstract */ }
void TTEngine::SCANTYPE(short type) { /* abstract */ }
void TTEngine::SCVTCI(short numCvtCutIns, short cvtCutInPpemSize[], F26Dot6 cvtCutInValue[]) { /* abstract */ }
void TTEngine::SetAspectRatioFlag(void) { /* abstract */ }
void TTEngine::SetGreyScalingFlag(void) { /* abstract */ }
void TTEngine::SetClearTypeCtrl(short ctrl) { /* abstract */ }
void TTEngine::CvtRegularization(bool relative, short cvtNum, short breakPpemSize, short parentCvtNum) { /* abstract */ }
void TTEngine::ResetPrepState(void) { /* abstract */ }
void TTEngine::SetFunctionNumberBias(short bias) { /* abstract */ }
short TTEngine::GetFunctionNumberBias(void) { return 0; /* abstract */ }
void TTEngine::InitTTEngine(bool legacyCompile, bool *memError) { /* abstract */ }
void TTEngine::TermTTEngine(TextBuffer *glyfText, bool *memError) { /* abstract */ }
TTEngine::TTEngine(void) { /* abstract */ }
TTEngine::~TTEngine(void) { /* abstract */ }

class TTSourceEngine : public TTEngine {
public:
	virtual void Emit(const wchar_t text[]);
	virtual void AssertFreeProjVector(TTVDirection dir);
	virtual void AssertTTVonLine(TTVector ttv, short parent0, short parent1, Vector P0, Vector P1, bool rot);
	virtual void AssertFVonCA(bool y);
	virtual void AssertPVonCA(bool y);
	virtual void AssertFVonPV(void);
	virtual TTVDirection FVDir(void);
	virtual TTVDirection PVDir(void);
	virtual void AssertRefPoint(short rp, short knot);
	virtual void AssertRefPointPair(short rp0, short rp1, short knot0, short knot1); // not necessarily in that order
	virtual short AssertEitherRefPointOnKnot(short rp0, short rp1, short knot); // returns the ref point actually asserted
	virtual short AssertEitherKnotOnRefPoint(short knot0, short knot1, short rp); // returns the knot actually asserted
	virtual void AssertMinDist(short minDists, short jumpPpemSize[], F26Dot6 pixelSize[]);
	virtual void AssertAutoFlip(bool on);
	virtual void AssertRounding(Rounding round);
	virtual void AssertRoundingBelowPpem(Rounding round, short ppem);
	virtual void AssertSuperRounding(short period, short phase, short thresHold);
	virtual void RoundDownToGridBelowPpem(short ppem);
	virtual void IfPpemBelow(short ppem);
	virtual void Else(void);
	virtual void End(bool invalidateRefPoints);
	virtual void MDAP(bool round, short knot);
	virtual void MIAP(bool round, short knot, short cvt);
	virtual void MDRP(bool minDist, bool round, short color, short knot);
	virtual void MIRP(bool minDist, bool round, short color, short knot, short cvt, bool negative);
	virtual void DMIRP(short knot, short cvt, short pvFrom, short pvTo);
	virtual void ALIGNRP(short knot);
	virtual void IP(short knots, short knot[]);
	virtual void SHP(short rp, short knots, short knot[]);
	virtual void SHPIX(short knots, short knot[], F26Dot6 amount);
	virtual void SLOOP(short count);
	virtual void ISECT(short intersection, short line0start, short line0end, short line1start, short line1end);
	virtual void IUP(bool y);
	virtual void IPRange(bool y, short parent0, short parent1, short start, short end);
	virtual void DLT(bool cvt, DeltaColor color, short knot, F26Dot6 amount, bool ppemSize[]);
	virtual void CALL24(short leftCvt, short rightCvt);
	virtual void CALL3456(short type, short knot3, short cvt3, short knot2, short cvt2, short knot1, short cvt1);
	virtual void CALL64(short parent, short child, short cvt, bool half, bool flip); // "special MIRP" for new italic strokes
	virtual void CALL656(bool crissCrossLinks, short knot0, short knot1, short knot2, short knot3, short cvt, short storage, bool xLinks, bool flip);
	virtual void CALL678(bool back, short knot, short sameSide, short cvt, short storage); // new italic strokes: extrapolate knot to cvt height or back again
	virtual void CALL012345(short type, short knot0, short knot1, short knot2, short cvt);
	virtual void CALL6(short knots, short knot[], short targetKnot);
	virtual void CALL378(short type, short targetKnot);
	virtual void CALL(short actParams, short anyNum[], short functNum);
	virtual void CALL88(short riseCvt, short runCvt);
	virtual void ResMIAP(short child, short cvt);
	virtual void ResIPMDAP(TTVDirection pvP, bool postRoundFlag, short parent0, short child, short parent1);
	virtual void ResMIRP(short parent, short child, short cvt, bool useMinDist);
	virtual void ResIPMIRP(TTVDirection pvGP, short strokeOptimizationFlag, short grandParent0, short parent, short child, short cvt, short grandParent1);
	virtual void ResDDMIRP(short parent0, short child0, TTVectorDesc fv0, short cvt0, short parent1, short child1, TTVectorDesc fv1, short cvt1);
	virtual void ResIPDMIRP(TTVDirection pvGP, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1);
	virtual void ResIPDDMIRP(TTVDirection pvGP, short grandParent0, short parent0, short child0, TTVectorDesc fv0, short cvt0, short parent1, short child1, TTVectorDesc fv1, short cvt1, short grandParent1);
	virtual void ResIIPDMIRP(short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1);
	virtual void ASM(bool inLine, wchar_t text[]);
	virtual void INSTCTRL(short fromPpem, short toPpem);
	virtual void SCANCTRL(short ctrl);
	virtual void SCANTYPE(short type);
	virtual void SCVTCI(short numCvtCutIns, short cvtCutInPpemSize[], F26Dot6 cvtCutInValue[]);
	virtual void SetAspectRatioFlag(void);
	virtual void SetGreyScalingFlag(void);
	virtual void SetClearTypeCtrl(short ctrl);
	virtual void CvtRegularization(bool relative, short cvtNum, short breakPpemSize, short parentCvtNum);
	virtual void ResetPrepState(void);
	virtual void SetFunctionNumberBias(short bias);
	virtual short GetFunctionNumberBias(void);
	virtual void InitTTEngine(bool legacyCompile, bool *memError);
	virtual void TermTTEngine(TextBuffer *glyfText, bool *memError);
	TTSourceEngine(void);
	virtual ~TTSourceEngine(void);
private:
	void InitTTEngineState(bool forNewGlyph); // false => after Call or Inline
	bool error;
	short fnBias;
	short rp[3]; // zp never changed
	F26Dot6 minDist;
	bool sRound;
	Rounding round;
	short period,phase,thresHold;
	TTVectorDesc ttv[pv-fv+1],Ttv[pv-fv+1];
	bool usedpv,Usedpv; // I'm not sure whether this is always updated correctly upon reflecting all kinds of side-effects
	bool autoFlip;
	short deltaBase,deltaShift;
	short lastChild; // to optimise the move flag in MDRP, MIRP
	long lastChildPos; // fixup position...
	long bufPos,bufLen;
	wchar_t *buf;
	wchar_t mov[2],min[2],rnd[2],col[numLinkColors][2]; // characters for encoding
	bool legacyCompile; 
};

void TTSourceEngine::Emit(const wchar_t text[]) {
	long len;
	wchar_t *newBuf;
	
	if (this->error) return; // no further code generation possible...
	len = (long)STRLENW(text);
	while (this->bufPos + len + 2 > this->bufLen) { // test for CR and trailing 0
		newBuf = (wchar_t*)NewP(2*this->bufLen * sizeof(wchar_t));
		this->error = newBuf == NULL;
		if (this->error) return; // no further code generation possible...
		memcpy(newBuf,this->buf,this->bufLen * sizeof(wchar_t));
		DisposeP((void**)&this->buf);
		this->buf = newBuf;
		this->bufLen = 2*this->bufLen;
	}
	memcpy(&this->buf[this->bufPos],text,len * sizeof(wchar_t));
	this->bufPos += len;
	this->buf[this->bufPos++] = L'\x0D';
	this->buf[this->bufPos] = 0; // add trailing 0
} // TTSourceEngine::Emit

void TTSourceEngine::AssertFreeProjVector(TTVDirection dir) {
	const TTVDirection fvDirMap[numTTVDirections] = {xRomanDir, yRomanDir, xRomanDir, yItalDir, xRomanDir, yAdjItalDir, diagDir, perpDiagDir},
					   pvDirMap[numTTVDirections] = {xRomanDir, yRomanDir, xItalDir, yRomanDir, xAdjItalDir, yRomanDir, diagDir, perpDiagDir};
	wchar_t code[64];
	TTVDirection fvDir = fvDirMap[dir],
				 pvDir = pvDirMap[dir];

	if (fvDir != this->ttv[fv].dir || pvDir != this->ttv[pv].dir) {
		switch (dir) {
		case xRomanDir:
			swprintf(code,L"SVTCA[X]");
			break;
		case yRomanDir:
			swprintf(code,L"SVTCA[Y]");
			break;
		case xItalDir:
			swprintf(code,L"CALL[], %li",this->fnBias + setTTVtoXItalDirFn);
			break;
		case yItalDir:
			swprintf(code,L"CALL[], %li",this->fnBias + setTTVtoYItalDirFn);
			break;
		case xAdjItalDir:
			swprintf(code,L"CALL[], %li",this->fnBias + setTTVtoXAdjItalDirFn);
			break;
		case yAdjItalDir:
			swprintf(code,L"CALL[], %li",this->fnBias + setTTVtoYAdjItalDirFn);
			break;
		default:
			swprintf(code,L"/* illegal TT vector direction */");
			break;
		}
		this->Emit(code);
		this->ttv[fv].dir = fvDir; this->ttv[fv].from = this->ttv[fv].to = illegalKnotNum;
		this->ttv[pv].dir = pvDir; this->ttv[pv].from = this->ttv[pv].to = illegalKnotNum;
		this->usedpv = false;
	}
} // TTSourceEngine::AssertFreeProjVector

void TTSourceEngine::AssertTTVonLine(TTVector ttv, short parent0, short parent1, Vector P0, Vector P1, bool rot) {
	TTVDirection dir;
	wchar_t buf[32];
	TTVectorDesc *v = &this->ttv[ttv == fv ? fv : pv], *proj;
	
	if      (P0.x == P1.x && P0.y != P1.y) dir = (TTVDirection)((short)yRomanDir - (short)rot); // incidently vertical
	else if (P0.y == P1.y && P0.x != P1.x) dir = (TTVDirection)((short)xRomanDir + (short)rot); // ... horizontal
	else                                   dir = (TTVDirection)((short)diagDir   + (short)rot);
	if (dir < diagDir) {
		if (dir != v->dir) {
			swprintf(buf,L"S%cVTCA[%c]",ttv == fv ? L'F' : L'P',L'X' + (dir == yRomanDir)); this->Emit(buf);
			v->dir = dir; v->from = v->to = illegalKnotNum;
			if (ttv > fv) this->usedpv = ttv == dpv;
		}
	} else {
		if (!((v->from == parent0 && v->to == parent1 || v->from == parent1 && v->to == parent0) && dir == v->dir && (ttv != dpv || this->usedpv))) {
			
			proj = &this->ttv[pv];
			if (ttv == fv && (proj->from == parent0 && proj->to == parent1 || proj->from == parent1 && proj->to == parent0) && proj->dir == dir) {
				swprintf(buf,L"SFVTPV[]");
			} else {
				swprintf(buf,L"S%sVTL[%c], %hi, %hi",ttv == fv ? L"F" : (ttv == pv ? L"P" : L"DP"),dir == perpDiagDir ? L'R' : L'r',parent0,parent1);
			}
			this->Emit(buf);
			v->dir = dir; v->from = parent0; v->to = parent1;
			if (ttv > fv) this->usedpv = ttv == dpv;
		}
	}
} // TTSourceEngine::AssertTTVonLine

void TTSourceEngine::AssertFVonCA(bool y) {
	Vector P0 = {0, 0}, P1 = {0, 0};
	
	if (y) P1.y = 1; else P1.x = 1;
	this->AssertTTVonLine(fv,illegalKnotNum,illegalKnotNum,P0,P1,false);
} // TTSourceEngine::AssertFVonCA

void TTSourceEngine::AssertPVonCA(bool y) {
	Vector P0 = {0, 0}, P1 = {0, 0};
	
	if (y) P1.y = 1; else P1.x = 1;
	this->AssertTTVonLine(pv,illegalKnotNum,illegalKnotNum,P0,P1,false);
} // TTSourceEngine::AssertPVonCA

void TTSourceEngine::AssertFVonPV() {
	if (this->ttv[fv].dir != this->ttv[pv].dir || this->ttv[fv].from != this->ttv[pv].from || this->ttv[fv].to != this->ttv[pv].to) {
		this->Emit(L"SFVTPV[]");
		this->ttv[fv] = this->ttv[pv];
	}
} // TTSourceEngine::AssertFVonPV

TTVDirection TTSourceEngine::FVDir(void) {
	return this->ttv[fv].dir;
} // TTSourceEngine::FVDir
	
TTVDirection TTSourceEngine::PVDir(void) {
	return this->ttv[pv].dir;
} // TTSourceEngine::PVDir
	
void TTSourceEngine::AssertRefPoint(short rp, short knot) {
	wchar_t buf[32];
	
	if (this->rp[rp] != knot) {
		if (rp == 0 && this->lastChild == knot) { // set reference point by turning on the "move" flag
			this->buf[this->lastChildPos] = this->mov[true]; // fixup
		} else { // set reference point by the respective TT code...
			swprintf(buf,L"SRP%hi[], %hi",rp,knot); this->Emit(buf);
		}
		this->rp[rp] = knot;
	}
	if (rp == 0) this->lastChild = illegalKnotNum;
} // TTSourceEngine::AssertRefPoint

void TTSourceEngine::AssertRefPointPair(short rp0, short rp1, short knot0, short knot1) {
	if (this->rp[rp0] == knot1 || this->rp[rp1] == knot0) {
		this->AssertRefPoint(rp0,knot1);
		this->AssertRefPoint(rp1,knot0);
	} else { // and if we have to set the reference points, might as well assert sequentially
		this->AssertRefPoint(rp0,knot0);
		this->AssertRefPoint(rp1,knot1);
	}
} // TTSourceEngine::AssertRefPointPair

short TTSourceEngine::AssertEitherRefPointOnKnot(short rp0, short rp1, short knot) {
	if (this->rp[rp1] == knot) return rp1;
	else { this->AssertRefPoint(rp0,knot); return rp0; }
} // TTSourceEngine::AssertEitherRefPointOnKnot

short TTSourceEngine::AssertEitherKnotOnRefPoint(short knot0, short knot1, short rp) {
	if (this->rp[rp] == knot1) return knot1;
	else { this->AssertRefPoint(rp,knot0); return knot0; }
} // TTSourceEngine::AssertEitherKnotOnRefPoint

void TTSourceEngine::AssertMinDist(short minDists, short jumpPpemSize[], F26Dot6 pixelSize[]) {
	wchar_t buf[32];
	
	switch (minDists) {
		case 1:
			if (this->minDist != pixelSize[0]) {
				swprintf(buf,L"SMD[], %li",pixelSize[0]); this->Emit(buf);
				this->minDist = pixelSize[0];
			}
			break;
		case 2:
			this->Emit(		L"MPPEM[]");
			if (this->minDist == pixelSize[1]) {
				swprintf(buf,L"GT[], %hi, *",jumpPpemSize[1]); this->Emit(buf);	// [TOS](= jumpPpemSize[1]) > [TOS-1](= MPPEM[]) ?
				this->Emit( L"IF[], *");											// MPPEM[] < jumpPpemSize[1] ?
				this->Emit( L"#BEGIN");											// but current minDist is pixelSize[1]
				swprintf(buf,L"SMD[], %li",pixelSize[0]); this->Emit(buf);		// hence use pixelSize[0] instead
			} else if (this->minDist == pixelSize[0]) {
				swprintf(buf,L"LTEQ[], %hi, *",jumpPpemSize[1]); this->Emit(buf);	// [TOS](= jumpPpemSize[1]) ² [TOS-1](= MPPEM[]) ?
				this->Emit( L"IF[], *");											// MPPEM[] ³ jumpPpemSize[1]
				this->Emit( L"#BEGIN");											// but current minDist is pixelSize[0]
				swprintf(buf,L"SMD[], %li",pixelSize[1]); this->Emit(buf);		// use pixelSize[1] instead
			} else {
				swprintf(buf,L"GT[], %hi, *",jumpPpemSize[1]); this->Emit(buf);
				this->Emit( L"IF[], *");
				this->Emit( L"#BEGIN");
				swprintf(buf,L"SMD[], %li",pixelSize[0]); this->Emit(buf);
				this->Emit( L"#END");
				this->Emit( L"ELSE[]");
				this->Emit( L"#BEGIN");
				swprintf(buf,L"SMD[], %li",pixelSize[1]); this->Emit(buf);
			}
			this->Emit(		L"#END");
			this->Emit(		L"EIF[]");
			this->minDist = -1; // undefined, because we don't know which branch was taken
			break;
		default:
			break;
	}
} // TTSourceEngine::AssertMinDist

void TTSourceEngine::AssertAutoFlip(bool on) {
	if (this->autoFlip != on) {
		if (on) this->Emit(L"FLIPON[]"); else this->Emit(L"FLIPOFF[]");
		this->autoFlip = on;
	}
} // TTSourceEngine::AssertAutoFlip

void TTSourceEngine::AssertRounding(Rounding round) {
	if (this->sRound || round != this->round) {
		switch (round) {
			case rthg: this->Emit(L"RTHG[]"); break;
			case rtdg: this->Emit(L"RTDG[]"); break;
			case rtg:  this->Emit(L"RTG[]");  break;
			case rdtg: this->Emit(L"RDTG[]"); break;
			case rutg: this->Emit(L"RUTG[]"); break;
			default:   this->Emit(L"ROFF[]"); break;
		}
		this->sRound = false; this->round = round;
	}
} // TTSourceEngine::AssertRounding

void TTSourceEngine::AssertRoundingBelowPpem(Rounding round, short ppem) {
	wchar_t buf[32];

	if (round == roff || ppem < 0)
		this->AssertRounding(round);
	else {
		this->Emit( L"MPPEM[]");
		swprintf(buf,L"GT[], *, %hi",ppem); this->Emit(buf);
		this->Emit( L"IF[], *");
		this->Emit( L"#BEGIN");
		switch (round) {
			case rthg: this->Emit(L"RTHG[]"); break;
			case rtdg: this->Emit(L"RTDG[]"); break;
			case rtg:  this->Emit(L"RTG[]");  break;
			case rdtg: this->Emit(L"RDTG[]"); break;
			case rutg: this->Emit(L"RUTG[]"); break;
			default:   this->Emit(L"ROFF[]"); break;
		}
		this->Emit( L"#END");
		this->Emit( L"ELSE[]");
		this->Emit( L"#BEGIN");
		this->Emit( L"ROFF[]");
		this->Emit( L"#END");
		this->Emit( L"EIF[]");
		this->sRound = false; this->round = rnone;
	}
} // TTSourceEngine::AssertRoundingBelowPpem

void TTSourceEngine::AssertSuperRounding(short period, short phase, short thresHold) {
	wchar_t buf[32];
	
	if (!this->sRound || period != this->period || phase != this->phase || thresHold != this->thresHold) {
		swprintf(buf,L"SROUND[], %hi",(period << 6) + (phase << 4) + (thresHold + 4)); this->Emit(buf); // CEILING_ROUND never used, use threshold == -4 instead...
		this->sRound = true; this->period = period; this->phase = phase; this->thresHold = thresHold;
	}
} // TTSourceEngine::AssertSuperRounding

void TTSourceEngine::RoundDownToGridBelowPpem(short ppem) {
	wchar_t buf[32];

	if (ppem < 0)
		this->AssertRounding(rdtg);
	else {
		swprintf(buf,L"CALL[], %hi, %hi",ppem,this->fnBias + roundDownToGridBelowPpemFn); this->Emit(buf);
		this->sRound = false; this->round = rnone;
	}
} // TTSourceEngine::RoundDownToGridBelowPpem

void TTSourceEngine::IfPpemBelow(short ppem) { // this is not nestable, nor general enough to deal with the entire graphics state, but can grow later, as needed
	wchar_t buf[32];

	this->Emit( L"MPPEM[]");
	swprintf(buf,L"GT[], *, %hi",ppem); this->Emit(buf);
	this->Emit( L"IF[], *");
	this->Emit( L"#BEGIN");
	this->Ttv[fv] = this->ttv[fv];
	this->Ttv[pv] = this->ttv[pv];
	this->Usedpv = this->usedpv;
} // TTSourceEngine::IfPpemBelow

void TTSourceEngine::Else(void) {
	this->Emit( L"#END");
	this->Emit( L"ELSE[]");
	this->Emit( L"#BEGIN");
	this->ttv[fv] = this->Ttv[fv];
	this->ttv[pv] = this->Ttv[pv];
	this->usedpv = this->Usedpv;
} // TTSourceEngine::Else

void TTSourceEngine::End(bool invalidateRefPoints) {
	this->Emit( L"#END");
	this->Emit( L"EIF[]");
	this->ttv[fv].dir = diagDir; this->ttv[fv].from = this->ttv[fv].to = illegalKnotNum; // assume any kind of side-effect, cf. InitTTEngine(true)
	this->ttv[pv] = this->ttv[fv];
	this->usedpv = false; // ?
	if (invalidateRefPoints) this->rp[0] = this->rp[1] = this->rp[2] = illegalKnotNum;
} // TTSourceEngine::End

void TTSourceEngine::MDAP(bool round, short knot) {
	wchar_t buf[32];
	
	swprintf(buf,L"MDAP[%c], %hi",this->rnd[round],knot); this->Emit(buf);
	this->rp[0] = this->rp[1] = knot; this->lastChild = illegalKnotNum;
} // TTSourceEngine::MDAP

void TTSourceEngine::MIAP(bool round, short knot, short cvt) {
	wchar_t buf[32];
	
	swprintf(buf,L"MIAP[%c], %hi, %hi",this->rnd[round],knot,cvt); this->Emit(buf);
	this->rp[0] = this->rp[1] = knot; this->lastChild = illegalKnotNum;
} // TTSourceEngine::MIAP

void TTSourceEngine::MDRP(bool minDist, bool round, short color, short knot) {
	wchar_t buf[32];
	
	this->lastChild = knot;
	this->lastChildPos = this->bufPos + 5; // a bit of a hack, but just a small one...
	swprintf(buf,L"MDRP[%c%c%c%c%c], %hi",this->mov[false],this->min[minDist],this->rnd[round],this->col[color][0],this->col[color][1],knot); this->Emit(buf);
	this->rp[1] = this->rp[0]; this->rp[2] = knot;
} // TTSourceEngine::MDRP

void TTSourceEngine::MIRP(bool minDist, bool round, short color, short knot, short cvt, bool negative) {
	wchar_t buf[32];
	
	if (negative) {
		swprintf(buf,L"RCVT[], %hi",cvt); this->Emit(buf);
		this->Emit( L"NEG[], *");
		cvt = SECONDTMPCVT;
		swprintf(buf,L"WCVTP[], %hi, *",cvt); this->Emit(buf);
	}
	this->lastChild = knot;
	this->lastChildPos = this->bufPos + 5; // a bit of a hack, but just a small one...
	swprintf(buf,L"MIRP[%c%c%c%c%c], %hi, %hi",this->mov[false],this->min[minDist],this->rnd[round],this->col[color][0],this->col[color][1],knot,cvt); this->Emit(buf);
	this->rp[1] = this->rp[0]; this->rp[2] = knot;
} // TTSourceEngine::MIRP

void TTSourceEngine::DMIRP(short knot, short cvt, short pvFrom, short pvTo) {
	wchar_t buf[64];
	
	swprintf(buf,L"CALL[], %hd, %hd, %hd, %hd, %hi",knot,cvt,pvFrom,pvTo,this->fnBias + oldDiagMirpFn); this->Emit(buf);
	this->rp[1] = this->rp[0]; this->rp[2] = knot; this->lastChild = illegalKnotNum;
	this->round = rtg;
	this->ttv[pv].dir = perpDiagDir; this->ttv[pv].from = pvFrom; this->ttv[pv].to = pvTo;
	this->usedpv = true;
} // TTSourceEngine::DMIRP

void TTSourceEngine::ALIGNRP(short knot) {
	wchar_t buf[32];
	
	swprintf(buf,L"ALIGNRP[], %hi",knot); this->Emit(buf);
} // TTSourceEngine::ALIGNRP
	
void TTSourceEngine::IP(short knots, short knot[]) {
	wchar_t buf[8*maxParams];
	short i;
	
	swprintf(buf,L"IP[]");
	for (i = 0; i < knots; i++) swprintf(&buf[STRLENW(buf)],L", %hi",knot[i]);
	this->Emit(buf);
} // TTSourceEngine::IP

void TTSourceEngine::SHP(short rp, short knots, short knot[]) {
	wchar_t buf[8*maxParams];
	short i;
	
	swprintf(buf,L"SHP[%hi]",rp);
	for (i = 0; i < knots; i++) swprintf(&buf[STRLENW(buf)],L", %hi",knot[i]);
	this->Emit(buf);
} // TTSourceEngine::SHP

void TTSourceEngine::SHPIX(short knots, short knot[], F26Dot6 amount) {
	wchar_t buf[8*maxParams];
	short i;
	
	swprintf(buf,L"SHPIX[]");
	for (i = 0; i < knots; i++) swprintf(&buf[STRLENW(buf)],L", %hi",knot[i]);
	swprintf(&buf[STRLENW(buf)],L", %li",amount);
	this->Emit(buf);
} // TTSourceEngine::SHPIX

void TTSourceEngine::SLOOP(short count) {
	wchar_t buf[32];
	
	swprintf(buf,L"SLOOP[], %hi",count); this->Emit(buf);
} // TTSourceEngine::SLOOP

void TTSourceEngine::ISECT(short intersection, short line0start, short line0end, short line1start, short line1end) {
	wchar_t buf[48];
	
	swprintf(buf,L"ISECT[], %hi, %hi, %hi, %hi, %hi",intersection,line0start,line0end,line1start,line1end); this->Emit(buf);
} // TTSourceEngine::ISECT

void TTSourceEngine::IUP(bool y) {
	wchar_t buf[8];
	
	swprintf(buf,L"IUP[%c]",L'X' + (y & 1)); this->Emit(buf);
} // TTSourceEngine::IUP

void TTSourceEngine::IPRange(bool y, short parent0, short parent1, short start, short end) {
	wchar_t buf[64];

	if (parent0 <= parent1) {
		swprintf(buf,L"CALL[], %hi, %hi, %hi",parent0,parent1,this->fnBias + iIUPSimpleFn); this->Emit(buf);
	} else {
		swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi",parent0,end,start,parent1,this->fnBias + iIUPBoundaryFn); this->Emit(buf);
	}
} // TTSourceEngine::IPRange

/*****
void FifthLoop(short *ppem, bool ppemSize[]);
void FifthLoop(short *ppem, bool ppemSize[]) {
	do (*ppem)++; while (*ppem < maxPpemSize && !ppemSize[*ppem]); // next ppem to do
}

void TTSourceEngine::DLT(short knot, F26Dot6 amount, bool ppemSize[]) {
	short amnt,sign,steps,maxShift,magnitude[3],shift[3],ppem,deltaSubBase,deltas;
	char buf[128];
	
//	with a maximum esolution of 1 and a minimum resolution of 1/64, we can delta in range -8..+8 in steps of 1/64.
//	this means that upto 3 steps have to be performed: doing the entire pixels, doing the 1/8 pixels, and finally
//	doing the 1/64. If we delta by, say, 33/32, this can be done in two steps, e.g. 4/4 and 1/32. Likewise, 65/64
//	can be done as 1/1 and 1/64
	
	amnt = (short)Abs(amount); sign = Sgn(amount); steps = 0; maxShift = 6;
//	old version
//	while (amnt != 0) {
//		while (maxShift > 0 && (amnt & 1) == 0) { amnt >>= 1; maxShift--; }
//		if (amnt > 8) {
//			magnitude[steps] = sign*(amnt & 7); amnt = amnt >> 3;
//		} else { // allow 8/1 (or e.g. 8/8 in a stream of other deltas with delta shift = 3)
//			magnitude[steps] = sign*amnt; amnt = 0;
//		}
//		shift[steps] = maxShift; maxShift = Max(0,maxShift-3);
//		steps++;	
//	}
//	end of old version
	if ((amnt & (short)0xffff << 6-this->deltaShift) == amnt && amnt >> 6-this->deltaShift <= 8) { // amount is "addressable" by current delta shift
		magnitude[steps] = sign*(amnt >> 6-this->deltaShift);
		shift[steps] = this->deltaShift;
		steps++;
	} else if ((amnt & (short)0xffff << stdDeltaShift) == amnt && amnt >> stdDeltaShift <= 8) { // amount is "addressable" by standard delta shift
		magnitude[steps] = sign*(amnt >> stdDeltaShift);
		shift[steps] = stdDeltaShift;
		steps++;
	} else {
		while (amnt > 0) {
			while (amnt > 8 && (amnt & 1) == 0) { amnt >>= 1; maxShift--; }
			if (amnt > 8) {
				magnitude[steps] = sign*(amnt & 7); amnt = amnt >> 3;
			} else { // allow 8/1 (or e.g. 8/8 in a stream of other deltas with delta shift = 3)
				magnitude[steps] = sign*amnt; amnt = 0;
			}
			shift[steps] = maxShift; maxShift = Max(0,maxShift-3);
			steps++;	
		}
	}
	while (steps > 0) { // while there are steps to do
		steps--;
		if (this->deltaShift != shift[steps]) { // resolution is not the right one for this step
			this->deltaShift = shift[steps];
			swprintf(buf,L"SDS[], %hi",this->deltaShift); this->Emit(buf);
		}
		ppem = 0;
		while (ppem < maxPpemSize && !ppemSize[ppem]) ppem++;
		while (ppem < maxPpemSize) { // while there are ppems to do 
			if (ppem < this->deltaBase || this->deltaBase + 47 < ppem) { // ppem is outside range "addressable" by DLTP1, DLTP2, or DLTP3
				this->deltaBase = ppem;
				swprintf(buf,L"SDB[], %hi",this->deltaBase); this->Emit(buf);
			}
			deltaSubBase = this->deltaBase;
			while (deltaSubBase + 15 < ppem) deltaSubBase += 16;
			while (ppem < maxPpemSize && deltaSubBase <= this->deltaBase + 47) { // while there are ppems to do and they are "addressable" by DLTP1, DLTP2, or DLTP3
				if (ppem <= deltaSubBase + 15) { // if there are ppems to do which are "addressable" by "current" DLTPi
					swprintf(buf,L"DLTP%hi[",((deltaSubBase-this->deltaBase) >> 4) + 1); deltas = 0;
					do {
						if (deltas == 4) { this->Emit(buf); swprintf(buf,L"      "); deltas = 0; }
						swprintf(&buf[STRLENW(buf)],L"(%hi @%hi %hi)",knot,ppem-deltaSubBase,magnitude[steps]); deltas++;
						
						FifthLoop(&ppem,ppemSize); // Think C compiler 7.0 doesn't like a 5th loop with code optimizations turned on, 7.0.3 does, though...
					
					//	do ppem++; while (ppem < maxPpemSize && !ppemSize[ppem]); // next ppem to do
					
					} while (ppem <= deltaSubBase + 15); // while there are ppems to do which are "addressable" by "current" DLTPi
					swprintf(&buf[STRLENW(buf)],L"]");
					this->Emit(buf);
				}
				deltaSubBase += 16;
			}
		}
	}
} // TTSourceEngine::DLT
*****/

void SplitPpemSize(short threshold, bool ppemSize[], short *singleSizes, short singleSize[], short *ranges, short rangeLow[], short rangeHigh[]);
void SplitPpemSize(short threshold, bool ppemSize[], short *singleSizes, short singleSize[], short *ranges, short rangeLow[], short rangeHigh[]) {
	short ppem,low;

	*singleSizes = *ranges = ppem = 0;
	while (ppem < maxPpemSize && !ppemSize[ppem]) ppem++;
	while (ppem < maxPpemSize) {
		low = ppem;
		while (ppem < maxPpemSize && ppemSize[ppem]) ppem++;
		if (ppem - low >= threshold) {
			rangeLow[*ranges] = low;
			rangeHigh[(*ranges)++] = ppem-1;
		} else {
			while (low < ppem)
				singleSize[(*singleSizes)++] = low++;
		}
		while (ppem < maxPpemSize && !ppemSize[ppem]) ppem++;
	}
} // SplitPpemSize

void TTSourceEngine::DLT(bool cvt, DeltaColor color, short knot, F26Dot6 amount, bool ppemSize[]) {
	short amnt,sign,magnitude,shift,size,deltaSubBase,deltas;
	bool singleStep;
	wchar_t charFn,buf[128];
	short singlePpemSizes,singlePpemSize[maxPpemSize],ppemRanges,ppemRangeLow[maxPpemSize],ppemRangeHigh[maxPpemSize],offsFn;
	
//	with a maximum resolution of 1 and a minimum resolution of 1/64, we can delta in range -8..+8 in steps of 1/64.
//	this means that upto 3 steps have to be performed: doing the entire pixels, doing the 1/8 pixels, and finally
//	doing the 1/64. If we delta by, say, 33/32, this can be done in two steps, e.g. 4/4 and 1/32. Likewise, 65/64
//	can be done as 1/1 and 1/64
	
	amnt = (short)Abs(amount); sign = Sgn(amount);
	if (this->deltaShift >= 0 && (amnt & (short)0xffff << (6-this->deltaShift)) == amnt && amnt >> (6-this->deltaShift) <= 8) { // amount is "addressable" by current delta shift
		magnitude = sign*(amnt >> (6-this->deltaShift));
		shift = this->deltaShift;
		singleStep = true;
	} else if (this->deltaShift >= 0 && (amnt & (short)0xffff << stdDeltaShift) == amnt && amnt >> stdDeltaShift <= 8) { // amount is "addressable" by standard delta shift
		magnitude = sign*(amnt >> stdDeltaShift);
		shift = stdDeltaShift;
		singleStep = true;
	} else { // may need a new delta shift, if we get away with a single delta at all
		shift = 6;
		while (amnt > 8 && (amnt & 1) == 0) { amnt >>= 1; shift--; }
		magnitude = sign*amnt;
		singleStep = amnt <= 8;
	}
	
//	if it takes 2 steps (or more) to do a delta, we need two SDS[] and two DLTPi using 12 bytes (or three of each using 18 bytes),
//	as opposed to a function call using 6 to 9 bytes only (6 for positive amount, 9 for negative due to sign extension and prepush
	SplitPpemSize(singleStep && color == alwaysDelta ? threshold2UseDeltaFn : 1,ppemSize,&singlePpemSizes,singlePpemSize,&ppemRanges,ppemRangeLow,ppemRangeHigh);	
	
	if (ppemRanges > 0) {
		offsFn = cvt ? offsetCvtFn : 0;
		for (size = 0; size < ppemRanges; size++) {
			if (ppemRangeLow[size] == ppemRangeHigh[size])
				if (color == alwaysDelta)
					if (cvt)
						swprintf(buf,L"CALL[], %li, %hi, %hi, %hi",amount,knot,ppemRangeLow[size],this->fnBias + deltaCvtSinglePpemFn);
					else
						swprintf(buf,L"CALL[], %hi, %li, %hi, %hi",knot,amount,ppemRangeLow[size],this->fnBias + deltaKnotSinglePpemFn);
				else
					if (cvt)
						swprintf(buf,L"CALL[], %li, %hi, %hi, %hi, %hi",amount,knot,ppemRangeLow[size],ByteOfDeltaColor(color),this->fnBias + colorDeltaCvtSinglePpemFn);
					else
						swprintf(buf,L"CALL[], %hi, %li, %hi, %hi, %hi",knot,amount,ppemRangeLow[size],ByteOfDeltaColor(color),this->fnBias + colorDeltaKnotSinglePpemFn);
			else
				if (color == alwaysDelta)
					if (cvt)
						swprintf(buf,L"CALL[], %li, %hi, %hi, %hi, %hi",amount,knot,ppemRangeLow[size],ppemRangeHigh[size],this->fnBias + deltaCvtPpemRangeFn);
					else
						swprintf(buf,L"CALL[], %hi, %li, %hi, %hi, %hi",knot,amount,ppemRangeLow[size],ppemRangeHigh[size],this->fnBias + deltaKnotPpemRangeFn);
				else
					if (cvt)
						swprintf(buf,L"CALL[], %li, %hi, %hi, %hi, %hi, %hi",amount,knot,ppemRangeLow[size],ppemRangeHigh[size],ByteOfDeltaColor(color),this->fnBias + colorDeltaCvtPpemRangeFn);
					else
						swprintf(buf,L"CALL[], %hi, %li, %hi, %hi, %hi, %hi",knot,amount,ppemRangeLow[size],ppemRangeHigh[size],ByteOfDeltaColor(color),this->fnBias + colorDeltaKnotPpemRangeFn);
			this->Emit(buf);
		}
	}
	if (singlePpemSizes > 0) {
		charFn = cvt ? L'C' : L'P';
		if (this->deltaShift != shift) { // resolution is not the right one for this step
			this->deltaShift = shift;
			swprintf(buf,L"SDS[], %hi",this->deltaShift); this->Emit(buf);
		}
		size = 0;
		while (size < singlePpemSizes) { // while there are ppems to do
			if (singlePpemSize[size] < this->deltaBase || this->deltaBase + 47 < singlePpemSize[size]) { // ppem is outside range "addressable" by DLTP1, DLTP2, or DLTP3
				this->deltaBase = singlePpemSize[size];
				swprintf(buf,L"SDB[], %hi",this->deltaBase); this->Emit(buf);
			}
			deltaSubBase = this->deltaBase;
			while (size < singlePpemSizes && deltaSubBase <= this->deltaBase + 47) { // while there are ppems to do and they are "addressable" by DLTP1, DLTP2, or DLTP3
				if (size < singlePpemSizes && singlePpemSize[size] <= deltaSubBase + 15) { // if there are ppems to do which are "addressable" by "current" DLTPi
					swprintf(buf,L"DLT%c%hi[",charFn,((deltaSubBase-this->deltaBase) >> 4) + 1); deltas = 0;
					do {
						if (deltas == 4) { this->Emit(buf); swprintf(buf,L"      "); deltas = 0; }
						swprintf(&buf[STRLENW(buf)],L"(%hi @%hi %hi)",knot,singlePpemSize[size]-deltaSubBase,magnitude); deltas++;
						size++;
					} while (size < singlePpemSizes && singlePpemSize[size] <= deltaSubBase + 15); // while there are ppems to do which are "addressable" by "current" DLTPi
					swprintf(&buf[STRLENW(buf)],L"]");
					this->Emit(buf);
				}
				deltaSubBase += 16;
			}
		}
	}
} // TTSourceEngine::DLT

void TTSourceEngine::CALL24(short leftCvt, short rightCvt) {
	wchar_t buf[32];
	
	swprintf(buf,L"CALL[], %hi, %hi, %hi",leftCvt,rightCvt,this->fnBias + 24); this->Emit(buf);
} // TTSourceEngine::CALL24

void TTSourceEngine::CALL3456(short type, short knot3, short cvt3, short knot2, short cvt2, short knot1, short cvt1) {
	wchar_t buf[64];
	
	switch (type) {
		case 34:
		case 35:
			swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi, %hi",knot3,cvt3,cvt2,knot1,cvt1,this->fnBias + type);
			break;
		case 36:
			swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi, %hi, %hi",knot3,cvt3,knot2,cvt2,knot1,cvt1,this->fnBias + type);
			break;
	}
	this->Emit(buf);
	this->ttv[fv].dir = yRomanDir; this->ttv[fv].from = this->ttv[fv].to = illegalKnotNum;
	this->ttv[pv] = this->ttv[fv];
	this->rp[0] = knot1; this->lastChild = illegalKnotNum;
	this->rp[1] = knot1;
	this->rp[2] = knot3;
} // TTSourceEngine::CALL3456

void TTSourceEngine::CALL64(short parent, short child, short cvt, bool half, bool flip) {
	wchar_t buf[64];
	
 	swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi, %hi",parent,child,cvt,half,flip,this->fnBias + 64); this->Emit(buf);
	this->rp[0] = this->rp[1] = parent; this->lastChild = illegalKnotNum;
	this->rp[2] = child;
} // TTSourceEngine::CALL64

void TTSourceEngine::CALL656(bool crissCrossLinks, short knot0, short knot1, short knot2, short knot3, short cvt, short storage, bool xLinks, bool flip) {
	wchar_t buf[64];
	TTVectorDesc *v;
	
	swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi, %hi, %hi, %hi, %hi",knot0,knot1,knot2,knot3,cvt,storage,(short)xLinks,(short)flip,this->fnBias + 65 + (short)crissCrossLinks); this->Emit(buf);
	this->rp[0] = this->rp[1] = this->rp[2] = illegalKnotNum; this->lastChild = illegalKnotNum;
	v = &this->ttv[pv]; v->dir = diagDir; v->from = v->to = illegalKnotNum; // assume any kind of side-effect, cf. InitTTEngine(true)
	this->ttv[fv] = this->ttv[pv];
} // TTSourceEngine::CALL656

void TTSourceEngine::CALL678(bool back, short knot, short sameSide, short cvt, short storage) {
	wchar_t buf[64];
	TTVectorDesc *v;
	
	if (back) swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi",knot,sameSide,storage,this->fnBias + 68);
	else swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi",knot,sameSide,cvt,storage,this->fnBias + 67);
	this->Emit(buf);
	v = &this->ttv[pv]; v->dir = diagDir; v->from = v->to = illegalKnotNum; // assume any kind of side-effect, cf. InitTTEngine(true)
	this->ttv[fv] = this->ttv[pv];
	this->ttv[pv].dir = yRomanDir; // no reference points changed
} // TTSourceEngine::CALL678

void TTSourceEngine::CALL012345(short type, short knot0, short knot1, short knot2, short cvt) {
	wchar_t buf[64];
	
	swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi",knot0,knot1,knot2,cvt,this->fnBias + type); this->Emit(buf);
	// no side-effects
} // TTSourceEngine::CALL012345

void TTSourceEngine::CALL6(short knots, short knot[], short targetKnot) {
	short i;
	wchar_t buf[256];
	
	if (knots > 1) {
		swprintf(buf,L"LOOPCALL[]");
		for (i = 0; i < knots; i++) swprintf(&buf[STRLENW(buf)],L", %hi, %hi",knot[i],targetKnot);
		swprintf(&buf[STRLENW(buf)],L", %hi, %hi",knots,this->fnBias + 6);
	} else if (knots == 1)
		swprintf(buf,L"CALL[], %hi, %hi, %hi",knot[0],targetKnot,this->fnBias + 6);
	if (knots > 0) {
		this->rp[0] = targetKnot; this->lastChild = illegalKnotNum;
		this->ttv[fv].dir = yRomanDir; this->ttv[fv].from = this->ttv[fv].to = illegalKnotNum;
		this->ttv[pv] = this->ttv[fv];
		this->Emit(buf);
	}
} // TTSourceEngine::CALL6

void TTSourceEngine::CALL378(short type, short targetKnot) {
	wchar_t buf[32];
	
	swprintf(buf,L"CALL[], %hi, %hi",targetKnot,this->fnBias + type); this->Emit(buf);
	this->rp[0] = type == 37 ? targetKnot + 1 : targetKnot - 1; this->lastChild = illegalKnotNum;
	this->ttv[fv].dir = yRomanDir; this->ttv[fv].from = this->ttv[fv].to = illegalKnotNum;
	this->ttv[pv] = this->ttv[fv];
} // TTSourceEngine::CALL378

void TTSourceEngine::CALL(short actParams, short anyNum[], short functNum) {
	wchar_t buf[8*maxParams];
	short i;
	
	swprintf(buf,L"CALL[]");
	for (i = 0; i < actParams; i++) swprintf(&buf[STRLENW(buf)],L", %hi",anyNum[i]);
	swprintf(&buf[STRLENW(buf)],L", %hi",functNum); // here functNum is passed explicitly as a parameter, hence we expect caller to correctly bias function number
	this->Emit(buf);
	this->InitTTEngineState(false);
} // TTSourceEngine::CALL

void TTSourceEngine::CALL88(short riseCvt, short runCvt) {
	wchar_t buf[32];
	
	swprintf(buf,L"CALL[], %hi, %hi, %hi",riseCvt,runCvt,this->fnBias + italicRiseRunFn);
	this->Emit(buf);
	this->rp[0] = this->rp[1] = illegalKnotNum; this->lastChild = illegalKnotNum;
	this->sRound = false; this->round = rtg;
	this->ttv[fv].dir = xRomanDir; this->ttv[fv].from = this->ttv[fv].to = illegalKnotNum;
	this->ttv[pv] = this->ttv[fv];
} // TTSourceEngine::CALL88

void TTSourceEngine::ResMIAP(short child, short cvt) {
	wchar_t buf[64];
	
	swprintf(buf,L"CALL[], %hi, %hi, %hi",child,cvt,this->fnBias + resMIAPFn);
	this->Emit(buf);
	
	this->lastChild = illegalKnotNum; // MSIRP in fpgm, cannot patch code in fpgm...
	this->rp[0] = this->rp[1] = this->rp[2] = child; // MSIRP[M] used in fpgm
} // TTSourceEngine::ResMIAP

void TTSourceEngine::ResIPMDAP(TTVDirection pvP, bool postRoundFlag, short parent0, short child, short parent1) {
	wchar_t buf[64];
	short dirFlag;

	dirFlag = ((short)postRoundFlag)*8 + (short)pvP;	
	swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi",dirFlag,parent0,child,parent1,this->fnBias + resIPMDAPFn);
	this->Emit(buf);
	
	// SO FAR, WILL NEED TO MAKE MORE GENERAL TO ACCOMODATE fvP and fvC
	if (postRoundFlag) { // only allowed on ResXIPAnchor
		this->ttv[fv].dir = xRomanDir; this->ttv[pv].from = this->ttv[pv].to = illegalKnotNum;
		this->ttv[fv] = this->ttv[pv];
		this->rp[1] = child;
	} else {
		this->ttv[pv].dir = pvP; this->ttv[pv].from = this->ttv[pv].to = illegalKnotNum;
		this->ttv[fv] = this->ttv[pv];
		this->rp[1] = parent0;
	}
	this->usedpv = false;
	this->lastChild = illegalKnotNum;
	this->rp[0] = this->rp[2] = child;
	parent1;
} // TTSourceEngine::ResIPMDAP

void TTSourceEngine::ResMIRP(short parent, short child, short cvt, bool useMinDist) {
	bool useCvt;
	long pos;
	wchar_t buf[64];
	
	useCvt = cvt != illegalCvtNum;
	pos = swprintf(buf,L"CALL[], %hi, %hi",parent,child);
	if (useCvt) pos += swprintf(&buf[pos],L", %hi",cvt);
	pos += swprintf(&buf[pos],L", %hi",this->fnBias + resMIRPFn00 + 2*(long)useCvt + (long)useMinDist);

	this->Emit(buf);
	
	this->lastChild = illegalKnotNum; // MSIRP in fpgm, cannot patch code in fpgm...
	this->rp[1] = parent;
	this->rp[0] = this->rp[2] = child; // MSIRP[M] used in fpgm
} // TTSourceEngine::ResMIRP

void TTSourceEngine::ResIPMIRP(TTVDirection pvGP, short strokeOptimizationFlag, short grandParent0, short parent, short child, short cvt, short grandParent1) {
	wchar_t buf[64];
	
	swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi, %hi, %hi, %hi",pvGP,grandParent0,parent,child,cvt,grandParent1,strokeOptimizationFlag,this->fnBias + resIPMIRPFn);
	this->Emit(buf);
	
	this->ttv[pv].dir = (TTVDirection)((long)pvGP % 2); this->ttv[pv].from = this->ttv[pv].to = illegalKnotNum;
	this->ttv[fv] = this->ttv[pv]; // SO FAR, WILL NEED TO MAKE MORE GENERAL TO ACCOMODATE fvP and fvC
	this->usedpv = false;
	this->lastChild = illegalKnotNum;
	this->rp[0] = this->rp[1] = this->rp[2] = parent; // parent SRP0[]ed and MSIRP[m]ed last in emulate interpolation of median
} // TTSourceEngine::ResIPMIRP

void TTSourceEngine::ResDDMIRP(short parent0, short child0, TTVectorDesc fv0, short cvt0, short parent1, short child1, TTVectorDesc fv1, short cvt1) {
	wchar_t buf[128];
	long pos;
	
	// CAUTION: this scheme doesn't support setting the fv PERPENDICULAR to a line (so far it doesn't need to)
	pos = swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi, %hi, ",parent0,child0,parent1,child1,cvt0,cvt1);
	if (fv0.dir <= yRomanDir && fv1.dir <= yRomanDir) // simple case
		swprintf(&buf[pos],L"%hi, %hi, %hi",(short)fv0.dir,(short)fv1.dir,this->fnBias + resDDMIRP0Fn);
	else if (fv0.dir <= yRomanDir) // fv1.type set to line
		swprintf(&buf[pos],L"%hi, %hi, %hi, %hi",(short)fv0.dir,fv1.from,fv1.to,this->fnBias + resDDMIRP1Fn);
	else if (fv1.dir <= yRomanDir) // fv0.type set to line
		swprintf(&buf[pos],L"%hi, %hi, %hi, %hi",fv0.from,fv0.to,(short)fv1.dir,this->fnBias + resDDMIRP2Fn); 
	else // both fv0.type and fv1.type set to line
		swprintf(&buf[pos],L"%hi, %hi, %hi, %hi, %hi",fv0.from,fv0.to,fv1.from,fv1.to,this->fnBias + resDDMIRP3Fn); 
	this->Emit(buf);

	this->lastChild = illegalKnotNum;
	this->rp[0] = parent0; // this->rp[1] = this->rp[2] = illegalKnotNum; unchanged
	this->ttv[fv] = fv1;
	this->ttv[pv].dir = perpDiagDir; this->ttv[pv].from = parent0; this->ttv[pv].to = child1;
	this->usedpv = false;
} // TTSourceEngine::ResDDMIRP

void TTSourceEngine::ResIPDMIRP(TTVDirection pvGP, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1) {
	wchar_t buf[128];
	
	swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi, %hi, %hi, %hi, %hi, %hi",pvGP,grandParent0,parent0,child0,cvt0,parent1,child1,cvt1,grandParent1,this->fnBias + resIPDMIRPFn);
	this->Emit(buf);

	this->ttv[pv].dir = (TTVDirection)((long)pvGP % 2); this->ttv[pv].from = this->ttv[pv].to = illegalKnotNum;
	this->ttv[fv] = this->ttv[pv]; // so far; may have to make more general to accomodate generalized freedom vectors used by the last knot moved, or have fn reset state to pvGP
	this->usedpv = false;
	this->lastChild = illegalKnotNum;
	this->rp[0] = this->rp[1] = this->rp[2] = parent1; // parent1 SRP0[]ed and MSIRP[m]ed last in symDistFn
} // TTSourceEngine::ResIPDMIRP

void TTSourceEngine::ResIPDDMIRP(TTVDirection pvGP, short grandParent0, short parent0, short child0, TTVectorDesc fv0, short cvt0, short parent1, short child1, TTVectorDesc fv1, short cvt1, short grandParent1) {
	wchar_t buf[128];
	long pos;
	
	// CAUTION: this scheme doesn't support setting the fv PERPENDICULAR to a line (so far it doesn't need to)
	pos = swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi, %hi, %hi, %hi, %hi, ",pvGP,grandParent0,parent0,child0,cvt0,parent1,child1,cvt1,grandParent1);
	if (fv0.dir < diagDir && fv1.dir < diagDir) // simple case
		swprintf(&buf[pos],L"%hi, %hi, %hi",(long)fv0.dir,(long)fv1.dir,this->fnBias + resIPDDMIRPGlue0Fn);
	else if (fv1.dir < diagDir) // fv0.type set to line
		swprintf(&buf[pos],L"%hi, %hi, %hi, %hi",fv0.from,fv0.to,(long)fv1.dir,this->fnBias + resIPDDMIRPGlue1Fn);
	else if (fv0.dir < diagDir) // fv1.type set to line
		swprintf(&buf[pos],L"%hi, %hi, %hi, %hi",(long)fv0.dir,fv1.from,fv1.to,this->fnBias + resIPDDMIRPGlue2Fn); 
	else // both fv0.type and fv1.type set to line
		swprintf(&buf[pos],L"%hi, %hi, %hi, %hi, %hi",fv0.from,fv0.to,fv1.from,fv1.to,this->fnBias + resIPDDMIRPGlue3Fn); 
	this->Emit(buf);

	this->ttv[fv] = fv1;
	this->ttv[pv].dir = perpDiagDir; this->ttv[pv].from = parent0; this->ttv[pv].to = parent1;
	this->usedpv = false;
	this->lastChild = illegalKnotNum;
	this->rp[0] = this->rp[1] = this->rp[2] = child1; // child1 MSIRP[m]ed last to re-center the median after asymmetric linking
} // TTSourceEngine::ResIPDDMIRP

void TTSourceEngine::ResIIPDMIRP(short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1) {
	wchar_t buf[128];
	
	swprintf(buf,L"CALL[], %hi, %hi, %hi, %hi, %hi, %hi, %hi, %hi, %hi",grandParent0,grandParent1,parent0,parent1,child0,child1,cvt0,cvt1,this->fnBias + resIIPDMIRPFn);
	this->Emit(buf);

	this->lastChild = illegalKnotNum;

	this->rp[0] = this->rp[1] = this->rp[2] = illegalKnotNum; // because don't know which branch taken

	this->round = rtg;
	this->ttv[pv].dir = diagDir; this->ttv[pv].from = this->ttv[pv].to = illegalKnotNum; // branch may or may not have been taken
	this->ttv[fv].dir = xRomanDir; this->ttv[fv].from = this->ttv[fv].to = illegalKnotNum;
	this->usedpv = false;
} // TTSourceEngine::ResIIPDMIRP

void TTSourceEngine::ASM(bool inLine, wchar_t text[]) {
	this->Emit(text);
	if (inLine) this->InitTTEngineState(false);
} // TTSourceEngine::ASM

void TTSourceEngine::INSTCTRL(short fromPpem, short toPpem) {
	wchar_t code[512];

	// GETINFO 6 below corresponding to ROTATEDINTERPRETERQUERY | STRETCHEDINTERPRETERQUERY, cf. Interp.c
	// turn off instructions if outside ppem range or rasterizer version >= 1.8 and (rotated or stretched)
	swprintf(code,L"#PUSHOFF"        BRK
				  L"MPPEM[]"         BRK
				  L"#PUSH, %hi"      BRK
				  L"GT[]"            BRK
				  L"MPPEM[]"         BRK
				  L"#PUSH, %hi"      BRK
				  L"LT[]"            BRK
			 	  L"OR[]"            BRK
			 	  L"#PUSH, 1"        BRK
				  L"GETINFO[]"       BRK
				  L"#PUSH, 37"       BRK
				  L"GTEQ[]"          BRK
				  L"#PUSH, 1"        BRK
				  L"GETINFO[]"       BRK
				  L"#PUSH, 64"       BRK
				  L"LTEQ[]"          BRK
				  L"AND[]"           BRK
				  L"#PUSH, 6"        BRK
				  L"GETINFO[]"       BRK
				  L"#PUSH, 0"	     BRK
				  L"NEQ[]"           BRK
				  L"AND[]"           BRK
				  L"OR[]"            BRK
				  L"IF[]"            BRK
				  L"    #PUSH, 1, 1" BRK
				  L"    INSTCTRL[]"  BRK
				  L"EIF[]"           BRK
				  L"#PUSHON"	     BRK,toPpem,fromPpem);

	this->Emit(code);
} // TTSourceEngine::INSTCTRL

void TTSourceEngine::SCANCTRL(short ctrl) {
	wchar_t code[maxLineSize];

	swprintf(code,L"SCANCTRL[], %hi" BRK,ctrl);
	this->Emit(code);
} // TTSourceEngine::SCANCTRL

void TTSourceEngine::SCANTYPE(short type) {
	long pos;
	wchar_t code[maxLineSize];

	pos = type <= 4 ? 0 : swprintf(code,L"SCANTYPE[], %hi" BRK,type-4); // Mac rasterizer doesn't handle types > 4
	swprintf(&code[pos],L"SCANTYPE[], %hi" BRK,type);
	this->Emit(code);
} // TTSourceEngine::SCANTYPE

void TTSourceEngine::SCVTCI(short numCvtCutIns, short cvtCutInPpemSize[], F26Dot6 cvtCutInValue[]) {
	short cvtCutIn;
	wchar_t code[maxLineSize];

	if(this->legacyCompile)
	{
		swprintf(code, L"WS[], 22, 1 /* s[22] = diagonal control: on (by default) */"); this->Emit(code);
	}

	if (numCvtCutIns > 0) {
		swprintf(code,L"SCVTCI[], %li",cvtCutInValue[0]); this->Emit(code);
		if (!this->legacyCompile)
		{
			swprintf(code, L"WS[], %li, %li", cvtCutInStorage, cvtCutInValue[0]); this->Emit(code);
		}
	}
	if (numCvtCutIns > 1) {
		this->Emit(L"#PUSHOFF");
		cvtCutIn = 1;
		while (cvtCutIn < numCvtCutIns) {

			if (!this->legacyCompile)
			{

				swprintf(code, L"MPPEM[]" BRK
					L"#PUSH, %hi" BRK
					L"GTEQ[]" BRK
					L"IF[]" BRK
					L"#PUSH, %li, %li, %li" BRK
					L"SCVTCI[]" BRK
					L"WS[]", cvtCutInPpemSize[cvtCutIn], cvtCutInStorage, cvtCutInValue[cvtCutIn], cvtCutInValue[cvtCutIn]);
			}
			else
			{
				swprintf(code,L"MPPEM[]" BRK
					L"#PUSH, %hi" BRK
					L"GTEQ[]" BRK
					L"IF[]" BRK
					L"#PUSH, %li" BRK
					L"SCVTCI[]",cvtCutInPpemSize[cvtCutIn],cvtCutInValue[cvtCutIn]);
			}

			this->Emit(code);
			cvtCutIn++;
			if (this->legacyCompile && cvtCutIn == numCvtCutIns) {
				swprintf(code,L"#PUSH, 22, 0" BRK
							  L"WS[] /* s[22] = diagonal control: off (by now) */"); this->Emit(code);
			}
			this->Emit(	 L"EIF[]");
		}
		this->Emit(L"#PUSHON");
		this->Emit(L"");
	}
} // TTSourceEngine::SCVTCI

void TTSourceEngine::SetAspectRatioFlag(void) {
	wchar_t code[maxLineSize];

	this->Emit(L"/* Square aspect ratio? */");
	swprintf(code,L"CALL[], %hi",this->fnBias + aspectRatioFn); this->Emit(code);
	this->Emit(L"");
	this->ttv[fv].dir = xRomanDir; this->ttv[fv].from = this->ttv[fv].to = illegalKnotNum;
	this->ttv[pv] = this->ttv[fv]; this->ttv[pv].dir = yRomanDir;
	this->usedpv = false;
} // TTSourceEngine::SetAspectRatioFlag

void TTSourceEngine::SetGreyScalingFlag(void) {
	wchar_t code[maxLineSize];

	this->Emit(L"/* Grey scaling? */");
	swprintf(code,L"CALL[], %hi",this->fnBias + greyScalingFn); this->Emit(code);
	this->Emit(L"");
} // TTSourceEngine::SetGreyScalingFlag

void TTSourceEngine::SetClearTypeCtrl(short ctrl) {
	wchar_t code[maxLineSize];

	if (ctrl > 0) {
		swprintf(code,L"#PUSHOFF"          BRK
					  L"#PUSH, 2, 2"       BRK
					  L"RS[]"              BRK
					  L"LTEQ[]"            BRK
					  L"IF[]"              BRK
					  L"    #PUSH, %li, 3" BRK
					  L"    INSTCTRL[]"    BRK
					  L"EIF[]"             BRK
					  L"#PUSHON"           BRK,4);
	} else {
		swprintf(code,L"/* (font not tuned for ClearType) */" BRK);
	}
	this->Emit(code);
} // TTSourceEngine::SetClearTypeCtrl

void TTSourceEngine::CvtRegularization(bool relative, short cvtNum, short breakPpemSize, short parentCvtNum) {
	wchar_t code[maxLineSize];

	if (relative) {
		swprintf(code,L"CALL[], %hi, %hi, %hi, %hi",cvtNum,breakPpemSize,parentCvtNum,this->fnBias + doubleHeightFn);
		this->sRound = false; this->round = rnone; // don't know which branch was taken...
	} else {
		swprintf(code,L"CALL[], %hi, %hi, %hi, %hi",cvtNum,parentCvtNum,breakPpemSize,this->fnBias + singleWeightFn);
	}
	this->Emit(code);
} // TTSourceEngine::CvtRegularization

void TTSourceEngine::ResetPrepState(void) {
	wchar_t code[16];

	if (this->round != rtg) {
		this->sRound = false; this->round = rtg;
		this->Emit(L"RTG[]");
	}
	if (this->deltaShift != defaultDeltaShift) {
		this->deltaShift = defaultDeltaShift;
		swprintf(code,L"SDS[], %hi",this->deltaShift); this->Emit(code);
	}
	if (this->deltaBase != defaultDeltaBase) {
		this->deltaBase = defaultDeltaBase;
		swprintf(code,L"SDB[], %hi",this->deltaBase); this->Emit(code);
	}
} // TTSourceEngine::ResetPrepState

void TTSourceEngine::SetFunctionNumberBias(short bias) {
	this->fnBias = Max(0,bias);
} // TTSourceEngine::SetFunctionNumberBias

short TTSourceEngine::GetFunctionNumberBias(void) {
	return this->fnBias;
} // TTSourceEngine::GetFunctionNumberBias

void TTSourceEngine::InitTTEngine(bool legacyCompile, bool *memError) {
	this->error = false;
	
	this->InitTTEngineState(true);
	this->fnBias = 0;
	this->bufPos = 0;
	this->bufLen = minBufLen;
	this->buf = (wchar_t*)NewP(this->bufLen * sizeof(wchar_t));
	this->error = this->buf == NULL;
	if (!this->error) this->buf[this->bufPos] = 0; // add trailing 0 assuming minBufLen > 0
	this->mov[false] = L'm'; this->min[false] = L'<'; this->rnd[false] = L'r';
	this->mov[true]  = L'M'; this->min[true]  = L'>'; this->rnd[true]  = L'R';
	this->col[linkAnyColor][0] = L' '; this->col[linkGrey][0] = L'G'; this->col[linkBlack][0] = L'B'; this->col[linkWhite][0] = L'W';
	this->col[linkAnyColor][1] = L' '; this->col[linkGrey][1] = L'r'; this->col[linkBlack][1] = L'l'; this->col[linkWhite][1] = L'h';
	this->legacyCompile = legacyCompile; 
	*memError = this->error;
} // TTSourceEngine::InitTTEngine

void TTSourceEngine::TermTTEngine(TextBuffer *glyfText, bool *memError) {
	if (!this->error && glyfText) {
		glyfText->Delete(0,glyfText->TheLength());
		glyfText->AppendRange(this->buf,0,this->bufPos);
	}
	if (this->buf != NULL) DisposeP((void**)&this->buf);
	*memError = this->error;
} // TTSourceEngine::TermTTEngine

TTSourceEngine::TTSourceEngine(void) { /* nix */ } /* TTSourceEngine::TTSourceEngine */
TTSourceEngine::~TTSourceEngine(void) { /* nix */ } /* TTSourceEngine::~TTSourceEngine */

void TTSourceEngine::InitTTEngineState(bool forNewGlyph) {
	if (forNewGlyph) { // at start of glyf program, assume TrueType engine's defaults
		this->rp[0] = this->rp[1] = this->rp[2] = 0;
		this->minDist = one6;
		this->sRound = false;
		this->round = rtg;
		this->ttv[fv].dir = xRomanDir; this->ttv[fv].from = this->ttv[fv].to = illegalKnotNum;
		this->ttv[pv] = this->ttv[fv];
		this->usedpv = false;
		this->autoFlip = true;
		this->deltaBase = defaultDeltaBase; this->deltaShift = defaultDeltaShift;
		this->lastChild = illegalKnotNum;
	} else { // after Call or Inline, assume any kind of side-effect
		this->rp[0] = this->rp[1] = this->rp[2] = illegalKnotNum;
		this->minDist = -1;
		this->sRound = false;
		this->round = rnone;
		this->ttv[fv].dir = diagDir; this->ttv[fv].from = this->ttv[fv].to = illegalKnotNum;
		this->ttv[pv] = this->ttv[fv];
		this->usedpv = false; // ?
		this->autoFlip = (bool)-1;
		this->deltaBase = -48; this->deltaShift = -1;
		this->lastChild = illegalKnotNum;
	}
} // TTSourceEngine::InitTTEngineState


void GenGuardCond(TextBuffer *text, AltCodePath path) {
	wchar_t codePath[32];

	path = (AltCodePath)Min(Max(firstAltCodePath,path),lastAltCodePath);

	swprintf(codePath,L"#PUSH, %li, 2",path); text->AppendLine(codePath);
	text->AppendLine(L"RS[]");
	swprintf(codePath,L"%sEQ[]",path < altCodePathMonochromeOnly ? L"N" : L"LT"); text->AppendLine(codePath);
} // GenGuardCond

void GenTalkIf(TextBuffer *talk, AltCodePath path, long fpgmBias) {
	wchar_t codePath[32];

	talk->AppendLine(L"Inline(\x22");
	talk->AppendLine(L"#PUSHOFF");
	GenGuardCond(talk,path);
	talk->AppendLine(L"IF[]");
	talk->AppendLine(L"#PUSHON");
	talk->AppendLine(L"#BEGIN");
	talk->AppendLine(L"\x22)");
	talk->AppendLine(L"");
	swprintf(codePath,L"BeginCodePath(%li)",fpgmBias); talk->AppendLine(codePath);
	talk->AppendLine(L"");
} // GenTalkIf

void GenTalkElse(TextBuffer *talk, long fpgmBias) {
	wchar_t codePath[32];

	talk->AppendLine(L"");
	talk->AppendLine(L"EndCodePath()");
	talk->AppendLine(L"");
	talk->AppendLine(L"Inline(\x22");
	talk->AppendLine(L"#END");
	talk->AppendLine(L"ELSE[]");
	talk->AppendLine(L"#BEGIN");
	talk->AppendLine(L"\x22)");
	talk->AppendLine(L"");
	swprintf(codePath,L"BeginCodePath(%li)",fpgmBias); talk->AppendLine(codePath);
	talk->AppendLine(L"");
} // GenTalkElse

void GenTalkEndIf(TextBuffer *talk) {
	talk->AppendLine(L"");
	talk->AppendLine(L"EndCodePath()");
	talk->AppendLine(L"");
	talk->AppendLine(L"Inline(\x22");
	talk->AppendLine(L"#END");
	talk->AppendLine(L"EIF[]");
	talk->AppendLine(L"\x22)");
} // GenTalkEndIf

void GenPrepIf(TextBuffer *prep, AltCodePath path) {
	wchar_t codePath[32];

	prep->AppendLine(L"#PUSHOFF");
	swprintf(codePath,L"#PUSH, %li",greyScalingFn); prep->AppendLine(codePath);
	prep->AppendLine(L"CALL[]");
	GenGuardCond(prep,path);
	prep->AppendLine(L"IF[]");
	prep->AppendLine(L"#PUSHON");
	prep->AppendLine(L"#BEGIN");
	prep->AppendLine(L"");
} // GenPrepIf

void GenPrepElse(TextBuffer *prep) {
	prep->AppendLine(L"");
	prep->AppendLine(L"#END");
	prep->AppendLine(L"ELSE[]");
	prep->AppendLine(L"#BEGIN");
	prep->AppendLine(L"");
} // GenPrepElse

void GenPrepEndIf(TextBuffer *prep) {
	prep->AppendLine(L"");
	prep->AppendLine(L"#END");
	prep->AppendLine(L"EIF[]");
} // GenPrepEndIf


TTEngine *NewTTSourceEngine(void) {
	return new TTSourceEngine;
}