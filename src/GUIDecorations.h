// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef GUIDecorations_dot_h
#define GUIDecorations_dot_h

#define cvtHighwayShapeSizes 3

// defined here (as opposed to GlyphStruct.h) to avoid mutual includes
typedef enum {roundToNothing = 0, roundToGrid, roundToHalfGrid, roundToDoubleGrid, roundDownToGrid, roundUpToGrid} RoundMethod;
#define firstRoundMethod roundToNothing
#define lastRoundMethod roundUpToGrid
#define numRoundMethods (RoundMethod)(lastRoundMethod - firstRoundMethod + 1)

typedef enum {xDir = 0, yDir} Direction;
#define firstDirection xDir
#define lastDirection yDir
#define numDirections (Direction)(lastDirection - firstDirection + 1)

typedef enum {negative = 0, positive} Sign;
#define firstSign negative
#define lastSign positive
#define numSigns (Sign)(lastSign - firstSign + 1)

typedef enum {defaultMinDist = 0, assertMinDist, dontAssertMinDist} MinDistance;
#define firstMinDistanceMethod defaultMinDist
#define lastMinDistanceMethod dontAssertMinDist
#define numMinDistanceMethods (MinDistance)(lastMinDistanceMethod - firstMinDistanceMethod + 1)

typedef enum {defaultPpemLimit = 0, ppemLimitLocked, ppemLimitOpen} PpemLimit;
#define firstPpemLimit defaultPpemLimit
#define lastPpemLimit ppemLimitOpen
#define numPpemLimits (PpemLimit)(lastPpemLimit - firstPpemLimit + 1)
#define noPpemLimit (-1) // specifying no ppem limit is equivalent to locking the stroke for all ppem sizes

typedef enum {freedomStraight = 0, freedomItalic, freedomAdjustedItalic} FreedomDirection;
#define firstFreedomDirection freedomStraight
#define lastFreedomDirection freedomAdjustedItalic
#define numFreedomDirections (FreedomDirection)(lastFreedomDirection - firstFreedomDirection + 1)

typedef enum {fvInherit = -2, fvOldMethod = -1, fvForceX = 0, fvStandard, fvForceY} FVOverride;
#define firstFVOverride fvForceX
#define lastFVOverride fvForceY
#define numFVOverrides (FVOverride)(lastFVOverride - firstFVOverride + 1)

typedef enum {inLine = 0, preIUP = 1, postIUP = 2, genericLoc = 3} MoveDeltaLocation;
#define firstMoveDeltaLocation inLine
#define lastUsedMoveDeltaLocation postIUP
#define lastMoveDeltaLocation genericLoc
#define numUsedMoveDeltaLocations (MoveDeltaLocation)(lastUsedMoveDeltaLocation - firstMoveDeltaLocation + 1)
#define numMoveDeltaLocations (MoveDeltaLocation)(lastMoveDeltaLocation - firstMoveDeltaLocation + 1)

typedef enum {alwaysDelta = 0, 
			  blackDelta, greyDelta,
			  // Nat: Natural Advance Widths,  Com: Compatible Advance Widths
			  // Ver: Vertical Striping,       Hor: Horizontal Striping
			  // RGB: Red Green Blue Striping, BGR: Blue Green Red Striping
			  // IAW: Integer Advance Widths,  FAW: Fractional Advance Widths
			  // BLY: bi-level y-rendering     YAA: y-direction anti-aliasing
			  ctNatVerRGBIAWBLYDelta, ctComVerRGBIAWBLYDelta, ctNatHorRGBIAWBLYDelta, ctComHorRGBIAWBLYDelta,
			  ctNatVerBGRIAWBLYDelta, ctComVerBGRIAWBLYDelta, ctNatHorBGRIAWBLYDelta, ctComHorBGRIAWBLYDelta,
			  ctNatVerRGBFAWBLYDelta, ctComVerRGBFAWBLYDelta, ctNatHorRGBFAWBLYDelta, ctComHorRGBFAWBLYDelta,
			  ctNatVerBGRFAWBLYDelta, ctComVerBGRFAWBLYDelta, ctNatHorBGRFAWBLYDelta, ctComHorBGRFAWBLYDelta,
			  ctNatVerRGBIAWYAADelta, ctComVerRGBIAWYAADelta, ctNatHorRGBIAWYAADelta, ctComHorRGBIAWYAADelta,
			  ctNatVerBGRIAWYAADelta, ctComVerBGRIAWYAADelta, ctNatHorBGRIAWYAADelta, ctComHorBGRIAWYAADelta,
			  ctNatVerRGBFAWYAADelta, ctComVerRGBFAWYAADelta, ctNatHorRGBFAWYAADelta, ctComHorRGBFAWYAADelta,
			  ctNatVerBGRFAWYAADelta, ctComVerBGRFAWYAADelta, ctNatHorBGRFAWYAADelta, ctComHorBGRFAWYAADelta,
			  // see also PixelValuePanel::DrawColPict
			  invalidDelta } DeltaColor;
#define firstDeltaColor alwaysDelta
#define firstCTDeltaColor ctNatVerRGBIAWBLYDelta
#define lastCTDeltaColor ctComHorBGRFAWYAADelta
#define lastDeltaColor invalidDelta
#define numDeltaColors (lastDeltaColor - firstDeltaColor + 1)

#define maxRoundArrowHeads 9



#endif // GUIDecorations_dot_h