/*****

	TTGenerator.c - New TypeMan Talk Compiler - Code Generators

	Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

*****/
#define _CRT_SECURE_NO_DEPRECATE 
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <stdio.h>
#include <string.h>
#include "pch.h"

//#define maxBitsKnot 12 /* depends on MAXPOINTS, which is (currently) 2048... */
//#define knotMask	4095L
//#define maxBitsCvt	16 /* cvt numbers seem to be 16bit-integers */
//#define cvtMask		65535L /* allow for -1 */
#define strokeFudge STRAIGHTANGLEFUDGE /* = 1.5 degrees */
#define serifFudge	5 /* degrees */
#define diagFudge	5 /* degrees */
#define diagFudgeMT	5 /* degrees */
#define noJunction	25 /* degrees */
#define gmndItFudge 12
#define neighFudge	7.5 /* degrees */
#define vacuFudge(font) ((this)->emHeight/200)

#define LSBTMP (FIRSTTMPCVT + 3)
#define RSBTMP (FIRSTTMPCVT + 4)
#define RESERVED_HEIGHTSPACE_START 36
#define ALIGN_TOLERANCE 0.5 // degrees
#define MAXSTROKESIDEANGLEDIFFERENCE 5.0 /* degrees */
#define ND_HEIGHT_STORE_1	23
#define ND_ITALIC_STORE_1	43
#define MAXIMUM_ITALIC_ANGLE_DEVIATION 3.5
#define MAIN_STROKE_IS_ONE_PIXEL_STORAGE 8	/* boolean	*/

#define maxVacuForms 16L

typedef struct {
	Rounding round[2];	// xRomanDir, yRomanDir
	short cvt;			// YAnchor's cvt for IStroke optimization
	bool touched[2]; // xRomanDir, yRomanDir, have to keep track of for FixDStrokes (?)
	bool dStroke;	// part of a Stroke which is neither diagonal nor horizontal
	bool iStroke;	// part of a Stroke which is sufficiently close to the main stroke angle
	bool on;			// on-curve point
	bool vacu;		// temp use in VacuFormRound
	short deltaAngle;	// (rounded) degrees, left turn => positive, right turn => negative, straight: 0
} Attribute;
typedef Attribute* AttributePtr;

typedef struct {
	short type;
	short radius;
	short cvt;
	bool forward[2];
	short knot[4];
} VacuFormParams;

typedef Vector* VectorPtr;

const Rounding deltaRounding[roff-rthg+1][roff-rthg+1] = {{rtg,  rtdg, rthg, roff, roff, roff},
														  {roff, rtdg, roff, roff, roff, roff},
														  {rthg, rtdg, rtg,  rdtg, rutg, roff},
														  {rthg, rtdg, rtg,  rdtg, rutg, roff},
														  {rthg, rtdg, rtg,  rdtg, rutg, roff},
														  {roff, roff, roff, roff, roff, roff}};

const F26Dot6 defaultMinDistAmount[roff-rthg+1][roff-rthg+1] = {{one6,  half6, half6, half6, half6, 0},
																{half6, half6, half6, half6, half6, 0},
																{half6, half6, one6,  0,     one6,  0},
																{half6, half6, one6,  0,     one6,  0},
																{half6, half6, one6,  0,     one6,  0},
																{0,     0,     0,     0,     0,     0}};

void TTGenerator::MainStrokeAngle(short angle100, wchar_t error[]) { /* abstract */ }
void TTGenerator::GlyphStrokeAngle(short riseCvt, short runCvt, wchar_t error[]) { /* abstract */ }
void TTGenerator::SetRounding(bool y, Rounding round, short params, short param[]) { /* abstract */ }
void TTGenerator::SetItalicStroke(bool phase, wchar_t error[]) { /* abstract */ }
void TTGenerator::Anchor(bool y, ProjFreeVector *projFreeVector, short knot, short cvt, bool round, wchar_t error[]) { /* abstract */ }
void TTGenerator::GrabHereInX(short left, short right, wchar_t error[]) { /* abstract */ }
void TTGenerator::Link(bool y, bool dist, ProjFreeVector *projFreeVector, bool postRoundFlag, short parent, short child, CvtCategory category, short cvt, short minDists, short jumpPpemSize[], F26Dot6 pixelSize[], short *actualCvt, wchar_t error[]) { /* abstract */ }
void TTGenerator::Interpolate(bool y, ProjFreeVector *projFreeVector, bool postRoundFlag, short parent0, short children, short child[], short parent1, bool round, wchar_t error[]) { /* abstract */ }
void TTGenerator::BeginCodePath(short fpgmBias, wchar_t error[]) { /* abstract */ }
void TTGenerator::EndCodePath(wchar_t error[]) { /* abstract */ }
void TTGenerator::ResAnchor(bool y, ProjFreeVector *projFreeVector, short child, short cvt, wchar_t error[]) { /* abstract */ }
void TTGenerator::ResIPAnchor(bool y, ProjFreeVector *projFreeVector, bool postRoundFlag, short parent0, short child, short parent1, wchar_t error[]) { /* abstract */ }
void TTGenerator::ResLink(bool y, bool dist, ProjFreeVector *projFreeVector, short parent, short child, short cvt, short minDists, wchar_t error[]) { /* abstract */ }
void TTGenerator::ResIPLink(bool y, bool dist, ProjFreeVector *projFreeVector, short strokeOptimizationFlag, short grandParent0, short parent, short child, short cvt, short grandParent1, wchar_t error[]) { /* abstract */ }
void TTGenerator::ResDDLink(bool y, bool dist, ProjFreeVector *projFreeVector, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, wchar_t error[]) { /* abstract */ }
void TTGenerator::ResIPDLink(bool y, bool dist, ProjFreeVector *projFreeVector, short strokeOptimizationFlag, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1, wchar_t error[]) { /* abstract */ }
void TTGenerator::ResIPDDLink(bool y, bool dist, ProjFreeVector *projFreeVector, short strokeOptimizationFlag, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1, wchar_t error[]) { /* abstract */ }
void TTGenerator::ResIIPDLink(bool dist, ProjFreeVector *projFreeVector, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1, wchar_t error[]) { /* abstract */ }
void TTGenerator::Intersect(short intersection, short line0start, short line0end, short line1start, short line1end, short ppem0, short ppem1, wchar_t error[]) { /* abstract */ }
void TTGenerator::Align(FVOverride fvOverride, short parent0, short children, short child[], short parent1, short ppem, wchar_t error[]) { /* abstract */ }
void TTGenerator::Move(bool y, F26Dot6 amount, short knots, short knot[], wchar_t errMsg[]) { /* abstract */ }
void TTGenerator::Shift(bool y, ProjFreeVector *projFreeVector, short parent, short children, short child[], wchar_t error[]) { /* abstract */ }
void TTGenerator::Stroke(FVOverride fvOverride, bool leftStationary[], short knot[], short cvt, short ppem, short *actualCvt, wchar_t error[]) { /* abstract */ }
void TTGenerator::DStroke(bool leftStationary[], short knot[], short cvt, short *actualCvt, wchar_t error[]) { /* abstract */ }
void TTGenerator::IStroke(bool leftStationary[], short knot[], short height[], short phase, short cvt, short *actualCvt, wchar_t error[]) { /* abstract */ }
void TTGenerator::FixDStrokes(void) { /* abstract */ }
void TTGenerator::Serif(bool forward, short type, short knots, short knot[], wchar_t error[]) { /* abstract */ }
void TTGenerator::Scoop(short parent0, short child, short parent1, wchar_t error[]) { /* abstract */ }
void TTGenerator::Smooth(short y, short italicFlag) { /* abstract */ }
void TTGenerator::Delta(bool y, DeltaColor color, short knot, F26Dot6 amount, bool ppemSize[], wchar_t errMsg[]) { /* abstract */ }
void TTGenerator::VacuFormLimit(short ppem) { /* abstract */ }
void TTGenerator::VacuFormRound(short type, short radius, bool forward[], short knot[], wchar_t error[]) { /* abstract */ }
void TTGenerator::Call(short actParams, short anyNum[], short functNum) { /* abstract */ }
void TTGenerator::Asm(bool inLine, wchar_t text[], wchar_t error[]) { /* abstract */ }
void TTGenerator::Quit(void) { /* abstract */ }
void TTGenerator::InitTTGenerator(TrueTypeFont *font, TrueTypeGlyph *glyph, int32_t glyphIndex, TTEngine *tt, bool legacyCompile, bool *memError) { /* abstract */ }
void TTGenerator::TermTTGenerator(void) { /* abstract */ }
TTGenerator::TTGenerator(void) { /* abstract */ }
TTGenerator::~TTGenerator(void) { /* abstract */ }

class DiagParam : public ListElem {
public:
	DiagParam(void);
	virtual ~DiagParam(void);
	bool leftStationary[2];
	short knot[4];
};

class AlignParam : public ListElem {
public:
	AlignParam(void);
	virtual ~AlignParam(void);
	short parent0,parent1,children,child[maxParams];
};

DiagParam::DiagParam(void) {
	// nix, but have to have at least a (pair of) method(s) or else the compiler complains...
} // DiagParam::DiagParam

DiagParam::~DiagParam(void) {
	// nix, but have to have at least a (pair of) method(s) or else the compiler complains...
} // DiagParam::~DiagParam

AlignParam::AlignParam(void) {
	// nix, but have to have at least a (pair of) method(s) or else the compiler complains...
} // AlignParam::AlignParam

AlignParam::~AlignParam(void) {
	// nix, but have to have at least a (pair of) method(s) or else the compiler complains...
} // AlignParam::~AlignParam

typedef enum {fvOnX, fvOnY, fvOnPV, fvOnLine} FVMTDirection;

class TTSourceGenerator : public TTGenerator {
public:
	virtual void MainStrokeAngle(short angle100, wchar_t error[]);
	virtual void GlyphStrokeAngle(short riseCvt, short runCvt, wchar_t error[]);
	virtual void SetRounding(bool y, Rounding round, short params, short param[]);
	virtual void SetItalicStroke(bool phase, wchar_t error[]);
	virtual void Anchor(bool y, ProjFreeVector *projFreeVector, short knot, short cvt, bool round, wchar_t error[]);
	virtual void GrabHereInX(short left, short right, wchar_t error[]);
	virtual void Link(bool y, bool dist, ProjFreeVector *projFreeVector, bool postRoundFlag, short parent, short child, CvtCategory category, short cvt, short minDists, short jumpPpemSize[], F26Dot6 pixelSize[], short *actualCvt, wchar_t error[]);
	virtual void Interpolate(bool y, ProjFreeVector *projFreeVector, bool postRoundFlag, short parent0, short children, short child[], short parent1, bool round, wchar_t error[]);
	virtual void BeginCodePath(short fpgmBias, wchar_t error[]);
	virtual void EndCodePath(wchar_t error[]);
	virtual void ResAnchor(bool y, ProjFreeVector *projFreeVector, short child, short cvt, wchar_t error[]);
	virtual void ResIPAnchor(bool y, ProjFreeVector *projFreeVector, bool postRoundFlag, short parent0, short child, short parent1, wchar_t error[]);
	virtual void ResLink(bool y, bool dist, ProjFreeVector *projFreeVector, short parent, short child, short cvt, short minDists, wchar_t error[]);
	virtual void ResIPLink(bool y, bool dist, ProjFreeVector *projFreeVector, short strokeOptimizationFlag, short grandParent0, short parent, short child, short cvt, short grandParent1, wchar_t error[]);
	virtual void ResDDLink(bool y, bool dist, ProjFreeVector *projFreeVector, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, wchar_t error[]);
	virtual void ResIPDLink(bool y, bool dist, ProjFreeVector *projFreeVector, short strokeOptimizationFlag, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1, wchar_t error[]);
	virtual void ResIPDDLink(bool y, bool dist, ProjFreeVector *projFreeVector, short strokeOptimizationFlag, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1, wchar_t error[]);
	virtual void ResIIPDLink(bool dist, ProjFreeVector *projFreeVector, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1, wchar_t error[]);
	virtual void Intersect(short intersection, short line0start, short line0end, short line1start, short line1end, short ppem0, short ppem1, wchar_t error[]);
	virtual void Align(FVOverride fvOverride, short parent0, short children, short child[], short parent1, short ppem, wchar_t error[]);
	virtual void Move(bool y, F26Dot6 amount, short knots, short knot[], wchar_t errMsg[]);
	virtual void Shift(bool y, ProjFreeVector *projFreeVector, short parent, short children, short child[], wchar_t error[]);
	virtual void Stroke(FVOverride fvOverride, bool leftStationary[], short knot[], short cvt, short ppem, short *actualCvt, wchar_t error[]);
	virtual void DStroke(bool leftStationary[], short knot[], short cvt, short *actualCvt, wchar_t error[]);
	virtual void IStroke(bool leftStationary[], short knot[], short height[], short phase, short cvt, short *actualCvt, wchar_t error[]);
	virtual void FixDStrokes(void);
	virtual void Serif(bool forward, short type, short knots, short knot[], wchar_t error[]);
	virtual void Scoop(short parent0, short child, short parent1, wchar_t error[]);
	virtual void Smooth(short y, short italicFlag);
	virtual void VacuFormLimit(short ppem);
	virtual void VacuFormRound(short type, short radius, bool forward[], short knot[], wchar_t error[]);
	virtual void Delta(bool y, DeltaColor color, short knot, F26Dot6 amount, bool ppemSize[], wchar_t errMsg[]);
	virtual void Call(short actParams, short anyNum[], short functNum);
	virtual void Asm(bool inLine, wchar_t text[], wchar_t error[]);
	virtual void Quit(void);
	virtual void InitTTGenerator(TrueTypeFont *font, TrueTypeGlyph *glyph, int32_t glyphIndex, TTEngine *tt, bool legacyCompile, bool *memError);
	virtual void InitCodePathState(void);
	virtual void TermCodePathState(void);
	virtual void TermTTGenerator(void);
	TTSourceGenerator(void);
	virtual ~TTSourceGenerator(void);
private:
	ProjFreeVector xRomanPV,yRomanPV,xItalPV,yItalPV,xAdjItalPV,yAdjItalPV; // generalized projection freedom vectors used as actual parameters in frequent standard cases 
	RVector xAxis,yAxis,slope; // of italics, normalised
	short riseCvt,runCvt; // for GlyphStrokeAngle
	double cosF,tanF; // of strokeFudge
	double cosF0; // of MAXSTROKESIDEANGLEDIFFERENCE
	double cosF1; // of ALIGN_TOLERANCE
	double cosF2,sinF22; // of MAXIMUM_ITALIC_ANGLE_DEVIATION, 2*MAXIMUM_ITALIC_ANGLE_DEVIATION
	double sinF3; // of serifFudge
	double tanF4; // of diagFudge
	double cosF5; // of gmndItFudge
	double cosF6; // of neighFudge
	double cosMT; // of diagFudgeMT
	double tanMT; // of diagFudgeMT
	bool italic; // an italic font with mainStrokeAngle other than 90 degree ±STRAIGHTANGLEFUDGE
	bool mainStrokeAngle,glyphStrokeAngle,setItalicStrokePhase,setItalicStrokeAngle;
	bool xSmooth,ySmooth;
	short leftAnchor,rightAnchor; // for GrabHereInX
	TrueTypeFont *font;
	TrueTypeGlyph *glyph;
	int32_t glyphIndex,charCode,emHeight;
	CharGroup charGroup; // mapped from current glyph
	short knots;
	AttributePtr attrib; // allocate just as many as we need
	VectorPtr V; // here, too
	LinearListStruct *diagonals;
	LinearListStruct *aligns;
	short vacuFormLimit; // ppem size at which vacuforming is turned off
	short vacuForms;
	VacuFormParams vacuForm[maxVacuForms]; // but not here
	bool legacyCompile; 
	
	TTEngine *tt; // if no TTEngine is used, then assume we are building up the cvt
	
	bool ItalicAngleAllowed(ProjFreeVector *projFreeVector, wchar_t error[]);
	RVector MakeRVector(const TTVectorDesc *ttv, bool pv);
	bool AlmostPerpendicular(const TTVectorDesc *pv, const TTVectorDesc *fv, wchar_t error[]);
	void AssertFreeProjVector(const TTVectorDesc *pv, const TTVectorDesc *fv);
	short ProjectedDistance(bool signedDistance, short parent, short child, ProjFreeVector *projFreeVector);
	void AssertPositiveProjectedDistance(short *parent, short *child, ProjFreeVector *projFreeVector);
	void CondRoundInterpolees(bool y, short children, short child[], Rounding actual[], Rounding targeted); // used only once, why a method, then?
	void AssertStrokePhaseAngle(FVOverride fv, bool leftStationary[], short knot[]);
	short Neighbour(short parent0, short parent1, short child, bool immediate);
	void AssertVectorsAtPotentialJunction(TTVector pv, short parent0, short parent1, short child);
	FVMTDirection CalcDiagonalFVMT(FVOverride fv, short parent0, short parent1, short child, RVector strokeDirection, short *refPoint0, short *refPoint1);
	FVMTDirection CalcAlignFVMT(FVOverride fv, short parent0, short parent1, short child, RVector alignDirection, short *refPoint0, short *refPoint1);
	void AssertFVMT(FVMTDirection fvmt, short point0, short point1);
	void Touched(short knot, TTVDirection dir);
	short TheCvt(short parent, short child, LinkColor color, LinkDirection direction, CvtCategory category, short distance); // color, category, and distance: -1 or illegalCvtNum for default
	void DoVacuFormRound(void);
};

short Next(short knot, short base, short n, short delta) {
	return (knot - base + delta)%n + base;
} // Next

bool SameVectorsForAllChildren(ProjFreeVector *projFreeVector, int32_t children) {
	int32_t i;
	TTVectorDesc fv;

	if (children <= 1) return true;

	// we only have to look at the freedom vectors, since there is only one projection vector to begin with
	fv = projFreeVector->fv[0];
	for (i = 1; i < children && projFreeVector->fv[i].dir == fv.dir && projFreeVector->fv[i].from == fv.from && projFreeVector->fv[i].to == fv.to; i++);

	return i == children;
} // SameVectorsForAllChildren

void TTSourceGenerator::MainStrokeAngle(short angle100, wchar_t error[]) {
	double deg = (double)angle100/100.0,rad = Rad(deg);
	
	this->slope.x = 0.0;
	this->slope.y = 1.0;
	this->italic = false;
	
	if (this->mainStrokeAngle) {
		swprintf(error,L"cannot use MAINSTROKEANGLE more than once per glyph, or together with GLYPHSTROKEANGLE");
	} else {
		this->mainStrokeAngle = this->glyphStrokeAngle = true;
		if (deg < 90.0 - strokeFudge || 90.0 + strokeFudge < deg) {
			this->slope.x = cos(rad);
			this->slope.y = sin(rad);
			this->italic = true;
		}
	}
} /* TTSourceGenerator::MainStrokeAngle */

void TTSourceGenerator::GlyphStrokeAngle(short riseCvt, short runCvt, wchar_t error[]) {
	short riseCvtValue,runCvtValue;
	short deg;
	double rad;

	if (this->glyphStrokeAngle)
		swprintf(error,L"cannot use GLYPHSTROKEANGLE more than once per glyph, or together with MAINSTROKEANGLE");
	else {
		this->font->TheCvt()->GetCvtValue(riseCvt,&riseCvtValue);
		this->font->TheCvt()->GetCvtValue(runCvt, &runCvtValue);
		if (!riseCvtValue) { swprintf(error,L"Cvt value of italic rise %hi cannot be 0",riseCvt); return; }
		if (!runCvtValue) { swprintf(error,L"Cvt value of italic run %hi cannot be 0",runCvt); return; }
		this->riseCvt = riseCvt;
		this->runCvt = runCvt;
		rad = atan((double)riseCvtValue/(double)runCvtValue); deg = (short)Round(100*Deg(rad));
		this->MainStrokeAngle(deg,error);
		this->tt->CALL88(riseCvt,runCvt);
	}
} // TTSourceGenerator::GlyphStrokeAngle

RVector TTSourceGenerator::MakeRVector(const TTVectorDesc *ttv, bool pv) {
	RVector v;
	double len;

	switch (ttv->dir) {
		case xRomanDir:
			v.x = 1; v.y = 0;
			break;
		case yRomanDir:
			v.x = 0; v.y = 1;
			break;
		case xItalDir:
		case xAdjItalDir:
			if (pv) {
				v.x = this->slope.y; v.y = -this->slope.x; // not yet distinguishing adjItalDir from italDir
			} else {
				v.x = 1; v.y = 0;
			}
			break;
		case yItalDir:
		case yAdjItalDir:
			if (pv) {
				v.x = 0; v.y = 1;
			} else {
				v.x = this->slope.x; v.y = this->slope.y; // not yet distinguishing adjItalDir from italDir
			}
			break;
		case diagDir:
			v.x = this->glyph->x[ttv->to] - this->glyph->x[ttv->from];
			v.y = this->glyph->y[ttv->to] - this->glyph->y[ttv->from];
			len = LengthR(v);
			v.x /= len;
			v.y /= len;
			break;
		case perpDiagDir:
			v.x = this->glyph->y[ttv->to] - this->glyph->y[ttv->from];
			v.y = this->glyph->x[ttv->from] - this->glyph->x[ttv->to];
			len = LengthR(v);
			v.x /= len;
			v.y /= len;
			break;
	}
	return v;
} // TTSourceGenerator::MakeRVector

bool TTSourceGenerator::ItalicAngleAllowed(ProjFreeVector *projFreeVector, wchar_t error[]) {
	bool italic = xItalDir <= projFreeVector->pv.dir && projFreeVector->pv.dir <= yAdjItalDir;

	if (italic && !this->italic) {
		swprintf(error,L"cannot use / (italic angle) or // (adjusted italic angle) unless GLYPHSTROKEANGLE specifies an italic glyph");
		return false;
	}
	return true;
} // TTSourceGenerator::ItalicAngleAllowed

bool TTSourceGenerator::AlmostPerpendicular(const TTVectorDesc *pv, const TTVectorDesc *fv, wchar_t error[]) {
	RVector rpv,rfv;

	rpv = this->MakeRVector(pv,true);
	rfv = this->MakeRVector(fv,false);

	if (Abs(ScalProdRV(rpv,rfv)) >= 1.0/16.0) return false;

	swprintf(error,L"cannot accept vector override (projection and freedom vectors are [almost] perpendicular)");
	return true;
} // TTSourceGenerator::AlmostPerpendicular

void TTSourceGenerator::SetRounding(bool y, Rounding round, short params, short param[]) {
	short i;
	
	for (i = 0; i < params; i++) this->attrib[param[i]].round[y] = round;
} /* TTSourceGenerator::SetRounding */

void TTSourceGenerator::SetItalicStroke(bool phase, wchar_t error[]) {
	if (phase) {
		if (this->setItalicStrokePhase)
			swprintf(error,L"cannot use SETITALICSTROKEPHASE more than once per glyph");
		else
			this->setItalicStrokePhase = true;
	} else {
		if (this->setItalicStrokeAngle)
			swprintf(error,L"cannot use SETITALICSTROKEANGLE more than once per glyph");
		else
			this->setItalicStrokeAngle = true;
	}
} /* TTSourceGenerator::SetItalicStroke */

void TTSourceGenerator::Anchor(bool y, ProjFreeVector *projFreeVector, short knot, short cvt, bool round, wchar_t error[]) {
	Rounding knotR;
	bool negativeDist;

	knotR = this->attrib[knot].round[y];
	if (knot < this->knots - PHANTOMPOINTS || (!y && (knotR == rdtg || knotR == rutg))) {
		if (!this->ItalicAngleAllowed(projFreeVector,error)) return;
		if (this->AlmostPerpendicular(&projFreeVector->pv,&projFreeVector->fv[0],error)) return;
		if (this->tt) {
			this->AssertFreeProjVector(&projFreeVector->pv,&projFreeVector->fv[0]);
			round = round && knotR != roff; // there is a rounding method to be asserted...
		//	same roundabout way as in Link to maintain philosophy that knot gets rounded, not (absolute or relative) distance
			negativeDist = y ? this->V[knot].y < 0 : this->V[knot].x < 0;
			if (negativeDist && rdtg <= knotR && knotR <= rutg) knotR = (Rounding)(((short)knotR - (short)rdtg + 1)%2 + (short)rdtg); // rdtg <=> rutg
			if (round) this->tt->AssertRounding(knotR);
			if (y) this->attrib[knot].cvt = cvt; // make it available to IStroke...
			if (cvt < 0) this->tt->MDAP(round,knot); else this->tt->MIAP(round,knot,cvt); // if (still) no cvt...
			this->Touched(knot,projFreeVector->fv[0].dir);
		} else {
			/* nix */
		}
	} else if (y) {
		swprintf(error,L"cannot YANCHOR the side-bearing points");
	} else {
		swprintf(error,L"can XANCHOR the side-bearing points only to grid, down to grid, or up to grid");
	}
} /* TTSourceGenerator::Anchor */

void TTSourceGenerator::GrabHereInX(short left, short right, wchar_t error[]) {
	short leftDistance,leftCvt,rightDistance,rightCvt;
	double dist;
	Vector link;
	
	if (left < this->knots - PHANTOMPOINTS && right < this->knots - PHANTOMPOINTS) {
		link = SubV(this->V[left],this->V[this->knots - 2]);
		dist = link.x*this->slope.y - link.y*this->slope.x; // signed projection onto the normal to the direction of the main stroke angle
		leftDistance = (short)Round(dist);
		link = SubV(this->V[this->knots - 1],this->V[right]);
		dist = link.x*this->slope.y - link.y*this->slope.x; // signed projection onto the normal to the direction of the main stroke angle
		rightDistance = (short)Round(dist);
		if (this->tt) {
			leftCvt = this->TheCvt(-1,-1,linkGrey,linkX,cvtLsb,leftDistance);
			rightCvt = this->TheCvt(-1,-1,linkWhite,linkX,cvtRsb,rightDistance);
			if (leftCvt < 0 )
				swprintf(error,L"cannot accept GRABHEREINX (no cvt found from %hi to %hi)",this->knots - 2,left);
			else if (rightCvt < 0)
				swprintf(error,L"cannot accept GRABHEREINX (no cvt found from %hi to %hi)",right,this->knots - 1);
			else {
				this->leftAnchor = left; this->rightAnchor = right;
				this->tt->CALL24(leftCvt,rightCvt);
			}
		}
	} else
		swprintf(error,L"cannot accept GRABHEREINX (%hi is a side-bearing point)",left >= this->knots - PHANTOMPOINTS ? left : right);
} /* TTSourceGenerator::GrabHereInX */

short TTSourceGenerator::ProjectedDistance(bool signedDistance, short parent, short child, ProjFreeVector *projFreeVector) {
	int32_t distance;
	Vector link;
	RVector pv;
	double temp;

	if (projFreeVector->pv.dir == xRomanDir) {
		distance = this->V[child].x - this->V[parent].x;
	} else if (projFreeVector->pv.dir == yRomanDir || projFreeVector->pv.dir == yItalDir || projFreeVector->pv.dir == yAdjItalDir) {
		// yItalDir and yAdjItalDir have their pv in Y (but their fv in italic direction)
		distance = this->V[child].y - this->V[parent].y;
	} else {
		// general case: need to actually project
		if (projFreeVector->pv.dir == xItalDir || projFreeVector->pv.dir == xAdjItalDir) {
			// xItalDir and xAdjItalDir have their pv perp italic direction (but their fv in X)
			pv.x =  this->slope.y;
			pv.y = -this->slope.x;
		} else {
			pv = RDirectionV(this->V[projFreeVector->pv.from],this->V[projFreeVector->pv.to]);
			if (projFreeVector->pv.dir == perpDiagDir) { temp = pv.x; pv.x = pv.y; pv.y = -temp; }
		}
		link = SubV(this->V[child],this->V[parent]);
		distance = Round(link.x*pv.x + link.y*pv.y);
	}
	return (short)(signedDistance ? distance : Abs(distance));
} // TTSourceGenerator::ProjectedDistance

void TTSourceGenerator::AssertPositiveProjectedDistance(short *parent, short *child, ProjFreeVector *projFreeVector) {
	short temp;

	if (this->ProjectedDistance(true,*parent,*child,projFreeVector) < 0) {
		temp = *parent; *parent = *child; *child = temp;
	}
} // TTSourceGenerator::AssertPositiveProjectedDistance

void TTSourceGenerator::Link(bool y, bool dist, ProjFreeVector *projFreeVector, bool postRoundFlag, short parent, short child, CvtCategory category, short cvt, short minDists, short jumpPpemSize[], F26Dot6 pixelSize[], short *actualCvt, wchar_t error[]) {
	TTVDirection dir;
	Vector link;
	RVector linkDirection;
	double vectProd,scalProd;
	short distance,parentC,childC;
	LinkColor color;
	Rounding deltaR,parentR,childR;
	bool italicLink,lsbLink,rsbLink,negativeDist,negativeMirp;
	
	linkDirection.x = 0;
	linkDirection.y = 0;

	lsbLink = rsbLink = false;
	if (!this->ItalicAngleAllowed(projFreeVector,error)) return;
	if (this->AlmostPerpendicular(&projFreeVector->pv,&projFreeVector->fv[0],error)) return;
	dir = projFreeVector->pv.dir;
	italicLink = this->italic && (dir == xItalDir || dir == xAdjItalDir);
	negativeDist = negativeMirp = false;
	if (y) {
		color = this->glyph->TheColor(parent,child);
		link = SubV(this->V[child],this->V[parent]); linkDirection = RDirectionV(this->V[child],this->V[parent]);
		negativeDist = link.y < 0;
		if (!this->legacyCompile)
		{
			distance = this->ProjectedDistance(false, parent, child, projFreeVector);
		}
		else
		{
			distance = (short)Abs(link.y);
		}
	} else { // check for links related to GrabHereInX, using predefined colors and cvt numbers
		if ((parent == this->leftAnchor && child == this->knots - 2) || (parent == this->knots - 2 && child == this->leftAnchor)) {
			lsbLink = true; negativeMirp = parent == this->leftAnchor;
			if (dist) swprintf(error,L"cannot use an XDIST command when a GRABHEREINX command has defined a cvt number");
			if (cvt >= 0) swprintf(error,L"cannot override a cvt number defined via a GRABHEREINX command");
			color = linkGrey; cvt = LSBTMP;
		} else if ((parent == this->rightAnchor && child == this->knots - 1) || (parent == this->knots - 1 && child == this->rightAnchor)) {
			rsbLink = true; negativeMirp = parent == this->rightAnchor;
			if (dist) swprintf(error,L"cannot use an XDIST command when a GRABHEREINX command has defined a cvt number");
			if (cvt >= 0) swprintf(error,L"cannot override a cvt number defined via a GRABHEREINX command");
			color = linkWhite; cvt = RSBTMP;
		} else {
			color = this->glyph->TheColor(parent,child);
			link = SubV(this->V[child],this->V[parent]); linkDirection = RDirectionV(this->V[child],this->V[parent]);
			if (italicLink) {
				vectProd = link.x*this->slope.y - link.y*this->slope.x;
				negativeDist = vectProd < 0; distance = (short)Round(Abs(vectProd)); // unsigned distance... project onto the normal to the direction of the main stroke angle
			} else {
				negativeDist = link.x < 0;
				if (!this->legacyCompile)
				{
					distance = this->ProjectedDistance(false, parent, child, projFreeVector);
				}
				else
				{
					distance = (short)Abs(link.x);
				}
			}
		}
	}
	parentR = this->attrib[parent].round[y];
	childR = this->attrib[child].round[y];
		
	if (this->tt) this->AssertFreeProjVector(&projFreeVector->pv,&projFreeVector->fv[0]);
		
	scalProd = linkDirection.x*this->slope.x + linkDirection.y*this->slope.y;
	if (parentR == childR && !lsbLink && !rsbLink && (distance == 0 || (italicLink && scalProd > this->cosF1))) { // angle between link and slope < 0.5°
		if (this->tt) {
			this->tt->AssertRefPoint(0,parent);
		//	ALIGNRP is less optimal than (but equivalent to) an unrounded MDRP, since for chains of Dists,
		//	we keep setting the reference point explicitely, while MDRP can simply move it aint32_t
			if (distance == 0) {
				this->tt->AssertRounding(rtg);
				this->tt->MDRP(false,true,linkGrey,child); // almost same as in ::Align
			} else
				this->tt->ALIGNRP(child);			
		}
	} else {
		if (this->tt) {
			if (!this->legacyCompile)
			{
				//	we simply round the distance according to the child point's rounding method; see also further comments below
				deltaR = postRoundFlag ? roff : childR;
				//	if (parentR == rthg && childR == rthg) deltaR = rtg;
				//	else if (parentR == rthg && childR == rtg) deltaR = rthg;
			}
			else
			{
				deltaR = postRoundFlag ? roff : deltaRounding[parentR][childR];
			}
			if (negativeDist && rdtg <= deltaR && deltaR <= rutg) deltaR = (Rounding)(((short)deltaR - (short)rdtg + 1)%2 + (short)rdtg); // rdtg <=> rutg
			if (minDists < 0) { // no minDist override => get viable defaults
				minDists = 0;
				if (color == linkBlack) {
					for (parentC = 0; (parentC < this->glyph->numContoursInGlyph && this->glyph->endPoint[parentC] < parent); parentC++);
					for (childC  = 0; (childC  < this->glyph->numContoursInGlyph && this->glyph->endPoint[childC]  < child);  childC++);
					if ((distance >= this->emHeight/100 && parentC == childC) || (distance >= this->emHeight/50 && parentC != childC)) {
					//	this uses just the same assumptions as did the old compiler to guess whether or not it makes sense to assert a minimum distance
						jumpPpemSize[0] = 1;
						if (!this->legacyCompile)
						{
							pixelSize[0] = defaultMinDistAmount[childR][childR];
						}
						else
						{
							pixelSize[0] = defaultMinDistAmount[parentR][childR];
						}
						if (negativeDist && rdtg <= deltaR && deltaR <= rutg) pixelSize[0] = (pixelSize[0] + one6)%two6; // one6 <=> 0
						if (pixelSize[0] > 0) minDists++;
					}
				}
			}
			this->tt->AssertMinDist(minDists,jumpPpemSize,pixelSize);
			if (!postRoundFlag && deltaR != roff) this->tt->AssertRounding(deltaR);
			this->tt->AssertRefPoint(0,parent);
		}
		if (italicLink && !lsbLink && !rsbLink && scalProd > this->cosF2) { // angle between link and slope < MAXIMUM_ITALIC_ANGLE_DEVIATION
			if (this->tt) this->tt->MDRP(minDists > 0,deltaR != roff,color,child);
		} else if (this->tt) {
			if (!dist && cvt < 0) { // Link command and (still) no cvt => get default from cvt table
				cvt = this->TheCvt(parent,child,color,y ? linkY : linkX,category,distance);
			}
			if (dist || cvt < 0) { // Dist command or Link command and (still) no cvt (depends on how cvt "comments" were setup)
				this->tt->MDRP(minDists > 0,deltaR != roff,color,child);
			} else {
				this->tt->AssertAutoFlip(!lsbLink && !rsbLink); // autoflip affects only MIRP
				this->tt->MIRP(minDists > 0,deltaR != roff,color,child,cvt,negativeMirp);
			}
		} // else cvt entered in GrabHereInX
		if (this->tt && !postRoundFlag && deltaR == roff && childR != roff) { // can't do otherwise
		//	I think the original idea here was to ensure that the rounding method is an attribute of the knot,
		//	i.e. if I wanted the knot "down-to-grid" then it will end up "down-to-grid", and not the distance
		//	inbetween. As int32_t as our fv and pv are X or Y, this is achieved by either adjusting the rounding
		//	method of the distance accordingly (deltaRounding table), or else don't round the distance and do
		//	round the knot afterwards. As soon as we're using a pv other than X or Y, and especially when we're
		//	using the dpv, the whole idea makes little sense, since the respective knots are no int32_ter parallel
		//	to any grid. Furthermore, when using the dpv, rounding afterwards would be pretty useless anyway,
		//	because MDAP never uses the dpv in the first place. Therefore, and currently #ifdef'd for VTT_PRO_SP_YAA_AUTO_COM
		//	only, the distance is rounded as per the rounding method of the child point.
			this->tt->AssertRounding(childR);
			this->tt->MDAP(true,child);
		}
	}
	if (this->tt && postRoundFlag && childR != roff) {
		this->tt->AssertFreeProjVector(xRomanDir); // only allowed on XDist, XLink
		this->tt->AssertRounding(childR);
		this->tt->MDAP(true,child);
	}
	if (this->tt) this->Touched(child,projFreeVector->fv[0].dir);	
	*actualCvt = lsbLink || rsbLink ? illegalCvtNum : cvt;
} /* TTSourceGenerator::Link */

/***** have to test this on the visual level, because we might end up with the following scenario:
- yinterpolate two children with exactly the same y-coordinate (e.g. points 1 and 47 in Times New Roman 'M').
- round first of them to grid, translating to a YIPAnchor and a YInterpolate
- YInterpolate has rounded interpolee as parent of unrounded interpolee (Ian's and Vinnie's wish)
- but now unrounded interpolee is at same y-coordinate as its (temporary) parent
bool ValidateInterpolee(bool y, short parent0, short child, short parent1, Vector V[], wchar_t error[]);
bool ValidateInterpolee(bool y, short parent0, short child, short parent1, Vector V[], wchar_t error[]) {
	int32_t low,mid,high;
	short parent;
	wchar_t dir;

	if (y) {
		low = Min(V[parent0].y,V[parent1].y);
		high = Max(V[parent0].y,V[parent1].y);
		mid = V[child].y;
	} else {
		low = Min(V[parent0].x,V[parent1].x);
		high = Max(V[parent0].x,V[parent1].x);
		mid = V[child].x;
	}
	if (low < mid && mid < high) return true; // accept
	dir = 'x' + (y & true);
	if (low == mid || mid == high) {
		parent = y ? (mid == V[parent0].y ? parent0 : parent1) : (mid == V[parent0].x ? parent0 : parent1);
		swprintf(error,L"cannot %cInterpolate child point %hi (cannot be on same %c-coordinate as its parent point %hi)",Cap(dir),child,dir,parent);
	} else {
		swprintf(error,L"cannot %cInterpolate child point %hi (cannot be outside the range of %c-coordinates defined by its parent points %hi and %hi)",Cap(dir),child,dir,parent0,parent1);
	}
	return false;
} // ValidateInterpolee
*****/

void TTSourceGenerator::Interpolate(bool y, ProjFreeVector *projFreeVector, bool postRoundFlag, short parent0, short children, short child[], short parent1, bool round, wchar_t error[]) {
	short i,r;
	Rounding rounding[maxParams];
	
	if (!this->ItalicAngleAllowed(projFreeVector,error)) return;
	if (this->tt) {
		this->tt->AssertRefPointPair(1,2,parent0,parent1);

		if (SameVectorsForAllChildren(projFreeVector,children)) {
			if (this->AlmostPerpendicular(&projFreeVector->pv,&projFreeVector->fv[0],error)) return;
			this->AssertFreeProjVector(&projFreeVector->pv,&projFreeVector->fv[0]);
			if (children <= 2) { // optimise for 2 or less interpolations
				for (i = 0; i < children; i++) this->tt->IP(1,&child[i]);
			} else {
				this->tt->SLOOP(children);
				this->tt->IP(children,child);
			}
			for (i = 0; i < children; i++) this->Touched(child[i],projFreeVector->fv[i].dir);
		} else {
			for (i = 0; i < children; i++) {
				if (this->AlmostPerpendicular(&projFreeVector->pv,&projFreeVector->fv[i],error)) return;
				this->AssertFreeProjVector(&projFreeVector->pv,&projFreeVector->fv[i]);;
				this->tt->IP(1,&child[i]);
				this->Touched(child[i],projFreeVector->fv[i].dir);
			}
		}
		if (round || postRoundFlag) {
			if (postRoundFlag) this->tt->AssertFreeProjVector(xRomanDir); // only allowed on XInterpolate, XIPAnchor
			for (i = 0; i < children; i++) rounding[i] = this->attrib[child[i]].round[y];
			for (r = (short)rthg; r <= (short)rutg; r++) this->CondRoundInterpolees(y,children,child,rounding,(Rounding)r);
		}
		// else, strictly speaking, we should set this->attrib[child[i]].round[y] to roff (the default is rtg, which gets modified by Interpolate to roff)
	}
} /* TTSourceGenerator::Interpolate */

void TTSourceGenerator::BeginCodePath(short fpgmBias, wchar_t error[]) {
	this->tt->SetFunctionNumberBias(fpgmBias);
	this->InitCodePathState();
} // TTSourceGenerator::BeginCodePath

void TTSourceGenerator::EndCodePath(wchar_t error[]) {
	this->TermCodePathState();
} // TTSourceGenerator::EndCodePath

// Notice that for the implementation of the new Rendering Environment Specific VTT Talk commands we tend to set the reference points from within the respective functions hence there is no need to assert them here.
// The main reason for doing so is the absence of "get-reference-point" instructions: the functions implementing these commands tend to need the parents' (refernece points') original and/or current coordinates.
void TTSourceGenerator::ResAnchor(bool y, ProjFreeVector *projFreeVector, short child, short cvt, wchar_t error[]) {
	if (this->tt == NULL) return;
	if (child >= this->knots - PHANTOMPOINTS) { swprintf(error,L"cannot Res%cAnchor the side-bearing points",y ? L'Y' : L'X'); return; }
	if (!this->ItalicAngleAllowed(projFreeVector,error)) return;
	if (this->AlmostPerpendicular(&projFreeVector->pv,&projFreeVector->fv[0],error)) return;
	this->AssertFreeProjVector(&projFreeVector->pv,&projFreeVector->fv[0]);
	this->tt->ResMIAP(child,cvt);
	this->Touched(child,projFreeVector->fv[0].dir);
} // TTSourceGenerator::ResAnchor

void TTSourceGenerator::ResIPAnchor(bool y, ProjFreeVector *projFreeVector, bool postRoundFlag, short parent0, short child, short parent1, wchar_t error[]) {
	if (this->tt == NULL) return;
	if (!this->ItalicAngleAllowed(projFreeVector,error)) return;
	if (this->AlmostPerpendicular(&projFreeVector->pv,&projFreeVector->fv[0],error)) return;
	// limited pv/fv implemented so far only, asserted within fn
	this->tt->ResIPMDAP(projFreeVector->pv.dir,postRoundFlag,parent0,child,parent1);
	this->Touched(child,projFreeVector->fv[0].dir);
} // TTSourceGenerator::ResIPAnchor

void TTSourceGenerator::ResLink(bool y, bool dist, ProjFreeVector *projFreeVector, short parent, short child, short cvt, short minDists, wchar_t error[]) {
	bool useMinDist;

	if (this->tt == NULL) return;
//	if (parent >= this->knots - PHANTOMPOINTS && child >= this->knots - PHANTOMPOINTS) { swprintf(error,L"cannot Res%cLink the advance width",y ? 'Y' : 'X'); return; }
	if (!this->ItalicAngleAllowed(projFreeVector,error)) return;
	if (this->AlmostPerpendicular(&projFreeVector->pv,&projFreeVector->fv[0],error)) return;
	
	useMinDist = minDists > 0 || (minDists < 0 && this->glyph->TheColor(parent,child) == linkBlack);
	
	this->AssertFreeProjVector(&projFreeVector->pv,&projFreeVector->fv[0]);
	this->tt->ResMIRP(parent,child,cvt,useMinDist);
	this->Touched(child,projFreeVector->fv[0].dir);
} // TTSourceGenerator::ResLink

void TTSourceGenerator::ResIPLink(bool y, bool dist, ProjFreeVector *projFreeVector, short strokeOptimizationFlag, short grandParent0, short parent, short child, short cvt, short grandParent1, wchar_t error[]) {
	if (this->tt == NULL) return;
	if (!this->ItalicAngleAllowed(projFreeVector,error)) return;
	for (short i = 0; i < 2; i++) if (this->AlmostPerpendicular(&projFreeVector->pv,&projFreeVector->fv[i],error)) return;
	// ResIPMIRP doesn't like negative distances and complementary phases
	this->AssertPositiveProjectedDistance(&parent,&child,projFreeVector);
	this->AssertPositiveProjectedDistance(&grandParent0,&grandParent1,projFreeVector);
	// limited pv/fv implemented so far only, asserted within fn
	this->tt->ResIPMIRP(projFreeVector->pv.dir,strokeOptimizationFlag,grandParent0,parent,child,cvt,grandParent1);
	this->Touched(parent,projFreeVector->fv[0].dir);
	this->Touched(child, projFreeVector->fv[1].dir);
} // TTSourceGenerator::ResIPLink

void TTSourceGenerator::ResDDLink(bool y, bool dist, ProjFreeVector *projFreeVector, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, wchar_t error[]) {
	TTVectorDesc pv[2];
	
	if (this->tt == NULL) return;
	if (!this->ItalicAngleAllowed(projFreeVector,error)) return;
	pv[0].dir = perpDiagDir; pv[0].from = parent0; pv[0].to = child1; // criss-crossed...
	pv[1].dir = perpDiagDir; pv[1].from = parent1; pv[1].to = child0; // ...diagonal links
	for (short i = 0; i < 2; i++) if (this->AlmostPerpendicular(&pv[i],&projFreeVector->fv[i],error)) return;
	// pv will be dealt with in fn (that's the very raison d'être of DLink...)
	this->tt->ResDDMIRP(parent0,child0,projFreeVector->fv[0],cvt0,parent1,child1,projFreeVector->fv[1],cvt1);
	this->Touched(child0,projFreeVector->fv[0].dir);
	this->Touched(child1,projFreeVector->fv[1].dir);
} // TTSourceGenerator::ResDDLink

// it looks like the following three methods are quite similar, but they have rather different semantics:
// ResIPDLink constrains a pair of strokes in x or y by a single constraint each, while ResIPDDLink/ResIIPDLink constrain a diagonal/an italic stroke by a pair of constraints
void TTSourceGenerator::ResIPDLink(bool y, bool dist, ProjFreeVector *projFreeVector, short strokeOptimizationFlag, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1, wchar_t error[]) {
	TTVDirection dir = y ? yRomanDir : xRomanDir;
	
	// strokeOptimizationFlag not yet supported
	if (this->tt == NULL) return;
//	if (!this->ItalicAngleAllowed(projFreeVector,error)) return;
//	for (short i = 0; i < 2; i++) if (this->AlmostPerpendicular(&projFreeVector->pv,&projFreeVector->fv[i],error)) return;
	// ResIPDMIRP doesn't like negative distances and complementary phases
	this->AssertPositiveProjectedDistance(&parent0,&child0,projFreeVector);
	this->AssertPositiveProjectedDistance(&parent1,&child1,projFreeVector);
	this->AssertPositiveProjectedDistance(&grandParent0,&grandParent1,projFreeVector);
	// pv/fv limited to x or y by syntax so far
	this->tt->ResIPDMIRP(projFreeVector->pv.dir,grandParent0,parent0,child0,cvt0,parent1,child1,cvt1,grandParent1);
	this->Touched(parent0,dir);
	this->Touched(child0, dir);
	this->Touched(child1, dir);
	this->Touched(parent1,dir);
} // TTSourceGenerator::ResIPDLink

void TTSourceGenerator::ResIPDDLink(bool y, bool dist, ProjFreeVector *projFreeVector, short strokeOptimizationFlag, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1, wchar_t error[]) {
	TTVDirection dir = y ? yRomanDir : xRomanDir;

	// strokeOptimizationFlag not yet supported
	if (this->tt == NULL) return;
//	if (!this->ItalicAngleAllowed(projFreeVector,error)) return;
//	for (short i = 0; i < 2; i++) if (this->AlmostPerpendicular(&projFreeVector->pv,&projFreeVector->fv[i],error)) return;
	/*****
	// ResIPDDMIRP doesn't like negative distances and complementary phases
	this->AssertPositiveProjectedDistance(&parent0,&child0,projFreeVector);
	this->AssertPositiveProjectedDistance(&parent1,&child1,projFreeVector);
	*****/
	this->AssertPositiveProjectedDistance(&grandParent0,&grandParent1,projFreeVector);
	this->tt->ResIPDDMIRP(projFreeVector->pv.dir,grandParent0,parent0,child0,projFreeVector->fv[0],cvt0,parent1,child1,projFreeVector->fv[1],cvt1,grandParent1);
	this->Touched(parent0,dir);
	this->Touched(child0, projFreeVector->fv[0].dir);
	this->Touched(child1, projFreeVector->fv[1].dir);
	this->Touched(parent1,dir);
} // TTSourceGenerator::ResIPDDLink

void TTSourceGenerator::ResIIPDLink(bool dist, ProjFreeVector *projFreeVector, short grandParent0, short parent0, short child0, short cvt0, short parent1, short child1, short cvt1, short grandParent1, wchar_t error[]) {
	if (this->tt == NULL) return;
	if (!this->ItalicAngleAllowed(projFreeVector,error)) return;
	for (short i = 0; i < 2; i++) if (this->AlmostPerpendicular(&projFreeVector->pv,&projFreeVector->fv[i],error)) return;
	// pv xItalDir, asserted within fn, hence fv = xRomanDir
	this->tt->ResIIPDMIRP(grandParent0,parent0,child0,cvt0,parent1,child1,cvt1,grandParent1);
	this->Touched(parent0,xRomanDir);
	this->Touched(child0, xRomanDir);
	this->Touched(child1, xRomanDir);
	this->Touched(parent1,xRomanDir);
} // TTSourceGenerator::ResIIPDLink

void TTSourceGenerator::Intersect(short intersection, short line0start, short line0end, short line1start, short line1end, short ppem0, short ppem1, wchar_t error[]) {
	if (this->tt) {
		if (ppem0 == noPpemLimit && ppem1 == noPpemLimit) {
			this->tt->ISECT(intersection,line0start,line0end,line1start,line1end);
		} else if (ppem1 == noPpemLimit) {
			this->tt->IfPpemBelow(ppem0);
			this->tt->ISECT(intersection,line0start,line0end,line1start,line1end);
			this->tt->Else();
			this->Align(fvStandard,line0start,1,&intersection,line0end,ppem0,error);
			this->Align(fvStandard,line1start,1,&intersection,line1end,noPpemLimit,error);
			this->tt->End(true);
		} else { // both directions have ppem limits
			this->tt->IfPpemBelow(Min(ppem0,ppem1));
			this->tt->ISECT(intersection,line0start,line0end,line1start,line1end);
			this->tt->Else();
			this->Align(fvStandard,line0start,1,&intersection,line0end,ppem0,error);
			this->Align(fvStandard,line1start,1,&intersection,line1end,ppem1,error);
			this->tt->End(true);
		}
		this->Touched(intersection,diagDir);
	}
} // TTSourceGenerator::Intersect

bool ClassifyAlign(Vector parent0, Vector child, Vector parent1, short ppem) { // within rectangular hull of ±0.5° from "parent line"?
	double tanAlignTolerance;
	Vector p,q;
	int32_t pq,p2;
	
	if (ppem > 0) return true; // ppem limit specified, hence we'll gladly align anything
	
	tanAlignTolerance = tan(Rad(ALIGN_TOLERANCE));
	p = SubV(parent1,parent0);
	q = SubV(child,parent0);
	p2 = p.x*p.x + p.y*p.y;
	pq = ScalProdV(p,q);
	return 0 <= pq && pq <= p2 && Abs(VectProdV(p,q)) <= p2*tanAlignTolerance;
} // ClassifyAlign

void TTSourceGenerator::Align(FVOverride fvOverride, short parent0, short children, short child[], short parent1, short ppem, wchar_t error[]) {
	short i,ch,iChildren[2],iChild[2][maxParams],refPoint[maxParams][2];
	int32_t minX,minY,x,y,maxX,maxY;
	AlignParam *align;
	wchar_t buf[8*maxParams];
	RVector alignDirection = RDirectionV(this->V[parent0],this->V[parent1]);
	FVMTDirection fvmt[maxParams];

	if (this->tt) {
		switch (fvOverride) {
		case fvOldMethod: // Align
			swprintf(buf,L"/* Align [%hi...%hi] */",parent0,parent1); this->tt->Emit(buf);
			
			this->tt->AssertEitherKnotOnRefPoint(parent0,parent1,0);
			for (i = 0; i < children; i++) {
				ch = child[i];
				this->AssertVectorsAtPotentialJunction(pv,parent0,parent1,ch);
				if (ClassifyAlign(this->V[parent0],this->V[ch],this->V[parent1],ppem))
					this->tt->ALIGNRP(ch);
				else {
					this->tt->AssertRounding(rdtg);
					this->tt->MDRP(false,true,linkGrey,ch);
				}
				this->Touched(ch,this->tt->FVDir());
			}
			break;
		case fvForceX: // XAlign
		case fvForceY:	// YAlign
		case fvStandard: // DAlign
			swprintf(buf,L"/* %cAlign [%hi...%hi] */",fvOverride == fvStandard ? L'D' : (fvOverride == fvForceX ? L'X' : L'Y'),parent0,parent1); this->tt->Emit(buf);
			
			minX = Min(this->V[parent0].x,this->V[parent1].x); maxX = Max(this->V[parent0].x,this->V[parent1].x);
			minY = Min(this->V[parent0].y,this->V[parent1].y); maxY = Max(this->V[parent0].y,this->V[parent1].y);

			for (i = iChildren[0] = iChildren[1] = 0; i < children; i++) {
				ch = child[i];

				fvmt[i] = CalcAlignFVMT(fvOverride,parent0,parent1,ch,alignDirection,&refPoint[i][0],&refPoint[i][1]);
			//	if (fvOverride != fvStandard) {
			//	always touch point in both directions or else aligning is undermined
					x = this->V[ch].x;
					y = this->V[ch].y;
					if (fvmt[i] == fvOnX && !this->attrib[ch].touched[yRomanDir] && !this->ySmooth && minY < y && y < maxY) iChild[true][iChildren[true]++] = ch;
					if (fvmt[i] == fvOnY && !this->attrib[ch].touched[xRomanDir] && !this->xSmooth && minX < x && x < maxX) iChild[false][iChildren[false]++] = ch;
			//	}

			}
			if (iChildren[true] > 0) this->Interpolate(true,&this->yRomanPV,false,parent0,iChildren[true],iChild[true],parent1,false,error);
			if (iChildren[false] > 0) this->Interpolate(false,&this->xRomanPV,false,parent0,iChildren[false],iChild[false],parent1,false,error);
			
			this->tt->AssertEitherKnotOnRefPoint(parent0,parent1,0);
		//	/****** if (ppem >= 0) *****/ this->tt->AssertRoundingBelowPpem(rdtg,ppem);
			if (ppem != 1) this->tt->RoundDownToGridBelowPpem(ppem); // Jelle special optimisation
			this->tt->AssertTTVonLine(dpv,parent0,parent1,this->V[parent0],this->V[parent1],true);
			for (i = 0; i < children; i++) {
				ch = child[i];
				this->AssertFVMT(fvmt[i],refPoint[i][0],refPoint[i][1]);
				this->tt->MDRP(false,ppem != 1/***** true *****//***** ppem >= 0 *****/,linkGrey,ch);
				this->Touched(ch,this->tt->FVDir());
			}
			
			break;

		//	swprintf(buf,L"/* DAlign [%hi...%hi] */",parent0,parent1); this->tt->Emit(buf);
			
		//	this->tt->AssertEitherKnotOnRefPoint(parent0,parent1,0);
		//	if (ppem >= 0) this->tt->AssertRoundingBelowPpem(rdtg,ppem);
		//	for (i = 0; i < children; i++) {
		//		ch = child[i];
		//		this->AssertVectorsAtPotentialJunction(dpv,parent0,parent1,ch);
		//		this->tt->MDRP(false,ppem >= 0,linkGrey,ch);
		//		this->Touched(ch,this->tt->FVDir());
		//	}
		//	break;
		default:
			break;
		}
		
		//	***** an experimental variant *****
		//	alignDirection = RDirectionV(this->V[parent0],this->V[parent1]);
		//	this->tt->IfPpemBelow(ppem);
		//	tt->AssertTTVonLine(pv,parent0,parent1,this->V[parent0],this->V[parent1],true);
		//	for (i = 0; i < children; i++) {
		//		ch = child[i];
		//		this->AssertFVMT(parent0,parent1,ch,alignDirection);
		//		this->tt->ALIGNRP(ch);
		//		this->Touched(ch,this->tt->FVDir());
		//	}
		//	this->tt->Else();
		//	tt->AssertTTVonLine(dpv,parent0,parent1,this->V[parent0],this->V[parent1],true);
		//	for (i = 0; i < children; i++) {
		//		ch = child[i];
		//		this->AssertFVMT(parent0,parent1,ch,alignDirection);
		//		this->tt->MDRP(false,false,linkGrey,ch);
		//		this->Touched(ch,this->tt->FVDir());
		//	}
		//	this->tt->End();
		//	***** another experimental variant
		//	this->tt->IfPpemBelow(ppem);
		//	for (i = 0; i < children; i++) {
		//		ch = child[i];
		//		this->AssertVectorsAtPotentialJunction(pv,parent0,parent1,ch);
		//		this->tt->ALIGNRP(ch);
		//		this->Touched(ch,this->tt->FVDir());
		//	}
		//	this->tt->Else();
		//	for (i = 0; i < children; i++) {
		//		ch = child[i];
		//		this->AssertVectorsAtPotentialJunction(dpv,parent0,parent1,ch);
		//		this->tt->MDRP(false,false,linkGrey,ch);
		//		this->Touched(ch,this->tt->FVDir());
		//	}
		//	this->tt->End();
		//	*****
		
		align = new AlignParam;
		align->parent0 = parent0; align->parent1 = parent1;
		align->children = children;
		for (i = 0; i < children; i++) align->child[i] = child[i];
		this->aligns->InsertAtEnd(align);
	}

//	if (this->tt) {
//		this->tt->AssertEitherKnotOnRefPoint(parent0,parent1,0);
//		if (ppem < 0) {
//			for (i = 0; i < children; i++) {
//				ch = child[i];
//				this->AssertVectorsAtPotentialJunction(pv,parent0,parent1,ch);
//				if (ClassifyAlign(this->V[parent0],this->V[ch],this->V[parent1],ppem))
//					this->tt->ALIGNRP(ch);
//				else
//					this->tt->MDRP(false,false,linkGrey,ch);
//				this->Touched(ch,this->tt->FVDir());
//			}
//		} else {
//			this->tt->AssertRoundingBelowPpem(rdtg,ppem);
//			for (i = 0; i < children; i++) {
//				ch = child[i];
//				this->AssertVectorsAtPotentialJunction(dpv,parent0,parent1,ch);
//				this->tt->MDRP(false,true,linkGrey,ch);
//				this->Touched(ch,this->tt->FVDir());
//			}
//		}
//	}
} // TTSourceGenerator::Align

/*****
void TTSourceGenerator::Align(short parent0, short children, short child[], short parent1, short ppem, wchar_t error[]) {
	short i,ch;
	RVector parentDir,childDir;
	
	if (this->tt) {
		parentDir = RDirectionV(this->V[parent1],this->V[parent0]);
		for (i = 0; i < children; i++) {
			ch = child[i];
			this->AssertVectorsAtPotentialJunction(pv,parent0,parent1,ch);
			this->tt->AssertEitherKnotOnRefPoint(parent0,parent1,0);
			childDir = RDirectionV(this->V[ch],this->V[parent0]);
		//	distance = childV.x*slope.y - childV.y*slope.x; // Sampo's test: project onto normal to "parent line"
		//	if (Abs(distance) < this->emHeight/2048.0 + 0.707107) // close enough to be considered "aligned"...
			if (childDir.x*parentDir.x + childDir.y*parentDir.y > this->cosF1)
				this->tt->ALIGNRP(ch);
			else {
				this->tt->AssertRounding(rdtg);
				this->tt->MDRP(false,true,linkGrey,ch);
			}
			this->Touched(ch,this->tt->FVDir());
		}
	}
} // TTSourceGenerator::Align
*****/

void TTSourceGenerator::Move(bool y, F26Dot6 amount, short knots, short knot[], wchar_t errMsg[]) {
	TTVDirection dir;
	short i;
	
	if (this->tt) {
		dir = y ? yRomanDir : xRomanDir; // no [adjusted] italic directions...
		this->tt->AssertFreeProjVector(dir);
		if (knots > 1) this->tt->SLOOP(knots);
		this->tt->SHPIX(knots,knot,amount);
		for (i = 0; i < knots; i++) this->Touched(knot[i],dir);
	}
} /* TTSourceGenerator::Move */

void TTSourceGenerator::Shift(bool y, ProjFreeVector *projFreeVector, short parent, short children, short child[], wchar_t error[]) {
	short i,rp;
	
	if (this->tt) {
		rp = this->tt->AssertEitherRefPointOnKnot(1,2,parent);
		if (SameVectorsForAllChildren(projFreeVector,children)) {
			this->AssertFreeProjVector(&projFreeVector->pv,&projFreeVector->fv[0]);
			if (children <= 2) { // optimise for 2 or less shifted children
				for (i = 0; i < children; i++) this->tt->SHP(rp,1,&child[i]);
			} else {
				this->tt->SLOOP(children);
				this->tt->SHP(rp,children,child);
			}
			for (i = 0; i < children; i++) this->Touched(child[i],projFreeVector->fv[i].dir);
		} else {
			for (i = 0; i < children; i++) {
				this->AssertFreeProjVector(&projFreeVector->pv,&projFreeVector->fv[i]);;
				this->tt->SHP(rp,1,&child[i]);
				this->Touched(child[i],projFreeVector->fv[i].dir);
			}
		}
	}
} /* TTSourceGenerator::Shift */

short RectilinearDistanceOfDiagonal(bool x, const Vector V0, const Vector V1, const RVector strokeDirection) {
	Vector link;
	double dist;
	
	link = SubV(V1,V0);
	dist = x ? link.x - link.y*strokeDirection.x/strokeDirection.y
			 : link.y - link.x*strokeDirection.y/strokeDirection.x;
	return Abs((short)Round(dist));
} // RectilinearDistanceOfDiagonal

bool ClassifyStroke(Vector A1, Vector A2, Vector B1, Vector B2, short ppem, bool *crissCross, RVector *strokeDirection, bool *xLinks, short distance[], wchar_t error[]) {
	double cosF0;
	int32_t sgn0,sgn1;
	Vector aux;
	RVector leftDirection,rightDirection;
	
	sgn0 = VectProdP(A1,B1,A1,A2); sgn0 = Sgn(sgn0);
	sgn1 = VectProdP(A1,B1,A1,B2); sgn1 = Sgn(sgn1);
	*crissCross = sgn0 != sgn1;
	if (*crissCross) { aux = B1; B1 = B2; B2 = aux; } // we're criss-crossing; swap the second pair for further analysis
	
	leftDirection  = RDirectionV(B1,A1);
	rightDirection = RDirectionV(B2,A2);
	cosF0 = cos(Rad(MAXSTROKESIDEANGLEDIFFERENCE));
	if (leftDirection.x*rightDirection.x + leftDirection.y*rightDirection.y < (ppem < 0 ? cosF0 : 2*cosF0*cosF0 - 1)) { // edges are "anti-parallel" || not nearly-parallel, RScalProdV(left,right);...
		swprintf(error,L"cannot accept (X|Y)STROKE (edges differ by %f degrees or more)",(double)(ppem < 0 ? MAXSTROKESIDEANGLEDIFFERENCE : 2*MAXSTROKESIDEANGLEDIFFERENCE));
		return false;
	}
	*strokeDirection = RAvgDirectionV(leftDirection,rightDirection);
	*xLinks = Abs(strokeDirection->x) <= Abs(strokeDirection->y);
	distance[0] = RectilinearDistanceOfDiagonal(*xLinks,A1,A2,*strokeDirection);
	distance[1] = RectilinearDistanceOfDiagonal(*xLinks,B1,B2,*strokeDirection);
	return true; // by now
} // ClassifyStroke

void TTSourceGenerator::AssertStrokePhaseAngle(FVOverride fv, bool leftStationary[], short knot[]) {
	short A1,B1,lsb;
	wchar_t code[64];
	
	// for the the italic stroke phase/angle adjustment to make sense, it always looks at the first knot pair,
	// regardless of whether or not this will be the mirp side further down. However, in these cases linking
	// criss-cross doesn't make sense, for it would undo the effects of an adjusted italic stroke phase.
	
	A1 =   !leftStationary[0];
	B1 = 2+!leftStationary[1];
	if (fv != fvOldMethod && this->V[knot[B1]].y < this->V[knot[A1]].y) A1 = B1; // pull down the lower of the two parents
	lsb = this->knots - PHANTOMPOINTS;
	if (this->setItalicStrokePhase) {
		if (this->V[knot[A1]].y != 0) {
			if (fv != fvOldMethod && this->attrib[knot[A1]].touched[yRomanDir]) { // remember current y position
				this->tt->AssertFreeProjVector(yRomanDir);
				swprintf(code,L"GC[N], %hi",knot[A1]); this->tt->Emit(code);
				this->tt->Emit(L"#BEGIN");			// need to start new block or else pre-push interferes with GC
			}
			this->tt->AssertFreeProjVector(yItalDir);
			this->tt->AssertRefPoint(0,lsb);
			this->tt->ALIGNRP(knot[A1]);
			this->Touched(knot[A1],yItalDir);
		}
		this->tt->AssertFreeProjVector(xRomanDir);	// this used to be a call to fn 23, suggesting to be able to change the italic stroke
		this->tt->AssertRounding(rtg);				// phase globally. However, fn 23 can't be changed without changing the compiler, for
		this->tt->MDAP(true,knot[A1]);				// it has side effects which the compiler has to be aware of!!!
		this->Touched(knot[A1],xRomanDir);
	}
	if (this->setItalicStrokeAngle) {
		this->tt->AssertFreeProjVector(xAdjItalDir);
		this->tt->AssertRefPoint(0,knot[A1]);
		this->tt->ALIGNRP(knot[(A1+2)%4]);
		this->Touched(knot[(A1+2)%4],xAdjItalDir);
		if (this->V[knot[A1]].y != 0) {
			this->tt->AssertFreeProjVector(yAdjItalDir);
			this->tt->AssertRefPoint(0,lsb);
			if (fv == fvOldMethod || !this->attrib[knot[A1]].touched[yRomanDir]) {
				this->tt->MDRP(false,false,linkGrey,knot[A1]);
			} else {								// push it back up where it once beint32_ted
				this->tt->Emit(L"#END");				// end block started above
				this->tt->AssertPVonCA(yRomanDir);
				swprintf(code,L"SCFS[], %hi, *",knot[A1]); this->tt->Emit(code);
			}
			this->Touched(knot[A1],yAdjItalDir);
		}
	}
} // TTSourceGenerator::AssertStrokePhaseAngle

void TTSourceGenerator::Stroke(FVOverride fvOverride, bool leftStationary[], short knot[], short cvt, short ppem, short *actualCvt, wchar_t error[]) {
	if (fvOverride == fvOldMethod) {
		short i,/*lsb,*/mirpSide,A2,A1,B2,B1,cvtIndex[2],distance[2];
		RVector strokeDirection;
		bool crissCross,xLinks,iStroke;
		wchar_t buf[64];
		
		if (ClassifyStroke(this->V[knot[0]],this->V[knot[1]],this->V[knot[2]],this->V[knot[3]],ppem,&crissCross,&strokeDirection,&xLinks,distance,error)) {
			iStroke = strokeDirection.x*this->slope.x + strokeDirection.y*this->slope.y > this->cosF2; // angle between stroke and slope < MAXIMUM_ITALIC_ANGLE_DEVIATION
			for (i = 0; i < 4; i++) {
				this->attrib[knot[i]].dStroke = true;
				this->attrib[knot[i]].iStroke |= iStroke;
			}
			if (this->tt) {
				for (i = 0; i < 2; i++) cvtIndex[i] = cvt < 0 ? this->TheCvt(-1,-1,linkBlack,linkDiag,cvtStroke,Abs(distance[i])) : cvt;
				if (cvtIndex[0] < 0 && cvtIndex[1] < 0)
					swprintf(error,L"cannot accept STROKE (no cvt number found)");
				else {
					if (cvtIndex[0] < 0) cvtIndex[0] = cvtIndex[1];
					else if (cvtIndex[1] < 0) cvtIndex[1] = cvtIndex[0];
					mirpSide = Abs(distance[1]) > Abs(distance[0]) ? 1 : 0; // mirp the larger end
					cvt = cvtIndex[mirpSide];
					
					swprintf(buf,L"/* Stroke [%hi,%hi]%hi-[%hi,%hi]%hi */",knot[0],knot[1],distance[0],knot[2],knot[3],distance[1]); this->tt->Emit(buf);
					
					if (iStroke) this->AssertStrokePhaseAngle(fvOverride,leftStationary,knot);
					
					/*****
					// for the the italic stroke phase/angle adjustment to make sense, it always looks at the first knot pair,
					// regardless of whether or not this will be the mirp side further down. However, in these cases linking
					// criss-cross doesn't make sense, for it would undo the effects of an adjusted italic stroke phase.
					
					A1 = !leftStationary[0];
					lsb = this->knots - PHANTOMPOINTS;
					if (this->italicStrokePhase && iStroke) {
						if (this->V[knot[A1]].y != 0) {
							this->tt->AssertFreeProjVector(yItalDir);
							this->tt->AssertRefPoint(0,lsb);
							this->tt->ALIGNRP(knot[A1]);
							this->Touched(knot[A1],yItalDir);
						}
						this->tt->AssertFreeProjVector(xRomanDir);	// this used to be a call to fn 23, suggesting to be able to change the italic stroke
						this->tt->AssertRounding(rtg);				// phase globally. However, fn 23 can't be changed without changing the compiler, for
						this->tt->MDAP(true,knot[A1]);				// it has side effects which the compiler has to be aware of!!!
						this->Touched(knot[A1],xRomanDir);
					}
					if (this->italicStrokeAngle && iStroke) {
						this->tt->AssertFreeProjVector(xAdjItalDir);
						this->tt->AssertRefPoint(0,knot[A1]);
						this->tt->ALIGNRP(knot[A1+2]);
						this->Touched(knot[A1+2],xAdjItalDir);
						if (this->V[knot[A1]].y != 0) {
							this->tt->AssertFreeProjVector(yAdjItalDir);
							this->tt->AssertRefPoint(0,lsb);
							this->tt->MDRP(false,false,linkGrey,knot[A1]);
							this->Touched(knot[A1],yAdjItalDir);
						}
					}
					*****/
					
				//							leftStationary[0] := [A1]--->[A2]
				//							leftStationary[1] := [B1]--->[B2]
				//	       [B1]				mirpSide := either 0 for [A1]<--->[A2] or 1 for [B1]<--->[B2] depending on which has the larger cvt entry (if any at all...)
				//	       *   *** 			
				//	      *       [B2]		to understand all combinations, it is enough to assume STROKE([A1]--->[A2],[B1]--->[B2]) with the mirpSide = [A1]<--->[A2]
				//	      *          *		the other combinations are taken care of by 4 assignments below. Then, what the Stroke does, is essentially:
				//	     *           *		
				//	     *          *		SDPTL[R]	[A1],[(A1+2)%4]
				//	    *           *		  SFVTPV[]					// either (A2 not assumed to be at a junction)
				//	    *           *		  SFVTL[r]	[A2],neighbour	// or (A2 assumed to be at a junction, taken care of in AssertVectorsAtPotentialJunction
				//	   *           *		MDAP[r]		[A1]
				//	   *           *		DMIRP		[A2]
				//	  *            *		SDPTL[R]	[(B1+2)%4],[B1]
				//	  *           *			  SFVTPV[]					// again, either (B2 not assumed to be at a junction)
				//	 *            *			  SFVTL[r]	[B2],neighbour	// or (B2 assumed to be at a junction, taken care of in AssertVectorsAtPotentialJunction
				//	 *        *[A2]			MDRP[m<RGr]	[B2]
				//	*    *****				
				//	[A1]*					The tricky part is that leftStationary[i] == true is equivalent to leftStationary[i] == 1, hence add !leftStationary[i] to add 0,
				//							and the fact that the projection vector will pass through both points on the same side, regardless of whether or not the other
				//							point is stationary as well, hence the expressions (A1+2)%4 and (B1+2)%4 rather than B1 and A1 respectively to index knot[]
					
					A1 =  mirpSide*2+!leftStationary[ mirpSide];
					A2 =  mirpSide*2+ leftStationary[ mirpSide];
					B1 = !mirpSide*2+!leftStationary[!mirpSide];
					B2 = !mirpSide*2+ leftStationary[!mirpSide];
					
					this->AssertVectorsAtPotentialJunction(dpv,knot[A1],knot[(A1+2)%4],knot[A2]);
					this->tt->MDAP(false,knot[A1]); // don't round stationary point
					this->Touched(knot[A1],this->tt->FVDir());
					this->tt->AssertRounding(rtg);
					this->tt->AssertRefPoint(0,knot[A1]);
					this->tt->AssertAutoFlip(true);
					this->tt->DMIRP(knot[A2],cvt,knot[A1],knot[(A1+2)%4]);
					this->Touched(knot[A2],this->tt->FVDir());
					
					this->AssertVectorsAtPotentialJunction(dpv,knot[(B1+2)%4],knot[B1],knot[B2]);
				//	this->tt->AssertRoundingBelowPpem(rdtg,ppem);
					this->tt->RoundDownToGridBelowPpem(ppem);
					this->tt->AssertRefPoint(0,knot[(B2+2)%4]);
					this->tt->MDRP(false,true,linkGrey,knot[B2]);
					this->Touched(knot[B2],this->tt->FVDir());
				}
			}
		}
	} else { // one of the new methods
		short i,A2,A1,B2,B1,distance[2],refPoint0,refPoint1;
		RVector strokeDirection;
		double dotProd;
		bool crissCross,xLinks,iStroke;
		DiagParam *diagonal;
		wchar_t buf[64];
		FVMTDirection fvmt;

		if (ClassifyStroke(this->V[knot[0]],this->V[knot[1]],this->V[knot[2]],this->V[knot[3]],ppem,&crissCross,&strokeDirection,&xLinks,distance,error)) {
			if (cvt < 0)
				cvt = this->TheCvt(-1,-1,linkBlack,linkDiag,cvtStroke,Abs(distance[0]));
			if (cvt < 0)
				swprintf(error,L"cannot accept DIAGONAL (no cvt number found)");
			else {
				
			//	       [B1]				leftStationary[0] := [A1]--->[A2]
			//	       *   *** 			leftStationary[1] := [B1]--->[B2]
			//	      *       [B2]		to understand all combinations, it is enough to assume DIAGONAL([A1]--->[A2],[B1]--->[B2])
			//	      *          *		the other combinations are taken care of by 4 assignments below. Then, what the DIAGONAL does, is essentially:
			//	     *           *		
			//	     *          *		SDPTL[R]	[A1],[(A1+2)%4]
			//	    *           *		SFV??[]						// set freedom vector according to the 15 rules in ::AssertFVMT
			//		*			*		MDAP[r]		[A1]
			//	   *           *		DMIRP		[A2]			// mirpSide := always [A1]<--->[A2] per MT request
			//	   *           *		SDPTL[R]	[(B1+2)%4],[B1]
			//	  *            *		SFV??[]						// set freedom vector according to the 15 rules in ::AssertFVMT
			//	  *           *			MDRP[m<RGr]	[B2]
			//	 *            *			
			//	 *        *[A2]			The tricky part is that leftStationary[i] == true is equivalent to leftStationary[i] == 1, hence add !leftStationary[i] to add 0,
			//	*    *****				and the fact that the projection vector will pass through both points on the same side, regardless of whether or not the other
			//	[A1]*					point is stationary as well, hence the expressions (A1+2)%4 and (B1+2)%4 rather than B1 and A1 respectively to index knot[]
				
				swprintf(buf,L"/* Diagonal [%hi,%hi]%hi-[%hi,%hi]%hi */",knot[0],knot[1],distance[0],knot[2],knot[3],distance[1]); this->tt->Emit(buf);
				
				dotProd = strokeDirection.x*this->slope.x + strokeDirection.y*this->slope.y;
				iStroke = Abs(dotProd) > this->cosF2; // angle between stroke and slope < MAXIMUM_ITALIC_ANGLE_DEVIATION
				if (iStroke) this->AssertStrokePhaseAngle(fvOverride,leftStationary,knot);
					
				A1 =   !leftStationary[0];
				A2 =    leftStationary[0];
				B1 = 2+!leftStationary[1];
				B2 = 2+ leftStationary[1];
				
			//	this->tt->AssertTTVonLine(dpv,knot[A1],knot[(A1+2)%4],this->V[knot[A1]],this->V[knot[(A1+2)%4]],true);
			//	this->AssertFVMT(knot[A1],knot[(A1+2)%4],knot[A2],strokeDirection);
			//	this->tt->MDAP(false,knot[A1]); // don't round stationary point ONLY NEEDED IF NOT TOUCHED A1 YET!!!
			//	this->Touched(knot[A1],this->tt->FVDir());
				fvmt = CalcDiagonalFVMT(fvOverride,knot[A1],knot[(A1+2)%4],knot[A2],strokeDirection,&refPoint0,&refPoint1);
			/*****/
				if ((fvmt == fvOnY && !this->attrib[knot[A2]].touched[xRomanDir] && !this->xSmooth) || (fvmt == fvOnX && !this->attrib[knot[A2]].touched[yRomanDir] && !this->ySmooth)) {
					this->tt->AssertRefPoint(0,knot[A1]);
					this->tt->AssertFreeProjVector(fvmt == fvOnY ? xRomanDir : yRomanDir);
					this->tt->MDRP(false,false,linkBlack,knot[A2]);
					this->Touched(knot[A2],this->tt->FVDir());
				}
			/*****/
				this->tt->AssertTTVonLine(dpv,knot[A1],knot[(A1+2)%4],this->V[knot[A1]],this->V[knot[(A1+2)%4]],true);
				this->AssertFVMT(fvmt,refPoint0,refPoint1);
				this->tt->AssertRounding(rtg);
				this->tt->AssertRefPoint(0,knot[A1]);
				this->tt->AssertAutoFlip(true);
				this->tt->DMIRP(knot[A2],cvt,knot[A1],knot[(A1+2)%4]);
			///	this->tt->MIRP(true,true,linkBlack,knot[A2],cvt,false); ///
				this->Touched(knot[A2],this->tt->FVDir());
				
			//	this->tt->AssertTTVonLine(dpv,knot[(B1+2)%4],knot[B1],this->V[knot[(B1+2)%4]],this->V[knot[B1]],true);
			//	this->AssertFVMT(knot[(B1+2)%4],knot[B1],knot[B2],strokeDirection);
				fvmt = CalcDiagonalFVMT(fvOverride,knot[(B1+2)%4],knot[B1],knot[B2],strokeDirection,&refPoint0,&refPoint1);
			/*****/
				if ((fvmt == fvOnY && !this->attrib[knot[B2]].touched[xRomanDir] && !this->xSmooth) || (fvmt == fvOnX && !this->attrib[knot[B2]].touched[yRomanDir] && !this->ySmooth)) {
					this->tt->AssertRefPoint(0,knot[B1]);
					this->tt->AssertFreeProjVector(fvmt == fvOnY ? xRomanDir : yRomanDir);
					this->tt->MDRP(false,false,linkBlack,knot[B2]);
					this->Touched(knot[B2],this->tt->FVDir());
				}
			/*****/
				this->tt->AssertTTVonLine(dpv,knot[(B1+2)%4],knot[B1],this->V[knot[(B1+2)%4]],this->V[knot[B1]],true);
				this->AssertFVMT(fvmt,refPoint0,refPoint1);
			//	this->tt->AssertRoundingBelowPpem(rdtg,ppem);
				this->tt->RoundDownToGridBelowPpem(ppem);
				this->tt->AssertRefPoint(0,knot[(B2+2)%4]);
				this->tt->MDRP(false,true,linkGrey,knot[B2]);
			///	this->tt->AssertRoundingBelowPpem(rtg,ppem); ///
			///	this->tt->AssertRefPoint(0,knot[B1]); ///
			///	this->tt->MIRP(true,true,linkBlack,knot[B2],cvt,false); ///
				this->Touched(knot[B2],this->tt->FVDir());

				diagonal = new DiagParam;
				for (i = 0; i < 2; i++) diagonal->leftStationary[i] = leftStationary[i];
				for (i = 0; i < 4; i++) diagonal->knot[i] = knot[i];
				this->diagonals->InsertAtEnd(diagonal);
			}
		}
	}
	*actualCvt = cvt;
} // TTSourceGenerator::Stroke

/*****
void TTSourceGenerator::Stroke(bool leftStationary[], short knot[], short cvt, short ppem, short *actualCvt, wchar_t error[]) {
	short i,lsb,mirpSide,A2,A1,B2,B1,cvtIndex[2],distance[2];
	Vector link;
	RVector leftDirection,rightDirection,strokeDirection;
	bool iStroke;
	double dist;
	int32_t infoBits;
	wchar_t buf[64];
	
	leftDirection  = RDirectionV(this->V[knot[2]],this->V[knot[0]]);
	rightDirection = RDirectionV(this->V[knot[3]],this->V[knot[1]]);
	if (leftDirection.x*rightDirection.x + leftDirection.y*rightDirection.y < (ppem < 0 ? this->cosF0 : 2*this->cosF0*this->cosF0 - 1)) { // edges are "anti-parallel" || not nearly-parallel, RScalProdV(left,right);...
		swprintf(error,L"cannot accept (X|Y)STROKE (edges differ by %f degrees or more)",(double)(ppem < 0 ? MAXSTROKESIDEANGLEDIFFERENCE : 2*MAXSTROKESIDEANGLEDIFFERENCE));
	} else { // general stroke "action" command
		for (i = 0; i < 4; i++) this->attrib[knot[i]].dStroke = true;
		strokeDirection = RAvgDirectionV(leftDirection,rightDirection);
		iStroke = strokeDirection.x*this->slope.x + strokeDirection.y*this->slope.y > this->cosF2; // angle between stroke and slope < MAXIMUM_ITALIC_ANGLE_DEVIATION
		if (iStroke) {
			for (i = 0; i < 4; i++) this->attrib[knot[i]].iStroke = true;
		}
		for (i = 0; i < 2; i++) {
			link = SubV(this->V[knot[i*2+1]],this->V[knot[i*2]]);
			dist = link.x*strokeDirection.y - link.y*strokeDirection.x; // project onto normal to stroke direction
			distance[i] = (short)Round(dist);
		}
		if (this->tt) {
			for (i = 0; i < 2; i++) cvtIndex[i] = cvt < 0 ? this->TheCvt(-1,-1,linkBlack,linkDiag,cvtStroke,Abs(distance[i])) : cvt;
			if (cvtIndex[0] < 0 && cvtIndex[1] < 0)
				swprintf(error,L"cannot accept STROKE (no cvt number found)");
			else {
				if (cvtIndex[0] < 0) cvtIndex[0] = cvtIndex[1];
				else if (cvtIndex[1] < 0) cvtIndex[1] = cvtIndex[0];
				mirpSide = Abs(distance[1]) > Abs(distance[0]) ? 1 : 0; // mirp the larger end
				cvt = cvtIndex[mirpSide];
				
				// for the the italic stroke phase/angle adjustment to make sense, it always looks at the first knot pair,
				// regardless of whether or not this will be the mirp side further down. However, in these cases linking
				// criss-cross doesn't make sense, for it would undo the effects of an adjusted italic stroke phase.
				
				A1 = !leftStationary[0];
				lsb = this->knots - PHANTOMPOINTS;
				if (this->italicStrokePhase && iStroke) {
					if (this->V[knot[A1]].y != 0) {
						this->tt->AssertFreeProjVector(yItalDir);
						this->tt->AssertRefPoint(0,lsb);
						this->tt->ALIGNRP(knot[A1]);
						this->Touched(knot[A1],yItalDir);
					}
					this->tt->AssertFreeProjVector(xRomanDir);	// this used to be a call to fn 23, suggesting to be able to change the italic stroke
					this->tt->AssertRounding(rtg);		 	// phase globally. However, fn 23 can't be changed without changing the compiler, for
					this->tt->MDAP(true,knot[A1]);			// it has side effects which the compiler has to be aware of!!!
					this->Touched(knot[A1],xRomanDir);
				}
				if (this->italicStrokeAngle && iStroke) {
					this->tt->AssertFreeProjVector(xAdjItalDir);
					this->tt->AssertRefPoint(0,knot[A1]);
					this->tt->ALIGNRP(knot[A1+2]);
					this->Touched(knot[A1+2],xAdjItalDir);
					if (this->V[knot[A1]].y != 0) {
						this->tt->AssertFreeProjVector(yAdjItalDir);
						this->tt->AssertRefPoint(0,lsb);
						this->tt->MDRP(false,false,linkGrey,knot[A1]);
						this->Touched(knot[A1],yAdjItalDir);
					}
				}
				
			//							leftStationary[0] := [A1]--->[A2]
			//							leftStationary[1] := [B1]--->[B2]
			//	       [B1]				mirpSide := either 0 for [A1]<--->[A2] or 1 for [B1]<--->[B2] depending on which has the larger cvt entry (if any at all...)
			//	       *   *** 			
			//	      *       [B2]		to understand all combinations, it is enough to assume STROKE([A1]--->[A2],[B1]--->[B2]) with the mirpSide = [A1]<--->[A2]
			//	      *          *		the other combinations are taken care of by 4 assignments below. Then, what the Stroke does, is essentially:
			//	     *           *		
			//	     *          *		SDPTL[R]	[A1],[(A1+2)%4]
			//	    *           *		  SFVTPV[]					// either (A2 not assumed to be at a junction)
			//	    *           *		  SFVTL[r]	[A2],neighbour	// or (A2 assumed to be at a junction, taken care of in AssertVectorsAtPotentialJunction
			//	   *           *		MDAP[r]		[A1]
			//	   *           *		DMIRP		[A2]
			//	  *            *		SDPTL[R]	[(B1+2)%4],[B1]
			//	  *           *			  SFVTPV[]					// again, either (B2 not assumed to be at a junction)
			//	 *            *			  SFVTL[r]	[B2],neighbour	// or (B2 assumed to be at a junction, taken care of in AssertVectorsAtPotentialJunction
			//	 *        *[A2]			MDRP[m<RGr]	[B2]
			//	*    *****				
			//	[A1]*					The tricky part is that leftStationary[i] == true is equivalent to leftStationary[i] == 1, hence add !leftStationary[i] to add 0,
			//							and the fact that the projection vector will pass through both points on the same side, regardless of whether or not the other
			//							point is stationary as well, hence the expressions (A1+2)%4 and (B1+2)%4 rather than B1 and A1 respectively to index knot[]
				
				swprintf(buf,L"/- Stroke [%hi,%hi]%hi-[%hi,%hi]%hi -/",knot[0],knot[1],distance[0],knot[2],knot[3],distance[1]); this->tt->Emit(buf);
				
				A1 =  mirpSide*2+!leftStationary[ mirpSide];
				A2 =  mirpSide*2+ leftStationary[ mirpSide];
				B1 = !mirpSide*2+!leftStationary[!mirpSide];
				B2 = !mirpSide*2+ leftStationary[!mirpSide];
				
				this->AssertVectorsAtPotentialJunction(dpv,knot[A1],knot[(A1+2)%4],knot[A2]);
				this->tt->MDAP(false,knot[A1]); // don't round stationary point
				this->Touched(knot[A1],this->tt->FVDir());
				this->tt->AssertRounding(rtg);
				this->tt->AssertRefPoint(0,knot[A1]);
				this->tt->AssertAutoFlip(true);
				this->tt->DMIRP(knot[A2],cvt,knot[A1],knot[(A1+2)%4]);
				this->Touched(knot[A2],this->tt->FVDir());
				
				this->AssertVectorsAtPotentialJunction(dpv,knot[(B1+2)%4],knot[B1],knot[B2]);
				this->tt->AssertRoundingBelowPpem(rdtg,ppem);
				this->tt->AssertRefPoint(0,knot[(B2+2)%4]);
				this->tt->MDRP(false,true,linkGrey,knot[B2]);
				this->Touched(knot[B2],this->tt->FVDir());
			}
		} else {
			infoBits = 0;
			if (this->italic && strokeDirection.x*this->slope.x + strokeDirection.y*this->slope.y > this->cosF5) { // Garamond Italic 156 ???
				if 		(this->charCode == 'I') infoBits = UCISTROKE;
				else if (this->charCode == 'l') infoBits = LClSTROKE; // lower case 'L', that is...
				else if (this->charCode == '1') infoBits = FIGSTROKE;
			}
			this->EnterCvt(knot[0],knot[1],(LinkColor)-1,linkDiag,cvtStroke,distance[0],true,0);
			this->EnterCvt(knot[2],knot[3],(LinkColor)-1,linkDiag,cvtStroke,distance[1],true,infoBits);
		}
	}
	*actualCvt = cvt;
} // TTSourceGenerator::Stroke
*****/

void TTSourceGenerator::DStroke(bool leftStationary[], short knot[], short cvt, short *actualCvt, wchar_t error[]) {
	RVector strokeDirection,fixedDirection;
	short i,distance[2],cvtIndex[2];
	bool xLinks,flip;
	wchar_t buf[64];

	strokeDirection = RAvgDirectionV(RDirectionV(this->V[knot[2]],this->V[knot[0]]),RDirectionV(this->V[knot[3]],this->V[knot[1]]));
	xLinks = Abs(strokeDirection.x) <= Abs(strokeDirection.y);
	if (leftStationary[0] != leftStationary[1]) { // Links from opposite sides
		fixedDirection = RDirectionV(this->V[knot[2+!leftStationary[1]]],this->V[knot[!leftStationary[0]]]);
		fixedDirection.x = Abs(fixedDirection.x);
		fixedDirection.y = Abs(fixedDirection.y);
		/* if we are within diagFudge of xAxis || yAxis, it seems to be better to define xLink by the fixedDirection... */
		if (fixedDirection.y < fixedDirection.x*this->tanF4 || fixedDirection.x < fixedDirection.y*this->tanF4) xLinks = fixedDirection.x <= fixedDirection.y;
	}
	for (i = 0; i < 2; i++) distance[i] = RectilinearDistanceOfDiagonal(xLinks,this->V[knot[i*2]],this->V[knot[i*2+1]],strokeDirection);
	flip = distance[0] < 0; // the other distance doesn't seem to matter...
	
	if (this->tt) {
		for (i = 0; i < 2; i++) cvtIndex[i] = cvt < 0 ? this->TheCvt(-1,-1,linkBlack,xLinks ? linkX : linkY,cvtNewDiagStroke,Abs(distance[i])) : cvt;
		if (cvtIndex[0] < 0 && cvtIndex[1] < 0)
			swprintf(error,L"cannot accept DSTROKE (no cvt number found)");
		else {
			if (cvtIndex[0] < 0) cvt = cvtIndex[1];
			else if (cvtIndex[1] < 0) cvt = cvtIndex[0];
			else if (Abs(distance[0]) < Abs(distance[1])) cvt = cvtIndex[1];
			else cvt = cvtIndex[0];
			
			swprintf(buf,L"/* DStroke [%hi,%hi]%hi-[%hi,%hi]%hi */",knot[0],knot[1],distance[0],knot[2],knot[3],distance[1]); this->tt->Emit(buf);
			this->tt->AssertAutoFlip(false);
			this->tt->CALL656(leftStationary[0] != leftStationary[1],knot[  !leftStationary[0]],knot[  leftStationary[0]],
																	 knot[2+!leftStationary[0]],knot[2+leftStationary[0]],cvt,ND_HEIGHT_STORE_1,xLinks,leftStationary[0] ? flip : !flip);
			this->Touched(knot[  leftStationary[0]],diagDir); // if the left point is stationary, then the 0+leftStationary[0]'th point is floating...
			this->Touched(knot[2+leftStationary[1]],diagDir);
		}
	}
	*actualCvt = cvt;
} /* TTSourceGenerator::DStroke */

void TTSourceGenerator::IStroke(bool leftStationary[], short knot[], short height[], short phase, short cvt, short *actualCvt, wchar_t error[]) {
/***** leftStationary[] are not used for anything currently, linking is always done left-to-right, or leftStationary[0] == leftStationary[1] == true *****/
	RVector strokeDirection;
	short i,distance[2],cvtIndex[2],parent,child;
	bool xLinks,flip[2],move[4];
	wchar_t buf[64];

	strokeDirection = RAvgDirectionV(RDirectionV(this->V[knot[2]],this->V[knot[0]]),RDirectionV(this->V[knot[3]],this->V[knot[1]]));
	xLinks = Abs(strokeDirection.x) <= Abs(strokeDirection.y);
	if (xLinks) {
		for (i = 0; i < 2; i++) {
			distance[i] = RectilinearDistanceOfDiagonal(xLinks,this->V[knot[i*2]],this->V[knot[i*2+1]],strokeDirection);
			flip[i] = distance[i] < 0;
		}
		if (this->tt) {
			for (i = 0; i < 2; i++) cvtIndex[i] = cvt < 0 ? this->TheCvt(-1,-1,linkBlack,xLinks ? linkX : linkY,cvtNewDiagStroke,Abs(distance[i])) : cvt;
			if (cvtIndex[0] < 0 && cvtIndex[1] < 0)
				swprintf(error,L"cannot accept ISTROKE (no cvt number found)");
			else {
				if (cvtIndex[0] < 0) cvt = cvtIndex[1];
				else if (cvtIndex[1] < 0) cvt = cvtIndex[0];
				else if (Abs(distance[0]) < Abs(distance[1])) cvt = cvtIndex[1];
				else cvt = cvtIndex[0];
					
				swprintf(buf,L"/* IStroke [%hi,%hi]%hi-[%hi,%hi]%hi */",knot[0],knot[1],distance[0],knot[2],knot[3],distance[1]); this->tt->Emit(buf);
				
				for (i = 0; i < 4; i++) {
					move[i] = this->attrib[knot[i]].cvt != height[i/2];
					if (move[i]) {
						this->tt->CALL678(false,knot[i],knot[(i+2)%4],height[i/2],ND_ITALIC_STORE_1 + i);
						this->Touched(knot[i],diagDir);
					}
				}
				this->tt->AssertFreeProjVector(xRomanDir);
				this->tt->AssertSuperRounding(1,phase,4);
				this->tt->MDAP(true,knot[0]);
				this->Touched(knot[0],xRomanDir);
				this->tt->AssertAutoFlip(false);
				this->tt->CALL64(knot[0],knot[2],RESERVED_HEIGHTSPACE_START, phase & 1,false);
				for (i = 0; i <= 1; i++) {
					parent = knot[i*2]; child = knot[i*2+1];
					this->tt->CALL64(parent,child,cvt,deltaRounding[this->attrib[parent].round[false]][this->attrib[child].round[false]] == rthg,flip[i]);
				}
				for (i = 1; i <= 3; i++) this->Touched(knot[i],xRomanDir);
				for (i = 0; i < 4; i++)
					if (move[i])
						this->tt->CALL678(true,knot[i],knot[(i+2)%4],height[i/2],ND_ITALIC_STORE_1 + i);
			}
		}
	} else {
		swprintf(error,L"cannot accept ISTROKE (can be used for italic strokes only)");
	}
	*actualCvt = cvt;
} /* TTSourceGenerator::IStroke */

void TTSourceGenerator::FixDStrokes(void) {
	TTVDirection fv,pv,dir[2];
	short i,j;
	Attribute *attrib;
	
	if (this->tt) {
		fv = this->tt->FVDir();
		pv = this->tt->PVDir();
		if (fv == xRomanDir && pv == xRomanDir) {
			dir[0] = xRomanDir; dir[1] = yRomanDir;
		} else {
			dir[0] = yRomanDir; dir[1] = xRomanDir;
		}
		for (i = 0; i < 2; i++) {
			for (j = 0; j < this->knots - PHANTOMPOINTS; j++) {
				attrib = &this->attrib[j];
				if ((attrib->dStroke || attrib->iStroke) && !attrib->touched[dir[i]]) {
					this->tt->AssertFreeProjVector(dir[i]);
					this->tt->MDAP(false,j);
				}
			}
		}
	}
} /* TTSourceGenerator::FixDStrokes */

void TTSourceGenerator::Serif(bool forward, short type, short knots, short knot[], wchar_t error[]) {
	short ppem[1] = {1},actualCvt;
	F26Dot6 dist[1] = {one6};
	wchar_t buf[64];
	
	switch (type) {
		case 0: /*****
			
				****[0]****				  [2]**[3]
			****		   ****			**		 *
							   ****[1]**		 *
				****[6]****						 *
			****		   ******				  *
							     ****			  *
								     ***		  *
									 	**		  *
										  *		   *
										   *	   *
											*	   *
											*	   *
											 *	    *
											 *	    *
											 *      *
											 [5]**[4]
			
			*****/
			if (this->tt) {
				swprintf(buf,L"/* Round serif %hi %hi %hi %hi %hi %hi %hi */",knot[0],knot[1],knot[2],knot[3],knot[4],knot[5],knot[6]); this->tt->Emit(buf);
				this->Link(true, false,&this->yRomanPV,false,knot[0],knot[1],cvtSerifOther,illegalCvtNum,1,ppem,dist,&actualCvt,error);
				this->Link(true, false,&this->yRomanPV,false,knot[0],knot[3],cvtSerifOther,illegalCvtNum,0,NULL,NULL,&actualCvt,error);
				this->Link(true, false,&this->yRomanPV,false,knot[3],knot[4],cvtSerifOther,illegalCvtNum,1,ppem,dist,&actualCvt,error);
				this->Link(false,false,&this->xRomanPV,false,knot[4],knot[5],cvtSerifThin, illegalCvtNum,1,ppem,dist,&actualCvt,error);
				this->Link(false,false,&this->xRomanPV,false,knot[4],knot[3],cvtSerifOther,illegalCvtNum,0,NULL,NULL,&actualCvt,error);
				this->Link(false,false,&this->xRomanPV,false,knot[3],knot[2],cvtSerifThin, illegalCvtNum,1,ppem,dist,&actualCvt,error);
			}
			break;
		case 1: /*****
			
								****[3]
							****	  *
						****		  *
					****			  *
				****	 			  *
			[2]*	  ****			  *
			*	   ***	  **		  *
			 *	 **			*		  *
			 [1]*			*		  *
			 				*		  *
			 				*		  *
			 				*		  *
			 				*		  *
			 				*		  *
			 				[0]		  *
			 
			
			*****/
			if (this->tt) {
				swprintf(buf,L"/* Triangular serif %hi %hi %hi %hi */",knot[0],knot[1],knot[2],knot[3]); this->tt->Emit(buf);
				this->Link(true, false,&this->yRomanPV,false,knot[3],knot[2],cvtSerifCurveHeight,illegalCvtNum,0,NULL,NULL,&actualCvt,error);
				this->Link(true, false,&this->yRomanPV,false,knot[2],knot[1],cvtSerifThin,  		illegalCvtNum,1,ppem,dist,&actualCvt,error);
				this->Link(true, false,this->attrib[knot[0]].iStroke ? &this->yAdjItalPV : &this->yRomanPV,false,knot[2],knot[0],cvtSerifHeight,illegalCvtNum,1,ppem,dist,&actualCvt,error);
									// set vectors to yAdjItalDir if knot[0] is part of an iStroke...
				this->Link(false,false,&this->xRomanPV,false,knot[0],knot[2],cvtSerifExt,   		illegalCvtNum,0,NULL,NULL,&actualCvt,error);
			}
			break;
		case 2:
		case 3: /*****
			
                       [3]          *                          *          [3]                   [3]          *                          *          [3]            
                       *            *                          *            *                    *            *                          *            *            
                       *            *                          *            *                     *            *                          *            *            
                       *            *                          *            *                      *            *                          *            *            
                       *            *                          *            *                       *            *                          *            *            
                      *              *                        *              *                      *             **                        *             **            
                   ***                ***                  ***                ***                  *                ****                   *                ****        
            [2]****                      *******    *******                      ****[2]    [2]****                     ********    *******                     *****[2]
            *                                  *    *                                  *    *                                  *    *                                  *
            *                                  *    *                                  *    *                                  *    *                                  *
            [1]******************************[0]    [0]******************************[1]    [1]******************************[0]    [0]******************************[1]
			
			type 2, horzBase, forward				type 2, horzBase, !forward				type 3, horzBase, forward				type 3, horzBase, !forward
			
			
						   ****[0]					   [2]*[1]		                         ****[0]                     [2]*[1]
						   *	 *					   *	 *		                         *     *                     *     *
						   *	 *					   *	 *		                        *     *                     *     *
						   *	 *					   *	 *		                        *     *                     *     *
						   *	 *					   *	 *		                       *     *                     *     *
						  *		 *					  *		 *		                      *      *                    *      *
					   ***		 *				   ***		 *		                  ***       *                 ***       *
			***********			 *		[3]********			 *		       ***********          *      [3]********          *
								 *							 *		                           *                           *
								 *							 *		                           *                           *
								 *							 *		                          *	                          *
								 *							 *		                          *	                          *
								 *							 *		                         *	                         *
								 *							 *		                         *	                         *
			[3]********			 *		***********			 *		   [3]********          *	   ***********          *
					   ***		 *				   ***		 *		              ***       *	              ***       *
						  *		 *					  *		 *		                *      *	                *      *
						   *	 *					   *	 *		                 *     *	                 *     *
						   *	 *					   *	 *		                *     *		                *     *
						   *	 *					   *	 *		                *     *		                *     *
						   *	 *					   *	 *		               *     *		               *     *
						   [2]*[1]					   ****[0]		               [2]*[1]		               ****[0]
			
			type 2, vertBase, forward	type 2, vertBase, !forward	type 2, italBase, forward	type 2, italBase, !forward
			
			*****/ {
			bool horzBase,vertBase,italBase;
			RVector base;
			double absVectProd,length;
			short cvt[3],fun;
			
			base.x = this->V[knot[1]].x - this->V[knot[0]].x;
			base.y = this->V[knot[1]].y - this->V[knot[0]].y;
			length = LengthR(base);
			absVectProd = base.x*this->xAxis.y - base.y*this->xAxis.x; absVectProd = Abs(absVectProd); // RVectProdV... could do like nearHorz/Vert though...
			horzBase = absVectProd < length*this->sinF3; // angle between base and xAxis < serifFudge
			absVectProd = base.x*this->yAxis.y - base.y*this->yAxis.x; absVectProd = Abs(absVectProd); // RVectProdV... and therefore need no xAxis/yAxis!!!
			vertBase = absVectProd < length*this->sinF3; // angle between base and yAxis < serifFudge
			absVectProd = base.x*this->slope.y - base.y*this->slope.x; absVectProd = Abs(absVectProd); // RVectProdV...
			italBase = this->italic && absVectProd < length*this->sinF22; // angle between base and yAxis < 2*MAXIMUM_ITALIC_ANGLE_DEVIATION
			if (vertBase || horzBase || italBase) {
				if (this->tt) {
					swprintf(buf,L"/* %s serif %hi %hi %hi */",forward ? L"Forward" : L"Backward",knot[1],knot[2],knot[3]); this->tt->Emit(buf);
					if (horzBase && type == 2) { // seems to be some kind of optimisation for the frequent case of a simple serif such as in a Times 'I'...
						cvt[0] = this->TheCvt(knot[3],knot[1],linkGrey,linkX,cvtSerifExt,   -1);
						cvt[1] = this->TheCvt(knot[1],knot[2],linkGrey,linkY,cvtSerifThin,  -1);
						cvt[2] = this->TheCvt(knot[1],knot[3],linkGrey,linkY,cvtSerifHeight,-1);
						if (cvt[0] < 0)
							swprintf(error,L"cannot accept SERIF (need cvt numbers for XLINK(%hi,%hi))",knot[3],knot[1]);
						else if (cvt[1] < 0)
							swprintf(error,L"cannot accept SERIF (need cvt numbers for YLINK(%hi,%hi))",knot[1],knot[2]);
						else if (cvt[2] < 0)
							swprintf(error,L"cannot accept SERIF (need cvt numbers for YLINK(%hi,%hi))",knot[1],knot[3]);
						else {
							short jumpPpemSize[1];
							F26Dot6 pixelSize[1];
							
							jumpPpemSize[0] = 1;
							pixelSize[0] = one6;
							
							this->tt->AssertMinDist(1,jumpPpemSize,pixelSize);
							this->tt->AssertRounding(rtg);
							this->tt->AssertAutoFlip(true);
							
							if 		(knot[2] == knot[1]+1) fun = 34;
							else if (knot[2] == knot[1]-1) fun = 35;
							else						   fun = 36;
							this->tt->CALL3456(fun,knot[3],cvt[2],knot[2],cvt[1],knot[1],cvt[0]);
							/* this doesn't update the knots as being touched, but since this is only relevant for STROKEs that are
							   neither horizontal nor vertical, and the present serif beint32_ts to a vertical stroke, we couldn't care less... */
						}
						
						
					} else {
						if (italBase) this->Anchor(false,&this->xRomanPV,knot[1],illegalCvtNum,false,error);
						this->Link(horzBase,false,horzBase ? &this->yRomanPV : &this->yRomanPV,false,knot[1],knot[2],cvtSerifThin,illegalCvtNum,1,ppem,dist,&actualCvt,error);
						if (type == 2) this->Link(horzBase,false,horzBase ? &this->yRomanPV : &this->yRomanPV,false,knot[1],knot[3],cvtSerifHeight,illegalCvtNum,1,ppem,dist,&actualCvt,error); // control serif height
						this->Link(!horzBase,this->attrib[knot[3]].dStroke || this->attrib[knot[3]].iStroke,!horzBase ? &this->yRomanPV : &this->yRomanPV,false,knot[3],knot[1],cvtSerifExt,illegalCvtNum,0,NULL,NULL,&actualCvt,error);
					}
				}
			} else
				swprintf(error,L"cannot accept SERIF (base differs from horizontal or vertical by %f degrees or more, or from italic angle by %f degrees or more)",
																									(double)serifFudge,(double)2*MAXIMUM_ITALIC_ANGLE_DEVIATION);
			break;
		}
		case 4: /*****
			
                         [5]      *                              *      [5]                 
                         *        *                              *        *                 
                         *        *                              *        *                 
                         *        *                              *        *                 
                         *        *                              *        *                 
                         *        *                              *        *                 
                         *        *                              *        *                 
              **[3]******[4]      ************        ************      [4]******[3]**      
             *                                *      *                                *     
            [2]                                *    *                                [2]    
             *                                *      *                                *     
              **[1]**********************[0]**        **[0]**********************[1]**      
			
			forward									!forward								plus the same variations as in cases 2 and 3
			
			*****/ {
			bool horzBase,vertBase,italBase;
			RVector base;
			double absVectProd,length;
			Vector /***** link21,link23,*****/link34;
			
			base.x = this->V[knot[1]].x - this->V[knot[0]].x;
			base.y = this->V[knot[1]].y - this->V[knot[0]].y;
			length = LengthR(base);
			absVectProd = base.x*this->xAxis.y - base.y*this->xAxis.x; absVectProd = Abs(absVectProd); // RVectProdV...
			horzBase = absVectProd < length*this->sinF3; // angle between base and xAxis < serifFudge
			absVectProd = base.x*this->yAxis.y - base.y*this->yAxis.x; absVectProd = Abs(absVectProd); // RVectProdV...
			vertBase = absVectProd < length*this->sinF3; // angle between base and yAxis < serifFudge
			absVectProd = base.x*this->slope.y - base.y*this->slope.x; absVectProd = Abs(absVectProd); // RVectProdV...
			italBase = this->italic && absVectProd < length*this->sinF22; // angle between base and yAxis < 2*MAXIMUM_ITALIC_ANGLE_DEVIATION
			if (vertBase || horzBase || italBase) {
/* universal serif used by the new feature recognition */
				if (this->tt) {
					swprintf(buf,L"/* %s univ-serif %hi %hi %hi %hi */",forward ? L"Forward" : L"Backward",knot[1],knot[2],knot[3],knot[4]); this->tt->Emit(buf);
					
					this->Link(horzBase,false,horzBase ? &this->yRomanPV : &this->yRomanPV,false,knot[1],knot[3],cvtSerifThin,illegalCvtNum,1,ppem,dist,&actualCvt,error);
					this->Link(!horzBase,false,!horzBase ? &this->yRomanPV : &this->yRomanPV,false,knot[4],knot[2],cvtSerifExt,illegalCvtNum,0,NULL,NULL,&actualCvt,error);
					link34 = SubV(this->V[knot[3]],this->V[knot[4]]);
					if (horzBase) {
						if ((Abs(link34.x) > 0)) { // could compare with a small value
							this->Link(horzBase,false,horzBase ? &this->yRomanPV : &this->yRomanPV,false,knot[1],knot[4],cvtSerifHeight,illegalCvtNum,1,ppem,dist,&actualCvt,error);
						}
					} else {
						if ((Abs(link34.y) > 0)) {
							this->Link(horzBase,false,horzBase ? &this->yRomanPV : &this->yRomanPV,false,knot[1],knot[4],cvtSerifHeight,illegalCvtNum,1,ppem,dist,&actualCvt,error);
						}
					}
				}
			} else
				swprintf(error,L"cannot accept SERIF (base differs from horizontal or vertical by %f degrees or more, or from italic angle by %f degrees or more)",
																									(double)serifFudge,(double)2*MAXIMUM_ITALIC_ANGLE_DEVIATION);
			break;
		}
	}
} /* TTSourceGenerator::Serif */

void TTSourceGenerator::Scoop(short parent0, short child, short parent1, wchar_t error[]) {
	Vector base;
	bool ok,y;
	short dist,actualCvt;
	
	base = SubV(this->V[parent1],this->V[parent0]);
	base.x = Abs(base.x);
	base.y = Abs(base.y);
	ok = true;
	if 		(base.y <= base.x*this->tanF) { dist = (short)base.y; y = true;  } // near horizontal
	else if (base.x <= base.y*this->tanF) { dist = (short)base.x; y = false; } // near vertical
	else {
		ok = false;
		swprintf(error,L"cannot accept SCOOP (base differs from horizontal or vertical by %f degrees or more)",(double)strokeFudge);
	}
	if (ok)
		if (this->tt) this->Link(y,false,y ? &this->yRomanPV : &this->yRomanPV,false,parent0,child,cvtScoopDepth,illegalCvtNum,0,NULL,NULL,&actualCvt,error);
} /* TTSourceGenerator::Scoop */

void TTSourceGenerator::Smooth(short y, short italicFlag) {
	short xyDir,contour,start,end,n,parent0,parent1,children;
	TTVDirection dir;

	if (this->tt) {
		if (y < 0) { // do Y and X unless legacy then do X and Y 
			if (this->legacyCompile) {
				this->Smooth(0, italicFlag);
				this->Smooth(1, -1);				
			}else {
				this->Smooth(1, -1);
				this->Smooth(0, italicFlag);
			}
		} else { // Y or X only
			if (italicFlag < 0) {
				this->tt->IUP(y == 1);
			} else {

				xyDir = y == 0 ? xRomanDir : yRomanDir;
				dir = y == 0 ? (italicFlag == 0 ? xItalDir : xAdjItalDir) : (italicFlag == 0 ? yItalDir : yAdjItalDir);

				this->tt->AssertFreeProjVector(dir);

				for (contour = 0; contour < this->glyph->numContoursInGlyph; contour++) {
					start = this->glyph->startPoint[contour];
					end   = this->glyph->endPoint[contour];
					n = (end - start + 1);
					for (parent0 = start; parent0 <= end && !this->attrib[parent0].touched[xyDir]; parent0++);
					while (parent0 <= end) { // => this->attrib[parent0].touched[xyDir]
						parent1 = (parent0-start+1)%n + start;
						children = 0;
						while (!this->attrib[parent1].touched[xyDir]) {
							children++;
							parent1 = (parent1-start+1)%n + start;
						}
						if (parent0 != parent1 && children > 0) {
							this->tt->IPRange(y == 1,parent0,parent1,start,end);
						}
						parent0 = parent0 + children + 1;
					}
				}
			}
			if (y == 1)
				this->ySmooth = true;
			else
				this->xSmooth = true;
		}
	}
} /* TTSourceGenerator::Smooth */

void TTSourceGenerator::VacuFormLimit(short ppem) {
	this->vacuFormLimit = ppem;
} /* TTSourceGenerator::VacuFormLimit */

void TTSourceGenerator::VacuFormRound(short type, short radius, bool forward[], short knot[], wchar_t error[]) {
	short i,cvt;
	VacuFormParams *vacu;
	
	if (this->tt) {
	//	if (this->font->controlRounds || this->font->onlyControlLCBranches && this->charGroup == lowerCase && type == 2) {
			if (this->vacuForms	< maxVacuForms) {
				cvt = this->TheCvt(-1,-1,linkGrey,linkAnyDir,type == 2 ? cvtBranchBendRadius : cvtBendRadius,radius);
				if (cvt >= 0) {
					vacu = &this->vacuForm[this->vacuForms];
					vacu->type = type;
					vacu->radius = radius;
					vacu->cvt = cvt;
					for (i = 0; i < 2; i++) vacu->forward[i] = forward[i];
					for (i = 0; i < 4; i++) vacu->knot[i] = knot[i];
					this->vacuForms++;
				} else
					swprintf(error,L"cannot accept VACUFORMROUND (no cvt number found)"); // if we have to test this at all!!!
			} else
				swprintf(error,L"too many VACUFORMROUND commands (cannot have more than %li VACUFORMROUNDs)",maxVacuForms);
	//	}
	}
} /* TTSourceGenerator::VacuFormRound */

void TTSourceGenerator::Delta(bool y, DeltaColor color, short knot, F26Dot6 amount, bool ppemSize[], wchar_t errMsg[]) {
	if (this->tt) {
		this->tt->AssertFreeProjVector(y ? yRomanDir : xRomanDir); // no [adjusted] italic directions...
		this->tt->DLT(false,color,knot,amount,ppemSize);
	}
} /* TTSourceGenerator::Delta */

void TTSourceGenerator::Call(short actParams, short anyNum[], short functNum) {
	if (this->tt) this->tt->CALL(actParams,anyNum,functNum);
} /* TTSourceGenerator::Call */

void TTSourceGenerator::Asm(bool inLine, wchar_t text[], wchar_t error[]) {
	if (this->tt) this->tt->ASM(inLine,text);
} /* TTSourceGenerator::Asm */

void TTSourceGenerator::Quit(void) {
	// nix
} /* TTSourceGenerator::Quit */

void InitFreeProjVector(TTVDirection pv, ProjFreeVector *projFreeVector) {
	short i;
	
	projFreeVector->pv.dir = pv;
	projFreeVector->pv.from = projFreeVector->pv.to = illegalKnotNum;
	for (i = 0; i < maxParams; i++) {
		projFreeVector->fv[i].dir = pv;
		projFreeVector->fv[i].from = projFreeVector->fv[i].to = illegalKnotNum;
	}
} // InitFreeProjVector

void TTSourceGenerator::InitTTGenerator(TrueTypeFont *font, TrueTypeGlyph *glyph, int32_t glyphIndex, TTEngine *tt, bool legacyCompile, bool *memError) {
	short i,j,n,cont;
	double vectProd,deg;
	Attribute *attrib;
	Vector V[3],D[2];
	RVector T[2];
	wchar_t dateTime[32],buf[128];

	this->legacyCompile = legacyCompile; 
	
	InitFreeProjVector(xRomanDir,  &this->xRomanPV);
	InitFreeProjVector(yRomanDir,  &this->yRomanPV);
	InitFreeProjVector(xItalDir,   &this->xItalPV);
	InitFreeProjVector(yItalDir,   &this->yItalPV);
	InitFreeProjVector(xAdjItalDir,&this->xAdjItalPV);
	InitFreeProjVector(yAdjItalDir,&this->yAdjItalPV);
	
	this->xAxis.x = 1; this->yAxis.x = 0; this->slope.x = 0;
	this->xAxis.y = 0; this->yAxis.y = 1; this->slope.y = 1;
	this->riseCvt = this->runCvt = illegalCvtNum;
	this->cosF = cos(Rad(strokeFudge));
	this->tanF = tan(Rad(strokeFudge));
	this->cosF0 = cos(Rad(MAXSTROKESIDEANGLEDIFFERENCE));
	this->cosF1 = cos(Rad(ALIGN_TOLERANCE)); // more fudge factors...
	this->cosF2 = cos(Rad(MAXIMUM_ITALIC_ANGLE_DEVIATION));
	this->sinF22 = sin(Rad(2*MAXIMUM_ITALIC_ANGLE_DEVIATION));
	this->sinF3 = sin(Rad(serifFudge));
	this->tanF4 = tan(Rad(diagFudge));
	this->cosF5 = cos(Rad(gmndItFudge));
	this->cosF6 = cos(Rad(neighFudge));
	this->cosMT = cos(Rad(diagFudgeMT));
	this->tanMT = tan(Rad(diagFudgeMT));

	this->font = font;
	this->glyph = glyph;
	this->glyphIndex = glyphIndex; this->charCode = this->font->CharCodeOf(this->glyphIndex);
	
	this->charGroup = this->font->CharGroupOf(this->glyphIndex);
	
	this->knots = (glyph->numContoursInGlyph ? glyph->endPoint[glyph->numContoursInGlyph - 1] + 1 : 0) + PHANTOMPOINTS;
	
	this->attrib = (AttributePtr)NewP(this->knots*sizeof(Attribute));
	*memError = this->attrib == NULL;
	this->V = (VectorPtr)NewP(this->knots*sizeof(Vector));
	*memError = *memError || this->V == NULL;
	this->diagonals = new LinearListStruct;
	this->aligns = new LinearListStruct;
	*memError = *memError || this->diagonals == NULL || this->aligns == NULL;
	if (*memError) return;

	this->vacuFormLimit = -1;
	this->vacuForms = 0;
	
	this->tt = tt;
	
	for (i = 0; i < this->knots; i++) {
		this->V[i].x = glyph->x[i];
		this->V[i].y = glyph->y[i];
	}
	
	for (cont = 0; cont < glyph->numContoursInGlyph; cont++) {
	 	i = glyph->startPoint[cont];
	 	n = glyph->endPoint[cont] - i + 1;
	 	V[0] = this->V[i+n-1]; V[1] = this->V[i]; D[0] = SubV(V[1],V[0]); T[0] = RDirectionV(V[1],V[0]);
	 	for (j = 0; j < n; j++) {
			V[2] = this->V[i + (j + 1)%n]; D[1] = SubV(V[2],V[1]); T[1] = RDirectionV(V[2],V[1]);
			vectProd = T[0].x*T[1].y - T[0].y*T[1].x;
			vectProd = Max(-1,Min(vectProd,1)); // force asin's arg into [-1..1]
			deg = Deg(asin(vectProd));
			attrib = &this->attrib[i + j];
			attrib->deltaAngle = (short)Round(deg);
			V[0] = V[1]; V[1] = V[2]; T[0] = T[1]; D[0] = D[1];
		}
	}
	
	this->InitCodePathState();

	if (this->tt) {
		DateTimeStrg(dateTime);
		swprintf(buf,L"/* TT glyph %li, char 0x%lx",this->glyphIndex,this->charCode);
		if (L' ' < this->charCode && this->charCode < 0x7f) { swprintf(&buf[STRLENW(buf)],L" (%c)",(wchar_t)this->charCode); }
		swprintf(&buf[STRLENW(buf)],L" */");
		this->tt->Emit(buf);
		swprintf(buf,L"/* VTT %s compiler %s */",VTTVersionString,dateTime); this->tt->Emit(buf);
	}
} /* TTSourceGenerator::InitTTGenerator */

void TTSourceGenerator::InitCodePathState(void) {
	short i,j;
	Attribute *attrib;

	this->italic = false;
	this->mainStrokeAngle = this->glyphStrokeAngle = this->setItalicStrokePhase = this->setItalicStrokeAngle = false;
	this->xSmooth = this->ySmooth = false;
	this->leftAnchor = this->rightAnchor = -1;
	
	for (i = 0; i < this->knots; i++) {
		attrib = &this->attrib[i];
		for (j = 0; j < 2; j++) attrib->round[j] = rtg; // default
		attrib->cvt = illegalCvtNum;
		for (j = 0; j < 2; j++) attrib->touched[j] = false;
		attrib->dStroke = false;
		attrib->iStroke = false;
		attrib->on = glyph->onCurve[i];
		attrib->vacu = false;
	}
} // TTSourceGenerator::InitCodePathState

void TTSourceGenerator::TermCodePathState(void) {
	if (this->tt) this->DoVacuFormRound();
} // TTSourceGenerator::TermCodePathState

void TTSourceGenerator::TermTTGenerator(void) {
	this->TermCodePathState();
	if (this->aligns) delete this->aligns;
	if (this->diagonals) delete this->diagonals;
	if (this->V) DisposeP((void**)&this->V);
	if (this->attrib) DisposeP((void**)&this->attrib);
} /* TTSourceGenerator::TermTTGenerator */

TTSourceGenerator::TTSourceGenerator(void) {
	/* nix */
} /* TTSourceGenerator::TTSourceGenerator */

TTSourceGenerator::~TTSourceGenerator(void) {
	/* nix */
} /* TTSourceGenerator::~TTSourceGenerator */

void TTSourceGenerator::AssertFreeProjVector(const TTVectorDesc *pv, const TTVectorDesc *fv) {
	if (pv->dir < diagDir && fv->dir < diagDir && pv->dir == fv->dir)
		this->tt->AssertFreeProjVector(pv->dir);
	else if (fv->dir == yItalDir || fv->dir == yAdjItalDir) { // first assert fv because fn call overwrites pv too...
		this->tt->AssertFreeProjVector(fv->dir);
		if (pv->dir < diagDir) {
			this->tt->AssertPVonCA(pv->dir == yRomanDir);
		} else {
			this->tt->AssertTTVonLine(dpv,pv->from,pv->to,this->V[pv->from],this->V[pv->to],pv->dir == perpDiagDir);
		}
	} else if (pv->dir == xItalDir || pv->dir == xAdjItalDir) { // first assert pv because fn call overwrites fv too...
		this->tt->AssertFreeProjVector(pv->dir);
		if (fv->dir < diagDir) {
			this->tt->AssertFVonCA(fv->dir == yRomanDir);
		} else {
			this->tt->AssertTTVonLine(::fv,fv->from,fv->to,this->V[fv->from],this->V[fv->to],fv->dir == perpDiagDir);
		}
	} else {
		if (pv->dir < diagDir) {
			this->tt->AssertPVonCA(pv->dir == yRomanDir);
		} else {
			this->tt->AssertTTVonLine(dpv,pv->from,pv->to,this->V[pv->from],this->V[pv->to],pv->dir == perpDiagDir);
		}
		if (fv->dir < diagDir) {
			this->tt->AssertFVonCA(fv->dir == yRomanDir);
		} else {
			this->tt->AssertTTVonLine(::fv,fv->from,fv->to,this->V[fv->from],this->V[fv->to],fv->dir == perpDiagDir);
		}
	}
} // TTSourceGenerator::AssertFreeProjVector
	
void TTSourceGenerator::CondRoundInterpolees(bool y, short children, short child[], Rounding actual[], Rounding targeted) {
	short i,knot;
	bool negativeDist;
	
	for (i = 0; i < children; i++) {
		if (actual[i] == targeted) {
		//	same roundabout way as in Link to maintain philosophy that knot gets rounded, not (absolute or relative) distance
			knot = child[i];
			negativeDist = y ? this->V[knot].y < 0 : this->V[knot].x < 0;
			if (negativeDist && rdtg <= targeted && targeted <= rutg) targeted = (Rounding)(((short)targeted - (short)rdtg + 1)%2 + (short)rdtg); // rdtg <=> rutg
		
			this->tt->AssertRounding(targeted); // this will set the rounding method only once per targeted rounding method, if once at all...
			this->tt->MDAP(true,knot);
		}
	}
} /* TTSourceGenerator::CondRoundInterpolees */

short TTSourceGenerator::Neighbour(short parent0, short parent1, short child, bool immediate) {
	short cont,base,n,inc,dec,pred,succ,temp;
	RVector predD,succD,tempD,parentD;
	double predP,succP;
	
	/*****
	
	aint32_t contour, pick child's neighbours (one of the parents or the neighbour we're looking for)
	and select the one with larger angle to "parent line". Notice that the neighbour may actually
	be the neighbour's neighbour, if it is too close, and will be taken from neighbour's neighbour
	aint32_t the contour as int32_t as it is not too far away and its angle is within neighFudge degrees...
	The problem here seems to be that we would like to set the FV onto the line [neighbour]-[child],
	but these two may not be where they used to be due to previous instructions, so we attempt to
	find a neighbour which is far enough on a roughly straight line in order to define the freedom
	direction reasonnably... What I am tempted to demand here is a Dual Freedom Vector that much like
	the Dual Projection Vector uses the coordinates of the original outline, rather than the grid-
	fitted one. Hm...
	
		   [parent1]
		   *
		  *
		  *
		 *
		 *
		[child]**********[neighbour] maybe this situation
		
	   
	   [child]**********[neighbour] or this one
	  *
	  *
	 *
	 *
	*
	[parent0]
	
	*****/

	for (cont = 0; this->glyph->endPoint[cont] < child; cont++);
	base = this->glyph->startPoint[cont]; n = this->glyph->endPoint[cont] - base + 1; inc = 1; dec = n - 1;
	
	pred = Next(child,base,n,dec);
	if (!immediate && DistV(this->V[child],this->V[pred]) < this->emHeight/100) pred = Next(pred,base,n,dec);
	predD = RDirectionV(this->V[pred],this->V[child]);
	if (!immediate) { // try to get a "bigger" picture
		temp = Next(pred,base,n,dec); tempD = RDirectionV(this->V[temp],this->V[child]);
		while (DistV(this->V[child],this->V[pred]) < this->emHeight/12 && predD.x*tempD.x + predD.y*tempD.y >= this->cosF6) {
			pred = temp; predD = tempD;
			temp = Next(pred,base,n,dec); tempD = RDirectionV(this->V[temp],this->V[child]);
		}
	}
	
	succ = Next(child,base,n,inc);
	if (!immediate && DistV(this->V[child],this->V[succ]) < this->emHeight/100) succ = Next(succ,base,n,inc);
	succD = RDirectionV(this->V[succ],this->V[child]);
	if (!immediate) { // try to get a "bigger" picture
		temp = Next(succ,base,n,inc); tempD = RDirectionV(this->V[temp],this->V[child]);
		while (DistV(this->V[child],this->V[succ]) < this->emHeight/12 && succD.x*tempD.x + succD.y*tempD.y >= this->cosF6) {
			succ = temp; succD = tempD;
			temp = Next(succ,base,n,inc); tempD = RDirectionV(this->V[temp],this->V[child]);
		}
	}
	
	parentD = RDirectionV(this->V[parent1],this->V[parent0]);
	predP = VectProdRV(predD,parentD);
	succP = VectProdRV(succD,parentD);
	return Abs(predP) > Abs(succP) ? pred : succ;
} // TTSourceGenerator::Neighbour

void TTSourceGenerator::AssertVectorsAtPotentialJunction(TTVector pv, short parent0, short parent1, short child) { // PV perpendicular to line to which to align
	// pv is either the real pv or the dual pv
	short deltaAngle,neighbour;
	
	this->tt->AssertTTVonLine(pv,parent0,parent1,this->V[parent0],this->V[parent1],true);
	deltaAngle = this->attrib[child].deltaAngle;
	if (-noJunction <= deltaAngle && deltaAngle <= +noJunction) { // we make a turn within ±noJunction° => align "perpendicularly" to "parent line"
		// for CompilerLogic(1) this used to choose among x-axis or y-axis, whichever is closer to the parent line...
		this->tt->AssertFVonPV();
	} else { // assume it is a junction of some kind, align "parallel" to joining line
		neighbour = this->Neighbour(parent0,parent1,child,false);
		this->tt->AssertTTVonLine(fv,child,neighbour,this->V[child],this->V[neighbour],false);
	}
} /* TTSourceGenerator::AssertVectorsAtPotentialJunction */

FVMTDirection TTSourceGenerator::CalcDiagonalFVMT(FVOverride fv, short parent0, short parent1, short child, RVector strokeDirection, short *refPoint0, short *refPoint1) {
	Attribute *attrib;
	ListElem *diag = nullptr,*align = nullptr;
	DiagParam *diagParam = nullptr;
	AlignParam *alignParam = nullptr;
	bool previousStroke,previousAlign;
	short neighbour = this->Neighbour(parent0,parent1,child,true);
	short deltaAngle = this->attrib[child].deltaAngle;
	short diagChild,alignChild,otherChild;
	Vector D = SubV(this->V[neighbour],this->V[child]);
	FVMTDirection fvmt;

	attrib = &this->attrib[child];
	
	for (diag = this->diagonals->first, previousStroke = false; diag && !previousStroke; diag = diag->next) {
		diagParam = (DiagParam *)diag;
		for (diagChild = 0; diagChild < 4 && diagParam->knot[diagChild] != child; diagChild++);
		previousStroke = diagChild < 4;
	}
	for (align = this->aligns->first, previousAlign = false; align && !previousAlign; align = align->next) {
		alignParam = (AlignParam *)align;
		for (alignChild = 0; alignChild < alignParam->children && alignParam->child[alignChild] != child; alignChild++);
		previousAlign = alignChild < alignParam->children;
	}
	
	if (this->xSmooth && this->ySmooth) { // follow rule set #2
		
		if (ScalProdRV(this->slope,strokeDirection) >= this->cosMT) { // rule #1: diagonal is within ±5° of main stroke angle
			fvmt = fvOnX;
		} else if (attrib->on && Abs(D.y) <= Abs(D.x)*this->tanMT) { // rule #2: neighbour within ±5° of x-axis
			fvmt = fvOnX;
		} else if (attrib->on && Abs(D.x) <= Abs(D.y)*this->tanMT) { // rule #3: neighbour within ±5° of y-axis
			fvmt = fvOnY;
		} else { // rule #4: kitchen sink case
			fvmt = fvOnLine; *refPoint0 = child; *refPoint1 = neighbour;
		}

	} else { // follow rule set #1
	//	for the first two cases, make a more or less logical decision
		if (previousStroke) { // rule #1: child has already been involved in a [diagonal] stroke (more than two strokes would be difficult...)
			otherChild = diagParam->knot[(diagChild + 2)%4];
			fvmt = fvOnLine; *refPoint0 = child; *refPoint1 = otherChild;
		} else if (previousAlign) { // rule #1B: child has already been involved in an align
		//	such as when doing the lower leg of an Arial K after aligning the junction points with the lower edge of the upper leg
			fvmt = fvOnLine; *refPoint0 = alignParam->parent0; *refPoint1 = alignParam->parent1;
	//	for all the following cases, make an educated guess
		} else if (ScalProdRV(this->slope,strokeDirection) >= this->cosMT) { // rule #2: diagonal is within ±5° of main stroke angle
			fvmt = fvOnX;
		} else if (attrib->on && Abs(D.y) <= Abs(D.x)*this->tanMT) { // rule #3: neighbour within ±5° of x-axis
			fvmt = fvOnX; // fv = x
		} else if (attrib->on && Abs(D.x) <= Abs(D.y)*this->tanMT) { // rule #4: neighbour within ±5° of y-axis
			fvmt = fvOnY; // fv = y
		} else if (!attrib->on && D.x == 0 && this->attrib[neighbour].touched[xRomanDir]) { // rule #5: preserve tangent property of x-extremum
			fvmt = fvOnY; // dx == 0 => fv can only be y
		} else if (!attrib->on && D.y == 0 && this->attrib[neighbour].touched[yRomanDir]) { // rule #6: preserve tangent property of y-extremum
			fvmt = fvOnX; // dy == 0 => fv can only be x
		} else if (this->xSmooth || this->ySmooth) { // rules #7,8: all x or y are fixed, can only move in y or x respectively
			fvmt = this->xSmooth ? fvOnY : fvOnX;
	//	} else if (Abs(strokeDirection.x) != Abs(strokeDirection.y)) { // rules #9,10: kitchen sink case (all other rules haven't applied), diagonal is within ±45° of x or y-axis
		} else {
			fvmt = Abs(strokeDirection.x) > Abs(strokeDirection.y) ? fvOnY : fvOnX;
	//	} else { // rule #11: 45°-case of kitchen sink case...
	//		fvmt = fvOnPV;
		}
	}
	return fvmt;
} // TTSourceGenerator::CalcDiagonalFVMT

FVMTDirection TTSourceGenerator::CalcAlignFVMT(FVOverride fv, short parent0, short parent1, short child, RVector alignDirection, short *refPoint0, short *refPoint1) {
	Attribute *attrib = &this->attrib[child];
	ListElem *diag = nullptr,*align = nullptr;
	DiagParam *diagParam = nullptr;
	AlignParam *alignParam = nullptr;
	bool previousStroke,previousAlign;
	short neighbour = this->Neighbour(parent0,parent1,child,true);
	short deltaAngle = attrib->deltaAngle;
	short diagChild,alignChild,otherChild;
	Vector D = SubV(this->V[neighbour],this->V[child]);
	FVMTDirection fvmt;

	for (diag = this->diagonals->first, previousStroke = false; diag && !previousStroke; diag = diag->next) {
		diagParam = (DiagParam *)diag;
		for (diagChild = 0; diagChild < 4 && diagParam->knot[diagChild] != child; diagChild++);
		previousStroke = diagChild < 4;
	}
	for (align = this->aligns->first, previousAlign = false; align && !previousAlign; align = align->next) {
		alignParam = (AlignParam *)align;
		for (alignChild = 0; alignChild < alignParam->children && alignParam->child[alignChild] != child; alignChild++);
		previousAlign = alignChild < alignParam->children;
	}

	if (this->xSmooth && this->ySmooth) { // follow rule set #2
		if (ScalProdRV(this->slope,alignDirection) >= this->cosMT) { // rule #1: diagonal is within ±5° of main stroke angle
			fvmt = fvOnX;
		} else if (attrib->on && Abs(D.y) <= Abs(D.x)*this->tanMT) { // rule #2: neighbour within ±5° of x-axis
			fvmt = fvOnX;
		} else if (attrib->on && Abs(D.x) <= Abs(D.y)*this->tanMT) { // rule #3: neighbour within ±5° of y-axis
			fvmt = fvOnY;
		} else if (-noJunction <= deltaAngle && deltaAngle <= +noJunction) { // rule #4a: don't put fv on line to neighbor...
			fvmt = fvOnPV;
		} else { // ...unless assumed to be a junction
			fvmt = fvOnLine; *refPoint0 = child; *refPoint1 = neighbour;
		}
	} else { // follow modified rule set #1
	//	for the first four cases, make a more or less logical decision
		if (previousStroke) { // rule #1: child has already been involved in a [diagonal] stroke (more than two strokes would be difficult...)
			otherChild = diagParam->knot[(diagChild + 2)%4];
			fvmt = fvOnLine; *refPoint0 = child; *refPoint1 = otherChild;
		} else if (previousAlign) { // rule #1B: child has already been involved in an align
			fvmt = fvOnLine; *refPoint0 = alignParam->parent0; *refPoint1 = alignParam->parent1;
		} else if (fv == fvForceX) {
			fvmt = fvOnX;
		} else if (fv == fvForceY) {
			fvmt = fvOnY;
	//	for all the following cases, make an educated guess
		} else if (attrib->touched[xRomanDir] != attrib->touched[yRomanDir]) { // x xor y are fixed, can only move in y or x respectively
			fvmt = attrib->touched[xRomanDir] ? fvOnY : fvOnX;
		} else if (this->xSmooth != this->ySmooth) { // rules #7,8: all x or y are fixed, can only move in y or x respectively
			fvmt = this->xSmooth ? fvOnY : fvOnX;
		} else if (ScalProdRV(this->slope,alignDirection) >= this->cosMT) { // rule #2: diagonal is within ±5° of main stroke angle
			fvmt = fvOnX;
		} else if (attrib->on && Abs(D.y) <= Abs(D.x)*this->tanMT) { // rule #3: neighbour within ±5° of x-axis
			fvmt = fvOnX; // fv = x
		} else if (attrib->on && Abs(D.x) <= Abs(D.y)*this->tanMT) { // rule #4: neighbour within ±5° of y-axis
			fvmt = fvOnY; // fv = y
//		} else if (!attrib->on && D.x == 0 && this->attrib[neighbour].touched[xRomanDir]) { // rule #5: preserve tangent property of x-extremum
//			fvmt = fvOnY; // dx == 0 => fv can only be y
//		} else if (!attrib->on && D.y == 0 && this->attrib[neighbour].touched[yRomanDir]) { // rule #6: preserve tangent property of y-extremum
//			fvmt = fvOnX; // dy == 0 => fv can only be x
//		} else if (Abs(alignDirection.x) != Abs(alignDirection.y)) { // rules #9,10: kitchen sink case (all other rules haven't applied), diagonal is within ±45° of x or y-axis
//		} else {
//			fvmt = Abs(alignDirection.x) > Abs(alignDirection.y) ? fvOnY : fvOnX;
		} else { // rule #11: 45°-case of kitchen sink case...
			fvmt = fvOnPV;
		}
	}
	return fvmt;
} // TTSourceGenerator::CalcAlignFVMT

void TTSourceGenerator::AssertFVMT(FVMTDirection fvmt, short point0, short point1) {
	switch (fvmt) {
		case fvOnX:		this->tt->AssertFVonCA(false); break;
		case fvOnY:		this->tt->AssertFVonCA(true);  break;
		case fvOnPV:	this->tt->AssertFVonPV();	   break;
		case fvOnLine:	this->tt->AssertTTVonLine(fv,point0,point1,this->V[point0],this->V[point1],false); break;
	}
} // TTSourceGenerator::AssertFVMT

void TTSourceGenerator::Touched(short knot, TTVDirection dir) {
	bool *touched = &this->attrib[knot].touched[0];
//	short i;
	
//	xRomanDir = 0, yRomanDir, xItalDir, yItalDir, xAdjItalDir, yAdjItalDir, diagDir, perpDiagDir
//	for italic angles in x, pv is perpendicular to the italic angle, while fv is x, hence touches x only
//	for italic angles in y, pv is y, while fv is parallel to the italic angle, hence touches x and y
	touched[xRomanDir] = touched[xRomanDir] || dir == xRomanDir || dir >= xItalDir;
	touched[yRomanDir] = touched[yRomanDir] || dir == yRomanDir || dir == yItalDir || dir == yAdjItalDir || dir >= diagDir;
//	for (i = xRomanDir; i < xItalDir; i++) touched[i] |= dir == i || dir >= xItalDir;
} /* TTSourceGenerator::Touched */

short TTSourceGenerator::TheCvt(short parent, short child, LinkColor color, LinkDirection direction, CvtCategory category, short distance) {
	Vector D;
		
	if (color < 0) color = this->glyph->TheColor(parent,child); // provide default color
	if (category < 0) category = cvtDistance; // provide default category
	if (distance < 0) { // provide default distance
		D = SubV(this->V[parent],this->V[child]);
		distance = direction == linkY ? (short)D.x : (short)D.y;
		distance = Abs(distance);
	}
	return (short)this->font->TheCvt()->GetBestCvtMatch(this->charGroup,color,direction,category,distance);
} /* TTSourceGenerator::TheCvt */

/***** The VacuFormRound code still contains some Sampo-like convolutions...
	   But before attempting to improve this, it would be worthwhile re-thinking the concept
	   of VacuFormRound in general: what do we want it to achieve, and how do we achieve this.
	   Hopefully, this reduces the many different functions necessary to implement VacuForm-
	   Round, and therefore makes the overall code simpler, not needing that many decisions,
	   which function is to be called depending on what VacuFormRound type and configuration
	   of knots... For the curious: cf. fdefs0.c *****/
void TTSourceGenerator::DoVacuFormRound(void) {
	short pass,labelNumber,labels,i,j,first,last,lo,hi,l,h,type,x1,x2,y1,y2,nextX1,nextX2,nextY1,nextY2,base1,n1,inc1,dec1,base2,n2,inc2,dec2,fun,knots,knot[maxVacuForms],next,dist;
	VacuFormParams *vacu;
	wchar_t buf[32];
	
	for (first = 0; first < this->vacuForms && this->vacuForm[first].type == 2; first++); // type 2 vacuforms need a guard for each instance, since the guard measures a distance,
	for (last = this->vacuForms-1; last >= 0 && this->vacuForm[last].type == 2; last--);  // while all other types get by on one single guard which depends on a ppem size.
	
	if (this->vacuFormLimit >= 0) {
		labels = 1;
		for (i = 0; i < this->vacuForms; i++) if (this->vacuForm[i].type == 2) labels++;
		this->tt->Emit(L"MPPEM[]");
		swprintf(buf,L"GT[], %hi, *",this->vacuFormLimit); this->tt->Emit(buf);
		swprintf(buf,L"JROF[], #LRnd%hi, *",labels); this->tt->Emit(buf); // branch if vacuFormLimit > ppem...
		this->tt->Emit(L"#BEGIN");
	}
	labelNumber = 0;
	for (pass = 0; pass <= 3; pass++) {
	
		for (i = 0; i < this->vacuForms; i++) {
			
			vacu = &this->vacuForm[i];
			type = vacu->type;
			
			if ((pass < 3) == (type != 2)) { // in passes 0 through 2 do all types != 2, and in pass 3 do type 2
			
				x1 = vacu->knot[0]; this->attrib[x1].vacu = true; // start points may be off-curve points
				x2 = vacu->knot[1]; this->attrib[x2].vacu = true; // hence include them into the candidates
				y1 = vacu->knot[2]; this->attrib[y1].vacu = true; // for points to be flipped to on-curve
				y2 = vacu->knot[3]; this->attrib[y2].vacu = true; // upon completing passes 2 and 3...
				
				if ((pass == 0 && i == first) || pass == 3) {
					if (pass == 3) { // type == 2
						this->tt->AssertFreeProjVector(yRomanDir);
						if (this->glyph->y[y2] > this->glyph->y[y1]) swprintf(buf,L"MD[N], %hi, %hi",y2,y1);
						else										 swprintf(buf,L"MD[N], %hi, %hi",y1,y2);
						this->tt->Emit(buf);
						swprintf(buf,L"EQ[], 64, *"); this->tt->Emit(buf);
					} else {
						swprintf(buf,L"RS[], %hi",MAIN_STROKE_IS_ONE_PIXEL_STORAGE); this->tt->Emit(buf);
					}
					swprintf(buf,L"JROF[], #LRnd%hi, *",labelNumber); this->tt->Emit(buf);
					this->tt->Emit(L"#BEGIN");
				}
				
				for (j = 0; x1 > this->glyph->endPoint[j]; j++);
				base1 = this->glyph->startPoint[j]; n1 = this->glyph->endPoint[j] - base1 + 1;
				if (vacu->forward[0]) { inc1 = 1; dec1 = n1 - 1; } else { inc1 = n1 - 1; dec1 = 1; }
				
				for (j = 0; x2 > this->glyph->endPoint[j]; j++);
				base2 = this->glyph->startPoint[j]; n2 = this->glyph->endPoint[j] - base2 + 1;
				if (vacu->forward[1]) { inc2 = 1; dec2 = n2 - 1; } else { inc2 = n2 - 1; dec2 = 1; }
				
				nextX1 = Next(x1,base1,n1,inc1); this->attrib[nextX1].vacu = true;
				nextX2 = Next(x2,base2,n2,inc2); this->attrib[nextX2].vacu = true;
				nextY1 = Next(y1,base1,n1,dec1); this->attrib[nextY1].vacu = true;
				nextY2 = Next(y2,base2,n2,dec2); this->attrib[nextY2].vacu = true;
				
				if (pass == 1 || pass == 3) { // move horizontal tangent points to make tangent comply with radius (defined by cvt)
					this->tt->AssertFreeProjVector(xRomanDir);
					dist = this->glyph->x[x1]-this->glyph->x[nextX1];
					if (Abs(dist) > vacuFudge(this->font)) {
						this->tt->AssertRefPoint(0,x1);
						this->tt->ALIGNRP(nextX1);
					}
					dist = this->glyph->x[x2]-this->glyph->x[nextX2];
					if (Abs(dist) > vacuFudge(this->font)) {
						this->tt->AssertRefPoint(0,x2);
						this->tt->ALIGNRP(nextX2);
					}
					if (this->glyph->x[x1] < this->glyph->x[y1]) { // with the axis
						fun = type == 3 ? 2 : 0;
						this->tt->CALL012345(fun,nextY2,nextY1,x2,vacu->cvt);
					} else { // against the axis
						fun = type == 3 ? 3 : 1;
						if (type == 2) this->tt->CALL012345(fun,nextY2,nextY1,x2,vacu->cvt);
						else 		   this->tt->CALL012345(fun,nextY1,nextY2,x1,vacu->cvt);
					}
				}
				
				if (pass == 0 || pass == 3) { // move vertical tangent points to make tangent comply with radius (defined by cvt)
					this->tt->AssertFreeProjVector(yRomanDir);
					dist = this->glyph->y[y1]-this->glyph->y[nextY1];
					if (Abs(dist) > vacuFudge(this->font)) {
						this->tt->AssertRefPoint(0,y1);
						this->tt->ALIGNRP(nextY1);
					}
					dist = this->glyph->y[y2]-this->glyph->y[nextY2];
					if (Abs(dist) > vacuFudge(this->font)) {
						this->tt->AssertRefPoint(0,y2);
						this->tt->ALIGNRP(nextY2);
					}
					if (type == 2) {
						fun = this->glyph->y[x1] > this->glyph->y[y1] ? 4 : 5; // with the axis and against the axis
						this->tt->CALL012345(fun,nextX2,nextX1,y2,vacu->cvt);
					} else {
						if (type == 4) {
							fun = this->glyph->y[x1] > this->glyph->y[y1] ? 2 : 3; // with the axis and against the axis
						} else { // type == 1
							fun = this->glyph->y[x1] > this->glyph->y[y1] ? 0 : 1; // with the axis and against the axis
						}
						if (this->glyph->x[x1] < this->glyph->x[y1]) this->tt->CALL012345(fun,nextX2,nextX1,y2,vacu->cvt);
						else										 this->tt->CALL012345(fun,nextX1,nextX2,y1,vacu->cvt);
					}
				}
				
				if (pass == 2 || pass == 3) { // move remaining points "diagonally" to (vertical) tangent points
					if (nextX1 != nextY1 && nextX1 != y1 && nextY1 != x1) { // inner contour (or outer)
						knots = 0; next = Next(nextX1,base1,n1,inc1);
						while (knots < maxVacuForms && next != nextY1) {
							knot[knots] = next; knots++;
							next = Next(next,base1,n1,inc1);
						}
						if (knots > 1)
							this->tt->CALL6(knots,knot,nextX1);
						else if (knots == 1) {
							if		(nextX1 == knot[0] + 1) this->tt->CALL378(37,knot[0]); // special case of function 6 for nextX1 = knot[0] + 1...
							else if (nextX1 == knot[0] - 1) this->tt->CALL378(38,knot[0]); // special case of function 6 for nextX1 = knot[0] - 1...
							else							this->tt->CALL6(knots,knot,nextX1);
						}
						for (j = 0; j < knots; j++) this->attrib[knot[j]].vacu = true;
					}
					if (nextX2 != nextY2 && nextX2 != y2 && nextY2 != x2) { // outer contour (or inner)
						knots = 0; next = Next(nextX2,base2,n2,inc2);
						while (knots < maxVacuForms && next != nextY2) {
							knot[knots] = next; knots++;
							next = Next(next,base2,n2,inc2);
						}
						if (knots > 1)
							this->tt->CALL6(knots,knot,nextX2);
						else if (knots == 1) {
							if		(nextX2 == knot[0] + 1) this->tt->CALL378(37,knot[0]); // special case of function 6 for nextX2 = knot[0] + 1...
							else if (nextX2 == knot[0] - 1) this->tt->CALL378(38,knot[0]); // special case of function 6 for nextX2 = knot[0] - 1...
							else							this->tt->CALL6(knots,knot,nextX2);
						}
						for (j = 0; j < knots; j++) this->attrib[knot[j]].vacu = true;
					}
				}
					
				if ((pass == 2 && i == last) || pass == 3) {
					lo = 0;
					while (lo < this->knots) { // determine a range of (off-curve) points that have been moved "diagonally" (vacu flag == true)
						while (lo < this->knots && !(this->attrib[lo].vacu || this->glyph->onCurve[lo])) lo++;
						hi = lo;
						// optimise for code size: include all points which are vacu or on-curve
						while (hi+1 < this->knots && (this->attrib[hi+1].vacu || this->glyph->onCurve[hi+1])) hi++;
						// optimise for speed: exclude any on-curve points on the fringe of the interval
						for (l = lo; l <= hi && this->glyph->onCurve[l]; l++);
						for (h = hi; h >= lo && this->glyph->onCurve[h]; h--);
						if (h > l) swprintf(buf,L"FLIPRGON[], %hi, %hi",l,h); else if (h == l) swprintf(buf,L"FLIPPT[], %hi",l);
						if (h >= l) this->tt->Emit(buf);
						lo = hi+1;
					}
					this->tt->Emit(L"#END");
					swprintf(buf,L"#LRnd%hi:",labelNumber); this->tt->Emit(buf);
					labelNumber++;
					for (j = 0; j < this->knots; j++) this->attrib[j].vacu = false; // reset
				}
			}
		}
	}
	if (this->vacuFormLimit >= 0) {
		this->tt->Emit(L"#END");
		swprintf(buf,L"#LRnd%hi:",labels); this->tt->Emit(buf);
	}

	this->vacuFormLimit = -1;
	this->vacuForms = 0;
} /* TTSourceGenerator::DoVacuFormRound */

TTGenerator *NewTTSourceGenerator(void) {
	return new TTSourceGenerator;
}

/*	Notice that FixDStrokes touches all untouched points of strokes that happen to be diagonal or italic
	(but not the DStroke or IStroke), but the only point that might stay untouched upon a stroke is the
	"upper left" (i.e. B1 in the stroke's sketch), which typically gets touched anyhow as a result of
	SetItalicStrokePhase/Angle, and which is a stationary point (because presumably it has a link to it),
	we could simply MDAP[r] it if SetItalicStrokePhase/Angle didn't touch it, and forget about keeping
	track of the touched flags... Put another way, most calls to Touched() are redundant, since they mark
	a point that was never involved in a non-horizontal and non-vertical STROKE. In any case it looks as
	if FixDStrokes, if it remains in use, could be an informative command. Another thing I could do is
	to move the Touched() update into the TTEngine, and in order for FixDStrokes to be able to work with
	the data hidden, provide for an inquiry function bool Touched(bool y). This could then get us
	rid of the FVDir() inquiry function...
	
	2007-04-19 Not anymore. In order to implement XSmooth/() we need to know which points have been touched,
	and if so, in x or y (or both). Furthermore, the more recent Res* commands encapsulate a plurality of
	TT instructions, hence updating a flag inside TTEngine does not make the task of tracking this any easier.

	Question: Does SetItalicStrokePhase/Angle really do what we want it to? Wouldn't we rather want to
	move the points down to the base line, then xlink/xinterpolate them (at which point they get rounded),
	and finally move them back up, aint32_t the (meanwhile) adjusted angle? Notice that this is what SetItalc-
	StrokePhase/Angle never did, but chances are, what it was intended to.
	
	In the future might rewrite also c1_BestClusterIndex and see, whether all of this makes sense the way
	it is.
	
	Plus, should assert that the hinter parameters are up-to-date (compiled or so) prior to code generation.
	
	Should also assert that eg. VacuFormRound doesn't attempt to 'do' the side-bearing points. This becomes
	the int32_ter the more an issue which maybe should be more centralised, such as in the parser.  The problem
	is that there are things about which only the code generator knows, which means the particular code
	generator at issue, such as implementation restrictions (how many links from a point etc.), so the parser
	would have to know more about that particular code generator than I like. On the other hand there are
	checks that apply to all code generators, and preferreably are done once and forever (ie. in the parser),
	such as not to try to VacuFormRound the side bearing points, etc.
	
	*/