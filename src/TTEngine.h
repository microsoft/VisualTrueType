// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef TTEngine_dot_h
#define TTEngine_dot_h

#include "MathUtils.h" // Vector etc.
#include "GUIDecorations.h" // DeltaColor

#define FIRSTTMPCVT 40
#define SECONDTMPCVT (FIRSTTMPCVT + 1) // used by function 18 and TTSourceEngine::MIRP

#define maxParams	64L
#define maxPpemSize 256L
#define maxMinDist	2L /* implementation restriction of TTEngine::AssertMinDist */

#define stdDeltaShift 3 // corresponding to stdDeltaResolution below
#define stdDeltaResolution (one6 >> stdDeltaShift) // 1/8th of a pixel
#define stdDeltaBase 9

#define maxPixelValue 8L*one6 /* corresponding to a magnitude 8 and deltaShift 1/2^0 */

#define maxAsmSize 0x2000L

#define illegalKnotNum (-1)

typedef enum {rthg = 0, rtdg, rtg, rdtg, rutg, roff, rnone} Rounding;

typedef enum {xRomanDir = 0, yRomanDir, xItalDir, yItalDir, xAdjItalDir, yAdjItalDir, diagDir, perpDiagDir} TTVDirection; // ordered
#define firstTTVDirection xRomanDir
#define lastTTVDirection perpDiagDir
#define numTTVDirections (lastTTVDirection - firstTTVDirection + 1)
// xItalDir and xAdjItalDir correspond to the respective y directions minus 90 degrees, diagDir and perpDiagDir are for arbitrary S(FV|PV)TLs

typedef enum {fv = 0, pv, dpv} TTVector;

typedef struct {
	TTVDirection dir;
	short from,to;
} TTVectorDesc;

DeltaColor DeltaColorOfByte(unsigned char byte);
DeltaColor DeltaColorOfOptions(bool grayScale, bool clearType, bool clearTypeCompWidth /* , bool clearTypeVertRGB */, bool clearTypeBGR, bool clearTypeFract, bool clearTypeYAA, bool clearTypeGray);
unsigned char ByteOfDeltaColor(DeltaColor color);
char *AllDeltaColorBytes(void);

typedef enum { noAltCodePath = -1, altCodePathBWOnly = 0, altCodePathGreyOnly = 1, altCodePathMonochromeOnly = 2} AltCodePath;
#define firstAltCodePath altCodePathBWOnly
#define lastAltCodePath  altCodePathMonochromeOnly
#define numAltCodePaths (lastAltCodePath - firstAltCodePath + 1)

class TTEngine {
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
	virtual void CALL3456(short type, short knot3, short cvt3, short knot2, short cvt2, short knot1, short cvt1); // upright serifs with horizontal base
	virtual void CALL64(short parent, short child, short cvt, bool half, bool flip); // "special MIRP" for new italic strokes
	virtual void CALL656(bool crissCrossLinks, short knot0, short knot1, short knot2, short knot3, short cvt, short storage, bool xLinks, bool flip); // new diagonal stroke
	virtual void CALL678(bool back, short knot, short sameSide, short cvt, short storage); // new italic strokes: extrapolate knot to cvt height or back again
	virtual void CALL012345(short type, short knot0, short knot1, short knot2, short cvt); // cf. fdefs.0
	virtual void CALL6(short knots, short knot[], short targetKnot); // cf. fdefs.0
	virtual void CALL378(short type, short targetKnot);
	virtual void CALL88(short riseCvt, short runCvt);
	virtual void ResMIAP(short child, short cvt);
	virtual void ResIPMDAP(TTVDirection pvP, bool postRoundFlag, short parent0, short child, short parent1);
	virtual void ResMIRP(short parent, short child, short cvt, bool useMinDist);
	virtual void ResIPMIRP(TTVDirection pvGP, short strokeOptimizationFlag, short grandParent0, short parent, short child, short cvt, short grandParent1);
	virtual void ResDDMIRP(short parent0, short child0, TTVectorDesc fv0, short cvt0, short parent1, short child1, TTVectorDesc fv1, short cvt1);
	virtual void ResIPDMIRP(TTVDirection pvGP, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1);
	virtual void ResIPDDMIRP(TTVDirection pvGP, short grandParent0, short parent0, short child0, TTVectorDesc fv0, short cvt0, short parent1, short child1, TTVectorDesc fv1, short cvt1, short grandParent1);
	virtual void ResIIPDMIRP(short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1);
	virtual void CALL(short actParams, short anyNum[], short functNum);
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
	TTEngine(void);
	virtual ~TTEngine(void);
};


void GenTalkIf(TextBuffer *talk, AltCodePath path, long fpgmBias);
void GenTalkElse(TextBuffer *talk, long fpgmBias);
void GenTalkEndIf(TextBuffer *talk);
void GenPrepIf(TextBuffer *prep, AltCodePath path);
void GenPrepElse(TextBuffer *prep);
void GenPrepEndIf(TextBuffer *prep);

TTEngine *NewTTSourceEngine(void);

/***** as a matter of centralization, it might be cleaner to integrate (most of) fdefs0|1|2.c
into (an extended version of) TTEngine, to properly encapsulate the graphic state and whatever
side-effect updates required as a result of a function call... *****/

#endif // TTEngine_dot_h