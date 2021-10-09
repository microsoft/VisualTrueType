// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/*****

	TTGenerator.h - New TypeMan Talk Compiler - Code Generators

*****/

#ifndef TTGenerator_dot_h
#define TTGenerator_dot_h

#define STRAIGHTANGLEFUDGE 1.5  /* degrees (7.5->5.0->1.5) */

typedef struct {
	TTVectorDesc pv;
	TTVectorDesc fv[maxParams];
//	TTVDirection pv;                             // if pvKnot1 != illegalKnotNum, SDPVTL[r] (!perpPV) or SDPVTL[R] (perpPV)
//	short pvKnot0,pvKnot1;                       // if pvKnot1 != illegalKnotNum, SDPVTL[?], pvKnot0, pvKnot1
//	TTVDirection fv[maxParams];                  // if fvKnot1[i] != illegalKnotNum, SFVTL[r] (!perpFV) or SFVTL[R] (perpFV) (for each of the params of Interpolate)
//	short fvKnot0[maxParams],fvKnot1[maxParams]; // if fvKnot1[i] != illegalKnotNum, SFVTL[?], fvKnot0, fvKnot1 (for each of the params of Interpolate)
} ProjFreeVector;

class TTGenerator {
public:
	virtual void MainStrokeAngle(short angle100, wchar_t error[]);
	virtual void GlyphStrokeAngle(short riseCvt, short runCvt, wchar_t error[]);
	virtual void SetRounding(bool y, Rounding round, short params, short param[]);
	virtual void SetItalicStroke(bool phase, wchar_t error[]); /* !phase => angle */
	virtual void Anchor(bool y, ProjFreeVector *projFreeVector, short knot, short cvt, bool round, wchar_t error[]);
	virtual void GrabHereInX(short left, short right, wchar_t error[]);
	// bool y kept around mostly for the GUI---display both links even though one or both may be diagDir or perpDiagDir
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
	virtual void DStroke(bool leftStationary[], short knot[], short cvt, short *actualCvt, wchar_t error[]); // new diagonal stroke
	virtual void IStroke(bool leftStationary[], short knot[], short height[], short phase, short cvt, short *actualCvt, wchar_t error[]); // new italic stroke
	virtual void FixDStrokes(void);
	virtual void Serif(bool forward, short type, short knots, short knot[], wchar_t error[]);
	virtual void Scoop(short parent0, short child, short parent1, wchar_t error[]);
	virtual void Smooth(short y, short italicFlag);
	virtual void Delta(bool y, DeltaColor color, short knot, F26Dot6 amount, bool ppemSize[], wchar_t errMsg[]);
	virtual void VacuFormLimit(short ppem);
	virtual void VacuFormRound(short type, short radius, bool forward[], short knot[], wchar_t error[]);
	virtual void Call(short actParams, short anyNum[], short functNum);
	virtual void Asm(bool inLine, wchar_t text[], wchar_t error[]);
	virtual void Quit(void);
	virtual void InitTTGenerator(TrueTypeFont *font, TrueTypeGlyph *glyph, int glyphIndex, TTEngine *tt, bool legacyCompile, bool *memError);
	virtual void TermTTGenerator(void);
	TTGenerator(void);
	virtual ~TTGenerator(void);
};

bool ClassifyAlign(Vector parent0, Vector child, Vector parent1, short ppem);
bool ClassifyStroke(Vector A1, Vector A2, Vector B1, Vector B2, short ppem, bool *crissCross, RVector *strokeDirection, bool *xLinks, short distance[], wchar_t error[]);

TTGenerator *NewTTSourceGenerator(void);

#endif // TTGenerator_dot_h