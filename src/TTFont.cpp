/// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define _CRT_SECURE_NO_DEPRECATE 
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <assert.h> // assert
#include <string.h>
#include <stdio.h>
#include "opentypedefs.h" // for SWAP etc.
#include "pch.h"
#include "CvtManager.h"
#include "TTAssembler.h"
#include "TTFont.h"

#define	PRIVATE_GLIT1	'TSI0' /* editor 0 */
#define	PRIVATE_PGM1	'TSI1' /* editor 1 */
#define PRIVATE_GLIT2	'TSI2' /* editor 2 */
#define PRIVATE_PGM2	'TSI3' /* editor 3 */
#define PRIVATE_CMAP	'TSI4' /* Copy of Cmap */
#define PRIVATE_GROUP	'TSI5' /* group type for each glyph : uppercase, lowercase,... */

#define PRIVATE_GSUB	'TSIS'
#define PRIVATE_GPOS	'TSIP'
#define PRIVATE_GDEF	'TSID'
#define PRIVATE_BASE	'TSIB'
#define PRIVATE_JSTF	'TSIJ'

#define PRIVATE_CVAR	'TSIC'

#define PRIVATE_STAMP_1	0xABFC1F34		/* fonts stored by char codes...	*/
#define PRIVATE_STAMP_2	0xCA0A2117		/* fonts stored by glyph index...	*/

#define GLIT_PAD	5
#define PRE_PGM_GLYPH_INDEX		0xFFFA
#define CVT_GLYPH_INDEX			0xFFFB
#define	HL_GLYPH_INDEX			0xFFFC
#define FONT_PGM_GLYPH_INDEX	0xFFFD
#define PRIVATE_STAMP_INDEX		0xFFFE
static const unsigned short glitPadIndex[GLIT_PAD] = {PRIVATE_STAMP_INDEX, PRE_PGM_GLYPH_INDEX, FONT_PGM_GLYPH_INDEX, CVT_GLYPH_INDEX, HL_GLYPH_INDEX};

#define ABSOLUTE_MAX_GLYPHS		0xFFFF // can't have more than 2^16-1 glyphs, unicode 0xffff reserved to indicate "byGlyphIndex..."

// notice that these macros won't convert big endians into little endians for the windows platform...
#define READALIGNEDWORD( p ) ( p += 2, *(short *)(p-2) )
#define READNONALIGNEDWORD( p ) ( p += 2, (short)( (((short)*(p-2)) << 8) | *(unsigned char *)(p-1) ) )
#define COPYALIGNEDWORD(d, s) (s += 2, d += 2, *(short *)(d-2) = *(short *)(s-2))
#define WRITEALIGNEDWORD( p, x ) ( *((short *)p) = (x), p += 2 )
#define WRITENONALIGNEDWORD( p, x ) ( *p++ = (unsigned char) ((x) >> 8), *p++ = (unsigned char) (x) )

#define DWordPad(x) ((x + 3) & ~3)

#define OnReserve(x,maxX)	((x)*5 >= (maxX)*4) // reached 80% = 4/5 capacity
#define AddReserve(x)		((x) + (x)/4)

// character group codes as encoded in a TrueType file; notice that this is not the same encoding as used in the "cvt comment"...
#define	TOTALTYPE		8	/* total number of type groups */
#define	CG_UNKNOWN		0
#define	CG_OTHER		1
#define CG_UPPERCASE 	2
#define CG_LOWERCASE	3
#define	CG_FIGURE		4
#define	CG_PUNCTUATION	5
#define	CG_MS_BOXES		6
#define	CG_DINGBATS		7

#define OFFSETIndex			0
#define SOFFSETIndex		1
#define ANCHORIndex			2
#define SANCHORIndex		3
#define OVERLAPIndex		4
#define NONOVERLAPIndex		5
#define USEMYMETRICSIndex	6
#define SCALEDCOMPONENTOFFSETIndex 7
#define UNSCALEDCOMPONENTOFFSETIndex 8

#define MYOFFSETTABLESIZE 1000

// https://docs.microsoft.com/en-us/typography/opentype/spec/otvarcommonformats
#define POINTS_ARE_WORDS		0x80 // Flag indicating the data type used for point numbers in this run. If set, the point numbers are stored as unsigned 16-bit values (uint16); if clear, the point numbers are stored as unsigned bytes (uint8).
#define POINT_RUN_COUNT_MASK	0x7F // Mask for the low 7 bits of the control byte to give the number of point number elements, minus 1.

#define DELTAS_ARE_ZERO			0x80 // Flag indicating that this run contains no data (no explicit delta values are stored), and that the deltas for this run are all zero.
#define DELTAS_ARE_WORDS		0x40 // Flag indicating the data type for delta values in the run. If set, the run contains 16-bit signed deltas (int16); if clear, the run contains 8-bit signed deltas (int8).
#define DELTA_RUN_COUNT_MASK	0x3F // Mask for the low 6 bits to provide the number of delta values in the run, minus one.

const short charGroupToIntInFile[TOTALTYPE] = {CG_UNKNOWN, CG_OTHER, CG_UPPERCASE, CG_LOWERCASE, CG_FIGURE, CG_UNKNOWN, CG_UNKNOWN, CG_UNKNOWN};

const CharGroup intInFileToCharGroup[TOTALTYPE] = {anyGroup, otherCase, upperCase, lowerCase, figureCase, otherCase, otherCase, otherCase}; // map CG_MS_BOXES to CG_OTHER as per elik

typedef struct {
	unsigned char bCharSet; // 0 = no subset; 1 = win ansi
	unsigned char xRatio;
	unsigned char yStartRatio;
	unsigned char yEndRatio;
} VDMXRatio;

typedef struct {
	unsigned short ppemHeight;
	short yMax;
	short yMin;
} VDMXEntry;

typedef struct {
	unsigned short numEntries;
	unsigned char startPpemHeight;
	unsigned char endPpemHeight;
	VDMXEntry entry[1];
} VDMXGroup;

typedef struct {
	unsigned short version;
	unsigned short numGroups;
	unsigned short numRatios;
	VDMXRatio ratio[1];
} VDMXTable;

typedef struct {
	unsigned int	glyphId;
	const char *	glyphName;
} PSNAMELIST;

TrueTypeGlyph::TrueTypeGlyph(void) {
} // TrueTypeGlyph::TrueTypeGlyph

TrueTypeGlyph::~TrueTypeGlyph(void) {
	// nix
} // TrueTypeGlyph::~TrueTypeGlyph

int32_t ColorTransition(Vector V[], Vector W[]);
int32_t ColorTransition(Vector V[], Vector W[]) { // cf. also Intersect in "MathUtils.h"
	int32_t a11,a12,a21,a22,b1,b2,det,t1,t2;
	
	b1 = V[0].x - W[1].x; a11 = V[0].x - V[1].x; a12 = W[2].x - W[1].x;
	b2 = V[0].y - W[1].y; a21 = V[0].y - V[1].y; a22 = W[2].y - W[1].y;
	det = a11*a22 - a12*a21;
	if (det == 0) return 0; // parallel
	t1 = a22*b1 - a12*b2;
	t2 = a11*b2 - a21*b1;
	return (Sgn(t1) == Sgn(det) && 0 < Abs(t1) && Abs(t1) < Abs(det) && // intersection point is inside line V
			((Sgn(t2) == Sgn(det) && 0 < Abs(t2) && Abs(t2) < Abs(det)) || // intersection point is inside line W
			(t2 == det && Sgn(VectProdP(V[0],W[1],V[0],W[2]))*Sgn(VectProdP(V[0],W[3],V[0],W[2])) < 0))) ? 1 : 0; // or at the end of W and W arrives/departs on different sides of V
} // ColorTransition

LinkColor TrueTypeGlyph::TheColor(short from, short to) {
	Vector V[3],D[2],C[2],W[4];
	int32_t cont,pred,knot,succ,start,zero,end,n,parity,sgn,dir[4];
	
	for (cont = 0; cont < this->numContoursInGlyph; cont++) {
	 	start = this->startPoint[cont];
	 	n = this->endPoint[cont] - start + 1;
	 	V[0].x = this->x[start+n-1]; V[1].x = this->x[start];
	 	V[0].y = this->y[start+n-1]; V[1].y = this->y[start];
	 	D[0] = SubV(V[1],V[0]);
	 	for (knot = 0; knot < n; knot++) {
			V[2].x = this->x[start + (knot + 1)%n];
			V[2].y = this->y[start + (knot + 1)%n];
			D[1] = SubV(V[2],V[1]);
			sgn = D[0].x*D[1].y - D[0].y*D[1].x;
			this->dirChange[start + knot] = (short)Sgn(sgn); // +1 => left turn, -1 => right turn, 0 => straight
			V[0] = V[1]; V[1] = V[2]; D[0] = D[1];
		}
	}
	
	V[0].x = this->x[from] << 1;
	V[0].y = this->y[from] << 1;
	V[1].x = this->x[to] << 1;
	V[1].y = this->y[to] << 1;
	C[0] = ShrV(AddV(V[0],V[1]),1); // use extra bit at end to avoid precision loss upon dividing
	C[1].x = -32768; C[1].y = C[0].y-1; // and make sure color transition test line does not align with any straight line in the glyph...
	parity = 0;
	for (cont = 0; cont < this->numContoursInGlyph; cont++) {
		start = this->startPoint[cont];
		end = this->endPoint[cont];
		n = end - start + 1;
		for (pred = start + n - 1; pred >= start     && !(dir[0] = this->dirChange[pred]); pred--); // treat aligned straight lines as one single straight line
		for (zero = start;		   zero <  start + n && !(dir[1] = this->dirChange[zero]); zero++); // treat aligned straight lines as one single straight line
		if (zero < start + n) { // else pathological case of flat polygon
			W[0].x = this->x[pred] << 1;
			W[0].y = this->y[pred] << 1;
			W[1].x = this->x[zero] << 1;
			W[1].y = this->y[zero] << 1;
			knot = zero;
			do {
				do knot = knot == end ? start : knot + 1; while (!(dir[2] = this->dirChange[knot]));
				W[2].x = this->x[knot] << 1;
				W[2].y = this->y[knot] << 1;
				succ = knot;
				do succ = succ == end ? start : succ + 1; while (!(dir[3] = this->dirChange[succ]));
				W[3].x = this->x[succ] << 1;
				W[3].y = this->y[succ] << 1;
				if (Collinear(W[1],V[0],W[2],notOutside) && Collinear(W[1],V[1],W[2],notOutside))
					return dir[1] + dir[2] == 2 ? linkWhite : linkBlack; // V completely contained in W, concave edges are white, convex ones and stairs are black
				else if (VectProdP(V[0],V[1],W[1],W[2]) == 0 && 
						(Collinear(W[1],V[0],W[2],inside) || Collinear(W[1],V[1],W[2],inside) || Collinear(V[0],W[1],V[1],inside) || Collinear(V[0],W[2],V[1],inside)))
					return linkGrey; // V partly overlaps W or v.v.
				else if (ColorTransition(V,W))
					return linkGrey;
				parity += ColorTransition(C,W); // else we are still either all black or all white, so keep track of parity
				W[0] = W[1]; W[1] = W[2]; dir[0] = dir[1]; dir[1] = dir[2];
			} while (knot != zero);
		}
	}
	return parity & 1 ? linkBlack : linkWhite;
} // TrueTypeGlyph::TheColor

TrueTypeFont::TrueTypeFont(void) {
	int32_t i;

	this->sfntHandle		 = NULL;
	this->sfntTmpHandle 	 = NULL;
	this->offsetTable		 = NULL;
	this->tmpOffsetTable	 = NULL;
	this->incrBuildSfntData	 = NULL;
	this->cvt				 = NULL;
	this->IndexToLoc		 = NULL;
	this->tmpIndexToLoc 	 = NULL;
	this->glit1				 = NULL;
	this->glit2				 = NULL;
	this->charCodeOf		 = NULL;
	this->charGroupOf		 = NULL;	
	this->glyphIndexMap      = NULL; 
	for (i = 0; i < Len(this->binData); i++) {
		this->binSize[i] = 0;
		this->binData[i] = NULL;
	}
	this->devMetricsPtr		 = NULL;
	this->ltshPtr			 = NULL;
	this->vdmxPtr			 = NULL;
	this->horMetric			 = NULL;
	this->postScriptNames	 = NULL; 

	this->bVariationTypeface_ = false;
	this->axisCount_ = 0;
} // TrueTypeFont::TrueTypeFont

bool TrueTypeFont::Create() {
//	create this part of font only once to avoid memory fragmentation
	this->maxSfntSize 					= trialSfntSize;
	this->maxTmpSfntSize				= trialSfntSize;
	this->maxGlitEntries				= trialNumGlyphs + 5;
#
	this->sfntHandle					= (unsigned char *)NewP(this->maxSfntSize);
	this->sfntTmpHandle					= (unsigned char *)NewP(this->maxSfntSize);

	this->offsetTable					= (sfnt_OffsetTable *) NewP(MYOFFSETTABLESIZE);
	this->tmpOffsetTable				= (sfnt_OffsetTable *) NewP(MYOFFSETTABLESIZE);
//	this->incrBuildSfntData allocated and deallocated locally
	this->cvt							= NewControlValueTable();
	this->IndexToLoc					= (uint32_t *)NewP(sizeof(int32_t)*this->maxGlitEntries);
	this->tmpIndexToLoc 				= (uint32_t *)NewP(sizeof(int32_t)*this->maxGlitEntries);
	this->glit1							= (sfnt_MemDataEntry *)NewP(sizeof(sfnt_MemDataEntry)*this->maxGlitEntries);
	this->glit2							= (sfnt_MemDataEntry *)NewP(sizeof(sfnt_MemDataEntry)*this->maxGlitEntries);
	this->charCodeOf					= (uint32_t *)NewP(sizeof(uint32_t)*this->maxGlitEntries);
	this->charGroupOf					= (unsigned char *)NewP(this->maxGlitEntries);	
	this->glyphIndexMap                 = new std::vector<UniGlyphMap>; 
	this->tmpFlags						= (unsigned char *)NewP(MAXPOINTS);
	this->hdmxBinSize                   = 0;
	this->ltshBinSize                   = 0;
	this->vdmxBinSize                   = 0;
	this->gaspBinSize                   = 0;
	this->tsicBinSize                   = 0;
    this->cvarBinSize                   = 0; 
	this->horMetric                     = (sfnt_HorizontalMetrics *)NewP(sizeof(sfnt_HorizontalMetrics)*this->maxGlitEntries);	
	this->gaspLoaded					= false; 
	
	return this->sfntHandle && this->sfntTmpHandle && this->offsetTable && this->tmpOffsetTable && this->cvt && this->IndexToLoc && this->tmpIndexToLoc &&
		   this->glit1 && this->glit2 && this->charCodeOf && this->charGroupOf && this->horMetric && this->glyphIndexMap; 
} // TrueTypeFont::Create

TrueTypeFont::~TrueTypeFont(void) {
	int32_t i;

	if (this->horMetric)			DisposeP((void **)&this->horMetric);
	if (this->vdmxPtr)				DisposeP((void **)&this->vdmxPtr);
	if (this->ltshPtr)				DisposeP((void **)&this->ltshPtr);
	if (this->devMetricsPtr)		DisposeP((void **)&this->devMetricsPtr);
	if (this->tmpFlags)				DisposeP((void **)&this->tmpFlags);
	for (i = 0; i < Len(this->binData); i++) {
		this->binSize[i] = 0;
		if (this->binData[i] != NULL) DisposeP((void **)&this->binData[i]);
	}	
	if (this->glyphIndexMap) {
		delete this->glyphIndexMap; 
		this->glyphIndexMap = NULL; 
	}
	if (this->charGroupOf)			DisposeP((void **)&this->charGroupOf);
	if (this->charCodeOf)			DisposeP((void **)&this->charCodeOf);
	if (this->glit2)				DisposeP((void **)&this->glit2);
	if (this->glit1)				DisposeP((void **)&this->glit1);
	if (this->tmpIndexToLoc)		DisposeP((void **)&this->tmpIndexToLoc);
	if (this->IndexToLoc)			DisposeP((void **)&this->IndexToLoc);
	if (this->cvt)					delete this->cvt;
//	this->incrBuildSfntData allocated and deallocated locally
	if (this->tmpOffsetTable)		DisposeP((void **)&this->tmpOffsetTable);
	if (this->offsetTable)			DisposeP((void **)&this->offsetTable);

	if (this->postScriptNames) {
		delete this->postScriptNames;
		this->postScriptNames = NULL;
	}

	if (this->sfntTmpHandle)		DisposeP((void **)&this->sfntTmpHandle);
	if (this->sfntHandle)			DisposeP((void **)&this->sfntHandle);

} // TrueTypeFont::~TrueTypeFont

void TrueTypeFont::AssertMaxSfntSize(uint32_t minSfntSize, bool assertMainHandle, bool assertTempHandle) {
	bool enough;

	minSfntSize = AddReserve(((minSfntSize + 0xfffff) >> 20) << 20); // round up to the nearest MB and add reserve
	
	enough = true; // assume
	if (assertMainHandle && minSfntSize > this->maxSfntSize) enough = false;
	if (assertTempHandle && minSfntSize > this->maxTmpSfntSize) enough = false;
	if (enough) return; // we're done, enough reserve

	if (assertTempHandle && this->sfntTmpHandle) DisposeP((void **)&this->sfntTmpHandle);
	if (assertMainHandle && this->sfntHandle) DisposeP((void **)&this->sfntHandle);

	if (assertMainHandle) {
		this->sfntHandle = (unsigned char *)NewP(minSfntSize);
#
		if (this->sfntHandle) {
			this->maxSfntSize = minSfntSize;
		} else {
			this->maxSfntSize = 0; // we've just de-allocated it and failed to allocate it
		}
	}
	if (assertTempHandle) {
		this->sfntTmpHandle	= (unsigned char *)NewP(minSfntSize);

		if (this->sfntTmpHandle) {
			this->maxTmpSfntSize = minSfntSize;
		} else {
			this->maxTmpSfntSize = 0; // we've just de-allocated it and failed to allocate it
		}
	}
} // TrueTypeFont::AssertMaxSfntSize

void TrueTypeFont::AssertMaxGlyphs(int32_t minGlyphs) {

	if (AddReserve(minGlyphs) < this->maxGlitEntries) return; // we're done, enough reserve
	
	if (this->horMetric)			DisposeP((void**)&this->horMetric);
	if (this->charGroupOf)			DisposeP((void**)&this->charGroupOf);
	if (this->charCodeOf)			DisposeP((void**)&this->charCodeOf);
	if (this->glit2)				DisposeP((void**)&this->glit2);
	if (this->glit1)				DisposeP((void**)&this->glit1);
	if (this->tmpIndexToLoc)		DisposeP((void**)&this->tmpIndexToLoc);
	if (this->IndexToLoc)			DisposeP((void**)&this->IndexToLoc);
	
	minGlyphs = AddReserve(((minGlyphs + 0x3ff) >> 10) << 10); // round up to the nearest k and add reserve
	minGlyphs = Min(minGlyphs,ABSOLUTE_MAX_GLYPHS); // but don't push the limit

	this->IndexToLoc	 = (uint32_t *)NewP(sizeof(int32_t)*minGlyphs);
	this->tmpIndexToLoc  = (uint32_t *)NewP(sizeof(int32_t)*minGlyphs);
	this->glit1			 = (sfnt_MemDataEntry *)NewP(sizeof(sfnt_MemDataEntry)*minGlyphs);
	this->glit2			 = (sfnt_MemDataEntry *)NewP(sizeof(sfnt_MemDataEntry)*minGlyphs);
	this->charCodeOf	 = (uint32_t *)NewP(sizeof(uint32_t)*minGlyphs);
	this->charGroupOf	 = (unsigned char *)NewP(minGlyphs);
	this->horMetric		 = (sfnt_HorizontalMetrics *)NewP(sizeof(sfnt_HorizontalMetrics)*minGlyphs);
	
	this->maxGlitEntries = this->IndexToLoc && this->tmpIndexToLoc && this->glit1 && this->glit2 && this->charCodeOf && this->charGroupOf && this->horMetric ? minGlyphs : 0;
} // TrueTypeFont::AssertMaxGlyphs

void MaxSfntSizeError(const wchar_t from[], int32_t size, wchar_t errMsg[]);
void MaxSfntSizeError(const wchar_t from[], int32_t size, wchar_t errMsg[]) {
	swprintf(errMsg,WIDE_STR_FORMAT L", \r" BULLET L" Unable to allocate %li to work on this font.",from, size);
} // MaxSfntSizeError

bool TrueTypeFont::SetSfnt(short platformID, short encodingID, wchar_t errMsg[])
{
	this->UnpackFvar();
	this->UnpackAvar(); 

	cvtVariationInterpolator_ = std::make_shared<Variation::CVTVariationInterpolator1>();

	if (instanceManager_ == nullptr)
	{
		instanceManager_ = std::make_shared<Variation::InstanceManager>(); 
	}

	variationAxisTags_ = std::make_shared<std::vector<uint32_t>>();
	
	if (this->HasFvar())
	{
		axisCount_ = fvar_.axisCount;
		bVariationTypeface_ = true;

		// Set name table names per axis
		for (size_t index = 0; index < axisCount_; index++)
		{			
			uint32_t tag = fvar_.axisRecords[index].axisTag;
			variationAxisTags_->insert(variationAxisTags_->end(), tag);			
		}
		
		instanceManager_->SetAxisCount(axisCount_);
			
		if (this->HasPrivateCvar())
		{
			TSICHeader tsic; 
			this->tsicError = !this->GetPrivateCvar(tsic); 
			if (this->tsicError)
			{
				std::wstring str = L"Error: Unable to load TSIC table!" BRK;
				return false; 
			}
			else
			{
				this->MergePrivateCvarWithInstanceManager(tsic);
			}
		}
				
		instanceManager_->Sort();
	}

	return true; 
} // SetSfnt

 void TrueTypeFont::UnpackFvar(void)
{
	unsigned char* charStart = this->GetTablePointer(tag_FontVariations);
	unsigned short* start = reinterpret_cast<unsigned short*>(charStart);
	if (start == nullptr)
		return;

	fvar_.axisRecords.clear(); 
	fvar_.instanceRecords.clear();

	unsigned short* packed = start; 

	fvar_.majorVersion = SWAPWINC(packed); 
	fvar_.minorVersion = SWAPWINC(packed); 	
	fvar_.offsetToAxesArray = SWAPWINC(packed); 
	fvar_.countSizePairs = SWAPWINC(packed);
	fvar_.axisCount = SWAPWINC(packed);
	fvar_.axisSize = SWAPWINC(packed);
	fvar_.instanceCount = SWAPWINC(packed);
	fvar_.instanceSize = SWAPWINC(packed);

	packed = (unsigned short*)(charStart + fvar_.offsetToAxesArray); 
	for (int16_t axisIndex = 0; axisIndex < fvar_.axisCount; axisIndex++)
	{
		VariationAxisRecord axisRecord; 
		unsigned short* packed1 = packed;

		uint32_t* ptag = reinterpret_cast<uint32_t*>(packed1);
		axisRecord.axisTag = *ptag; 
		packed1 += 2;

		int32_t* pminValue = reinterpret_cast<int32_t*>(packed1);
		int32_t minValue = SWAPL(*pminValue); 
		axisRecord.minValue.SetRawValue(minValue); 
		packed1 += 2;

		int32_t* pdefaultValue = reinterpret_cast<int32_t*>(packed1);
		int32_t defaultValue = SWAPL(*pminValue);
		axisRecord.defaultValue.SetRawValue(defaultValue);
		packed1 += 2;

		int32_t* pmaxValue = reinterpret_cast<int32_t*>(packed1);
		int32_t maxValue = SWAPL(*pminValue);
		axisRecord.maxValue.SetRawValue(maxValue);
		packed1 += 2;

		axisRecord.flags = SWAPWINC(packed1);
		axisRecord.nameID = SWAPWINC(packed1);

		fvar_.axisRecords.push_back(axisRecord);

		packed = reinterpret_cast<unsigned short*>(reinterpret_cast<unsigned char*>(packed) + fvar_.axisSize);
	}

	for (uint16_t instanceIndex = 0; instanceIndex < fvar_.instanceCount; instanceIndex++)
	{
		VariationInstanceRecord instanceRecord; 
		unsigned short* packed1 = packed; 

		instanceRecord.nameID = SWAPWINC(packed1);
		instanceRecord.flags = SWAPWINC(packed1);

		for (int16_t axisIndex = 0; axisIndex < fvar_.axisCount; axisIndex++)
		{
			int32_t* pcoord = reinterpret_cast<int32_t*>(packed1);
			Fixed16_16 coord(SWAPL(*pcoord)); 
			packed1 += 2;

			instanceRecord.coordinates.push_back(coord); 
		}

		fvar_.instanceRecords.push_back(instanceRecord);

		// don't bother with optional postScriptNameID for now

		packed = reinterpret_cast<unsigned short*>(reinterpret_cast<unsigned char*>(packed) + fvar_.instanceSize);
	}
}

void TrueTypeFont::UnpackAvar(void)
{
	unsigned short* packed = reinterpret_cast<unsigned short*>(this->GetTablePointer(tag_AxisVariations));
	if (packed == nullptr)
		return;

	avar_.axisSegmentMaps.clear(); 

	avar_.majorVersion = SWAPWINC(packed);
	avar_.minorVersion = SWAPWINC(packed);
	avar_.reserved = SWAPWINC(packed);
	avar_.axisCount = SWAPWINC(packed);

	// segment maps array
	for (uint16_t axisIndex = 0; axisIndex < avar_.axisCount; ++axisIndex)
	{
		ShortFracSegment segment;
		segment.pairCount = SWAPWINC(packed); 
		for (uint16_t pairIndex = 0; pairIndex < segment.pairCount; pairIndex++)
		{
			int16_t fromCoord = SWAPWINC(packed);
			int16_t toCoord = SWAPWINC(packed);
			Fixed2_14 from(fromCoord);
			Fixed2_14 to(toCoord); 
			ShortFracCorrespondence map{from ,to}; 
			segment.axisValueMaps.push_back(map); 
		}
		avar_.axisSegmentMaps.push_back(segment);
	}	
}

bool TrueTypeFont::IsMakeTupleName(const std::wstring &name) const
{
	std::wstring opt1 = L"User";
	std::wstring opt2 = L"Unnamed";

	if (name.size() >= opt1.size() && name.compare(0, opt1.size(), opt1) == 0)
		return true; 

	if (name.size() >= opt2.size() && name.compare(0, opt2.size(), opt2) == 0)
		return true;

	return false; 
} // TrueTypeFont::IsMakeTupleName

bool TrueTypeFont::Read(File *file, TrueTypeGlyph *glyph, short *platformID, short *encodingID, wchar_t errMsg[]) {
	int32_t glyphIndex;

	this->sfntSize = file->Length();

	this->AssertMaxSfntSize(this->sfntSize,true,true);

	if (this->sfntSize > this->maxSfntSize) {
		MaxSfntSizeError(L"Read: This font is too large",this->sfntSize,errMsg); return false;
	}
	file->ReadBytes(this->sfntSize,	this->sfntHandle);
	if (file->Error()) {
		swprintf(errMsg,L"Read: I/O error reading this font"); return false;
	}
	
	if (!this->UnpackHeadHheaMaxpHmtx(errMsg)) return false;

	if(*platformID == 3 && *encodingID == 1) 
		*encodingID = 10; // lets first try 3,10 and default lower if not present
	
	if (!this->CMapExists(*platformID,*encodingID) && !this->DefaultCMap(platformID,encodingID,errMsg)) return false;
	
	if (!(this->UnpackGlitsLoca(errMsg) && this->UpdateMaxPointsAndContours(errMsg) && this->UnpackCMap(*platformID,*encodingID,errMsg) && this->UnpackCharGroup(errMsg))) return false;
	
	// Clear for new font. 
	if (instanceManager_ != nullptr)
	{
		instanceManager_->clear(); 
	}

	if (!this->SetSfnt(*platformID, *encodingID, errMsg)) return false;

	// not the smartest way to get these numbers, another historical legacy
	if ((glyphIndex = this->GlyphIndexOf(L'H')) == ILLEGAL_GLYPH_INDEX) this->capHeight = this->unitsPerEm;
	else if (this->GetGlyph(glyphIndex,glyph,errMsg)) this->capHeight = glyph->ymax;
	else return false;
	if ((glyphIndex = this->GlyphIndexOf(L'x')) == ILLEGAL_GLYPH_INDEX) this->xHeight = this->unitsPerEm;
	else if (this->GetGlyph(glyphIndex,glyph,errMsg)) this->xHeight = glyph->ymax;
	else return false;
	if ((glyphIndex = this->GlyphIndexOf(L'p')) == ILLEGAL_GLYPH_INDEX) this->descenderHeight = 0;
	else if (this->GetGlyph(glyphIndex,glyph,errMsg)) this->descenderHeight = glyph->ymin;
	else return false;	

	// Clear for new font.
	if (this->postScriptNames) {
		delete this->postScriptNames;
		this->postScriptNames = NULL;
	}
		
	return true; // by now
} // TrueTypeFont::Read

bool TrueTypeFont::Write(File *file, wchar_t errMsg[]) {
	file->WriteBytes(this->sfntSize, this->sfntHandle);
	//if (!file->Error()) file->SetPos(this->sfntSize,true); // truncate file to current size
	if (file->Error()) {
		swprintf(errMsg,L"I/O error writing this font"); return false;
	}
	return true; // by now
} // TrueTypeFont::Write

ControlValueTable *TrueTypeFont::TheCvt(void) {
	return this->cvt;
} // TrueTypeFont::TheCvt

bool TrueTypeFont::GetCvt(TextBuffer *cvtText, wchar_t errMsg[]) {
	int32_t size,entries,i;
	short *cvt;
	wchar_t buffer[maxLineSize];
	
	cvt = (short*)this->GetTablePointer(tag_ControlValue);
	size = this->GetTableLength(tag_ControlValue);
	entries = size >> 1;
	if (entries < 0 || MAXCVT < entries) { swprintf(errMsg,L"GetCvt: There are too many cvt entries %li",entries); return false; }
	
	if (!this->GetSource(true,CVT_GLYPH_INDEX,cvtText,errMsg)) return false;

	if (cvtText->TheLength() == 0) { // decode binary cvt
		for (i = 0; i < entries; i++) { swprintf(buffer,L"%li: %hi\r",i,SWAPW(cvt[i])); cvtText->Append(buffer); }
		
	}
	this->cvt->PutCvtBinary(size,(unsigned char *)cvt);
	return true; // by now
} // TrueTypeFont::GetCvt

bool TrueTypeFont::GetPrep(TextBuffer *prepText, wchar_t errMsg[]) {
	unsigned char *data;
	int32_t size;	

	errMsg[0] = L'\0';
	data = this->GetTablePointer(tag_PreProgram);
	size = this->GetTableLength(tag_PreProgram);
	if (size > MAXBINSIZE) {
		swprintf(errMsg,L"GetPrep: pre-program is %li bytes long (cannot be longer than %li bytes)",size,MAXBINSIZE);
		return false;
	}	
	return this->UpdateBinData(asmPREP,size,data) && this->GetSource(true,PRE_PGM_GLYPH_INDEX,prepText,errMsg); // get source after binary, in case binary exists but source doesn't...
} // TrueTypeFont::GetPrep

int32_t TrueTypeFont::PrepBinSize(void) {
	return this->binSize[asmPREP];
} // TrueTypeFont::PrepBinSize

bool TrueTypeFont::GetFpgm(TextBuffer *fpgmText, wchar_t errMsg[]) {
	unsigned char *data;
	int32_t size;	
	
	errMsg[0] = L'\0';
	data = this->GetTablePointer(tag_FontProgram);
	size = this->GetTableLength(tag_FontProgram);
	if (size > MAXBINSIZE) {
		swprintf(errMsg,L"GetFpgm: font program is %li bytes long (cannot be longer than %li bytes)",size,MAXBINSIZE);
		return false;
	}
	return this->UpdateBinData(asmFPGM,size,data) && this->GetSource(true,FONT_PGM_GLYPH_INDEX,fpgmText,errMsg); // get source after binary, in case binary exists but source doesn't...
} // TrueTypeFont::GetFpgm

int32_t TrueTypeFont::FpgmBinSize(void) {
	return this->binSize[asmFPGM];
} // TrueTypeFont::FpgmBinSize

bool TrueTypeFont::GetGlyf(int32_t glyphIndex, TextBuffer *glyfText, wchar_t errMsg[]) {	
	return this->GetSource(true,glyphIndex,glyfText,errMsg);		
	// here we don't get any binary, this is done in GetGlyph, which also deals with the glyph's bounding box or composite information
} // TrueTypeFont::GetGlyf


bool TrueTypeFont::GetTalk(int32_t glyphIndex, TextBuffer *talkText, wchar_t errMsg[]) {
	return this->GetSource(false,glyphIndex,talkText,errMsg);
} // TrueTypeFont::GetTalk


bool TrueTypeFont::GetGlyph(int32_t glyphIndex, TrueTypeGlyph *glyph, wchar_t errMsg[]) {
	short i,end;
	bool emptyGlyph,weHaveInstructions;
	unsigned char *sp;
	char signedByte;
	short numContoursInGlyph;
	unsigned short flags,cgIdx;
	short signedWord;
	unsigned short unsignedWord;
	int32_t numKnots,numContours,componentDepth,binSize;
	TrueTypeComponent *component;
	int32_t lsb = 0, advWidth = 0; 
		
	if (glyphIndex < 0 || this->numLocaEntries <= glyphIndex) {
		swprintf(errMsg,L"TrueTypeFont::GetGlyph: glyphIndex %li is out of range",glyphIndex); return false;
	}

	GetHMTXEntry(glyphIndex, &lsb, &advWidth);
	
	sp = (unsigned char *)this->GetTablePointer(tag_GlyphData) + this->IndexToLoc[glyphIndex];
	emptyGlyph = this->IndexToLoc[glyphIndex] == this->IndexToLoc[glyphIndex+1];
	
	glyph->numContoursInGlyph = 0;
	glyph->composite = glyph->useMyMetrics = false;
	this->UpdateBinData(asmGLYF,0,NULL);
	glyph->componentSize = 0; 	
	
	if (emptyGlyph) { 
		glyph->xx[PHANTOMPOINTS] -= glyph->xx[PHANTOMPOINTS-1]; 
		glyph->xx[PHANTOMPOINTS-1] = glyph->xx[0] = 0;
		glyph->yy[PHANTOMPOINTS] -= glyph->yy[PHANTOMPOINTS-1]; 
		glyph->yy[PHANTOMPOINTS-1] = glyph->yy[0] = 0;
		glyph->onCurve[PHANTOMPOINTS] = glyph->onCurve[PHANTOMPOINTS-1] = true; 
		glyph->onCurve[0] = false;
	
		glyph->startPoint[0] = glyph->endPoint[0] = 0;
		glyph->numContoursInGlyph = 0;
	}
	
	if (emptyGlyph) return true; // nothing else we could unpack
	
	signedWord = READALIGNEDWORD( sp );
	numContoursInGlyph = SWAPW(signedWord);

	if (numContoursInGlyph > MAXCONTOURS) {
		swprintf(errMsg,L"GetGlyph: glyph %li has %hi contours (cannot have more than %hi)",glyphIndex,numContoursInGlyph,MAXCONTOURS); return false;
	}

	glyph->startPoint[0] = 0;
	glyph->endPoint[0] = 0;
	glyph->onCurve[0] = true;
	glyph->x[0] = 0;
	glyph->y[0] = 0;
	glyph->xx[0] = 0; 
	glyph->yy[0] = 0;

	glyph->numContoursInGlyph = numContoursInGlyph; 
	signedWord = READALIGNEDWORD(sp);
	glyph->xmin = SWAPW(signedWord);

	signedWord = READALIGNEDWORD(sp);
	glyph->ymin = SWAPW(signedWord);

	signedWord = READALIGNEDWORD(sp);
	glyph->xmax = SWAPW(signedWord);

	signedWord = READALIGNEDWORD(sp);
	glyph->ymax = SWAPW(signedWord);		
	
	glyph->bluePrint.numComponents = 0;
	glyph->bluePrint.useMyMetrics = ILLEGAL_GLYPH_INDEX; // use nobody's metrics so far

	if ( numContoursInGlyph < 0  ) {
 		glyph->composite = true;
		weHaveInstructions = false;
 		do {
			if (glyph->bluePrint.numComponents >= MAXNUMCOMPONENTS) {
				swprintf(errMsg,L"GetGlyph: glyph %hu has more than %hu components ",glyphIndex,(unsigned short)MAXNUMCOMPONENTS); return false;
			}

 			component = &glyph->bluePrint.component[glyph->bluePrint.numComponents];

			flags = *(unsigned short *)(sp+0); flags = SWAPW(flags);
			cgIdx = *(unsigned short *)(sp+2); cgIdx = SWAPW(cgIdx);
 			
			glyph->componentData[ glyph->componentSize++] = READALIGNEDWORD( sp );
 			glyph->componentData[ glyph->componentSize++] = READALIGNEDWORD( sp );

			weHaveInstructions = (flags & WE_HAVE_INSTRUCTIONS) != 0; // otherwise VC++ 5.0 does the bitwise AND in 16 bit and then moves a BYTE, dropping the WE_HAVE_INSTRUCTIONS (0x0100) bit
			
			component->glyphIndex = cgIdx;
			component->unicode = this->charCodeOf[cgIdx];
			component->rounded = (flags & ROUND_XY_TO_GRID)   != 0;
			component->anchor  = (flags & ARGS_ARE_XY_VALUES) == 0;
			component->offsetX = component->offsetY = 0;
			component->parent  = component->child = 0;
			component->transform[0][0] = component->transform[1][1] = 16384; // 1 in 2.14 format
			component->transform[0][1] = component->transform[1][0] = 0;

			numKnots = numContours = componentDepth = 0;
			if (GetNumPointsAndContours(cgIdx,&numKnots,&numContours,&componentDepth)) {
				component->numContours = (unsigned short)numContours;
			} else {
				swprintf(errMsg,L"GetGlyph: failed to obtain number of contours for component %i of glyph %i",(int32_t)cgIdx,glyphIndex); return false;
			}

			if (flags & USE_MY_METRICS) { 
 				glyph->useMyMetrics = true;
				if (glyph->bluePrint.useMyMetrics == ILLEGAL_GLYPH_INDEX) glyph->bluePrint.useMyMetrics = cgIdx; // first one wins
 			}
 			if ( flags & ARG_1_AND_2_ARE_WORDS ) {
 				/* arg 1 and 2 */
				signedWord = *(signed short *)(sp+0); component->offsetX = SWAPW(signedWord);
				signedWord = *(signed short *)(sp+2); component->offsetY = SWAPW(signedWord);
				glyph->componentData[ glyph->componentSize++] = READALIGNEDWORD( sp );
				glyph->componentData[ glyph->componentSize++] = READALIGNEDWORD( sp );
			} else {
 				/* arg 1 and 2 as bytes */
				signedByte = *(signed char *)(sp+0); component->offsetX = signedByte;
				signedByte = *(signed char *)(sp+1); component->offsetY = signedByte;
 	 			glyph->componentData[ glyph->componentSize++] = READALIGNEDWORD( sp );
 			}
			
			if (component->anchor) {
				component->parent = component->offsetX; component->offsetX = 0;
				component->child  = component->offsetY; component->offsetY = 0;
			}
			
			if ( flags & WE_HAVE_A_SCALE ) {
 				/* scale */
				signedWord = *(signed short *)(sp+0); component->transform[0][0] = component->transform[1][1] = SWAPW(signedWord);
  	 			glyph->componentData[ glyph->componentSize++] = READALIGNEDWORD( sp );
 			} else if ( flags & WE_HAVE_AN_X_AND_Y_SCALE ) {
 				/* xscale, yscale */
   	 			signedWord = *(signed short *)(sp+0); component->transform[0][0] = SWAPW(signedWord);
  	 			signedWord = *(signed short *)(sp+2); component->transform[1][1] = SWAPW(signedWord);
  	 			glyph->componentData[ glyph->componentSize++] = READALIGNEDWORD( sp );
  	 			glyph->componentData[ glyph->componentSize++] = READALIGNEDWORD( sp );
 			} else if ( flags & WE_HAVE_A_TWO_BY_TWO ) {
 				/* xscale, scale01, scale10, yscale */
   	 			signedWord = *(signed short *)(sp+0); component->transform[0][0] = SWAPW(signedWord);
  	 			signedWord = *(signed short *)(sp+2); component->transform[0][1] = SWAPW(signedWord);
  	 			signedWord = *(signed short *)(sp+4); component->transform[1][0] = SWAPW(signedWord);
  	 			signedWord = *(signed short *)(sp+6); component->transform[1][1] = SWAPW(signedWord);
   	 			glyph->componentData[ glyph->componentSize++] = READALIGNEDWORD( sp );
  	 			glyph->componentData[ glyph->componentSize++] = READALIGNEDWORD( sp );
   	 			glyph->componentData[ glyph->componentSize++] = READALIGNEDWORD( sp );
  	 			glyph->componentData[ glyph->componentSize++] = READALIGNEDWORD( sp );
 			}
			component->identity = component->transform[0][0] == 16384 && component->transform[1][1] == 16384 // 1 in 2.14 format
							   && component->transform[0][1] ==     0 && component->transform[1][0] ==     0;
 			
			if ( glyph->componentSize > MAXCOMPONENTSIZE ) { // maybe too late already...
				swprintf(errMsg,L"GetGlyph: glyph %li has %hi bytes of component data (cannot have more than %hi)",glyphIndex,glyph->componentSize,MAXCOMPONENTSIZE); return false;
 			}

			glyph->bluePrint.numComponents++;

		} while ( flags & MORE_COMPONENTS );
		glyph->ComponentVersionNumber = numContoursInGlyph;

		if (weHaveInstructions) {
			binSize = READALIGNEDWORD(sp);

			binSize = SWAPW(binSize);
			if (binSize > MAXBINSIZE) {
				swprintf(errMsg, L"GetGlyph: glyph %li has %li bytes of instruction (cannot have more than %li)", glyphIndex, binSize, MAXBINSIZE); return false;
			}

			this->UpdateBinData(asmGLYF, binSize, sp);
			sp += binSize;
		}

 	} else {

		uint8 abyOnCurve[MAXPOINTS]; 		
		numKnots = 0; 

		if (numContoursInGlyph > 0)
		{
			unsignedWord = READALIGNEDWORD(sp);
			glyph->endPoint[0] = SWAPW(unsignedWord);

			numKnots = glyph->endPoint[0] + 1;

			for (int32 lContourIndex = 1; lContourIndex < numContoursInGlyph; lContourIndex++)
			{
				glyph->startPoint[lContourIndex] = glyph->endPoint[lContourIndex - 1] + 1;

				unsignedWord = READALIGNEDWORD(sp);
				glyph->endPoint[lContourIndex] = SWAPW(unsignedWord);

				if (numKnots > glyph->endPoint[lContourIndex] || numKnots <= 0)
				{
					return false;
				}
				numKnots = glyph->endPoint[lContourIndex] + 1;
			}
		}

		unsignedWord = READALIGNEDWORD(sp);
		binSize = SWAPW(unsignedWord);

		if (binSize > MAXBINSIZE) {
			swprintf(errMsg, L"GetGlyph: glyph %li has %li bytes of instruction (cannot have more than %li)", glyphIndex, binSize, MAXBINSIZE); return false;
		}

		this->UpdateBinData(asmGLYF, binSize, sp);
		sp += binSize;

		// flags

		unsigned short usRepeatCount = 0;
		uint8       byRepeatFlag;

		int32 lPointCount = numKnots;
		uint8* pbyFlags = abyOnCurve;

		while (lPointCount > 0)
		{
			if (usRepeatCount == 0)
			{
			
				*pbyFlags = *sp;

				if (*pbyFlags & REPEAT_FLAGS)
				{
					sp++;

					usRepeatCount = *sp;
				}
				sp++;
				pbyFlags++;
				lPointCount--;
			}
			else
			{
				byRepeatFlag = pbyFlags[-1];
				lPointCount -= usRepeatCount;

				if (lPointCount < 0)
				{
					return false;
				}

				while (usRepeatCount > 0)
				{
					*pbyFlags = byRepeatFlag;
					pbyFlags++;
					usRepeatCount--;
				}
			}
		}

		if (usRepeatCount > 0)
		{
			return false;
		}

		// X
		short sXValue = 0;
		F26Dot6* pf26OrigX = glyph->xx;
		pbyFlags = abyOnCurve;

		for (int32 lPointIndex = 0; lPointIndex < numKnots; lPointIndex++)
		{
			if (*pbyFlags & XSHORT)
			{
				if (*pbyFlags & SHORT_X_IS_POS)
				{
					sXValue += (int16)((uint8)*sp++);
				}
				else
				{
					sXValue -= (int16)((uint8)*sp++);
				}
			}
			else if (!(*pbyFlags & NEXT_X_IS_ZERO))
			{
				// two bytes
				sXValue += SWAPW(*((int16*)sp));
				sp += sizeof(int16);
			}
			*pf26OrigX = (F26Dot6)sXValue;
			pf26OrigX++;
			pbyFlags++;
		}

		// Y
		short sYValue = 0;
		F26Dot6* pf26OrigY = glyph->yy;
		pbyFlags = abyOnCurve;

		for (int32 lPointIndex = 0; lPointIndex < numKnots; lPointIndex++)
		{
			if (*pbyFlags & YSHORT)
			{				
				if (*pbyFlags & SHORT_Y_IS_POS)
				{
					sYValue += (int16)((uint8)*sp++);
				}
				else
				{
					sYValue -= (int16)((uint8)*sp++);
				}
			}
			else if (!(*pbyFlags & NEXT_Y_IS_ZERO))
			{
				// two bytes
				sYValue += SWAPW(*((int16*)sp));
				sp += sizeof(int16);
			}
			*pf26OrigY = (F26Dot6)sYValue;
			pf26OrigY++;

			*pbyFlags &= ONCURVE;
			glyph->onCurve[lPointIndex] = *pbyFlags & ONCURVE; 
			pbyFlags++;
		}	

		end = glyph->endPoint[glyph->numContoursInGlyph - 1];

		F26Dot6 fxXMinMinusLSB = glyph->xmin - lsb; 
		glyph->xx[end + 1 + LEFTSIDEBEARING] = fxXMinMinusLSB;
		glyph->xx[end + 1 + RIGHTSIDEBEARING] = fxXMinMinusLSB + advWidth;
		glyph->yy[end + 1 + LEFTSIDEBEARING] = 0; 
		glyph->yy[end + 1 + RIGHTSIDEBEARING] = 0; 

		for (i = 0; i <= end + PHANTOMPOINTS; i++) { // lsb, rsb
			glyph->x[i] = (short)(glyph->xx[i]); // >> places6);
			glyph->y[i] = (short)(glyph->yy[i]); // >> places6);
		}
	}	
	
	this->UpdateGlyphProfile(glyph);
	this->UpdateMetricProfile(glyph);
		
	return true; // by now
} // TrueTypeFont::GetGlyph

int32_t TrueTypeFont::GlyfBinSize(void) {
	return this->binSize[asmGLYF];
} // TrueTypeFont::GlyfBinSize

unsigned char* TrueTypeFont::GlyfBin(void) {
	return this->binData[asmGLYF]; 
}

bool TrueTypeFont::GetHMTXEntry(int32_t glyphIndex, int32_t *leftSideBearing, int32_t *advanceWidth) {
	sfnt_HorizontalMetrics *horMetric;

	*leftSideBearing = *advanceWidth = 0;

	if (glyphIndex < 0 || this->numberOfGlyphs <= glyphIndex) glyphIndex = 0; // map to missing glyph
	
	horMetric = &this->horMetric[glyphIndex];
	*leftSideBearing = horMetric->leftSideBearing;
	*advanceWidth = horMetric->advanceWidth;

	return true; // by now
} // TrueTypeFont::GetHMTXEntry

int32_t TrueTypeFont::NumberOfGlyphs(void) {
	return Min(this->GetTableLength(tag_IndexToLoc)/(this->shortIndexToLocTable ? sizeof(short) : sizeof(int32_t)) - 1,this->profile.numGlyphs);
	// in a correct font, there are this->profile.numGlyphs + 1 (ie. maxp->numGlyphs + 1) entries in the 'loca' table, however, according to 
	// claudebe there are still lots of fonts that have an incorrect size for the loca table, so it is safe to use the minimum of the two.
} // TrueTypeFont::NumberOfGlyphs

bool Compare_UniGlyphMap(UniGlyphMap first, UniGlyphMap second)
{
	// Comparison function that, taking two values of the same type than those 
    // contained in the list object, returns true if the first argument goes before the 
    // second argument in the specific order, and false otherwise. 

	return (first.unicode < second.unicode);		
}

int32_t TrueTypeFont::GlyphIndexOf(uint32_t charCode) {
	std::vector<UniGlyphMap>::iterator it; 
	UniGlyphMap map; 
	map.unicode = charCode;

	if(!std::binary_search(glyphIndexMap->begin(), glyphIndexMap->end(), map, Compare_UniGlyphMap))
		return ILLEGAL_GLYPH_INDEX; 

	it = std::lower_bound(glyphIndexMap->begin(), glyphIndexMap->end(), map, Compare_UniGlyphMap); 

	return it->glyphIndex; 
} // TrueTypeFont::GlyphIndexOf

bool TrueTypeFont::GlyphIndecesOf(wchar_t textString[], int32_t maxNumGlyphIndeces, int32_t glyphIndeces[], int32_t *numGlyphIndeces, wchar_t errMsg[]) {
	int32_t i,dec,hex;
	wchar_t ch;

	errMsg[0] = (char)0;
	i = *numGlyphIndeces = 0; ch = textString[i++];
	while (ch != 0 && *numGlyphIndeces < maxLineSize-1) {
		if (ch == L'^') { // (decimal) glyphIndex
			ch = textString[i++]; dec = 0;
			while (ch && ch != L'^') {
				if (L'0' <= ch && ch <= L'9') dec = 10*dec + (int32_t)(ch - L'0');
				else { swprintf(errMsg,L"illegal decimal digit in glyph index"); return false; }
				ch = textString[i++];
			}
			if (ch == L'^') ch = textString[i++];
			else { swprintf(errMsg,L"closing ^ missing"); return false; }
		} else if (ch == L'~') { // (hexadecimal) charCode
			ch = textString[i++]; hex = 0;
			while (ch && ch != L'~') {
				if (L'0' <= ch && ch <= L'9') hex = 16*hex + (int32_t)(ch - L'0');
				else if (L'A' <= ch && ch <= L'F') hex = 16*hex + (int32_t)(ch - L'A' + 10);
				else if (L'a' <= ch && ch <= L'f') hex = 16*hex + (int32_t)(ch - L'a' + 10);
				else { swprintf(errMsg,L"illegal hexadecimal digit in glyph index"); return false; }
				ch = textString[i++];
			}
			if (ch == L'~') ch = textString[i++];
			else { swprintf(errMsg,L"closing ~ missing"); return false; }
			dec = this->GlyphIndexOf(hex);
		} else {
			dec = this->GlyphIndexOf(ch);
			ch = textString[i++];
		}
		if (dec < 0 || this->numberOfGlyphs <= dec) dec = 0; // map to missing glyph
		glyphIndeces[(*numGlyphIndeces)++] = dec;
	}

	return true; // by now
} // TrueTypeFont::GlyphIndecesOf

uint32_t TrueTypeFont::CharCodeOf(int32_t glyphIndex) {
	return this->charCodeOf[glyphIndex];
} // TrueTypeFont::CharCodeOf

uint32_t TrueTypeFont::FirstChar() {
	return (glyphIndexMap->begin()->unicode);
}

uint32_t TrueTypeFont::AdjacentChar(uint32_t charCode, bool forward) {
	uint32_t returnCode = 0; 
	uint32_t size = (uint32_t)glyphIndexMap->size(); 
	UniGlyphMap map; 
	map.unicode = charCode;
	std::vector<UniGlyphMap>::iterator it;
	
	if(std::binary_search(glyphIndexMap->begin(), glyphIndexMap->end(), map, Compare_UniGlyphMap)) {
		it = std::lower_bound(glyphIndexMap->begin(), glyphIndexMap->end(), map, Compare_UniGlyphMap); 
		int32_t delta = forward ? 1 : size - 1; 
		returnCode = glyphIndexMap->at(((it - glyphIndexMap->begin()) + delta) % size).unicode; 
	}else {
		it = std::lower_bound(glyphIndexMap->begin(), glyphIndexMap->end(), map, Compare_UniGlyphMap); 
		int32_t delta = forward ? 0 : size - 1; 
		returnCode = glyphIndexMap->at(((it - glyphIndexMap->begin()) + delta) % size).unicode; 
	}	
	
	return returnCode;
} // TrueTypeFont::AdjacentChar

CharGroup TrueTypeFont::CharGroupOf(int32_t glyphIndex) {
	if (glyphIndex < 0 || this->profile.numGlyphs <= glyphIndex) glyphIndex = 0; // safety measure, just in case
	return (CharGroup)this->charGroupOf[glyphIndex];
} // TrueTypeFont::CharGroupOf
	
bool TrueTypeFont::CMapExists(short platformID, short encodingID) {
	sfnt_char2IndexDirectory *cmap = (sfnt_char2IndexDirectory *)this->GetTablePointer(tag_CharToIndexMap);
	short i;
	int num = SWAPW(cmap->numTables);

	platformID = SWAPW(platformID);
	encodingID = SWAPW(encodingID);
	for (i = 0; i < num && (cmap->platform[i].platformID != platformID || cmap->platform[i].specificID != encodingID); i++);
	return i < num;
} // TrueTypeFont::CMapExists

bool TrueTypeFont::DefaultCMap(short *platformID, short *encodingID, wchar_t errMsg[]) {
	sfnt_char2IndexDirectory *cmap = (sfnt_char2IndexDirectory *)this->GetTablePointer(tag_CharToIndexMap);
	int32_t i,num = SWAPW(cmap->numTables);
	
	if (num <= 0) { swprintf(errMsg,L"There is no cmap in this font"); return false; }
	*platformID = CSWAPW(plat_MS); *encodingID = CSWAPW(10);
	for (i = 0; i < num && (cmap->platform[i].platformID != *platformID || cmap->platform[i].specificID != *encodingID); i++);
	if (i < num) goto found; // found preferred cmap...
	*encodingID = CSWAPW(1);
	for (i = 0; i < num && (cmap->platform[i].platformID != *platformID || cmap->platform[i].specificID != *encodingID); i++);
	if (i < num) goto found; // found preferred cmap...
	for (i = 0; i < num && cmap->platform[i].platformID != *platformID; i++);
	if (i < num) { *encodingID = cmap->platform[i].specificID; goto found; } // found cmap in preferred platform...
	*platformID = cmap->platform[0].platformID;
	*encodingID = cmap->platform[0].specificID;
found:
	*platformID = SWAPW(*platformID); *encodingID = SWAPW(*encodingID);
	return true; // found any cmap at all
} // TrueTypeFont::DefaultCMap

bool TrueTypeFont::UnpackCMap(short platformID, short encodingID, wchar_t errMsg[]) {
	int32_t i,num;
	sfnt_char2IndexDirectory *cmap;
	sfnt_mappingTable *map;
	
	for (i = 0; i < this->maxGlitEntries; i++) this->charCodeOf[i] = 0xFFFF;
	this->glyphIndexMap->clear(); 	
	this->numberOfGlyphs = this->NumberOfGlyphs();
	this->numberOfChars = 0; // incremented in EnterChar above...
	
	cmap = (sfnt_char2IndexDirectory*)this->GetTablePointer(tag_CharToIndexMap);
	num = SWAPW(cmap->numTables);

	platformID = SWAPW(platformID);
	encodingID = SWAPW(encodingID);

	for (i = 0; i < num && (cmap->platform[i].platformID != platformID || cmap->platform[i].specificID != encodingID); i++);
	if (i == num) { swprintf(errMsg,L"Unpacking cmap: cmap for platform id %hi and encoding id %hi not found",platformID,encodingID); return false; }
	
	map = (sfnt_mappingTable*)(reinterpret_cast<unsigned char*>(cmap) + SWAPL(cmap->platform[i].offset));
	uint16 format = SWAPW(map->format); 
	switch (format) {
		case 0:  this->GetFmt0(map);  break;
		case 4:  this->GetFmt4(map);  break;
		case 6:  this->GetFmt6(map);  break;
		case 12: this->GetFmt12(map); break; 
		default: swprintf(errMsg,L"Unpacking cmap: cmap format %hi not implemented",map->format); return false;
	}
	return true; // by now
} // TrueTypeFont::UnpackCMap

bool TrueTypeFont::IsCvarTupleData()
{
	uint16_t recordCount = 0; 

	if (this->IsVariationFont())
	{
		auto instanceManager = this->GetInstanceManager(); 
		auto cvarTuples = instanceManager->GetCvarTuples();
				
		for (auto cvarTuplesIt = cvarTuples->cbegin(); cvarTuplesIt != cvarTuples->cend(); ++cvarTuplesIt)
		{
			recordCount += (uint16_t)(*cvarTuplesIt)->cvt.size();
		}
	}

	return recordCount > 0;
}

int32_t TrueTypeFont::EstimatePrivateCvar()
{
	int32_t size = 0;

	if (!this->IsVariationFont())
		return size;

	auto instanceManager = this->GetInstanceManager();
	auto tsicInstances = instanceManager->GetPrivateCvarInstances();
	auto axisCount = this->GetVariationAxisCount();

	bool editedValues = false; 
	// Any edited values?
	for (const auto & instance : *tsicInstances)
	{
		for (const auto & editedValue : instance->editedCvtValues)
		{
			if (!editedValues && editedValue.Edited())
			{
				editedValues = true; 
			}
		}
	}

	// Anything to do?
	if (!editedValues)
		return size; 

	size += sizeof(uint16_t); // majorVersion
	size += sizeof(uint16_t); // minorVersion
	size += sizeof(uint16_t); // flags
	size += sizeof(uint16_t); // axisCount
	size += sizeof(uint16_t); // recordCount
	size += sizeof(uint16_t); // reserved

	size += sizeof(uint32_t) * axisCount; // AxisArray
	size += sizeof(uint16_t) * static_cast<int32_t>(tsicInstances->size()) * axisCount; //RecordLocations

	// TSICRecords
	for (const auto & instance : *tsicInstances)
	{
		size += sizeof(uint16_t); // flags
		size += sizeof(uint16_t); // numCVTEntries
		size += sizeof(uint16_t); // nameLength
		std::wstring name = instance->GetName();
		size += static_cast<int32_t>(name.size() * sizeof(wchar_t)); 
		
		for (const auto & editedValue : instance->editedCvtValues)
		{
			if (editedValue.Edited())
			{
				size += sizeof(uint16_t); // CVTArray entry
				size += sizeof(int16_t);  // CVTValueArray entry 
			}
		}
	}

	return size; 
}

int32_t TrueTypeFont::UpdatePrivateCvar(int32_t *size, unsigned char data[])
{
	unsigned short *packed = reinterpret_cast<unsigned short*>(data);

	if (!this->IsVariationFont())
	{
		*size = 0;
		return *size;
	}

	auto instanceManager = this->GetInstanceManager();
	auto tsicInstances = instanceManager->GetPrivateCvarInstances();
	
	bool editedValues = false;
	// Any edited values?
	for (const auto & instance : *tsicInstances)
	{
		for (const auto & editedValue : instance->editedCvtValues)
		{
			if (!editedValues && editedValue.Edited())
			{
				editedValues = true;
			}
		}
	}

	// Anything to do?
	if (!editedValues)
	{
		*size = 0;
		return *size;
	}

	TSICHeader header; 
	header.majorVersion = 1; 
	// minorVersion 0 tags are mistakenly swapped
	// minorVersion 1 tags are not swapped
	header.minorVersion = 1; 
	header.flags = 0; 
	header.axisCount = this->GetVariationAxisCount(); 
	header.recordCount = static_cast<uint16_t>(tsicInstances->size()); 
	header.reserved = 0; 
	auto variationAxisTags = this->GetVariationAxisTags(); 

	*packed++ = SWAPW(header.majorVersion);
	*packed++ = SWAPW(header.minorVersion);
	*packed++ = SWAPW(header.flags);
	*packed++ = SWAPW(header.axisCount);
	*packed++ = SWAPW(header.recordCount);
	*packed++ = SWAPW(header.reserved); 

	assert(header.axisCount == variationAxisTags->size()); 
	// AxisArray
	for (const auto & tag : *variationAxisTags)
	{
		uint32_t* p = reinterpret_cast<uint32_t*>(packed); 
		*p = tag; 
		packed += 2; 
	}

	// RecordLocations
	for (const auto & instance : *tsicInstances)
	{
		assert(instance->peakF2Dot14.size() == header.axisCount);
		for (const auto & axisCoord : instance->peakF2Dot14)
		{
			uint16_t coord = axisCoord.GetRawValue(); 
			*packed++ = SWAPW(coord); 
		}
	}

	// TSICRecords
	for (const auto & instance : *tsicInstances)
	{
		TSICRecord record;
		record.flags = 0;

		record.numCVTEntries = 0;
		for (const auto & editedValue : instance->editedCvtValues)
		{
			if (editedValue.Edited())
			{
				record.numCVTEntries++;
			}
		}

		*packed++ = SWAPW(record.flags);
		*packed++ = SWAPW(record.numCVTEntries);

		record.nameLength = 0;
		std::wstring name = instance->GetName();
		if (!instance->IsNamedInstance() && !this->IsMakeTupleName(name))
		{
			record.nameLength = static_cast<uint16_t>(name.size());			
			*packed++ = SWAPW(record.nameLength);

			// NameArray
			for (const auto & character : name)
			{
				*packed++ = SWAPW(character);
			}
		}
		else
		{
			*packed++ = SWAPW(record.nameLength);
		}

		// CVTArray
		uint16_t cvt = 0; 
		for (const auto & editedValue : instance->editedCvtValues)
		{
			if (editedValue.Edited())
			{
				*packed++ = SWAPW(cvt); 
			}
			cvt++; 
		}

		// CVTValueArray
		for (const auto & editedValue : instance->editedCvtValues)
		{
			if (editedValue.Edited())
			{
				int16_t value = editedValue.Value<int16_t>(); 
				*packed++ = SWAPW(value);
			}
		}
	}

	unsigned char* end = reinterpret_cast<unsigned char*>(packed); 
	*size = static_cast<int32_t>(end - data); 

	return *size; 
}

bool TrueTypeFont::HasPrivateCvar()
{
	const int32_t privateCvarMinSize = 10 * sizeof(uint16_t) + sizeof(uint32_t);

	int32_t tableLength = this->GetTableLength(PRIVATE_CVAR);

	return(tableLength > privateCvarMinSize && !this->tsicError);
}

bool TrueTypeFont::GetPrivateCvar(TSICHeader &header)
{
	int32_t tableLength = this->GetTableLength(PRIVATE_CVAR);

	if (tableLength == 0 || !this->IsVariationFont())
		return true; 

	unsigned short *packed = reinterpret_cast<unsigned short*>(this->GetTablePointer(PRIVATE_CVAR));
	if (packed == nullptr)
		return true;

	auto tags = this->GetVariationAxisTags(); 
	if (tags == nullptr)
		return true; 
	
	header.majorVersion = SWAPWINC(packed); 
	header.minorVersion = SWAPWINC(packed);
	header.flags = SWAPWINC(packed);
	header.axisCount = SWAPWINC(packed);
	header.recordCount = SWAPWINC(packed);
	header.reserved = SWAPWINC(packed);

	if (header.axisCount != this->GetVariationAxisCount())
		return false; 

	// AxisArray
	assert(tags->size() == header.axisCount); 
	for (uint16_t axisIndex = 0; axisIndex < header.axisCount; axisIndex++)
	{
		uint32_t* ptag = reinterpret_cast<uint32_t*>(packed); 
		uint32_t tag = *ptag; 
		uint32_t swapTag = SWAPL(*ptag);
		header.axes.push_back(tag); 
		if (tag != tags->at(axisIndex) && swapTag != tags->at(axisIndex))
			return false; 

		packed += 2; 
	}

	// RecordLocations
	for (uint16_t recordIndex = 0; recordIndex < header.recordCount; recordIndex++)
	{
		std::vector<Fixed2_14> coords; 
		for (uint16_t axisIndex = 0; axisIndex < header.axisCount; axisIndex++)
		{
			uint16_t cval = SWAPWINC(packed); 
			Fixed2_14 coord(cval);
			coords.push_back(coord);
		}
		header.locations.push_back(coords);
	}

	// TSICRecords
	for (uint16_t recordIndex = 0; recordIndex < header.recordCount; recordIndex++)
	{
		TSICRecord record; 
		record.name = L""; 

		record.flags = SWAPWINC(packed);
		record.numCVTEntries = SWAPWINC(packed);
		record.nameLength = SWAPWINC(packed);

		// NameArray
		for (uint16_t nameIndex = 0; nameIndex < record.nameLength; nameIndex++)
		{
			wchar_t character = SWAPWINC(packed);
			record.name += character; 
		}

		// CVTArray
		for (uint16_t cvtIndex = 0; cvtIndex < record.numCVTEntries; cvtIndex++)
		{
			uint16_t cvt = SWAPWINC(packed);
			record.cvts.push_back(cvt);
		}

		// CVTValueArray
		for (uint16_t cvtIndex = 0; cvtIndex < record.numCVTEntries; cvtIndex++)
		{
			int16_t cvtValue = SWAPWINC(packed);
			record.cvtValues.push_back(cvtValue);
		}

		header.tsicRecords.push_back(record); 
	}

	return true;
}

bool  TrueTypeFont::MergePrivateCvarWithInstanceManager(const TSICHeader &header)
{
	if (!this->IsVariationFont())
		return true;

	auto instanceManager = this->GetInstanceManager();
	auto axisCount = this->GetVariationAxisCount();

	std::vector<bool> recordMerged;
	recordMerged.resize(header.recordCount, false); 

	// For each record in private see if there exists same location among instances.
	for (uint16_t recordIndex = 0; recordIndex < header.recordCount; recordIndex++)
	{
		// Get the location
		std::vector<Fixed2_14> location = header.locations.at(recordIndex); 
		// Get the cooresponding Record
		const TSICRecord &record = header.tsicRecords[recordIndex];
		assert(record.cvts.size() == record.cvtValues.size());
		
		for (uint16_t instanceIndex = 0; instanceIndex < instanceManager->size(); instanceIndex++)
		{
			Variation::Instance& instance = instanceManager->at(instanceIndex);
			std::vector<Fixed2_14> instLoc = instance.GetNormalizedFixed2_14Coordinates(); 
			if (Variation::IsFixed2_14CoordEqual(location, instLoc))
			{
				recordMerged.at(recordIndex) = true; 
				
				// Copy source cvt data to instance
				if (record.numCVTEntries > 0)
				{
					uint16_t highestCvt = 0; 
					for (const auto & cvt : record.cvts)
						highestCvt = std::max(cvt, highestCvt);

					instance.editedCvtValues.clear(); 
					instance.editedCvtValues.resize(highestCvt + 1, Variation::EditedCvtValue());

					for (uint16_t recordArrayIndex = 0; recordArrayIndex < record.cvts.size(); recordArrayIndex++)
					{
						instance.editedCvtValues.at(record.cvts.at(recordArrayIndex)).Update(record.cvtValues.at(recordArrayIndex)); 
					}
					instance.SetAsCvar(true);
				}

				// Copy name to instance
				if (record.nameLength > 0)
				{
					instance.SetName(record.name); 
				}
			}
		}
	}

	// Deal with left over records
	for (uint16_t recordIndex = 0; recordIndex < header.recordCount; recordIndex++)
	{
		if (recordMerged.at(recordIndex) == false)
		{
			// Get the location
			std::vector<Fixed2_14> location = header.locations.at(recordIndex);
			// Get the cooresponding Record
			const TSICRecord &record = header.tsicRecords[recordIndex];
			assert(record.cvts.size() == record.cvtValues.size());

			Variation::Instance instance(location); 

			// Copy source cvt data to instance
			if (record.numCVTEntries > 0)
			{
				uint16_t highestCvt = 0;
				for (const auto & cvt : record.cvts)
					highestCvt = std::max(cvt, highestCvt);

				instance.editedCvtValues.resize(highestCvt + 1, Variation::EditedCvtValue());

				for (uint16_t recordArrayIndex = 0; recordArrayIndex < record.cvts.size(); recordArrayIndex++)
				{
					instance.editedCvtValues.at(record.cvts.at(recordArrayIndex)).Update(record.cvtValues.at(recordArrayIndex));
				}
				instance.SetAsCvar(true);
			}

			// Copy name to instance
			if (record.nameLength > 0)
			{
				instance.SetName(record.name);
			}

			instanceManager->Add(instance); 
		}
	}

	return true; 
}

int32_t  TrueTypeFont::EstimateCvar()
{
	int32_t size = 0; 

	if (!this->IsVariationFont())
		return size;

	auto instanceManager = this->GetInstanceManager(); 
	auto cvarTuples = instanceManager->GetCvarTuples();
	auto axisCount = this->GetVariationAxisCount(); 
	size_t numTuplesWithData = 0;
	size_t numIntermediates = 0; // Among numTuplesWithData how many intermediates

	// Determine how many tuples have data and how many of those have intermediates
	for (const auto & tuple : *cvarTuples)
	{
		if (tuple->cvt.size() > 0)
		{
			numTuplesWithData += 1;
			if (tuple->intermediateStartF2Dot14.size() > 0 || tuple->intermediateEndF2Dot14.size() > 0)
			{
				numIntermediates += 1;
			}
		}
	}
	size = (int32_t)(sizeof(CvarHeader) +  // cvar header
		numTuplesWithData * (2 * sizeof(USHORT) + axisCount * sizeof(USHORT)) +  // TupleVariationHeader for tuples with data
		numIntermediates * (2 * axisCount * sizeof(USHORT))); // TupleVariationHeader addition for entries with intermediates (start and end)

	for (const auto & tuple : *cvarTuples)
	{
		// no data then skip this tuple
		if (tuple->cvt.size() == 0)
			continue;

		size += (int32_t)(tuple->cvt.size() * 2 * sizeof(USHORT) + 2 * sizeof(USHORT));
	}
	
	return (numTuplesWithData > 0) ? size : 0;
}

static bool Compare_Tuples_by_Order(const Variation::CvarTuple & first, const Variation::CvarTuple & second)
{
	// Comparison function that, taking two values of the same type than those 
	// contained in the list object, returns true if the first argument goes before the 
	// second argument in the specific order, and false otherwise. 
	return first.GetWriteOrder() < second.GetWriteOrder();
}

int32_t TrueTypeFont::UpdateCvar(int32_t *size, unsigned char data[])
{
	CvarHeader header; 
	unsigned short *packed = (unsigned short*)data; 
	
	if (!this->IsVariationFont())
	{
		*size = 0;
		return *size;
	}

	// Copy tuples and then sort according to embedded order.
	auto instanceManager = this->GetInstanceManager();
	auto cvarTuples = instanceManager->GetCvarTuplesCopy();
	std::stable_sort(cvarTuples.begin(), cvarTuples.end(), Compare_Tuples_by_Order); 	

	auto axisCount = this->GetVariationAxisCount();
	auto tupleCount = cvarTuples.size();
	size_t numTuplesWithData = 0;
	size_t numIntermediates = 0; // Among numTuplesWithData how many intermediates

	// Determine how many tuples have data and how many of those have intermediates
	for (const auto & tuple : cvarTuples)
	{
		if (tuple.cvt.size() > 0)
		{
			numTuplesWithData += 1;
			if (tuple.intermediateStartF2Dot14.size() > 0 || tuple.intermediateEndF2Dot14.size() > 0)
			{
				numIntermediates += 1;
			}
		}
	}

	// if no tuples have data we don't need a cvar table
	if (numTuplesWithData == 0)
	{
		*size = 0; 
		return *size;
	}

	header.majorVersion = 1;
	header.minorVersion = 0;
	header.tupleVariationCount = (unsigned short)numTuplesWithData; // no flags
	header.offsetToData = (unsigned short)(sizeof(CvarHeader) +  // cvar header
		numTuplesWithData * (2 * sizeof(USHORT) + axisCount * sizeof(USHORT)) +  // TupleVariationHeader for tuples with data
		numIntermediates * (2 * axisCount * sizeof(USHORT))); // TupleVariationHeader addition for entries with intermediates (start and end)

	*packed++ = SWAPW(header.majorVersion); 	
	*packed++ = SWAPW(header.minorVersion);	
	*packed++ = SWAPW(header.tupleVariationCount);	
	*packed++ = SWAPW(header.offsetToData);	

	unsigned char* packedByteData = data + header.offsetToData; 
	unsigned short byteOffset = 0; 

	// write serialized data
	for (auto cvarTuplesIt = cvarTuples.cbegin(); cvarTuplesIt != cvarTuples.cend(); ++cvarTuplesIt)
	{	
		// no data then skip this tuple
		if(cvarTuplesIt->cvt.size() == 0)
			continue; 

		uint16_t previousByteOffset = byteOffset; 
		uint16_t recordCount = (uint16_t)cvarTuplesIt->cvt.size(); 
		uint16_t cvtsToWrite = recordCount;
		if (recordCount > SCHAR_MAX)
		{
			USHORT* packedWordData = (unsigned short*)&packedByteData[byteOffset]; 
			USHORT c = 0x8000 | recordCount;
			*packedWordData = SWAPW(c);
			byteOffset += sizeof(USHORT);
		}
		else
		{
			unsigned char c = (unsigned char)recordCount;
			packedByteData[byteOffset] = c; 
			byteOffset += sizeof(unsigned char); 
		}

		// determine highest cvt
		uint16_t highestCvt = 0;
		for (int cIndex = 0; cIndex < recordCount; ++cIndex)
			highestCvt = Max(cvarTuplesIt->cvt[cIndex], highestCvt); 

		// write cvt numbers
		// break into runs if needed
		uint16_t cvtIndex = 0;
		uint16_t currentCvt = 0;
		while(cvtsToWrite > 0)
		{
			uint16_t runWrite = Min(cvtsToWrite, POINT_RUN_COUNT_MASK);

			// write control byte for run
			unsigned char c = (unsigned char)runWrite - 1; 
			if (highestCvt > UCHAR_MAX)
				c |= POINTS_ARE_WORDS;
			packedByteData[byteOffset] = c;
			byteOffset += sizeof(unsigned char);

			if (highestCvt <= UCHAR_MAX)
			{
				// use bytes
				for (int wIndex = 0; wIndex < runWrite; ++wIndex)
				{
					packedByteData[byteOffset] = cvarTuplesIt->cvt[cvtIndex] - currentCvt;
					currentCvt = cvarTuplesIt->cvt[cvtIndex++];
					byteOffset += sizeof(unsigned char); 
				}
			}
			else
			{
				// use words
				for (int wIndex = 0; wIndex < runWrite; ++wIndex)
				{
					USHORT* packedWordData = (unsigned short*)&packedByteData[byteOffset];
					uint16_t temp = cvarTuplesIt->cvt[cvtIndex] - currentCvt;
					*packedWordData = SWAPW(temp);
					currentCvt = cvarTuplesIt->cvt[cvtIndex++];
					byteOffset += sizeof(USHORT); 
				}
			}
			cvtsToWrite -= runWrite; 
		}

		// write deltas
		auto deltasToWrite = recordCount;
		uint16_t largestDelta = 0; 
		for (int dIndex = 0; dIndex < recordCount; ++dIndex)
			largestDelta = Max(std::abs(cvarTuplesIt->delta[dIndex]), largestDelta); 

		cvtIndex = 0;
		while (deltasToWrite > 0)
		{
			// write control byte for run
			uint16_t runWrite = Min(deltasToWrite, DELTA_RUN_COUNT_MASK);
			unsigned char c = (unsigned char)runWrite - 1; 
			if (largestDelta > SCHAR_MAX) 
				c |= DELTAS_ARE_WORDS;
			packedByteData[byteOffset] = c;
			byteOffset += sizeof(unsigned char);

			if (largestDelta <= SCHAR_MAX)
			{
				// use bytes
				for (int wIndex = 0; wIndex < runWrite; ++wIndex)
				{
					packedByteData[byteOffset] = (unsigned char)cvarTuplesIt->delta[cvtIndex++];
					byteOffset += sizeof(unsigned char);
				}
			}
			else
			{
				// use words
				for (int wIndex = 0; wIndex < runWrite; ++wIndex)
				{
					USHORT delta = cvarTuplesIt->delta[cvtIndex++];
					USHORT* packedWordData = (unsigned short*)&packedByteData[byteOffset];					
					*packedWordData = SWAPW(delta);
					byteOffset += sizeof(USHORT);
				}
			}
			deltasToWrite -= runWrite; 
		}

		// write header entry for this tuple
		uint16_t variationDataSize = byteOffset - previousByteOffset;
		*packed++ = SWAPW(variationDataSize);		

		// If you have one you need both!
		assert(cvarTuplesIt->intermediateEndF2Dot14.size() == cvarTuplesIt->intermediateStartF2Dot14.size());

		bool intermediate = cvarTuplesIt->intermediateEndF2Dot14.size() > 0 || cvarTuplesIt->intermediateStartF2Dot14.size() > 0; 
		uint16_t tupleIndex = 0x8000 | 0x2000; 
		if (intermediate)
			tupleIndex = tupleIndex | 0x4000; 		

		*packed++ = SWAPW(tupleIndex);	

		// Write the peakTuple
		for (size_t coordIndex = 0; coordIndex < cvarTuplesIt->peakF2Dot14.size(); ++coordIndex)
		{
			uint16_t coord = cvarTuplesIt->peakF2Dot14[coordIndex].GetRawValue(); 
			*packed++ = SWAPW(coord); 
		}

		if (intermediate)
		{
			// Write the intermediateStartTuple
			for (size_t coordIndex = 0; coordIndex < cvarTuplesIt->intermediateStartF2Dot14.size(); ++coordIndex)
			{
				uint16_t coord = cvarTuplesIt->intermediateStartF2Dot14[coordIndex].GetRawValue();
				*packed++ = SWAPW(coord);
			}

			// Write the intermediateEndTuple
			for (size_t coordIndex = 0; coordIndex < cvarTuplesIt->intermediateEndF2Dot14.size(); ++coordIndex)
			{
				uint16_t coord = cvarTuplesIt->intermediateEndF2Dot14[coordIndex].GetRawValue();
				*packed++ = SWAPW(coord);
			}
		}
	}

	*size = header.offsetToData + byteOffset;

	return *size;
} 

void TrueTypeFont::UpdateAdvanceWidthFlag(bool linear) {
	sfnt_FontHeader *head;
	uint16 flags;

	head = (sfnt_FontHeader*)this->GetTablePointer(tag_FontHeader);
	flags = SWAPW(head->flags);
	if (linear) flags &= ~USE_INTEGER_SCALING; else flags |= USE_INTEGER_SCALING; 
	head->flags = SWAPW(flags);
} // TrueTypeFont::UpdateAdvanceWidthFlag

#define OPTIMIZED_FOR_CLEARTYPE   0x2000 

bool TrueTypeFont::UpdateBinData(ASMType asmType, int32_t binSize, unsigned char *binData) {
	unsigned char *binTemp;
	
	if (asmType < firstASMType || lastASMType < asmType || binSize < 0) return false;

	binTemp = binSize > 0 ? (unsigned char *)NewP(binSize) : NULL;
	if (binSize > 0 && binTemp == NULL) return false;

	if (this->binData[asmType] != NULL) DisposeP((void **)&this->binData[asmType]);
	
	if (binSize > 0 && binData != NULL) memcpy(binTemp,binData,binSize);
	this->binSize[asmType] = binSize;
	this->binData[asmType] = binTemp;

	return true;
} // TrueTypeFont::UpdateBinData

bool TrueTypeFont::TableExists(sfnt_TableTag tag) {
	sfnt_OffsetTable *p;
	int32_t num,i;

	p = (sfnt_OffsetTable *)this->sfntHandle;
	num = SWAPW(p->numOffsets);
	tag = SWAPL(tag);
	for (i = 0; i < num && p->table[i].tag != tag; i++);
	return i < num; // found
} // TrueTypeFont::TableExists

int32_t TrueTypeFont::GetTableOffset(sfnt_TableTag tag) {
	sfnt_OffsetTable *p;
	int32_t num,i;

	p = (sfnt_OffsetTable *)this->sfntHandle;
	num = SWAPW(p->numOffsets);
	tag = SWAPL(tag);
	for (i = 0; i < num && p->table[i].tag != tag; i++);
	return ((i < num) && (SWAPL(p->table[i].length) > 0)) ? SWAPL(p->table[i].offset) : 0;
} // TrueTypeFont::GetTableOffset

int32_t TrueTypeFont::GetTableLength(sfnt_TableTag tag) {
	sfnt_OffsetTable *p;
	int32_t num,i;	

	p = (sfnt_OffsetTable *)this->sfntHandle;
	num = SWAPW(p->numOffsets);
	tag = SWAPL(tag);
	for (i = 0; i < num && p->table[i].tag != tag; i++);
	return (i < num) ? SWAPL(p->table[i].length) : 0;
} // TrueTypeFont::GetTableLength

unsigned char *TrueTypeFont::GetTablePointer(sfnt_TableTag tag) {
	int32_t offs = this->GetTableOffset(tag);
	return offs != 0 ? this->sfntHandle	+ offs : NULL;
//	return *this->sfntHandle + this->GetTableOffset(tag);
} // TrueTypeFont::GetTablePointer

void UnpackMaxp(unsigned char *sfnt, sfnt_maxProfileTable *profile) {
	sfnt_maxProfileTable *maxp = (sfnt_maxProfileTable *)sfnt;

	profile->version               = SWAPL(maxp->version); // for this table, set to 1.0
	profile->numGlyphs             = SWAPW(maxp->numGlyphs);
	profile->maxPoints             = SWAPW(maxp->maxPoints);              
	profile->maxContours           = SWAPW(maxp->maxContours);            
	profile->maxCompositePoints    = SWAPW(maxp->maxCompositePoints);     
	profile->maxCompositeContours  = SWAPW(maxp->maxCompositeContours);   
	profile->maxElements           = SWAPW(maxp->maxElements);            
	profile->maxTwilightPoints     = SWAPW(maxp->maxTwilightPoints);      
	profile->maxStorage            = SWAPW(maxp->maxStorage);             
	profile->maxFunctionDefs       = SWAPW(maxp->maxFunctionDefs);        
	profile->maxInstructionDefs    = SWAPW(maxp->maxInstructionDefs);     
	profile->maxStackElements      = SWAPW(maxp->maxStackElements);       
	profile->maxSizeOfInstructions = SWAPW(maxp->maxSizeOfInstructions);  
	profile->maxComponentElements  = SWAPW(maxp->maxComponentElements);   
	profile->maxComponentDepth     = SWAPW(maxp->maxComponentDepth);
} // UnpackMaxp

void PackMaxp(unsigned char *sfnt, sfnt_maxProfileTable *profile) {
	sfnt_maxProfileTable *maxp = (sfnt_maxProfileTable *)sfnt;
	
	maxp->version               = SWAPL(profile->version);
	maxp->numGlyphs             = SWAPW(profile->numGlyphs);
	maxp->maxPoints             = SWAPW(profile->maxPoints);
	maxp->maxContours           = SWAPW(profile->maxContours);
	maxp->maxCompositePoints    = SWAPW(profile->maxCompositePoints);
	maxp->maxCompositeContours  = SWAPW(profile->maxCompositeContours);
	maxp->maxElements           = SWAPW(profile->maxElements);
	maxp->maxTwilightPoints     = SWAPW(profile->maxTwilightPoints);
	maxp->maxStorage            = SWAPW(profile->maxStorage);
	maxp->maxFunctionDefs       = SWAPW(profile->maxFunctionDefs);
	maxp->maxInstructionDefs    = SWAPW(profile->maxInstructionDefs);
	maxp->maxStackElements      = SWAPW(profile->maxStackElements);
	maxp->maxSizeOfInstructions = SWAPW(profile->maxSizeOfInstructions);
	maxp->maxComponentElements  = SWAPW(profile->maxComponentElements);
	maxp->maxComponentDepth     = SWAPW(profile->maxComponentDepth);
} // PackMaxp

bool TrueTypeFont::UnpackHeadHheaMaxpHmtx(wchar_t errMsg[]) {
	sfnt_FontHeader *phead, head;
	sfnt_HorizontalHeader *phhea, hhea;
	unsigned short aw,lsb,*hmtx; // actually hhea->numberOf_LongHorMetrics of sfnt_HorizontalMetrics [aw,lsb] pairs the monospaced part where we have only lsb numbers 
	int32_t glitLen,numEntries,numGlyphs,i,k;

	unsigned char* pmaxp = this->GetTablePointer(tag_MaxProfile);
	if (pmaxp == nullptr)
	{
		swprintf(errMsg, L"Error fetching maxp table"); 
		return false; 
	}
	UnpackMaxp(pmaxp ,&this->profile);

	phead = (sfnt_FontHeader*)this->GetTablePointer(tag_FontHeader);
	if (phead == nullptr)
	{
		swprintf(errMsg, L"Unpacking head: error fetching table");
		return false;
	}
	head.version = SWAPL(phead->version);
	head.fontRevision = SWAPL(phead->fontRevision);
	head.checkSumAdjustment = SWAPL(phead->checkSumAdjustment);
	head.magicNumber = SWAPL(phead->magicNumber);
	head.flags = SWAPW(phead->flags);
	head.unitsPerEm = SWAPW(phead->unitsPerEm);
	//phead->created;
	//phead->modified;
	head.xMin = SWAPW(phead->xMin);
	head.yMin = SWAPW(phead->yMin);
	head.xMax = SWAPW(phead->xMax);
	head.yMax = SWAPW(phead->yMax);
	head.macStyle = SWAPW(phead->macStyle);
	head.lowestRecPPEM = SWAPW(phead->lowestRecPPEM);
	head.fontDirectionHint = SWAPW(phead->fontDirectionHint);
	head.indexToLocFormat = SWAPW(phead->indexToLocFormat);
	head.glyphDataFormat = SWAPW(phead->glyphDataFormat);
	
	this->unitsPerEm = head.unitsPerEm;
	this->macStyle = head.macStyle;
	this->metricProfile.xMin = head.xMin;
	this->metricProfile.yMin = head.yMin;
	this->metricProfile.xMax = head.xMax;
	this->metricProfile.yMax = head.yMax;
	if (head.unitsPerEm < 64 || head.unitsPerEm > 16384) { // this used to have an upper range of 17686
		swprintf(errMsg,L"Unpacking head: em-Height %hi not in range 64 through 16384",head.unitsPerEm); return false;
		// according to Greg, unitsPerEm > 16384 would cause overflow in the rasterizer, plus imho the number 17686 doesn't make any sense to me.
	}
	
	if (head.indexToLocFormat == SHORT_INDEX_TO_LOC_FORMAT) {
		this->shortIndexToLocTable = true;
	} else if (head.indexToLocFormat == LONG_INDEX_TO_LOC_FORMAT) {
		this->shortIndexToLocTable = false;
	} else {
		swprintf(errMsg,L"Unpacking head: Unknown indexToLocFormat %hi",head.indexToLocFormat); return false;
	}
	this->outShortIndexToLocTable = this->shortIndexToLocTable;
	
	glitLen = this->GetTableLength(PRIVATE_GLIT1);
//	it may be, though, that for very old fonts, created with TypeMan, we may run into trouble with the amateur version limitation with this code:
//	these fonts may have a (historical) 2048 number for maxGlyphs simply because the glits were allocated for that size, even though the font's
//	maxp has less than 256. 
	numGlyphs = this->NumberOfGlyphs();
	this->maxGlyphs = glitLen == 0 ? this->NumberOfGlyphs() : glitLen/sizeof(sfnt_FileDataEntry) - GLIT_PAD; // for the GLIT_PAD cf comment in UnpackGlitsLoca below

	this->AssertMaxGlyphs(this->maxGlyphs + GLIT_PAD);

	if (this->maxGlyphs + GLIT_PAD > this->maxGlitEntries) {  
		swprintf(errMsg,L"This font has too many glyphs, please\r" BULLET L" increase the amount of virtual memory in your system settings");  
		return false;
	}
	
	phhea = (sfnt_HorizontalHeader*)this->GetTablePointer(tag_HoriHeader);
	hhea.version = SWAPL(phhea->version);
	hhea.yAscender= SWAPW(phhea->yAscender);
	hhea.yDescender= SWAPW(phhea->yDescender);
	hhea.yLineGap= SWAPW(phhea->yLineGap);
	hhea.advanceWidthMax= SWAPW(phhea->advanceWidthMax);
	hhea.minLeftSideBearing= SWAPW(phhea->minLeftSideBearing);
	hhea.minRightSideBearing= SWAPW(phhea->minRightSideBearing);
	hhea.xMaxExtent= SWAPW(phhea->xMaxExtent);
	hhea.horizontalCaretSlopeNumerator= SWAPW(phhea->horizontalCaretSlopeNumerator);
	hhea.horizontalCaretSlopeDenominator= SWAPW(phhea->horizontalCaretSlopeDenominator);
	hhea.metricDataFormat= SWAPW(phhea->metricDataFormat);
	hhea.numberOf_LongHorMetrics= SWAPW(phhea->numberOf_LongHorMetrics);

	this->metricProfile.advanceWidthMax		= hhea.advanceWidthMax;
	this->metricProfile.minLeftSideBearing	= hhea.minLeftSideBearing;
	this->metricProfile.minRightSideBearing = hhea.minRightSideBearing;
	this->metricProfile.xMaxExtent			= hhea.xMaxExtent;
	
	hmtx = (unsigned short *)this->GetTablePointer(tag_HorizontalMetrics);
	k = 0; numEntries = hhea.numberOf_LongHorMetrics;
	for (i = 0; i < numEntries; i++) {  
		aw = hmtx[k++];
		this->horMetric[i].advanceWidth = SWAPW(aw);
		lsb = hmtx[k++];
		this->horMetric[i].leftSideBearing = SWAPW(lsb);
	}
	for (i = numEntries; i < numGlyphs; i++) {  // all the remaining ones have the same aw of the last regular [aw,lsb] pair
		this->horMetric[i].advanceWidth = SWAPW(aw);
		lsb = hmtx[k++];
		this->horMetric[i].leftSideBearing = SWAPW(lsb);
	}
	
	this->InitNewProfiles(); // silence BC errors

	// see comments in TrueTypeFont::UpdateAssemblerProfile
	for (i = 0; i < Len(this->maxStackElements); i++) this->maxStackElements[i] = 0;
	
	return true; // by now
} // TrueTypeFont::UnpackHeadHheaMaxpHmtx

bool TrueTypeFont::UnpackGlitsLoca(wchar_t errMsg[]) {
/*	the 'glit' tables are private tables to index the private tables that store the TMT and TT sources.
	however, due to reasons that I don't remember, the (TT) sources for the prep and fpgm, as well as for
	the cvt and the hinter parameters, are not stored in individual private tables, but as special glyphs
	with glyph indeces 0xFFFA..0xFFFD (as per #defines at the top). on top of that, one of the glyph indeces
	holds a magic number, which explains the ominous numberOfGlyph + GLIT_PAD below and the cascade of else ifs upon
	unpacking the glit.
	  Additionnally, the length field on the file is only 16 bits, but the cvt source, which is one of the
	glyph, may exceed this range. Therefore, internally the length is kept as 32 bits and computed from two
	successive offsets, if the length is the "magic" number 0x8000, rather than taken from the file. In the
	end, storing both the offsets (which are always 32 bits) *and* the lengths is redundant, anyhow, but
	that is historical.
	  Finally, if the glits shouldn't contain enough entries, they are extended, by moving the last elements
	up in the array. Escapes me, why this is done like this... */
	
	int32_t tableLength,progrLength,i,j,numberOfGlyphs,oldMaxGlyph;
	sfnt_FileDataEntry *fileGlit;
	sfnt_MemDataEntry *glit1, *glit2;
	uint32_t *longIndexToLoc;
	unsigned short *shortIndexToLoc;
	
	numberOfGlyphs = this->NumberOfGlyphs();
	
	this->glit1Entries = 0;
	tableLength = this->GetTableLength(PRIVATE_GLIT1);
	progrLength = this->GetTableLength(PRIVATE_PGM1);
	if (tableLength > 0 /* && this->glit1Entries == 0 */) {
		fileGlit = (sfnt_FileDataEntry*)this->GetTablePointer(PRIVATE_GLIT1);
		this->glit1Entries = tableLength / sizeof(sfnt_FileDataEntry);
		for (j = 0; j < this->glit1Entries; j++) {
			this->glit1[j].glyphCode = SWAPW(fileGlit[j].glyphCode);
			
			if (j+6 == this->glit1Entries) { // last regular glyph, compute length but skip magic number (cf. below)
				this->glit1[j].length = SWAPL(fileGlit[j+2].offset) - SWAPL(fileGlit[j].offset);
			} else if (j+5 == this->glit1Entries) { // magic number, has no length
				this->glit1[j].length = 0;
			} else if (j+1 == this->glit1Entries) { // last special "glyph", compute length from table length
				this->glit1[j].length = progrLength - SWAPL(fileGlit[j].offset);
			} else if ((unsigned short)SWAPW(fileGlit[j].length) != (unsigned short)0x8000) { // length is short
			// unless cast to unsigned short, compiler will sign extend the result of SWAPW to 32 bit signed,
			// turning 0x8000 into 0ffff8000 and subsequently compare it to 0x00008000 !!!
				this->glit1[j].length = SWAPW(fileGlit[j].length);
			} else { // length is (regular case) long
				this->glit1[j].length = SWAPL(fileGlit[j+1].offset) - SWAPL(fileGlit[j].offset);
			}
			this->glit1[j].offset = SWAPL(fileGlit[j].offset);
		}
	}
	
	this->glit2Entries = 0;
	tableLength = this->GetTableLength(PRIVATE_GLIT2);
	progrLength = this->GetTableLength(PRIVATE_PGM2);
	if (tableLength > 0 /* && this->glit2Entries == 0 */) {
		fileGlit = (sfnt_FileDataEntry*)this->GetTablePointer(PRIVATE_GLIT2);
		this->glit2Entries = tableLength / sizeof(sfnt_FileDataEntry);
		for (j = 0; j < this->glit2Entries; j++) {
			this->glit2[j].glyphCode = SWAPW(fileGlit[j].glyphCode);
			
			if (j+6 == this->glit2Entries) {
				this->glit2[j].length = SWAPL(fileGlit[j+2].offset) - SWAPL(fileGlit[j].offset);
			} else if (j+5 == this->glit2Entries) {
				this->glit2[j].length = 0;
			} else if (j+1 == this->glit2Entries) {
				this->glit2[j].length = progrLength - SWAPL(fileGlit[j].offset);
			} else if ((unsigned short)SWAPW(fileGlit[j].length) != (unsigned short)0x8000) {
			// unless cast to unsigned short, compiler will sign extend the result of SWAPW to 32 bit signed,
			// turning 0x8000 into 0ffff8000 and subsequently compare it to 0x00008000 !!!
				this->glit2[j].length = SWAPW(fileGlit[j].length);
			} else {
				this->glit2[j].length = SWAPL(fileGlit[j+1].offset) - SWAPL(fileGlit[j].offset);
			}
			this->glit2[j].offset = SWAPL(fileGlit[j].offset);
		}
	}
	
// claudebe I know that there should be a better place to put this but I assume that
// we need to read again the loca table at the same time as we need to read the glit table
// this is an ugly programming under heavy time stress !!!

	longIndexToLoc = (uint32_t *)this->GetTablePointer(tag_IndexToLoc );
	shortIndexToLoc = (unsigned short *)longIndexToLoc;
	this->numLocaEntries = this->GetTableLength(tag_IndexToLoc)/(this->shortIndexToLocTable ? sizeof(short) : sizeof(int32_t)) - 1;
	for (i = 0; i <= this->numLocaEntries; i++) 
		this->IndexToLoc[i] = this->shortIndexToLocTable ? ((int32_t)((unsigned short)SWAPW(shortIndexToLoc[i]))) << 1 : SWAPL(longIndexToLoc[i]);
	
//	for (i = 0; i < Min(numberOfGlyphs,this->numLocaEntries); i++) // 2nd Ed Win98 fonts somehow got a loca table with one extra entry
	for (i = 0; i < this->numLocaEntries; i++) // 2nd Ed Win98 fonts somehow got a loca table with one extra entry
		if (this->IndexToLoc[i] > this->IndexToLoc[i+1]) {
			swprintf(errMsg,L"Unpacking loca: loca table not in ascending order %i %i %i",i, this->IndexToLoc[i], this->IndexToLoc[i + 1]);
			return false;
		}
	
	if (tableLength > 0 && (numberOfGlyphs + GLIT_PAD > this->glit1Entries || numberOfGlyphs + GLIT_PAD > this->glit2Entries)) {
		oldMaxGlyph = this->glit1Entries - GLIT_PAD;
		this->maxGlyphs = numberOfGlyphs;
		glit1 = this->glit1;
		glit2 = this->glit2;
		for (i = this->maxGlyphs + 4,j = oldMaxGlyph + 4; i >= this->maxGlyphs ; i--, j-- ) { 
			glit1[i].glyphCode = glit1[j].glyphCode;
			glit1[i].length = glit1[j].length;
			glit1[i].offset = glit1[j].offset;

			glit2[i].glyphCode = glit2[j].glyphCode;
			glit2[i].length = glit2[j].length;
			glit2[i].offset = glit2[j].offset;
		}
		for (i = oldMaxGlyph; i < this->maxGlyphs ; i++ ) { 
			glit1[i].glyphCode = (short)i;
			glit1[i].length = 0;
			glit1[i].offset = 0;

			glit2[i].glyphCode = (short)i;
			glit2[i].length = 0;
			glit2[i].offset = 0;
		}
		this->glit1Entries = this->maxGlyphs + GLIT_PAD;
		this->glit2Entries = this->maxGlyphs + GLIT_PAD;
	}
	return true; // by now
} // TrueTypeFont::UnpackGlitsLoca

bool TrueTypeFont::UpdateMaxPointsAndContours(wchar_t errMsg[]) {
	// fonts like the Trebuchet have insufficient values for maxPoints, maxContours, maxCompositePoints, and maxCompositeContours,
	// which creates a bootstrapping problem, given that TrueTypeFont::GetGlyph expects a correct font to be installed in the rasterizer.
	// If this fails, we never get a chance to update the above profile values, hence we do it in here for prophylactic reasons...
	
	const wchar_t failureMsg[3][32] = {L"invalid 'maxp'", L"invalid 'glyf'", L"corrupt 'glyf'"};
	
	int32_t glyphIndex,numGlyphs,numKnots,numContours,componentDepth,maxPoints,maxContours,maxCompositePoints,maxCompositeContours,failureCode;
	unsigned char *maxpTable,*glyfTable;

	failureCode = 0;
	maxpTable = this->GetTablePointer(tag_MaxProfile);
	if (maxpTable == NULL) goto failure;
	
	failureCode = 1;
	glyfTable = this->GetTablePointer(tag_GlyphData);
	if (glyfTable == NULL) goto failure;

	failureCode = 2;
	numGlyphs = this->numLocaEntries;
	maxPoints   = 0; maxCompositePoints   = 0;
	maxContours = 0; maxCompositeContours = 0;
	for (glyphIndex = 0; glyphIndex < numGlyphs; glyphIndex++) {
		numKnots = numContours = componentDepth = 0;
		
		if (!this->GetNumPointsAndContours(glyphIndex,&numKnots,&numContours,&componentDepth)) goto failure;
		
		if (componentDepth == 0) { // simple glyph
			maxPoints   = Max(maxPoints,numKnots);
			maxContours = Max(maxContours,numContours);
		} else { // composite glyph
			maxCompositePoints   = Max(maxCompositePoints,numKnots);
			maxCompositeContours = Max(maxCompositeContours,numContours);
		}
	}

	UnpackMaxp(maxpTable,&this->profile);
	this->profile.maxPoints   = (uint16)maxPoints;
	this->profile.maxContours = (uint16)maxContours;
	this->profile.maxCompositePoints   = (uint16)maxCompositePoints;
	this->profile.maxCompositeContours = (uint16)maxCompositeContours;
	PackMaxp(maxpTable,&this->profile);

	return true;
failure:
	swprintf(errMsg,L"Failed to update max points and contours due to " WIDE_STR_FORMAT L" table",failureMsg[failureCode]);
	return false;
} // TrueTypeFont::UpdateMaxPointsAndContours

void TrueTypeFont::EnterChar(int32_t glyphIndex, uint32_t charCode) {
	UniGlyphMap entry; 
	entry.glyphIndex = (unsigned short) glyphIndex;
	entry.unicode = charCode; 
	glyphIndexMap->insert(glyphIndexMap->end(),entry); 
		
	this->charCodeOf[glyphIndex] = charCode;
	this->numberOfChars++;	 
} // TrueTypeFont::EnterChar

void TrueTypeFont::SortGlyphMap() {
	std::sort(glyphIndexMap->begin(), glyphIndexMap->end(), Compare_UniGlyphMap); 
}

void TrueTypeFont::GetFmt0(sfnt_mappingTable *map) { // I made no attempt to understand this
	int32_t i;
	uint8 *glyphIdArray = (uint8 *)(&map[1]);

	for (i = 0; i < 256; i++) this->EnterChar(glyphIdArray[i],i);

	this->SortGlyphMap(); 
} // TrueTypeFont::GetFmt0

typedef struct {
	uint16  segCountX2;
	uint16  searchRange;
	uint16  entrySelector;
	uint16  rangeShift;
	uint16  endCount[1];
} sfnt_cmap4hdr;

void TrueTypeFont::GetFmt4(sfnt_mappingTable *map) { // I made no attempt to understand this
	unsigned short j;
	int16 i,segCount,*idDelta,delta;
	uint16 gid, end, *endCount, start, *startCount, *idRangeOffset, offset, *glyphIdArray;
	sfnt_cmap4hdr *cmap4hdr = (sfnt_cmap4hdr*)(&map[1]);
	
	segCount = SWAPW(cmap4hdr->segCountX2) >> 1;
	endCount = (uint16 *)(&(cmap4hdr->endCount[0]));
	startCount = endCount + (segCount + 1);
	idDelta = (int16 *)&startCount[segCount];
	idRangeOffset = (uint16 *)&idDelta[segCount];
	glyphIdArray = &idRangeOffset[segCount];


	for (i = 0; i < segCount && endCount[i] != 0xFFFF; i++) {
		start = SWAPW(startCount[i]);
		end = SWAPW(endCount[i]);
		delta = SWAPW(idDelta[i]);
		offset = SWAPW(idRangeOffset[i]);

		if (0 == offset) {
			for (j = start; j <= end; j++) this->EnterChar((uint16)(j + delta), j);
		} else {
			for (j = start; j <= end; j++) {
				gid = idRangeOffset[offset/2 + j - start + i];
				this->EnterChar((uint16)SWAPW(gid), j);
			}
		}
	}
	this->SortGlyphMap(); 
} // TrueTypeFont::GetFmt4

void TrueTypeFont::GetFmt6(sfnt_mappingTable *map) { // I made no attempt to understand this
	int32_t i,entries,firstCode,glyphIndex;
	sfnt_mappingTable6 *Fmt6Table = (sfnt_mappingTable6 *)(reinterpret_cast<unsigned char*>(map) + sizeof(sfnt_mappingTable6));
		
	firstCode = (int32_t)SWAPW(Fmt6Table->firstCode);
	entries = (int32_t)SWAPW(Fmt6Table->entryCount);
	for (i = 0; i < entries; i++) {
		glyphIndex = SWAPW(Fmt6Table->glyphIdArray[i]);
		this->EnterChar(glyphIndex,firstCode + i);
	}
	this->SortGlyphMap(); 
} // TrueTypeFont::GetFmt6

void TrueTypeFont::GetFmt12(sfnt_mappingTable *map)
{
	sfnt_mappingTable12 *fntTable = reinterpret_cast<sfnt_mappingTable12*>(map); 
	sfnt_mappingTable12Record *record; 
	uint32 i, j, count, nGroups = SWAPL(fntTable->nGroups); 
	
	for (i = 0; i < nGroups; i++) {
		record = reinterpret_cast<sfnt_mappingTable12Record*>(reinterpret_cast<unsigned char*>(map) + sizeof(sfnt_mappingTable12) + i * sizeof(sfnt_mappingTable12Record)); 
		uint32 startCharCode = SWAPL(record->startCharCode);
		uint32 endCharCode = SWAPL(record->endCharCode); 
		uint32 startGlyphID = SWAPL(record->startGlyphCode); 
		for ( j = startCharCode, count = 0; j <= endCharCode; j++, count++)
			this->EnterChar(startGlyphID + count, j); 
	}
	this->SortGlyphMap(); 
}

bool TrueTypeFont::UnpackCharGroup(wchar_t errMsg[]) {
	int32_t i,numberOfGlyph;
	short *charGroup,tempGroup;
	
	for (i = 0; i < this->maxGlitEntries; i++) this->charGroupOf[i] = otherCase;
	
	if (this->TableExists(PRIVATE_GROUP)) {
		numberOfGlyph = this->GetTableLength(PRIVATE_GROUP) >> 1;
		if (numberOfGlyph <= this->maxGlitEntries) {
			charGroup = (short *)this->GetTablePointer(PRIVATE_GROUP);

			for (i = 0; i < numberOfGlyph; i++) {
				tempGroup = SWAPW(charGroup[i]);
				if (tempGroup < TOTALTYPE) tempGroup = intInFileToCharGroup[tempGroup]; // translate old char groups
				this->charGroupOf[i] = (unsigned char)tempGroup;
			}

		}
	}
	return true; // by now
} // TrueTypeFont::UnpackCharGroup

bool TrueTypeFont::GetSource(bool lowLevel, int32_t glyphIndex, TextBuffer *source, wchar_t errMsg[]) {
	sfnt_MemDataEntry *glit;
	int32_t len,glitEntries,glitIndex,pgmID;	
	
	if (lowLevel) {
		pgmID = PRIVATE_PGM1; glit = this->glit1; glitEntries = this->glit1Entries;
	} else {
		pgmID = PRIVATE_PGM2; glit = this->glit2; glitEntries = this->glit2Entries;
	}
	
	for (glitIndex = 0; glitIndex < glitEntries && glit[glitIndex].glyphCode != glyphIndex; glitIndex++);
	len = this->GetTableLength(pgmID);
	if (len > 0 && glitIndex < glitEntries && glit[this->maxGlyphs].offset == PRIVATE_STAMP_1)
	{		
		source->SetText(glit[glitIndex].length,(char*)(this->GetTablePointer(pgmID) + glit[glitIndex].offset));		
	}
	else 
	{		
		swprintf(errMsg,L"Unpacking source: ");
		if (len == 0)
			swprintf(&errMsg[STRLENW(errMsg)],L"private " WIDE_STR_FORMAT L"level table empty",lowLevel ? L"low" : L"high");
		else if (glitIndex == glitEntries)
			swprintf(&errMsg[STRLENW(errMsg)],L"glyph %li not in private glit",glyphIndex);
		else
			swprintf(&errMsg[STRLENW(errMsg)],L"bad private stamp 1");
		source->SetText(0,(wchar_t*)NULL);
		return len == 0; // ok to have no sources
	}
	return true; // by now
} // TrueTypeFont::GetSource

bool TrueTypeFont::GetTTOTable(sfnt_TableTag srcTag, TextBuffer *src, sfnt_TableTag binTag, ASMType asmType) {
	src->SetText(this->GetTableLength(srcTag),(char *)this->GetTablePointer(srcTag));
	return this->UpdateBinData(asmType,this->GetTableLength(binTag),this->GetTablePointer(binTag));
} // TrueTypeFont::GetTTOTable


/* fix this */ // huh? I don't claim to understand all of this in BuildNewSfnt & sqq...
#define variableTables 16

class TableAvailability 
{
public:
	TableAvailability() : tag_(0), required_(false), strip_(stripNothing) {}
	TableAvailability(sfnt_TableTag tag, bool required, StripCommand strip) : tag_(tag), required_(required), strip_(strip) {}

	sfnt_TableTag Tag() const
	{
		return tag_;
	}

	bool Required() const
	{
		return required_; 
	}

	StripCommand Strip() const
	{
		return strip_; 
	}

private:
	sfnt_TableTag tag_;
	bool required_; // to work with VTT
	StripCommand strip_; // i.e. strip if incoming strip >= this value for strip
};

void SwapEntries(sfnt_DirectoryEntry tbl[], int32_t *a, int32_t *b) {
	sfnt_DirectoryEntry entry;
	int32_t i;

	entry = tbl[*a]; tbl[*a] = tbl[*b]; tbl[*b] = entry;
	i = *a; *a = *b; *b = i;
} // SwapEntries

void FillNewGlit(sfnt_MemDataEntry glit[], int32_t maxGlyphs, int32_t glitEntries) {
	int32_t i,j;
	
	for (i = 0; i < glitEntries; i++) {
		glit[i].glyphCode = (unsigned short)i;
		glit[i].length = 0;
		glit[i].offset = 0;
	}
	glit[maxGlyphs].offset = PRIVATE_STAMP_1; // Stamp it
	for (i = maxGlyphs, j = 0; j < GLIT_PAD; i++, j++) glit[i].glyphCode = glitPadIndex[j];
} // FillNewGlit

uint32_t PackGlit(sfnt_FileDataEntry *fileGlit, int32_t glitEntries, sfnt_MemDataEntry *memGlit) {
	int32_t i,length;
	
	for (i = 0; i < glitEntries; i++) {
		fileGlit[i].glyphCode = SWAPW(memGlit[i].glyphCode);

		length = memGlit[i].length > 0x08000 ? 0x08000 : (unsigned short)memGlit[i].length;
		fileGlit[i].length = SWAPW(length);
		fileGlit[i].offset = SWAPL(memGlit[i].offset);
	}

	return glitEntries*sizeof(sfnt_FileDataEntry);
} // PackGlit

bool BuildOffsetTables(unsigned char *sfnt, uint32_t maxSfntSize, sfnt_OffsetTable *offsetTable, sfnt_OffsetTable *tmpOffsetTable, int32_t numVariableTables, TableAvailability avail[], StripCommand strip, int32_t *headerSize) {
	int32_t i,j,j2,maxPowerOf2,headIdx,glyfIdx,locaIdx,hheaIdx,hmtxIdx,tag;
	sfnt_DirectoryEntry entry = {(sfnt_TableTag)0x3F3F3F3F /* '????' */, 0, 0, 0}; // tag, checksum, offset, length
		
//	first, do some sanity check on the offset table, to be done upon reading a font?
//	another inherited hack: a flat MYOFFSETTABLESIZE bytes are allocated for the internal copy of the sfnt directory
//	so first assert that this is enough to hold all directory entries (see sfnt.h for type definitions)
//	to do so, very first thing is to get the numOffsets
	memcpy((char*)offsetTable,(char*)sfnt,OFFSETTABLESIZE);
	offsetTable->numOffsets = SWAPW(offsetTable->numOffsets);
	assert(sizeof(sfnt_OffsetTable) + (offsetTable->numOffsets - 1)*sizeof(sfnt_DirectoryEntry) <= MYOFFSETTABLESIZE);
//	now get the rest
	memcpy((char*)&offsetTable->table[0],(char*)&sfnt[OFFSETTABLESIZE],MYOFFSETTABLESIZE-OFFSETTABLESIZE);

	for (i = 0; i < offsetTable->numOffsets; i++) {
		if (offsetTable->table[i].length != 0) {
			offsetTable->table[i].offset = SWAPL(offsetTable->table[i].offset);
			offsetTable->table[i].length = SWAPL(offsetTable->table[i].length);
			if (offsetTable->table[i].offset > maxSfntSize) return false;
		} else {
			offsetTable->table[i].offset = 0;
		}
		offsetTable->table[i].tag = SWAPL(offsetTable->table[i].tag);
		offsetTable->table[i].checkSum = SWAPL(offsetTable->table[i].checkSum);
	}
	
//	now build a temporary offset table containing the tables to be used in the sfnt to be built
	memcpy((char*)tmpOffsetTable,(char*)offsetTable, MYOFFSETTABLESIZE);
	for (i = 0, j2 = tmpOffsetTable->numOffsets; i < numVariableTables; i++) {	
		// all tables that we need
		for (j = 0; j < tmpOffsetTable->numOffsets && tmpOffsetTable->table[j].tag != avail[i].Tag(); j++)
			;
		if (avail[i].Required() && j == tmpOffsetTable->numOffsets) { // required but not present
			entry.tag = avail[i].Tag(); 
			tmpOffsetTable->table[j2++] = entry; // add
		}
	}
	tmpOffsetTable->numOffsets = (short)j2;
	for (j = 0, j2 = 0; j < tmpOffsetTable->numOffsets; j++) { 
		// all tables that we have already
		for (i = 0; i < numVariableTables && tmpOffsetTable->table[j].tag != avail[i].Tag(); i++)
			;
		if (i == numVariableTables || strip < avail[i].Strip()) { 
			// not strippable at all or not high enough stripping priviledges
			tmpOffsetTable->table[j2++] = tmpOffsetTable->table[j]; // keep
		}
	}
	tmpOffsetTable->numOffsets = (short)j2;
	
//	finish temporary offset table
	tmpOffsetTable->version = 0x00010000;
	maxPowerOf2 = 2;
	for (tmpOffsetTable->entrySelector = 0; maxPowerOf2 <= tmpOffsetTable->numOffsets; tmpOffsetTable->entrySelector++) maxPowerOf2 <<= 1;
	maxPowerOf2 >>= 1;
	tmpOffsetTable->searchRange = (uint16)(maxPowerOf2 * 16);
	tmpOffsetTable->rangeShift = tmpOffsetTable->numOffsets * 16 - tmpOffsetTable->searchRange;
	
//	remainder of sfnt starts at offset 'headerSize'
	*headerSize = OFFSETTABLESIZE + tmpOffsetTable->numOffsets*sizeof(sfnt_DirectoryEntry); // Header
	
//	make sure we get to do 'head' and 'glyf' before 'loca' because 'loca' depends on them;
//	likewise, 'hmtx' depends on 'hhea' (numberOfHMetrics), which depends on 'glyf' (metrics, useMyMetrics)
	headIdx = glyfIdx = locaIdx = hheaIdx = hmtxIdx = 0;
	for (i = 0; i < tmpOffsetTable->numOffsets; i++) {
		tag = tmpOffsetTable->table[i].tag;
		if		(tag == tag_FontHeader)			headIdx = i;
		else if (tag == tag_GlyphData)			glyfIdx = i;
		else if (tag == tag_IndexToLoc)			locaIdx = i;
		else if (tag == tag_HoriHeader)			hheaIdx = i;
		else if (tag == tag_HorizontalMetrics)	hmtxIdx = i;
	}
	if (glyfIdx > hheaIdx) SwapEntries(tmpOffsetTable->table,&glyfIdx,&hheaIdx);
	if (hheaIdx > hmtxIdx) SwapEntries(tmpOffsetTable->table,&hheaIdx,&hmtxIdx);
	if (headIdx > locaIdx) SwapEntries(tmpOffsetTable->table,&headIdx,&locaIdx);
	if (glyfIdx > locaIdx) SwapEntries(tmpOffsetTable->table,&glyfIdx,&locaIdx);

	return true; // by now
} // BuildOffsetTables

bool TrueTypeFont::BuildNewSfnt(StripCommand strip, CharGroup group, int32_t glyphIndex, TrueTypeGlyph *glyph,
								   TextBuffer *glyfText, TextBuffer *prepText, TextBuffer *cvtText,  TextBuffer *talkText, TextBuffer *fpgmText,
								   wchar_t errMsg[]) {
	unsigned char *sfnt,*tmpSfnt,*tmpSfntHandle;

	int32_t headerSize,i,j,tag,numberOfGlyphs,numberOfHMetrics,zero = 0L,pad;
	uint32_t sfntPos,tmpSfntSize,sizeOfTable,newSfntSizeEstimate;
	sfnt_FontHeader *head;
	sfnt_HorizontalHeader *hhea;
	bool result = false;

	// fileGlit will point into tmpSfnt as it is being built (tags PRIVATE_GLIT1 and PRIVATE_GLIT2)
	// its correct setup is relied on upon packing PRIVATE_PGM1 and PRIVATE_PGM2 (not BeatS design...)
	sfnt_MemDataEntry *memGlit = NULL;
	sfnt_FileDataEntry *fileGlit = NULL;

	// cannot have variables in initialization of array, hence...
	TableAvailability avail[variableTables];

	avail[ 0] = TableAvailability(tag_ControlValue, this->cvt->HighestCvtNum() >= 0, stripHints);   //  0
	avail[ 1] = TableAvailability(tag_FontProgram, this->binSize[asmFPGM] > 0, stripHints);         //  1
	avail[ 2] = TableAvailability(tag_HoriDeviceMetrics, this->hdmxBinSize > 0, stripBinary); 	    //  2
	avail[ 3] = TableAvailability(tag_LinearThreshold, this->ltshBinSize > 0, stripBinary); 	    //  3
	avail[ 4] = TableAvailability(tag_VertDeviceMetrics, this->vdmxBinSize > 0, stripBinary); 	    //  4
	avail[ 5] = TableAvailability(tag_GridfitAndScanProc, this->gaspBinSize > 0, stripEverything);  //  5
	avail[ 6] = TableAvailability(tag_PreProgram, this->binSize[asmPREP] > 0, stripHints); 	        //  6
	avail[ 7] = TableAvailability(tag_NamingTable, true, stripEverything);                          //  7
	avail[ 8] = TableAvailability(PRIVATE_GLIT1, true, stripSource); 	                            //  8
	avail[ 9] = TableAvailability(PRIVATE_PGM1, true, stripSource); 	                            //  9
	avail[10] = TableAvailability(PRIVATE_GLIT2, true, stripSource); 	                            // 10
	avail[11] = TableAvailability(PRIVATE_PGM2, true, stripSource); 	                            // 11
	avail[12] = TableAvailability(PRIVATE_GROUP, true, stripSource); 	                            // 12
	avail[13] = TableAvailability(PRIVATE_CMAP, false, stripNothing);                               // 13
	avail[14] = TableAvailability(tag_CVTVariations, this->IsCvarTupleData(), stripHints);          // 14
	avail[15] = TableAvailability(PRIVATE_CVAR, this->IsCvarTupleData(), stripSource);              // 15
	
	if (strip >= stripSource) {
		cvtText-> Delete(0,cvtText-> TheLength());
		fpgmText->Delete(0,fpgmText->TheLength());
		prepText->Delete(0,prepText->TheLength());
		glyfText->Delete(0,glyfText->TheLength());
		talkText->Delete(0,talkText->TheLength());
	}
	
	head = (sfnt_FontHeader *)this->GetTablePointer(tag_FontHeader);
	if (SWAPL(head->magicNumber) != SFNT_MAGIC) {
		swprintf(errMsg,L"BuildNewSfnt: Bad magic number in the head");
		goto term;
	}
	
	if (glyphIndex < 0 || this->profile.numGlyphs <= glyphIndex) {
		swprintf(errMsg,L"BuildNewSfnt: invalid glyph index %li of current glyph",glyphIndex);
		goto term;
	}
	
	if (strip >= stripHints) {
		//for (i = 0; i < numASMTypes; i++) this->UpdateBinData((ASMType)i,0,NULL);
        for (i = firstTTASMType; i <= lastTTASMType; i++) this->UpdateBinData((ASMType)i,0,NULL);
		this->cvt->PutCvtBinary(0,NULL); // in hopes that copying 0 bytes from address NULL doesn't cause a bus-error...
	}
	
//	numberOfGlyphs = this->NumberOfGlyphs();
//	we have to use profile.numGlyphs here because the font may have glyphs added to it while the loca table has not yet been updated and NumberOfGlyphs() uses the loca table...
	numberOfGlyphs = this->profile.numGlyphs; 
	
	this->UpdateGlyphProfile(glyph);
	this->PackMaxpHeadHhea();

	sfnt = this->sfntHandle; // alias it
	
	if (!BuildOffsetTables(sfnt,this->sfntSize,this->offsetTable,this->tmpOffsetTable,variableTables,avail,strip,&headerSize)) {
		swprintf(errMsg,L"BuildNewSfnt: offset is too large"); // claudebe, problem with a new font received from Sampo
		goto term;
	}

#ifdef _DEBUG
//	given MYOFFSETTABLESIZE for the size of the entire sfnt_OffsetTable, maxNumDirectoryEntries denotes
//	the (truncated) number of sfnt_DirectoryEntry entries that this amount of memory is good for
//	(see comments about inherited hack above)
#define maxNumDirectoryEntries ((MYOFFSETTABLESIZE - sizeof(sfnt_OffsetTable))/sizeof(sfnt_DirectoryEntry) + 1)
	uint32_t sizeEstimate[maxNumDirectoryEntries];
	char tagCopy[maxNumDirectoryEntries][8];
	for (i = 0; i < maxNumDirectoryEntries; i++) { sizeEstimate[i] = 0; tagCopy[i][0] = 0; }
#endif
	
	// 1. pass: get a reasonnable upper limit for the expected file size
	newSfntSizeEstimate = headerSize;
	
	for (i = 0; i < this->tmpOffsetTable->numOffsets; i++) {

		switch (tag = this->tmpOffsetTable->table[i].tag) {
			
			case tag_FontHeader:
				sizeOfTable = this->tmpOffsetTable->table[i].length;
				break;
			case tag_HoriHeader: {
				sizeOfTable = this->tmpOffsetTable->table[i].length;
				break;
			}
			case tag_HorizontalMetrics:
				// discovered in an old version of Courier New Italic, glyph "E" had a different AW than a composite using "E" (and E's metrics)
				// which was correctly propagated to the hmtx upon PackGlyph, after which point the actual packing routine for the hmtx worked
				// with a different numberOfHMetrics triggering an assert because a differing table size was detected
				// Therefore, for size estimate always assume each glyph as a different AW
				sizeOfTable = numberOfGlyphs*sizeof(sfnt_HorizontalMetrics);
				break;
			case tag_GlyphData:
				sizeOfTable = this->GetPackedGlyphsSizeEstimate(glyph,glyphIndex,this->IndexToLoc);
				break;
			case tag_IndexToLoc:
				// for size estimate, always assume long format 'loca' table; correct this->outShortIndexToLocTable determined in PackGlyph only
				sizeOfTable = (numberOfGlyphs + 1)*sizeof(int32_t);
				break;
			case PRIVATE_GROUP:
				sizeOfTable = numberOfGlyphs*sizeof(short);
				break;
			case PRIVATE_GLIT1:
			case PRIVATE_GLIT2: {
				int32_t glitEntries;
				bool newGlit;

				newGlit = this->tmpOffsetTable->table[i].length == 0 || this->tmpOffsetTable->table[i].offset == 0; // we've probably just added the table
				
				if (!newGlit)
					glitEntries = tag == PRIVATE_GLIT1 ? this->glit1Entries : this->glit2Entries;
				else {
					glitEntries = this->maxGlyphs + GLIT_PAD;
					memGlit = tag == PRIVATE_GLIT1 ? this->glit1 : this->glit2;
					FillNewGlit(memGlit,this->maxGlyphs,glitEntries);
				}

				sizeOfTable = glitEntries*sizeof(sfnt_FileDataEntry);
				break;
			}
			case PRIVATE_CVAR:
				sizeOfTable = this->tsicBinSize = this->EstimatePrivateCvar(); 
				break;
			case PRIVATE_PGM1:
			case PRIVATE_PGM2:
				sizeOfTable = GetPackedGlyphSourcesSize(glyfText,prepText,cvtText,talkText,fpgmText,
														tag == PRIVATE_PGM1 ? 1 : 2,glyphIndex,tag == PRIVATE_PGM1 ? this->glit1 : this->glit2);
				break;
			case tag_PreProgram:
				sizeOfTable = this->binSize[asmPREP];
				break;
			case tag_FontProgram:
				sizeOfTable = this->binSize[asmFPGM];
				break;
			case tag_ControlValue:
				sizeOfTable = this->cvt->GetCvtBinarySize();
				break;
			case tag_GridfitAndScanProc:
				sizeOfTable = this->gaspBinSize ? this->gaspBinSize : this->tmpOffsetTable->table[i].length;
				break;
            case tag_CVTVariations:
				sizeOfTable = this->cvarBinSize = this->EstimateCvar();
                break; 
			case tag_NamingTable:
				sizeOfTable = this->tmpOffsetTable->table[i].length == 0 ? 6 : this->tmpOffsetTable->table[i].length;
				break;
			default:
				sizeOfTable = this->tmpOffsetTable->table[i].length;
				break;
		} // switch (tag)

#ifdef _DEBUG
		sizeEstimate[i] = sizeOfTable;
		
		tagCopy[i][0] = (char)(tag >> 24);
		tagCopy[i][1] = (char)(tag >> 16);
		tagCopy[i][2] = (char)(tag >>  8);
		tagCopy[i][3] = (char)(tag);
		tagCopy[i][4] = (char)0;
#endif	

		sizeOfTable = DWordPad(sizeOfTable);
		newSfntSizeEstimate += sizeOfTable;
	}

	this->AssertMaxSfntSize(newSfntSizeEstimate,false,true); // make sure we have a large enough tmpSfnt

	if (newSfntSizeEstimate > this->maxTmpSfntSize) {
		MaxSfntSizeError(L"BuildNewSfnt: This font is getting too large",newSfntSizeEstimate,errMsg); goto term;
	}
	
	// 2. pass: build the new sfnt into the tmpSfntHandle

	tmpSfnt = this->sfntTmpHandle; // alias this one, too
	
	sfntPos = headerSize; // skip header, need to fill directory first, then plug in header as final step
	
	for (i = 0; i < this->tmpOffsetTable->numOffsets; i++) {

		switch (tag = this->tmpOffsetTable->table[i].tag) {
			
			case tag_FontHeader: {
#ifdef _DEBUG
				sizeOfTable = this->tmpOffsetTable->table[i].length;
#endif	
				memcpy((char*) &tmpSfnt[sfntPos],(char*)&sfnt[this->tmpOffsetTable->table[i].offset],this->tmpOffsetTable->table[i].length);
				head = (sfnt_FontHeader *)&tmpSfnt[sfntPos];
				long long temp_time = DateTime(); // ADD GREGH 20101209
				int32_t temp_lower = (int32_t)(temp_time & 0xFFFFFFFF); // ADD GREGH 20101209
				int32_t temp_upper = (int32_t)((temp_time >> 32) & 0xFFFFFFFF); // ADD GREGH 20101209
				head->modified.bc = SWAPL(temp_upper); // ADD GREGH 20101209
				head->modified.ad = SWAPL(temp_lower); // ADD GREGH 20101209
#ifdef _DEBUG
				assert(this->tmpOffsetTable->table[i].length == sizeOfTable);
#endif	
				break;
			}	
			case tag_HoriHeader: { // determine numberOfHMetrics, required in tag_HorizontalMetrics below!!!
				int32_t aw;
#ifdef _DEBUG
				sizeOfTable = this->tmpOffsetTable->table[i].length;
#endif	
				memcpy((char*) &tmpSfnt[sfntPos],(char*)&sfnt[this->tmpOffsetTable->table[i].offset],this->tmpOffsetTable->table[i].length);
				
				aw = this->horMetric[numberOfGlyphs-1].advanceWidth;
				for (numberOfHMetrics = numberOfGlyphs; numberOfHMetrics >= 2 && this->horMetric[numberOfHMetrics-2].advanceWidth == aw; numberOfHMetrics--);
				
				hhea = (sfnt_HorizontalHeader *)&tmpSfnt[sfntPos];
				hhea->numberOf_LongHorMetrics = CSWAPW((unsigned short)numberOfHMetrics);
#ifdef _DEBUG
				assert(this->tmpOffsetTable->table[i].length == sizeOfTable);
#endif	
				break;
				
			}
			case tag_HorizontalMetrics: {
				int32_t s,d;
				unsigned short *hmtx;
#ifdef _DEBUG
				// for size estimate always assume each glyph as a different AW
				sizeOfTable = numberOfGlyphs*sizeof(sfnt_HorizontalMetrics);
#endif	
				hmtx = (unsigned short *)&tmpSfnt[sfntPos];
				s = d = 0;
				while (s < numberOfHMetrics) {
					hmtx[d++] = SWAPW(this->horMetric[s].advanceWidth);
					hmtx[d++] = SWAPW(this->horMetric[s].leftSideBearing);
					s++;
				}
				while (s < numberOfGlyphs) {
					// remainder of hmtx consists of lsb's only
					hmtx[d++] = SWAPW(this->horMetric[s].leftSideBearing);
					s++;
				}
				
				this->tmpOffsetTable->table[i].length = d*sizeof(short);
#ifdef _DEBUG
				assert(this->tmpOffsetTable->table[i].length <= sizeOfTable);
#endif	
				break;
			}
			
			case tag_GlyphData:
#ifdef _DEBUG
				sizeOfTable = this->GetPackedGlyphsSizeEstimate(glyph,glyphIndex,this->IndexToLoc);
#endif	
				memcpy((char*)this->tmpIndexToLoc,(char*)this->IndexToLoc, (numberOfGlyphs + 1) * sizeof(int32_t));
				
				this->tmpOffsetTable->table[i].length = this->PackGlyphs(strip,glyph,glyphIndex,this->IndexToLoc,this->tmpIndexToLoc,&tmpSfnt[sfntPos]);
#ifdef _DEBUG
				assert(this->tmpOffsetTable->table[i].length <= sizeOfTable);
#endif	
				break;
			
			case tag_IndexToLoc:
				//	here 'head' is (still) valid and pointing to the header of the sfnt being built
				
				// for size estimate, always assume long format 'loca' table; correct this->outShortIndexToLocTable determined in PackGlyph only
#ifdef _DEBUG
				sizeOfTable = (numberOfGlyphs + 1)*sizeof(int32_t);
#endif	
				if (this->outShortIndexToLocTable) {
					unsigned short *shortIndexToLoc = (unsigned short *)&tmpSfnt[sfntPos];
					
					this->tmpOffsetTable->table[i].length = (numberOfGlyphs + 1) * sizeof(short);
					head->indexToLocFormat = CSWAPW(SHORT_INDEX_TO_LOC_FORMAT);
					for (j = 0; j <= numberOfGlyphs; j++)
					{
						unsigned short tempIdx = (unsigned short)(this->tmpIndexToLoc[j] >> 1); 
						shortIndexToLoc[j] = SWAPW(tempIdx);
					}
				} else {
					int32_t *longIndexLoca = (int32_t *)&tmpSfnt[sfntPos];

					this->tmpOffsetTable->table[i].length = (numberOfGlyphs + 1) * sizeof(int32_t);
					head->indexToLocFormat = CSWAPW(LONG_INDEX_TO_LOC_FORMAT);
					for (j = 0; j <= numberOfGlyphs; j++)
					{
						longIndexLoca[j] = SWAPL(this->tmpIndexToLoc[j]);
					}
				}
#ifdef _DEBUG
				assert(this->tmpOffsetTable->table[i].length <= sizeOfTable);
#endif	
				break;
			
			case PRIVATE_GROUP: {
				int32_t j;
				short *charGroup,tempGroup;
				
#ifdef _DEBUG
				sizeOfTable = numberOfGlyphs*sizeof(short);
#endif	
				this->tmpOffsetTable->table[i].length = numberOfGlyphs * sizeof(short);
				charGroup = (short *)&tmpSfnt[sfntPos];
				this->charGroupOf[glyphIndex] = group;
				for (j = 0; j < numberOfGlyphs; j++) {
					tempGroup = this->charGroupOf[j];
					if (tempGroup < TOTALTYPE) tempGroup = charGroupToIntInFile[tempGroup]; // translate old char groups
					charGroup[j] = SWAPW(tempGroup);
				}
#ifdef _DEBUG
				assert(this->tmpOffsetTable->table[i].length == sizeOfTable);
#endif	
				break;
			}
		
			case PRIVATE_GLIT1:
			case PRIVATE_GLIT2: {
				int32_t glitEntries;
				bool newGlit;

				newGlit = this->tmpOffsetTable->table[i].length == 0 || this->tmpOffsetTable->table[i].offset == 0; // we've probably just added the table
				
				if (tag == PRIVATE_GLIT1) {
					memGlit = this->glit1;
					glitEntries = this->glit1Entries;
				} else { // tag == PRIVATE_GLIT2
					memGlit = this->glit2;
					glitEntries = this->glit2Entries;
				}
				if (newGlit) {
					glitEntries = this->maxGlyphs + GLIT_PAD;
					FillNewGlit(memGlit,this->maxGlyphs,glitEntries);
				}
#ifdef _DEBUG
				sizeOfTable = glitEntries*sizeof(sfnt_FileDataEntry);
#endif	
				fileGlit = (sfnt_FileDataEntry *)&tmpSfnt[sfntPos]; // need to assign for case PRIVATE_PGM right below (not BeatS architecture...)
				this->tmpOffsetTable->table[i].length = PackGlit(fileGlit,glitEntries,memGlit);

#ifdef _DEBUG
				assert(this->tmpOffsetTable->table[i].length == sizeOfTable);
#endif	
				break;
			}

			case PRIVATE_PGM1:
			case PRIVATE_PGM2: {
			//	this assumes that we do the PRIVATE_GLIT1 and 2 prior to the respective PRIVATE_PGM1 and 2,
			//	which is a valid assumption as long as VTT is the only instance to add and remove either of them,
			//	otherwise the fileGlit and memGlit below are not pointing to the correct locations in the sfnt being built
#ifdef _DEBUG
				sizeOfTable = GetPackedGlyphSourcesSize(glyfText,prepText,cvtText,talkText,fpgmText,
														tag == PRIVATE_PGM1 ? 1 : 2,glyphIndex,memGlit);
#endif	
				this->PackGlyphSources(glyfText,prepText,cvtText,talkText,fpgmText,tag == PRIVATE_PGM1 ? 1 : 2,glyphIndex,
									   fileGlit,memGlit,(uint32_t*)(&this->tmpOffsetTable->table[i].length),&tmpSfnt[sfntPos]);
#ifdef _DEBUG
				assert(this->tmpOffsetTable->table[i].length == sizeOfTable);
#endif	
				break;
	
			}

			case PRIVATE_CVAR:
				sizeOfTable = this->tsicBinSize;
				this->UpdatePrivateCvar((int32_t*)&this->tmpOffsetTable->table[i].length, &tmpSfnt[sfntPos]);
				break;

			case tag_PreProgram:
#ifdef _DEBUG
				sizeOfTable = this->binSize[asmPREP];
#endif	
				memcpy((char*)&tmpSfnt[sfntPos],(char*)this->binData[asmPREP],this->binSize[asmPREP]);
				this->tmpOffsetTable->table[i].length = this->binSize[asmPREP];
#ifdef _DEBUG
				assert(this->tmpOffsetTable->table[i].length == sizeOfTable);
#endif	
				break;
			case tag_FontProgram:
#ifdef _DEBUG
				sizeOfTable = this->binSize[asmFPGM];
#endif	
				memcpy((char*)&tmpSfnt[sfntPos],(char*)this->binData[asmFPGM],this->binSize[asmFPGM]);
				this->tmpOffsetTable->table[i].length = this->binSize[asmFPGM];
#ifdef _DEBUG
				assert(this->tmpOffsetTable->table[i].length == sizeOfTable);
#endif	
				break;
			
			case tag_ControlValue:
#ifdef _DEBUG
				sizeOfTable = this->cvt->GetCvtBinarySize();
#endif	
				this->cvt->GetCvtBinary((int32_t*)&this->tmpOffsetTable->table[i].length,&tmpSfnt[sfntPos]);
#ifdef _DEBUG
				assert(this->tmpOffsetTable->table[i].length == sizeOfTable);
#endif	
				break;
	
		//	case tag_HoriDeviceMetrics: not handled in VTT anymore...
		//	case tag_LinearThreshold:   not handled in VTT anymore...
		//	case tag_VertDeviceMetrics: not handled in VTT anymore...

            case tag_CVTVariations:
			   sizeOfTable = this->cvarBinSize;               
			   this->UpdateCvar((int32_t*)&this->tmpOffsetTable->table[i].length, &tmpSfnt[sfntPos]);
               break; 

			case tag_GridfitAndScanProc:
				if (this->gaspBinSize) {

					unsigned short *packed;
					int32_t s;
#ifdef _DEBUG
					sizeOfTable = this->gaspBinSize;
#endif	
					this->tmpOffsetTable->table[i].length = this->gaspBinSize;
					packed = (unsigned short *)&tmpSfnt[sfntPos];
					*packed++ = SWAPW(this->gaspTable.version);
					*packed++ = SWAPW(this->gaspTable.numRanges);
					for (s = 0; s < this->gaspTable.numRanges; s++) {
						unsigned short rangeMaxPPEM = this->gaspTable.gaspRange.at(s).rangeMaxPPEM; 
						unsigned short rangeGaspBehavior = this->gaspTable.gaspRange.at(s).rangeGaspBehavior; 
						*packed++ = SWAPW(rangeMaxPPEM);
						*packed++ = SWAPW(rangeGaspBehavior);
					}
#ifdef _DEBUG
					assert(this->tmpOffsetTable->table[i].length == sizeOfTable);
#endif	
				} else {
					// if this->gaspBinSize = 0 then VTT hasn't changed gasp table, so copy from original
					goto copyOld;
				}
				break;
	
			case tag_NamingTable:
				if (this->tmpOffsetTable->table[i].length == 0) { // we've just added a name table; does this make sense at all? we're not editing name tables in here (anymore)
#ifdef _DEBUG
					sizeOfTable = 6;
#endif	
					this->tmpOffsetTable->table[i].length = 6;
					memcpy((char*)&tmpSfnt[sfntPos], "\0\0\0\0\0\6", 6L); // dummy name table
#ifdef _DEBUG
					assert(this->tmpOffsetTable->table[i].length == sizeOfTable);
#endif	
				} else {
					goto copyOld;
				}
	
				break;
	
			default:
			copyOld:
#ifdef _DEBUG
				sizeOfTable = this->tmpOffsetTable->table[i].length;
#endif	
				memcpy((char*)&tmpSfnt[sfntPos],(char*)&sfnt[this->tmpOffsetTable->table[i].offset], this->tmpOffsetTable->table[i].length);
				
#ifdef _DEBUG
				assert(this->tmpOffsetTable->table[i].length == sizeOfTable);
#endif	
				break;
		} // switch (tag)
#ifdef _DEBUG
		assert(sizeEstimate[i] == sizeOfTable && sizeEstimate[i] >= this->tmpOffsetTable->table[i].length);
#endif	
		this->tmpOffsetTable->table[i].offset = sfntPos;
		sfntPos += this->tmpOffsetTable->table[i].length;
		
		pad = DWordPad(sfntPos) - sfntPos;
		memcpy((char*)&tmpSfnt[sfntPos],(char*)&zero,pad); // zero pad for long word alignment
		sfntPos += pad;
	}
#ifdef _DEBUG
	assert(newSfntSizeEstimate >= sfntPos);
#endif	
//	once we've finished plugging in all the tables, plug in the directory (temporary offset table)

	{	sfnt_DirectoryEntry *entry;
		
		entry = &(this->tmpOffsetTable->table[0]);
		for (i=0; i<this->tmpOffsetTable->numOffsets; i++, entry++)
		{
			entry->tag = SWAPL(entry->tag);
			entry->checkSum = SWAPL(entry->checkSum);
			entry->offset = SWAPL(entry->offset);
			entry->length = SWAPL(entry->length);
		}
		this->tmpOffsetTable->version = SWAPL(this->tmpOffsetTable->version);
		this->tmpOffsetTable->numOffsets = SWAPW(this->tmpOffsetTable->numOffsets);
		this->tmpOffsetTable->searchRange = SWAPW(this->tmpOffsetTable->searchRange);
		this->tmpOffsetTable->entrySelector = SWAPW(this->tmpOffsetTable->entrySelector);
		this->tmpOffsetTable->rangeShift = SWAPW(this->tmpOffsetTable->rangeShift);
	}

	memcpy((char*)tmpSfnt,(char*)this->tmpOffsetTable, headerSize);
	
	result = true; // by now
	
term: // function termination code
	if (result) { // successful so far, now swap sfntTmpHandle <-> sfntHandle and install new sfnt; swap => no resize/handle deallocation/handle copy/memory fragmentation
		tmpSfntHandle = this->sfntHandle; this->sfntHandle = this->sfntTmpHandle; this->sfntTmpHandle = tmpSfntHandle;
		tmpSfntSize = this->maxSfntSize; this->maxSfntSize = this->maxTmpSfntSize; this->maxTmpSfntSize = tmpSfntSize;
		this->sfntSize = sfntPos;

		this->CalculateNewCheckSums();
		this->CalculateCheckSumAdjustment();
		this->SortTableDirectory();
		
	// claudebe the glit entries must be updated
		this->glit1Entries = this->glit2Entries = 0;
		this->shortIndexToLocTable = this->outShortIndexToLocTable;
		
		result = this->UnpackGlitsLoca(errMsg) && this->UpdateMaxPointsAndContours(errMsg) && this->UnpackCharGroup(errMsg) && this->SetSfnt( -1, -1, errMsg);
	}
	return result;
} // TrueTypeFont::BuildNewSfnt

#define maxNumGlyphs 0x10000 // 64k
#define minDynamicArraySize 0x100000 // 1 MB

typedef struct {
	int32_t size,used;
	int32_t dataPos[maxNumGlyphs];
	unsigned char *data;
} GlyphTableArray;

bool InitDynamicArray(GlyphTableArray *dyn) {
	dyn->data = (unsigned char *)NewP(minDynamicArraySize);
	dyn->size = dyn->data != NULL ? minDynamicArraySize : 0;
	dyn->used = 0;
	return dyn->data != NULL;
} // InitDynamicArray

bool AssertDynamicArraySize(GlyphTableArray *dyn, int32_t deltaSize) {
	int32_t newSize;
	unsigned char *tmpData;
	
	if (dyn->used + deltaSize <= dyn->size) return true;

	newSize = ((AddReserve(dyn->size + deltaSize) + minDynamicArraySize - 1)/minDynamicArraySize)*minDynamicArraySize;
	tmpData = (unsigned char *)NewP(newSize);
	if (tmpData != NULL) {
		memcpy(tmpData,dyn->data,dyn->used);
		DisposeP((void **)&dyn->data);
		dyn->size = newSize;
		dyn->data = tmpData;
	}
	return tmpData != NULL;
} // AssertDynamicArraySize

bool TermDynamicArray(GlyphTableArray *dyn) {
	if (dyn->data != NULL) DisposeP((void **)&dyn->data);
	dyn->size = 0;
	return true;
} // TermDynamicArray

typedef struct {
	bool binaryOnly;

	int32_t numGlyphs,currGlyph;

	GlyphTableArray binary;
	sfnt_HorizontalMetrics hmtx[maxNumGlyphs];

	// for binaryOnly == false
	GlyphTableArray asmSrc;
	GlyphTableArray vttSrc;

	unsigned char glyfGroup[maxNumGlyphs];
	bool composite[maxNumGlyphs];
} IncrBuildSfntData;

bool TrueTypeFont::GetNumPointsAndContours(int32_t glyphIndex, int32_t *numKnots, int32_t *numContours, int32_t *componentDepth) {
	unsigned char *data;
	int16 contours,knots;
	uint16 flags,glyph;
	int32_t glyphSize;

	data = this->GetTablePointer(tag_GlyphData);
	if (data == NULL) 
		return false;

	if (glyphIndex < 0 || this->numLocaEntries <= glyphIndex) 
		return false;
	data += this->IndexToLoc[glyphIndex];

	glyphSize = this->IndexToLoc[glyphIndex+1] - this->IndexToLoc[glyphIndex];
	if (glyphSize == 0) return true; // we've probably just added a range of empty glyphs in the process of importing glyphs
	if (glyphSize < 0) 
		return false; // this would be bad, however.,.
	
	contours = READALIGNEDWORD(data); contours = SWAPW(contours);
	data += 4*sizeof(uint16); // bounding box;
	if (contours < 0) { // composite glyph
		(*componentDepth)++;
		if (*componentDepth >= MAXCOMPONENTSIZE) 
			return false;
		do {
			flags = READALIGNEDWORD(data); flags = SWAPW(flags);
			glyph = READALIGNEDWORD(data); glyph = SWAPW(glyph);
			if (!(GetNumPointsAndContours(glyph,numKnots,numContours,componentDepth))) 
				return false;
			if (flags & ARG_1_AND_2_ARE_WORDS)
				data += 2*sizeof(uint16);
			else
				data += sizeof(uint16);
			if (flags & WE_HAVE_A_SCALE)
				data += sizeof(uint16);
			else if (flags & WE_HAVE_AN_X_AND_Y_SCALE)
				data += 2*sizeof(uint16);
			else if (flags & WE_HAVE_A_TWO_BY_TWO)
				data += 4*sizeof(uint16);
		} while (flags & MORE_COMPONENTS);
	} else { // (leaf) glyph
		if (*numContours + (int32_t)contours >= MAXCONTOURS) 
			return false;
		*numContours += (int32_t)contours;
		if (contours > 0) {
			data += (contours-1)*sizeof(uint16);
			knots = READALIGNEDWORD(data); knots = SWAPW(knots) + 1; // look at end point array and add 1
			if (*numKnots + (int32_t)knots >= MAXPOINTS) 
				return false;
			*numKnots += (int32_t)knots;
		}
	}
	return true; // by now
} // TrueTypeFont::GetNumPointsAndContours

bool TrueTypeFont::IncrBuildNewSfnt( wchar_t errMsg[]) {
#define variableTablesForIncrBuild 10
	IncrBuildSfntData *iSfnt;

	errMsg[0] = L'\0';

	if (this->incrBuildSfntData == NULL) { swprintf(errMsg, L"this->incrBuildSfntData is NULL"); return false; }
	iSfnt = (IncrBuildSfntData *)this->incrBuildSfntData;

	StripCommand strip = iSfnt->binaryOnly ? stripSource : stripNothing;

	unsigned char *sfnt, *tmpSfnt, *tmpSfntHandle;

	int32_t headerSize, i, j, j2, tag, numberOfGlyphs, numberOfHMetrics, zero = 0L, pad;
	uint32_t sfntPos, tmpSfntSize, sizeOfTable, newSfntSizeEstimate;
	sfnt_FontHeader *head;
	bool result = false;

	TableAvailability avail[variableTablesForIncrBuild];

	avail[0] = TableAvailability(tag_ControlValue, true, stripHints); 
	avail[1] = TableAvailability(tag_FontProgram, true, stripHints);
	avail[2] = TableAvailability(tag_PreProgram, true, stripHints);
	avail[3] = TableAvailability(PRIVATE_GLIT1, true, stripSource);
	avail[4] = TableAvailability(PRIVATE_PGM1, true, stripSource);
	avail[5] = TableAvailability(PRIVATE_GLIT2, true, stripSource);
	avail[6] = TableAvailability(PRIVATE_PGM2, true, stripSource);
	avail[7] = TableAvailability(PRIVATE_GROUP, true, stripSource);
	avail[8] = TableAvailability(tag_CVTVariations, this->IsCvarTupleData(), stripHints);
	avail[9] = TableAvailability(PRIVATE_CVAR, this->IsCvarTupleData(), stripSource);
	
	head = (sfnt_FontHeader *)this->GetTablePointer(tag_FontHeader);
	if (SWAPL(head->magicNumber) != SFNT_MAGIC) {
		swprintf(errMsg,L"IncrBuildNewSfnt: Bad magic number in the head");
		goto term;
	}
	
	numberOfGlyphs = this->NumberOfGlyphs();
	
	this->PackMaxpHeadHhea();

	sfnt = this->sfntHandle; // alias it
	
	if (!BuildOffsetTables(sfnt,this->sfntSize,this->offsetTable,this->tmpOffsetTable,variableTablesForIncrBuild,avail,strip,&headerSize)) {
		swprintf(errMsg,L"IncrBuildNewSfnt: offset is too large"); // claudebe, problem with a new font received from Sampo
		goto term;
	}

#ifdef _DEBUG
//	given MYOFFSETTABLESIZE for the size of the entire sfnt_OffsetTable, maxNumDirectoryEntries denotes
//	the (truncated) number of sfnt_DirectoryEntry entries that this amount of memory is good for
//	(see comments about inherited hack above)
#define maxNumDirectoryEntries ((MYOFFSETTABLESIZE - sizeof(sfnt_OffsetTable))/sizeof(sfnt_DirectoryEntry) + 1)
	uint32_t sizeEstimate[maxNumDirectoryEntries];
	char tagCopy[maxNumDirectoryEntries][8];
	for (i = 0; i < maxNumDirectoryEntries; i++) { sizeEstimate[i] = 0; tagCopy[i][0] = 0; }
#endif
	
	// 1. pass: get a reasonnable upper limit for the expected file size
	newSfntSizeEstimate = headerSize;
	
	for (i = 0; i < this->tmpOffsetTable->numOffsets; i++) {

		switch (tag = this->tmpOffsetTable->table[i].tag) {
			
			case tag_HorizontalMetrics:
				// discovered in an old version of Courier New Italic, glyph "E" had a different AW than a composite using "E" (and E's metrics)
				// which was correctly propagated to the hmtx upon PackGlyph, after which point the actual packing routine for the hmtx worked
				// with a different numberOfHMetrics triggering an assert because a differing table size was detected
				// Therefore, for size estimate always assume each glyph as a different AW
				sizeOfTable = numberOfGlyphs*sizeof(sfnt_HorizontalMetrics);
				break;
			case tag_GlyphData:
				sizeOfTable = iSfnt->binary.used;
				break;
			case tag_IndexToLoc:
				// for size estimate, always assume long format 'loca' table; correct this->outShortIndexToLocTable determined in PackGlyph only
				sizeOfTable = (numberOfGlyphs + 1)*sizeof(int32_t);
				break;
			case PRIVATE_GROUP:
				sizeOfTable = numberOfGlyphs*sizeof(short);
				break;
			case PRIVATE_GLIT1:
			case PRIVATE_GLIT2:
				sizeOfTable = (numberOfGlyphs + GLIT_PAD)*sizeof(sfnt_FileDataEntry);
				break;
			case PRIVATE_CVAR:
				sizeOfTable = this->tsicBinSize = this->EstimatePrivateCvar();
				break;
			case PRIVATE_PGM1:
				sizeOfTable = iSfnt->asmSrc.used;
				break;
			case PRIVATE_PGM2:
				sizeOfTable = iSfnt->vttSrc.used;
				break;
			case tag_PreProgram:
				sizeOfTable = this->binSize[asmPREP];
				break;
			case tag_FontProgram:
				sizeOfTable = this->binSize[asmFPGM];
				break;
			case tag_ControlValue:
				sizeOfTable = this->cvt->GetCvtBinarySize();
				break;
			case tag_GridfitAndScanProc:
				sizeOfTable = this->gaspBinSize ? this->gaspBinSize : this->tmpOffsetTable->table[i].length;
				break;
            case tag_CVTVariations:
				this->cvarBinSize = this->EstimateCvar();
                sizeOfTable = this->cvarBinSize ? this->cvarBinSize : this->tmpOffsetTable->table[i].length;
                break;
			default:
				sizeOfTable = this->tmpOffsetTable->table[i].length;
				break;
		} // switch (tag)

#ifdef _DEBUG
		tagCopy[i][0] = (char)(tag >> 24);
		tagCopy[i][1] = (char)(tag >> 16);
		tagCopy[i][2] = (char)(tag >>  8);
		tagCopy[i][3] = (char)(tag);
		tagCopy[i][4] = (char)0;
#endif	

		sizeOfTable = DWordPad(sizeOfTable);
		newSfntSizeEstimate += sizeOfTable;
	}

	this->AssertMaxSfntSize(newSfntSizeEstimate,false,true); // make sure we have a large enough tmpSfnt

	if (newSfntSizeEstimate > this->maxTmpSfntSize) {
		MaxSfntSizeError(L"IncrBuildNewSfnt: This font is getting too large",newSfntSizeEstimate,errMsg); goto term;
	}
	
	// 2. pass: build the new sfnt into the tmpSfntHandle
	tmpSfnt = this->sfntTmpHandle; // alias this one, too
	
	sfntPos = headerSize; // skip header, need to fill directory first, then plug in header as final step
	
	for (i = 0; i < this->tmpOffsetTable->numOffsets; i++) {

		switch (tag = this->tmpOffsetTable->table[i].tag) {
			
			case tag_FontHeader: {
				memcpy((char*) &tmpSfnt[sfntPos],(char*)&sfnt[this->tmpOffsetTable->table[i].offset],this->tmpOffsetTable->table[i].length);
				head = (sfnt_FontHeader *)&tmpSfnt[sfntPos];
				long long temp_time = DateTime(); // ADD GREGH 20101209
				uint32_t temp_lower = (uint32_t)(temp_time & 0xFFFFFFFF); // ADD GREGH 20101209
				int32_t temp_upper = (int32_t)((temp_time >> 32) & 0xFFFFFFFF); // ADD GREGH 20101209
				head->modified.bc = SWAPL(temp_upper); // ADD GREGH 20101209
				head->modified.ad = SWAPL(temp_lower); // ADD GREGH 20101209

				break;
			}	
			case tag_HoriHeader: { // determine numberOfHMetrics, required in tag_HorizontalMetrics below!!!
				int32_t aw;
				sfnt_HorizontalHeader *hhea;
				
				memcpy((char *)&tmpSfnt[sfntPos],(char *)&sfnt[this->tmpOffsetTable->table[i].offset],this->tmpOffsetTable->table[i].length);
				
				aw = iSfnt->hmtx[numberOfGlyphs-1].advanceWidth;
				for (numberOfHMetrics = numberOfGlyphs; numberOfHMetrics >= 2 && iSfnt->hmtx[numberOfHMetrics-2].advanceWidth == aw; numberOfHMetrics--);
				
				hhea = (sfnt_HorizontalHeader *)&tmpSfnt[sfntPos];
				hhea->numberOf_LongHorMetrics = CSWAPW((unsigned short)numberOfHMetrics);
				break;
			}
			case tag_HorizontalMetrics: {
				int32_t s,d;
				unsigned short *hmtx;

				hmtx = (unsigned short *)&tmpSfnt[sfntPos];
				s = d = 0;
				while (s < numberOfHMetrics) {
					hmtx[d++] = SWAPW(iSfnt->hmtx[s].advanceWidth);
					hmtx[d++] = SWAPW(iSfnt->hmtx[s].leftSideBearing);
					s++;
				}
				while (s < numberOfGlyphs) {
					// remainder of hmtx consists of lsb's only
					hmtx[d++] = SWAPW(iSfnt->hmtx[s].leftSideBearing);
					s++;
				}
				
				this->tmpOffsetTable->table[i].length = d*sizeof(short);
				break;
			}
			
			case tag_GlyphData: {
				this->tmpOffsetTable->table[i].length = iSfnt->binary.used;
				memcpy(&tmpSfnt[sfntPos],iSfnt->binary.data,iSfnt->binary.used);
				break;
			}
			case tag_IndexToLoc:
				//	here 'head' is (still) valid and pointing to the header of the sfnt being built
				
				this->outShortIndexToLocTable = iSfnt->binary.used < 65535*2;
				if (this->outShortIndexToLocTable) {
					unsigned short tempIdx; 
					unsigned short *shortIndexToLoc = (unsigned short *)&tmpSfnt[sfntPos];
					
					this->tmpOffsetTable->table[i].length = (numberOfGlyphs + 1) * sizeof(short);
					head->indexToLocFormat = CSWAPW(SHORT_INDEX_TO_LOC_FORMAT);
					for (j = 0; j < numberOfGlyphs; j++){
						tempIdx = (unsigned short)(iSfnt->binary.dataPos[j] >> 1);
						shortIndexToLoc[j] = SWAPW(tempIdx);
					}
					tempIdx = (unsigned short)(iSfnt->binary.used >> 1);
					shortIndexToLoc[numberOfGlyphs] = SWAPW(tempIdx);
				} else {
					int32_t *longIndexLoca = (int32_t *)&tmpSfnt[sfntPos];

					this->tmpOffsetTable->table[i].length = (numberOfGlyphs + 1) * sizeof(int32_t);
					head->indexToLocFormat = CSWAPW(LONG_INDEX_TO_LOC_FORMAT);
					for (j = 0; j < numberOfGlyphs; j++) {
						longIndexLoca[j] = SWAPL(iSfnt->binary.dataPos[j]);
					}
					longIndexLoca[numberOfGlyphs] = SWAPL(iSfnt->binary.used);
				}
				break;
			
			case PRIVATE_GROUP: {
				int32_t j;
				short *charGroup,tempGroup;
				
				this->tmpOffsetTable->table[i].length = numberOfGlyphs * sizeof(short);
				charGroup = (short *)&tmpSfnt[sfntPos];
				for (j = 0; j < numberOfGlyphs; j++) {
					tempGroup = iSfnt->glyfGroup[j];
					if (tempGroup < TOTALTYPE) tempGroup = charGroupToIntInFile[tempGroup]; // translate old char groups
					charGroup[j] = SWAPW(tempGroup);
				}
				break;
			}
	
			case PRIVATE_GLIT1:
			case PRIVATE_GLIT2: {
				int32_t glitEntries;
				sfnt_MemDataEntry *memGlit;
				GlyphTableArray *src;

				glitEntries = numberOfGlyphs + GLIT_PAD;

				if (tag == PRIVATE_GLIT1) {
					memGlit = this->glit1;
					src = &iSfnt->asmSrc;
				} else { // tag == PRIVATE_GLIT2
					memGlit = this->glit2;
					src = &iSfnt->vttSrc;
				}

				for (j = 0; j < numberOfGlyphs; j++) {
					memGlit[j].glyphCode = (unsigned short)j;
					memGlit[j].offset = src->dataPos[j];
					memGlit[j].length = src->dataPos[j+1] - src->dataPos[j];
				}
				for (j = numberOfGlyphs, j2 = 0; j2 < GLIT_PAD; j++, j2++) {
					memGlit[j].glyphCode = glitPadIndex[j2];
					memGlit[j].offset = src->dataPos[j];
					memGlit[j].length = (j < glitEntries-1 ? src->dataPos[j+1] : src->used) - src->dataPos[j];
				}
				memGlit[numberOfGlyphs].offset = PRIVATE_STAMP_1; // Stamp it;
				
				this->tmpOffsetTable->table[i].length = PackGlit((sfnt_FileDataEntry *)&tmpSfnt[sfntPos],glitEntries,memGlit);
				break;
			}
			case PRIVATE_PGM1: {
				this->tmpOffsetTable->table[i].length = iSfnt->asmSrc.used;
				memcpy(&tmpSfnt[sfntPos],iSfnt->asmSrc.data,iSfnt->asmSrc.used);
				break;
			}
			case PRIVATE_PGM2: {
				this->tmpOffsetTable->table[i].length = iSfnt->vttSrc.used;
				memcpy(&tmpSfnt[sfntPos],iSfnt->vttSrc.data,iSfnt->vttSrc.used);
				break;
			}
			case tag_PreProgram: {
				this->tmpOffsetTable->table[i].length = this->binSize[asmPREP];
				memcpy((char*)&tmpSfnt[sfntPos],(char*)this->binData[asmPREP],this->binSize[asmPREP]);
				break;
			}
			case tag_FontProgram: {
				this->tmpOffsetTable->table[i].length = this->binSize[asmFPGM];
				memcpy((char*)&tmpSfnt[sfntPos],(char*)this->binData[asmFPGM],this->binSize[asmFPGM]);
				break;
			}
			case tag_ControlValue: {
				this->cvt->GetCvtBinary((int32_t*)&this->tmpOffsetTable->table[i].length,&tmpSfnt[sfntPos]);
				break;
			}
			case PRIVATE_CVAR:
				if (this->tsicBinSize) {
					this->UpdatePrivateCvar((int32_t*)&this->tmpOffsetTable->table[i].length, &tmpSfnt[sfntPos]);
				} else {
					memcpy((char*)&tmpSfnt[sfntPos], (char*)&sfnt[this->tmpOffsetTable->table[i].offset], this->tmpOffsetTable->table[i].length);
				}
				break;
            case tag_CVTVariations: 
                if (this->cvarBinSize) {
					this->UpdateCvar((int32_t*)&this->tmpOffsetTable->table[i].length, &tmpSfnt[sfntPos]);
                } else {
					memcpy((char*)&tmpSfnt[sfntPos], (char*)&sfnt[this->tmpOffsetTable->table[i].offset], this->tmpOffsetTable->table[i].length);
				}
                break; 
			case tag_GridfitAndScanProc:
				if (this->gaspBinSize) {
					unsigned short *packed;
					int32_t s;

					this->tmpOffsetTable->table[i].length = this->gaspBinSize;
					packed = (unsigned short *)&tmpSfnt[sfntPos];
					*packed++ = SWAPW(this->gaspTable.version);
					*packed++ = SWAPW(this->gaspTable.numRanges);
					for (s = 0; s < this->gaspTable.numRanges; s++) {
						unsigned short rangeMaxPPEM = this->gaspTable.gaspRange.at(s).rangeMaxPPEM;
						unsigned short rangeGaspBehavior = this->gaspTable.gaspRange.at(s).rangeGaspBehavior;
						*packed++ = SWAPW(rangeMaxPPEM);
						*packed++ = SWAPW(rangeGaspBehavior);
					}
				} else {
					memcpy((char*)&tmpSfnt[sfntPos],(char*)&sfnt[this->tmpOffsetTable->table[i].offset], this->tmpOffsetTable->table[i].length);
				}
				break;
	
			default: {
				memcpy((char*)&tmpSfnt[sfntPos],(char*)&sfnt[this->tmpOffsetTable->table[i].offset], this->tmpOffsetTable->table[i].length);
				break;
			}
		} // switch (tag)

		this->tmpOffsetTable->table[i].offset = sfntPos;
		sfntPos += this->tmpOffsetTable->table[i].length;
		
		pad = DWordPad(sfntPos) - sfntPos;
		memcpy((char*)&tmpSfnt[sfntPos],(char*)&zero,pad); // zero pad for long word alignment
		sfntPos += pad;
	}
#ifdef _DEBUG
	assert(newSfntSizeEstimate >= sfntPos);
#endif	
//	once we've finished plugging in all the tables, plug in the directory (temporary offset table)

	{	sfnt_DirectoryEntry *entry;
		
		entry = &(this->tmpOffsetTable->table[0]);
		for (i=0; i<this->tmpOffsetTable->numOffsets; i++, entry++)
		{
			entry->tag = SWAPL(entry->tag);
			entry->checkSum = SWAPL(entry->checkSum);
			entry->offset = SWAPL(entry->offset);
			entry->length = SWAPL(entry->length);
		}
		this->tmpOffsetTable->version = SWAPL(this->tmpOffsetTable->version);
		this->tmpOffsetTable->numOffsets = SWAPW(this->tmpOffsetTable->numOffsets);
		this->tmpOffsetTable->searchRange = SWAPW(this->tmpOffsetTable->searchRange);
		this->tmpOffsetTable->entrySelector = SWAPW(this->tmpOffsetTable->entrySelector);
		this->tmpOffsetTable->rangeShift = SWAPW(this->tmpOffsetTable->rangeShift);
	}
	memcpy((char*)tmpSfnt,(char*)this->tmpOffsetTable, headerSize);
	
	result = true; // by now
	
term: // function termination code
	if (result) { // successful so far, now swap sfntTmpHandle <-> sfntHandle and install new sfnt; swap => no resize/handle deallocation/handle copy/memory fragmentation
		tmpSfntHandle = this->sfntHandle; this->sfntHandle = this->sfntTmpHandle; this->sfntTmpHandle = tmpSfntHandle;
		tmpSfntSize = this->maxSfntSize; this->maxSfntSize = this->maxTmpSfntSize; this->maxTmpSfntSize = tmpSfntSize;
		this->sfntSize = sfntPos;
	
		this->CalculateNewCheckSums();
		this->CalculateCheckSumAdjustment();
		this->SortTableDirectory();
		
	// claudebe the glit entries must be updated
		this->glit1Entries = this->glit2Entries = 0;
		this->shortIndexToLocTable = this->outShortIndexToLocTable;
		
		// adjust maxGlyphs to what we've stamped in PRIVATE_GLIT1 and PRIVATE_GLIT2 above
		this->maxGlyphs = numberOfGlyphs;

		result = this->UnpackGlitsLoca(errMsg) && this->UpdateMaxPointsAndContours(errMsg) && this->UnpackCharGroup(errMsg) && this->SetSfnt(-1,-1,errMsg);
	}
	return result;

} // TrueTypeFont::IncrBuildNewSfnt

bool TrueTypeFont::InitIncrBuildSfnt(bool binaryOnly, wchar_t errMsg[]) {
	IncrBuildSfntData *iSfnt;
	bool allocated;

	errMsg[0] = L'\0';

	this->incrBuildSfntData = iSfnt = (IncrBuildSfntData *)NewP(sizeof(IncrBuildSfntData));
	if (iSfnt == NULL) { swprintf(errMsg,L"Failed to allocate this->incrBuildSfntData"); return false; }

	iSfnt->binaryOnly = binaryOnly;
	iSfnt->numGlyphs = this->NumberOfGlyphs();
	iSfnt->currGlyph = 0;

	allocated = InitDynamicArray(&iSfnt->binary);
	if (allocated && !iSfnt->binaryOnly) allocated = InitDynamicArray(&iSfnt->asmSrc) && InitDynamicArray(&iSfnt->vttSrc);

	if (!allocated) { swprintf(errMsg,L"Failed to allocate iSfnt dynamic arrays"); return false; }

	this->InitNewProfiles(); // TrueTypeFont keeps two sets of profiles, both are updated after any assembly, but only one is reset here	
	
	return true; // by now
} // TrueTypeFont::InitIncrBuildSfnt

void AddTextToDynArray(GlyphTableArray *dynArray, int32_t glyphIndex, TextBuffer *text) {
	unsigned char *data;
	size_t packedSize;

	data = &dynArray->data[dynArray->used];
	if (text != NULL) text->GetText(&packedSize,(char *)data); else packedSize = 0;
	if (packedSize & 1) data[packedSize++] = '\x0d'; // CR-pad word align
	dynArray->dataPos[glyphIndex] = dynArray->used;
	dynArray->used += (int32_t)packedSize;
} // AddTextToDynArray

bool TrueTypeFont::AddGlyphToNewSfnt(CharGroup group, int32_t glyphIndex, TrueTypeGlyph *glyph, int32_t glyfBinSize, unsigned char *glyfBin, TextBuffer *glyfText, TextBuffer *talkText, wchar_t errMsg[]) {
	IncrBuildSfntData *iSfnt;
	bool allocated;
	unsigned char *data;
	int32_t packedSize;

	errMsg[0] = L'\0';

	if (this->incrBuildSfntData == NULL) { swprintf(errMsg,L"this->incrBuildSfntData is NULL"); return false; }
	iSfnt = (IncrBuildSfntData *)this->incrBuildSfntData;

	if (iSfnt->currGlyph != glyphIndex || glyphIndex < 0 || iSfnt->numGlyphs <= glyphIndex) {
		swprintf(errMsg,L"Calling TrueTypeFont::AddGlyphToNewSfnt out of sequence: expected glyph %li, received glyph %li",iSfnt->currGlyph,glyphIndex); return false;
	}

	iSfnt->glyfGroup[iSfnt->currGlyph] = (unsigned char)group;

	allocated = AssertDynamicArraySize(&iSfnt->binary,this->GetPackedGlyphSize(glyphIndex,glyph,glyfBinSize));
	if (allocated && !iSfnt->binaryOnly) 
	{
		allocated = AssertDynamicArraySize(&iSfnt->asmSrc,glyfText->TheLengthInUTF8()) && AssertDynamicArraySize(&iSfnt->vttSrc,talkText->TheLengthInUTF8());
	}

	if (!allocated)
	{ 
		swprintf(errMsg,L"Failed to assert iSfnt dynamic array sizes"); 
		return false; 
	}

	iSfnt->composite[iSfnt->currGlyph] = glyph->composite;
	
	data = &iSfnt->binary.data[iSfnt->binary.used];
	packedSize = this->PackGlyph(data,iSfnt->currGlyph,glyph,glyfBinSize,glyfBin,&iSfnt->hmtx[iSfnt->currGlyph]);
	if (packedSize & 1) data[packedSize++] = '\x00'; // zero-pad word align
	iSfnt->binary.dataPos[iSfnt->currGlyph] = iSfnt->binary.used;
	iSfnt->binary.used += packedSize;
	
	if (!iSfnt->binaryOnly) {
		AddTextToDynArray(&iSfnt->asmSrc,iSfnt->currGlyph,glyfText);
		AddTextToDynArray(&iSfnt->vttSrc,iSfnt->currGlyph,talkText);
	}
	
	iSfnt->currGlyph++;

	return true; // by now
} // TrueTypeFont::AddGlyphToNewSfnt

bool TrueTypeFont::TermIncrBuildSfnt(bool disposeOnly, TextBuffer *prepText, TextBuffer *cvtText, TextBuffer *fpgmText, wchar_t errMsg[]) {
	// notice that it may seem inconsistent that in the above AddGlyphToNewSfnt we add both sources and binary data (of individual glyphs),
	// while for TermIncrBuild we only add sources. This has to do with the weird architecture which, from the point of view of sources,
	// understands prep, fpgm, and cvt as "special" glyphs at the end of the respective private table, while from the point of view of
	// binary data, prep, fpgm, and cvt are individual TrueType tables. Not my first choice...

	IncrBuildSfntData *iSfnt;
	bool done;

	if (this->incrBuildSfntData == NULL) {
		// don't overwrite errMsg if we're merely called to "cleanup", see also calling pattern
		if (!disposeOnly) swprintf(errMsg,L"this->incrBuildSfntData is NULL");
		return false;
	}

	iSfnt = (IncrBuildSfntData *)this->incrBuildSfntData;

	if (disposeOnly) {
		// don't return true if we're merely called to "cleanup", see also calling pattern
		done = false;
	} else {
		done = true; // assume
		errMsg[0] = L'\0';
		
		if (!iSfnt->binaryOnly) {
			done = AssertDynamicArraySize(&iSfnt->asmSrc,prepText->TheLengthInUTF8() + cvtText->TheLengthInUTF8() + fpgmText->TheLengthInUTF8());
			if (done) {
				AddTextToDynArray(&iSfnt->asmSrc,iSfnt->currGlyph,  NULL); // dummy for future stamp
				AddTextToDynArray(&iSfnt->asmSrc,iSfnt->currGlyph+1,prepText); // the sources for the prep, fpgm, cvt, and parm
				AddTextToDynArray(&iSfnt->asmSrc,iSfnt->currGlyph+2,fpgmText); // must follow the same sequence as glitPadIndex!!!
				AddTextToDynArray(&iSfnt->asmSrc,iSfnt->currGlyph+3,cvtText);
				AddTextToDynArray(&iSfnt->asmSrc,iSfnt->currGlyph+4,NULL);

				AddTextToDynArray(&iSfnt->vttSrc,iSfnt->currGlyph,  NULL); // dummy for future stamp
				AddTextToDynArray(&iSfnt->vttSrc,iSfnt->currGlyph+1,NULL); // more dummies
				AddTextToDynArray(&iSfnt->vttSrc,iSfnt->currGlyph+2,NULL);
				AddTextToDynArray(&iSfnt->vttSrc,iSfnt->currGlyph+3,NULL);
				AddTextToDynArray(&iSfnt->vttSrc,iSfnt->currGlyph+4,NULL);
			} else {
				swprintf(errMsg,L"Failed to assert iSfnt dynamic array sizes");
			}
		}

		
		if (done) {
			this->UseNewProfiles(); //  copy new profiles into current ones and pack into current sfnt
			done = this->IncrBuildNewSfnt(errMsg);
		}

	}

	if (!iSfnt->binaryOnly) {
		TermDynamicArray(&iSfnt->vttSrc);
		TermDynamicArray(&iSfnt->asmSrc);
	}
	TermDynamicArray(&iSfnt->binary);

	DisposeP((void **)&iSfnt);
	this->incrBuildSfntData = NULL;

	return done;
} // TrueTypeFont::TermIncrBuildSfnt

sfnt_maxProfileTable TrueTypeFont::GetProfile(void) {
	return this->profile;
} // TrueTypeFont::GetProfile

int32_t TrueTypeFont::GetUnitsPerEm(void) {							// FUnits Per EM (2048 is typical)
	return this->unitsPerEm;
} // TrueTypeFont::GetUnitsPerEm

void TrueTypeFont::InitNewProfiles(void) {
	this->newProfile.version 				= 0x00010000;
	this->newProfile.numGlyphs 				= this->profile.numGlyphs;
	this->newProfile.maxPoints 				= 0;
	this->newProfile.maxContours 			= 0;
	this->newProfile.maxCompositePoints 	= 0;
	this->newProfile.maxCompositeContours 	= 0; 
	
	//	those values shouldn't be updated in recalc 'maxp', only reset during autohinting  GregH 8/23/95
	//	this->newProfile.maxElements 			= this->profile.maxElements; 
	//	this->newProfile.maxTwilightPoints		= this->profile.maxTwilightPoints; 
	//	this->newProfile.maxStorage 			= this->profile.maxStorage; 
	
	//	as per ClaudeBe 6/30/97 we can safely assume these minima
	this->newProfile.maxElements 			= Max(MIN_ELEMENTS,this->profile.maxElements); 
	this->newProfile.maxTwilightPoints		= Max(MIN_TWILIGHTPOINTS,this->profile.maxTwilightPoints); 
	this->newProfile.maxStorage 			= Max(MIN_STORAGE,this->profile.maxStorage); 
	
	this->newProfile.maxFunctionDefs		= this->profile.maxFunctionDefs;
	this->newProfile.maxInstructionDefs 	= this->profile.maxInstructionDefs;
	this->newProfile.maxStackElements		= 0;
	this->newProfile.maxSizeOfInstructions	= 0;
	this->newProfile.maxComponentElements	= 0;
	this->newProfile.maxComponentDepth		= 0;
	// see comments in TrueTypeFont::UpdateAssemblerProfile
	for (int32_t i = 0; i < Len(this->maxStackElements); i++) this->maxStackElements[i] = 0;
	
	this->newMetricProfile.xMin				= 0x7fff;
	this->newMetricProfile.yMin				= 0x7fff;
	this->newMetricProfile.xMax				= (short)0x8000;
	this->newMetricProfile.yMax				= (short)0x8000;

	this->newMetricProfile.advanceWidthMax		= (short)0x8000;
	this->newMetricProfile.minLeftSideBearing	= (short)0x7fff;
	this->newMetricProfile.minRightSideBearing	= 0x7fff;
	this->newMetricProfile.xMaxExtent			= (short)0x8000;
} // TrueTypeFont::InitNewProfiles

void TrueTypeFont::InheritProfiles(void)
{
	this->newProfile = this->profile; 
}

void TrueTypeFont::UseNewProfiles(void) {	

	this->profile = this->newProfile;	
	this->metricProfile = this->newMetricProfile;
	this->PackMaxpHeadHhea();
} // TrueTypeFont::UseNewProfiles

void TrueTypeFont::UpdateGlyphProfile(TrueTypeGlyph *glyph) {
	short numberOfPoints = glyph->numContoursInGlyph ? glyph->endPoint[glyph->numContoursInGlyph-1] + 1 : 0;

	if (!glyph->composite) { // cf. TT specs, "maxp": Max points/contours in a non-composite glyph
		this->profile.maxPoints = Max(this->profile.maxPoints,numberOfPoints);
		this->newProfile.maxPoints = Max(this->newProfile.maxPoints,numberOfPoints);
		this->profile.maxContours = Max(this->profile.maxContours,(short)glyph->numContoursInGlyph);
		this->newProfile.maxContours = Max(this->newProfile.maxContours,(short)glyph->numContoursInGlyph);
	}
} // TrueTypeFont::UpdateGlyphProfile

void TrueTypeFont::UpdateMetricProfile(TrueTypeGlyph *glyph) {
	short end = glyph->numContoursInGlyph > 0 ? glyph->endPoint[glyph->numContoursInGlyph-1] : -1,
		  advanceWidth = glyph->x[end + PHANTOMPOINTS] - glyph->x[end + PHANTOMPOINTS - 1]; // get whatever the rasterizer has deemed correct in terms of composite and useMyMetrics
	
	this->metricProfile.xMin	= Min(this->metricProfile.xMin,   glyph->xmin);
	this->newMetricProfile.xMin = Min(this->newMetricProfile.xMin,glyph->xmin);
	this->metricProfile.xMax	= Max(this->metricProfile.xMax,   glyph->xmax);
	this->newMetricProfile.xMax = Max(this->newMetricProfile.xMax,glyph->xmax);
	this->metricProfile.yMin	= Min(this->metricProfile.yMin,   glyph->ymin);
	this->newMetricProfile.yMin = Min(this->newMetricProfile.yMin,glyph->ymin);
	this->metricProfile.yMax	= Max(this->metricProfile.yMax,   glyph->ymax);
	this->newMetricProfile.yMax = Max(this->newMetricProfile.yMax,glyph->ymax);
	
	this->metricProfile.minLeftSideBearing	   = Min(this->metricProfile.minLeftSideBearing,	glyph->xmin);
	this->newMetricProfile.minLeftSideBearing  = Min(this->newMetricProfile.minLeftSideBearing, glyph->xmin);
	this->metricProfile.minRightSideBearing	   = Min(this->metricProfile.minRightSideBearing,	advanceWidth - glyph->xmax);
	this->newMetricProfile.minRightSideBearing = Min(this->newMetricProfile.minRightSideBearing,advanceWidth - glyph->xmax);
	this->metricProfile.advanceWidthMax		   = Max(this->metricProfile.advanceWidthMax,		advanceWidth);
	this->newMetricProfile.advanceWidthMax	   = Max(this->newMetricProfile.advanceWidthMax,	advanceWidth);
	this->metricProfile.xMaxExtent			   = Max(this->metricProfile.xMaxExtent,			glyph->xmax);
	this->newMetricProfile.xMaxExtent		   = Max(this->newMetricProfile.xMaxExtent,			glyph->xmax);
} // TrueTypeFont::UpdateMetricProfile

void TrueTypeFont::UpdateAssemblerProfile(ASMType asmType, short maxFunctionDefs, short maxStackElements, short maxSizeOfInstructions) {
	uint16 maxStackElementsPrepFpgm,maxStackElementsGlyfFpgm;

	this->profile.maxFunctionDefs = Max(this->profile.maxFunctionDefs,maxFunctionDefs);
	this->newProfile.maxFunctionDefs = Max(this->newProfile.maxFunctionDefs,maxFunctionDefs);
	
	this->profile.maxStackElements = Max(this->profile.maxStackElements,maxStackElements); // <--- relevant during individual assembly

	// Coming up with a good max value for the number of stack elements used at any one time
	// is a bit of a heuristic, because we cannot possibly test all glyphs against all sizes
	// and all combinations of rendering modes (black-and-white, grey-scaled, many versions
	// of cleartype) and all variants of the rasterizer. If there were no branches, loops, or
	// calls, then it would be straightforward to determine an upper limit for the fpgm (or
	// any function therein), for the prep, and for the maximum of all upper limits for any
	// of the glyfs. For branches, the argument could be made that each branch will have to
	// affect the stack depth in the same way, else the respective code will terminate with
	// an imbalance (stack underflow, or extra stack elements left over). To account for that
	// we can safely take the maximum of all branches (and recursively for branches within
	// branches). For loops theoretically each iteration could grow the stack, and the number
	// of iterations need not be known by the assembler, so this could theoretically lead to
	// unbound growth. This is a much less likely scenario though because I have not seen a
	// great many fonts with iterations, and the described scenario would again be an erro-
	// neous piece of code. Function calls, in simple cases, typically do not call other
	// functions, so the safe upper limit would be to take the deepest stack of any glyf and
	// add to that the deepest stack resulting from any function call, or the maximum depth
	// resulting from executing the prep and add to that the deepest stack resulting from any
	// function call. Calls within calls, and much less likely, recursive calls, would again
	// falsify this assumption, but bar testing the whole matrix mentioned above, we cannot
	// think of a better solution at this time. Notice, finally, that we assume that the
	// TTAssembler, which produces values for maxStackElements upon assembling the fpgm,
	// prep, or any of the glyfs, chooses the smartest strategy it can, but we didn't verify
	// this assumption because the code is a little bit too chaotic to understand what it
	// really does...
	
	// new heuristic for re-calc maxp or compiling/assembling everything for all glyphs only!!!

	// notice that in order to handle this->profile.maxStackElements in much the same way as
	// this->newProfile.maxStackElements, we'd have to store maxp values for maxStackElements 
	// separately, for each of the fpgm, prep, and glyf. Since historically we don't, what
	// comes in when opening a font is a this->profile.maxStackElements that is either updated
	// according to the old method, or it is a max of a sum of parts we don't know anymore.
	// Therefore this is the best we can do. Any problems produced by the incremental method
	// (such as after a newly assembled glyf) can always be corrected by re-calculating the
	// maxp from scratch.

	this->maxStackElements[asmType] = Max(this->maxStackElements[asmType],maxStackElements);
	// from both the deepest level in the prep or any glyf we could call a function, hence add the respective maxima
	maxStackElementsPrepFpgm = this->maxStackElements[asmPREP] + this->maxStackElements[asmFPGM];
	maxStackElementsGlyfFpgm = this->maxStackElements[asmGLYF] + this->maxStackElements[asmFPGM];
	// the max stack depth then becomes the maximum of the two intermediate maxima above
	this->newProfile.maxStackElements = Max(maxStackElementsPrepFpgm,maxStackElementsGlyfFpgm); // <--- relevant during complete assembly/re-calc maxp
	
	this->profile.maxSizeOfInstructions = Max(this->profile.maxSizeOfInstructions,maxSizeOfInstructions);
	this->newProfile.maxSizeOfInstructions = Max(this->newProfile.maxSizeOfInstructions,maxSizeOfInstructions);
} // TrueTypeFont::UpdateAssemblerProfile

void TrueTypeFont::UpdateAutohinterProfile(short maxElements, short maxTwilightPoints, short maxStorage) {
	// for autohinter pre-program
	this->profile.maxFunctionDefs = 0;
	this->newProfile.maxFunctionDefs = 0;

	this->profile.maxInstructionDefs = 0;
	this->newProfile.maxInstructionDefs = 0;

	this->profile.maxElements = maxElements;
	this->newProfile.maxElements = maxElements;
	
	this->profile.maxTwilightPoints = maxTwilightPoints;
	this->newProfile.maxTwilightPoints = maxTwilightPoints; 
	
	this->profile.maxStorage = maxStorage;
	this->newProfile.maxStorage = maxStorage;
} // TrueTypeFont::UpdateAutohinterProfile

bool TrueTypeFont::HasSource()
{
	return (this->glit1Entries != 0) && (this->glit2Entries != 0); 
}

bool TrueTypeFont::ReverseInterpolateCvarTuples()
{
	bool result = false; 
	
	if (!this->IsVariationFont())
		return true; // all done
	
	
	auto instanceManager = this->GetInstanceManager();
	auto cvarTuples = instanceManager->GetCvarTuples();

	std::vector<int16_t> defaultCvts;
	this->GetDefaultCvts(defaultCvts);

	// Iterate through tuples 
	for (uint16_t index = 0; index < cvarTuples->size(); ++index)
	{
		auto& editedCvtsFromTuple = cvarTuples->at(index)->editedCvtValues;
		auto& interpolatedCvtsFromTuple = cvarTuples->at(index)->interpolatedCvtValues;

		// Assert size of editedCvtValue with defaultCvt size
		editedCvtsFromTuple.resize(defaultCvts.size(), Variation::EditedCvtValue());
		// Initialize interpolatedCvtValues with defaultCvt values
		interpolatedCvtsFromTuple.resize(defaultCvts.size(), Variation::InterpolatedCvtValue());
		for (size_t cvt = 0; cvt < defaultCvts.size(); ++cvt)
		{
			interpolatedCvtsFromTuple[cvt] = Variation::InterpolatedCvtValue(defaultCvts[cvt]);
		}

		for (size_t cvt = 0; cvt < defaultCvts.size(); cvt++)
		{
			if (editedCvtsFromTuple.at(cvt).Edited())
			{
				interpolatedCvtsFromTuple[cvt].Update(editedCvtsFromTuple.at(cvt).Value<int32_t>());
			}
		}

	}

	result = cvtVariationInterpolator_->ReverseInterpolate(defaultCvts, axisCount_, *cvarTuples);	
	
	return result;
}


template<typename T>
bool almost_equal(T x, T y, int ulp)
{	
	return std::abs(x - y) <= ulp || std::abs(x + y) <= ulp; 		
}

#define ULP 2
#define FLAGS_CANT_CHANGE_VAR (ARG_1_AND_2_ARE_WORDS | ARGS_ARE_XY_VALUES | ROUND_XY_TO_GRID | WE_HAVE_A_SCALE | MORE_COMPONENTS | WE_HAVE_AN_X_AND_Y_SCALE | WE_HAVE_A_TWO_BY_TWO | WE_HAVE_A_TWO_BY_TWO)

CheckCompositeResult CheckCompositeVariationCompatible(const short* pFirst, short firstSize, const short* pSecond, short secondSize) 
{
	TrueTypeCompositeComponent first, second;
	int i1 = 0, i2 = 0;

	if (pFirst == nullptr || pSecond == nullptr || firstSize != secondSize)
		return CheckCompositeResult::Fail;

	do {
		const unsigned short noChangeFlags = FLAGS_CANT_CHANGE_VAR; 
		first.flags = pFirst[i1++]; first.flags = SWAPW(first.flags);
		second.flags = pSecond[i2++]; second.flags = SWAPW(second.flags); 

		if ((first.flags & noChangeFlags) != (second.flags & noChangeFlags))
			return CheckCompositeResult::Fail;

		first.glyphIndex = pFirst[i1++]; first.glyphIndex = SWAPW(first.glyphIndex); 
		second.glyphIndex = pSecond[i2++]; second.glyphIndex = SWAPW(second.glyphIndex); 

		if (first.glyphIndex != second.glyphIndex)
			return CheckCompositeResult::Fail;

		if (first.flags & ARG_1_AND_2_ARE_WORDS)
		{
			first.arg1 = pFirst[i1++]; first.arg1 = SWAPW(first.arg1);
			second.arg1 = pSecond[i2++]; second.arg1 = SWAPW(second.arg1);

			first.arg2 = pFirst[i1++]; first.arg2 = SWAPW(first.arg2);
			second.arg2 = pSecond[i2++]; second.arg2 = SWAPW(second.arg2);

			if (first.arg1 != second.arg1 || first.arg2 != second.arg2)
				return CheckCompositeResult::Fail;
		}
		else
		{
			// both args are packed in a uint16 so treat the same since we only compare about comparison
			first.arg1 = pFirst[i1++];
			second.arg1 = pSecond[i2++];

			if (first.arg1 != second.arg1)
				return CheckCompositeResult::Fail;
		}

		if (first.flags & WE_HAVE_A_SCALE)
		{
			first.xscale = pFirst[i1++]; first.xscale = SWAPW(first.xscale);
			second.xscale = pSecond[i2++]; second.xscale = SWAPW(second.xscale); 

			if (first.xscale == second.xscale)
				return CheckCompositeResult::Success;
			else if (almost_equal(first.xscale, second.xscale, ULP))
				return CheckCompositeResult::Tolerance;
			else return CheckCompositeResult::Fail; 
		}
		else if (first.flags & WE_HAVE_AN_X_AND_Y_SCALE)
		{
			first.xscale = pFirst[i1++]; first.xscale = SWAPW(first.xscale);
			second.xscale = pSecond[i2++]; second.xscale = SWAPW(second.xscale);

			first.yscale = pFirst[i1++]; first.yscale = SWAPW(first.yscale);
			second.yscale = pSecond[i2++]; second.yscale = SWAPW(second.yscale);

			if (first.xscale == second.xscale && first.yscale == second.yscale)
				return CheckCompositeResult::Success;
			else if(almost_equal(first.xscale, second.xscale, ULP) && almost_equal(first.yscale, second.yscale, ULP))
				return CheckCompositeResult::Tolerance;
			else return CheckCompositeResult::Fail; 
		}
		else if (first.flags & WE_HAVE_A_TWO_BY_TWO)
		{
			first.xscale = pFirst[i1++]; first.xscale = SWAPW(first.xscale);
			second.xscale = pSecond[i2++]; second.xscale = SWAPW(second.xscale);

			first.scale01 = pFirst[i1++]; first.scale01 = SWAPW(first.scale01);
			second.scale01 = pSecond[i2++]; second.scale01 = SWAPW(second.scale01);

			first.scale10 = pFirst[i1++]; first.scale10 = SWAPW(first.scale10);
			second.scale10 = pSecond[i2++]; second.scale10 = SWAPW(second.scale10);

			first.yscale = pFirst[i1++]; first.yscale = SWAPW(first.yscale);
			second.yscale = pSecond[i2++]; second.yscale = SWAPW(second.yscale);

			if (first.xscale == second.xscale && first.yscale == second.yscale && first.scale01 == second.scale01 && first.scale10 == second.scale10)
				return CheckCompositeResult::Success;
			else if (almost_equal(first.xscale, second.xscale, ULP) && almost_equal(first.yscale, second.yscale, ULP) && almost_equal(first.scale01, second.scale01, ULP) && almost_equal(first.scale10, second.scale10, ULP))
				return CheckCompositeResult::Tolerance;
			else return CheckCompositeResult::Fail;			
		}

	} while (first.flags & MORE_COMPONENTS); 
	
	assert(i1 == i2);
	assert(i1 <= firstSize); 

	return CheckCompositeResult::Success; 
}

/* 
 * update the contenents in the glyph structure:  glyph->componentData
 * startOfLinePtr :  for error spot indication
 * context = CO_CompInstrFollow if more Composite line followed;
 * context = CO_StdInstrFollow if more instructions followed;
 * context = CO_NothingFollows otherwise;
 * round : boolean variable [r] or [R];
 * name  : instruction name.
 * args  : arguments storage
 * argc  : number of arguments.
 * return "true" if sucessful, otherwise return "false"
 */

void TrueTypeFont::UpdateCompositeProfile(TrueTypeGlyph *glyph, TTCompositeProfile *compositeProfile, short context, short RoundingCode, short InstructionIndex, short *args, short argc, sfnt_glyphbbox *Newbbox, short *error) {
	short i, tmpShort,flags = 0, contours, points;
	unsigned short glyphIndex;

	short ComponentDepth;
	sfnt_glyphbbox sub_bbox, tmpbbox;
	
	
	glyph->ComponentVersionNumber  = -1;  /* Magic number */
	
	if ( compositeProfile->nextExitOffset ) {
		*error = co_AnchorNothingAbove;
		return;
	}
	

	switch (InstructionIndex ) {
		case OVERLAPIndex:
			compositeProfile->GlobalNON_OVERLAPPING   = 0; 
			if ( context != CO_CompInstrFollow ) *error = co_OverlapLastInstruction;
			return;
			break;
		case NONOVERLAPIndex:
			compositeProfile->GlobalNON_OVERLAPPING   = 1; 
			if ( context != CO_CompInstrFollow ) *error = co_NonOverlapLastInstruction;
			return; 
			break;
		case USEMYMETRICSIndex:
			compositeProfile->GlobalUSEMYMETRICS   = 1; 
			if ( context != CO_CompInstrFollow ) *error = co_UseMymetricsLastInstruction;
			return; 
			break;
		case SCALEDCOMPONENTOFFSETIndex:
			compositeProfile->GlobalSCALEDCOMPONENTOFFSET   = 1; 
			if ( context != CO_CompInstrFollow ) *error = co_ScaledComponentOffsetLastInstruction;
			if ( compositeProfile->GlobalUNSCALEDCOMPONENTOFFSET ) *error = co_UnscaledComponentOffsetAlreadySet;
			return; 
			break;
		case UNSCALEDCOMPONENTOFFSETIndex:
			compositeProfile->GlobalUNSCALEDCOMPONENTOFFSET   = 1; 
			if ( context != CO_CompInstrFollow ) *error = co_UnscaledComponentOffsetLastInstruction;
			if ( compositeProfile->GlobalSCALEDCOMPONENTOFFSET ) *error = co_ScaledComponentOffsetAlreadySet;
			return; 
			break;
				
		case ANCHORIndex:
		case SANCHORIndex:
 			if (!this->GetNumberOfPointsAndContours(args[0], &contours, &points, &ComponentDepth,&sub_bbox)) {
 				*error = co_ComponentSizeOverflow;
				return;
 			}
			if ( args[1] > compositeProfile->anchorPrevPoints || args[2] > points ) {
				*error = co_AnchorArgExceedMax; 
				return;
			}
			break;
	}

	
	// if ( compositeProfile->GlobalNON_OVERLAPPING ) flags |= NON_OVERLAPPING; else flags &= (~NON_OVERLAPPING);
	// per Apple's and Microsoft's specs, and per GregH (27 Jan 2003): this flag should always be set to 0 (zero)
	flags &= (~NON_OVERLAPPING);
	if ( compositeProfile->GlobalUSEMYMETRICS )	flags |= USE_MY_METRICS; else flags &= (~USE_MY_METRICS);
	if ( compositeProfile->GlobalUSEMYMETRICS )	compositeProfile->GlobalUSEMYMETRICS = 0;
	if ( compositeProfile->GlobalSCALEDCOMPONENTOFFSET )	flags |= SCALED_COMPONENT_OFFSET;
	if ( compositeProfile->GlobalSCALEDCOMPONENTOFFSET )	compositeProfile->GlobalSCALEDCOMPONENTOFFSET = 0;
	if ( compositeProfile->GlobalUNSCALEDCOMPONENTOFFSET )	flags |= UNSCALED_COMPONENT_OFFSET;
	if ( compositeProfile->GlobalUNSCALEDCOMPONENTOFFSET )	compositeProfile->GlobalUNSCALEDCOMPONENTOFFSET = 0;

	if (InstructionIndex == OFFSETIndex || InstructionIndex == SOFFSETIndex)
	{
		flags |=   ARGS_ARE_XY_VALUES;
	 	for ( i = 1; i <= 2; i++ ) {  
			if ( args[i] < -128 || args[i] > 127 ) {
				flags |=  ARG_1_AND_2_ARE_WORDS;
				break;
			}
		}	
	} else {
	 	for ( i = 1; i <= 2; i++ ) {  
			if ( args[i] < 0 || args[i] > 255 ) {
				flags |=  ARG_1_AND_2_ARE_WORDS;
				break;
			}
		}	
 	}

	if ( RoundingCode )   flags |=   ROUND_XY_TO_GRID;

	
	
	if ( argc == 7 )  	   {
		/* the ConvertDoubleToShort conversion is done now during parsing */
		if ( args[4] == 0 && args[5] == 0) {
			if ( args[3] == args[6]  ) {
				flags |= WE_HAVE_A_SCALE; 
 			} else  {   
 				flags |= WE_HAVE_AN_X_AND_Y_SCALE; 
 			}
		} else {
			flags |=   WE_HAVE_A_TWO_BY_TWO;
		}
	}

  	if ( context == CO_CompInstrFollow )	flags |= MORE_COMPONENTS; else flags &= (~MORE_COMPONENTS);
	if ( context == CO_StdInstrFollow )	{
		 flags |=   WE_HAVE_INSTRUCTIONS;
		 compositeProfile->nextExitOffset = true;
	}
	
	if (compositeProfile->GlobalMORE_COMPONENTS)  {
		compositeProfile->numberOfCompositeElements++;
 		glyph->componentData[ glyph->componentSize++] = SWAPW(flags);

		glyphIndex = (unsigned short)args[0];
 		glyph->componentData[ glyph->componentSize++] = SWAPW(glyphIndex);
 		if (  flags & ARG_1_AND_2_ARE_WORDS ) {
 			/* arg 1 and 2 */
	 		glyph->componentData[ glyph->componentSize++] = SWAPW(args[1]);
	 		glyph->componentData[ glyph->componentSize++] = SWAPW(args[2]);
 		} else {
 			/* arg 1 and 2 as bytes */
			unsigned char *uByte = (unsigned char *)&glyph->componentData[glyph->componentSize];

			uByte[0] = (unsigned char)args[1];
			uByte[1] = (unsigned char)args[2];
 	 		glyph->componentSize++;
 		}
 		/* scale  */
 		if ( flags & WE_HAVE_A_SCALE ) {
   	 		glyph->componentData[ glyph->componentSize++] = SWAPW(args[3]);
 		} else if ( flags & WE_HAVE_AN_X_AND_Y_SCALE ) {
   	 		glyph->componentData[ glyph->componentSize++] = SWAPW(args[3]);
  	 		glyph->componentData[ glyph->componentSize++] = SWAPW(args[6]);
 		} else  if (  flags & WE_HAVE_A_TWO_BY_TWO ) {
 			/* xscale, scale01, scale10, yscale */
   	 		glyph->componentData[ glyph->componentSize++] =  SWAPW(args[3]);
  	 		glyph->componentData[ glyph->componentSize++] =  SWAPW(args[4]);
   	 		glyph->componentData[ glyph->componentSize++] =  SWAPW(args[5]);
  	 		glyph->componentData[ glyph->componentSize++] =  SWAPW(args[6]);
 		}
 		if ( glyph->componentSize >= MAXCOMPONENTSIZE ) {
			*error = co_ComponentSizeOverflow; 
			return;
 		}

		this->GetNumberOfPointsAndContours(glyphIndex, &contours, &points, &ComponentDepth,&sub_bbox );
		/* claudebe, update the bounding box, inspired by sfnaccs.c, ReadComponentData */

		if (flags & (WE_HAVE_A_SCALE | WE_HAVE_AN_X_AND_Y_SCALE | WE_HAVE_A_TWO_BY_TWO))
		{
			if (flags & WE_HAVE_A_TWO_BY_TWO)
			{
			
				if ( args[3] == 0 && args[6] == 0) { /* we have a 90degree rotation, we can compute the new bbox just using the old one */

					tmpbbox = sub_bbox;
					sub_bbox.xmin = (short) ( ((int32_t)tmpbbox.ymin * (int32_t)args[5]) >> 14);
					sub_bbox.xmax = (short) ( ((int32_t)tmpbbox.ymax * (int32_t)args[5]) >> 14);
					sub_bbox.ymin = (short) ( ((int32_t)tmpbbox.xmin * (int32_t)args[4]) >> 14);
					sub_bbox.ymax = (short) ( ((int32_t)tmpbbox.xmax * (int32_t)args[4]) >> 14);
				
					if (sub_bbox.xmin > sub_bbox.xmax) { /* there was a negative scaling */
						tmpShort = sub_bbox.xmin;
						sub_bbox.xmin = sub_bbox.xmax;
						sub_bbox.xmax = tmpShort;
					}
					if (sub_bbox.ymin > sub_bbox.ymax) { /* there was a negative scaling */
						tmpShort = sub_bbox.ymin;
						sub_bbox.ymin = sub_bbox.ymax;
						sub_bbox.ymax = tmpShort;
					}
									
					/* first scale, then apply the offset */
					sub_bbox.xmin += args[1];
					sub_bbox.xmax += args[1];
					sub_bbox.ymin += args[2];
					sub_bbox.ymax += args[2];

					Newbbox->xmin = Min(Newbbox->xmin,sub_bbox.xmin);
					Newbbox->ymin = Min(Newbbox->ymin,sub_bbox.ymin);
					Newbbox->xmax = Max(Newbbox->xmax,sub_bbox.xmax);
					Newbbox->ymax = Max(Newbbox->ymax,sub_bbox.ymax);

				} else {
//					edit_ErrorMessage( L"Warning, DovMan doesn't know how to compute the glyph bounding box when rotating a glyph", glyph->componentSize );
				}

			}
			else
			{
				/* If we have a scale factor   */

				
				if (flags & WE_HAVE_AN_X_AND_Y_SCALE)
				{
					sub_bbox.xmin = (short) ( ((int32_t)sub_bbox.xmin * (int32_t)args[3]) >> 14);
					sub_bbox.xmax = (short) ( ((int32_t)sub_bbox.xmax * (int32_t)args[3]) >> 14);
					sub_bbox.ymin = (short) ( ((int32_t)sub_bbox.ymin * (int32_t)args[6]) >> 14);
					sub_bbox.ymax = (short) ( ((int32_t)sub_bbox.ymax * (int32_t)args[6]) >> 14);
				}
				else
				{
					sub_bbox.xmin = (short) ( ((int32_t)sub_bbox.xmin * (int32_t)args[3]) >> 14);
					sub_bbox.xmax = (short) ( ((int32_t)sub_bbox.xmax * (int32_t)args[3]) >> 14);
					sub_bbox.ymin = (short) ( ((int32_t)sub_bbox.ymin * (int32_t)args[3]) >> 14);
					sub_bbox.ymax = (short) ( ((int32_t)sub_bbox.ymax * (int32_t)args[3]) >> 14);
				}
				
				if (sub_bbox.xmin > sub_bbox.xmax) { /* there was a negative scaling */
					tmpShort = sub_bbox.xmin;
					sub_bbox.xmin = sub_bbox.xmax;
					sub_bbox.xmax = tmpShort;
				}
				if (sub_bbox.ymin > sub_bbox.ymax) { /* there was a negative scaling */
					tmpShort = sub_bbox.ymin;
					sub_bbox.ymin = sub_bbox.ymax;
					sub_bbox.ymax = tmpShort;
				}
									
				/* first scale, then apply the offset */
				sub_bbox.xmin += args[1];
				sub_bbox.xmax += args[1];
				sub_bbox.ymin += args[2];
				sub_bbox.ymax += args[2];

				Newbbox->xmin = Min(Newbbox->xmin,sub_bbox.xmin);
				Newbbox->ymin = Min(Newbbox->ymin,sub_bbox.ymin);
				Newbbox->xmax = Max(Newbbox->xmax,sub_bbox.xmax);
				Newbbox->ymax = Max(Newbbox->ymax,sub_bbox.ymax);
			}
		} else {

			if (flags & ARGS_ARE_XY_VALUES)
			{
			/* I don't neet to test ARG_1_AND_2_ARE_WORDS as I can get the arguments directely from args[] */
				sub_bbox.xmin += args[1];
				sub_bbox.xmax += args[1];
				sub_bbox.ymin += args[2];
				sub_bbox.ymax += args[2];
				
				Newbbox->xmin = Min(Newbbox->xmin,sub_bbox.xmin);
				Newbbox->ymin = Min(Newbbox->ymin,sub_bbox.ymin);
				Newbbox->xmax = Max(Newbbox->xmax,sub_bbox.xmax);
				Newbbox->ymax = Max(Newbbox->ymax,sub_bbox.ymax);
			}
			else
			{
				/* ClaudeBe, I'm not handling the anchor point case now */
			sub_bbox.xmin = sub_bbox.xmin; /* to set a break point */
			}
		}
		
		compositeProfile->anchorPrevPoints += points;	
		compositeProfile->numberOfCompositeContours += contours;
		compositeProfile->numberOfCompositePoints   += points;
		if ( compositeProfile->numberOfCompositeContours > this->profile.maxCompositeContours ) {
			this->profile.maxCompositeContours  = compositeProfile->numberOfCompositeContours;
		}
		if ( compositeProfile->numberOfCompositeContours > this->newProfile.maxCompositeContours ) {
			this->newProfile.maxCompositeContours  = compositeProfile->numberOfCompositeContours;
		}
		if ( compositeProfile->numberOfCompositePoints > this->profile.maxCompositePoints ) {
			this->profile.maxCompositePoints  = compositeProfile->numberOfCompositePoints;
		}
		if ( compositeProfile->numberOfCompositePoints > this->newProfile.maxCompositePoints ) {
			this->newProfile.maxCompositePoints  = compositeProfile->numberOfCompositePoints;
		}
		if ( compositeProfile->numberOfCompositeElements > this->profile.maxComponentElements ) {
			this->profile.maxComponentElements  = compositeProfile->numberOfCompositeElements;
		}
		if ( compositeProfile->numberOfCompositeElements > this->newProfile.maxComponentElements ) {
			this->newProfile.maxComponentElements  = compositeProfile->numberOfCompositeElements;
		}
		if ( ComponentDepth+1 > this->profile.maxComponentDepth ) {
			this->profile.maxComponentDepth  = ComponentDepth+1;
		}
		if ( ComponentDepth+1 > this->newProfile.maxComponentDepth ) {
			this->newProfile.maxComponentDepth  = ComponentDepth+1;
		}
		
	}
	compositeProfile->GlobalMORE_COMPONENTS = (flags & MORE_COMPONENTS );
		
	return;
} // TrueTypeFont::UpdateCompositeProfile

void TrueTypeFont::CalculateNewCheckSums(void) {
// for each table in font, calculate table's checksum
	unsigned char *sfnt = this->sfntHandle;
	sfnt_OffsetTable *directory;
	sfnt_DirectoryEntry *entry;
	unsigned short numTables;
	uint32_t *loopStart,checkSum,*loopEnd,*loop,i, length;
	sfnt_FontHeader	*head = (sfnt_FontHeader *)this->GetTablePointer(tag_FontHeader);
	
	head->checkSumAdjustment = 0; // checksum of head table to be computed with this set to 0
	directory = (sfnt_OffsetTable*)sfnt;
	numTables = SWAPW(directory->numOffsets);
	for (i = 0; i < numTables; i++) {
		entry = &directory->table[i];
		loopStart = (uint32_t *)(sfnt + SWAPL(entry->offset));
		length = SWAPL(entry->length);
		loopEnd = loopStart + DWordPad(length)/sizeof(uint32_t);
		checkSum = 0;
		for (loop = loopStart; loop < loopEnd; loop++)
		{
			checkSum += SWAPL(*loop);
		};
		entry->checkSum = SWAPL(checkSum);
	}
} // TrueTypeFont::CalculateNewCheckSums

void TrueTypeFont::CalculateCheckSumAdjustment(void) {

	unsigned char *sfnt = this->sfntHandle;
	sfnt_FontHeader	*head = (sfnt_FontHeader *)this->GetTablePointer(tag_FontHeader);
	uint32_t *loopStart,checkSum,*loopEnd,*loop;

	loopStart = (uint32_t *)sfnt;
	loopEnd = loopStart + DWordPad(this->sfntSize)/sizeof(uint32_t);
	for (loop = loopStart, checkSum = 0; loop < loopEnd; loop++)
	{
		checkSum += SWAPL(*loop);
	}
	head->checkSumAdjustment = CSWAPL(0xb1b0afba - checkSum);
} // TrueTypeFont::CalculateCheckSumAdjustment

void TrueTypeFont::SortTableDirectory(void)	{

	sfnt_OffsetTable *directory = (sfnt_OffsetTable *)this->sfntHandle;
	sfnt_DirectoryEntry *entry = directory->table,temp;
	unsigned short numTables = SWAPW(directory->numOffsets);
	int32_t i,j;
	bool	swap;
	
	swap = true;
	for (i = numTables - 1; i > 0 && swap; i--) { // Bubble-Sort
		swap = false;
		for (j = 0; j < i; j++) {
			if (SWAPL(entry[j].tag) > SWAPL(entry[j+1].tag)) {
				swap = true;
				temp = entry[j];
				entry[j] = entry[j+1];
				entry[j+1] = temp;
			}
		}
	}
} // TrueTypeFont::SortTableDirectory

void TrueTypeFont::PackMaxpHeadHhea(void) {
	sfnt_FontHeader *head;
	sfnt_HorizontalHeader *hhea;

	PackMaxp(this->GetTablePointer(tag_MaxProfile),&this->profile);
	
	head = (sfnt_FontHeader *)this->GetTablePointer(tag_FontHeader);
	head->macStyle = SWAPW(this->macStyle);

	head->xMin = SWAPW(this->metricProfile.xMin);
	head->yMin = SWAPW(this->metricProfile.yMin);
	head->xMax = SWAPW(this->metricProfile.xMax);
	head->yMax = SWAPW(this->metricProfile.yMax);

	hhea = (sfnt_HorizontalHeader *)this->GetTablePointer(tag_HoriHeader);
	
	hhea->advanceWidthMax	  = SWAPW(this->metricProfile.advanceWidthMax);
	hhea->minLeftSideBearing  = SWAPW(this->metricProfile.minLeftSideBearing);
	hhea->minRightSideBearing = SWAPW(this->metricProfile.minRightSideBearing);
	hhea->xMaxExtent		  = SWAPW(this->metricProfile.xMaxExtent);
} // TrueTypeFont::PackMaxpHeadHhea

uint32_t TrueTypeFont::GetPackedGlyphsSizeEstimate(TrueTypeGlyph *glyph, int32_t glyphIndex, uint32_t *oldIndexToLoc) {
//	for the purpose of asserting a big enough buffer (this->sfntHandle) it should be enough to determine a
//	reasonnable upper limit. Between two invocations of BuildNewSfnt, at most one glyph is changed. The
//	size of this change is calculated exactly. For all other glyphs we use the current size (difference
//	of position in oldIndexToLoca), regardless of the fact that the font may actually be stripped of its
//	code in the process of building the new sfnt
	
	uint32_t length,size;
	int32_t i,numEntries; //numContours;
	
	numEntries = this->NumberOfGlyphs();

	size = 0;
	for (i = 0; i < numEntries; i++) {
		if (i == glyphIndex) {
			length = this->GetPackedGlyphSize(glyphIndex,glyph,this->binSize[asmGLYF]);
		} else {
 		  	length = oldIndexToLoc[i+1] - oldIndexToLoc[i];
		}

		size += length;
		if (size & 1) size++; // zero-pad word align
 	}
	return size;
} // TrueTypeFont::GetPackedGlyphsSizeEstimate

uint32_t TrueTypeFont::GetPackedGlyphSize(int32_t glyphIndex, TrueTypeGlyph *glyph, int32_t glyfBinSize) {
	uint32_t size;
	short i, numberOfPoints, x, y, delta, j;
	unsigned char bitFlags;
	
	size = 0;
	
	if (glyph->numContoursInGlyph == 0 || (glyph->numContoursInGlyph == 1 && glyph->startPoint[0] == 0 && glyph->endPoint[0] == 0 && glyph->x[0] == 0 && glyph->y[0] == 0 && !glyph->onCurve[0] && glyfBinSize == 0)) {
		return size; // assume this is a dummy point that is neither instructed nor serves as an anchor, hence turn into an empty glyph by leaving with size == 0...
	}
	
	// by now, we have at least one control point
	numberOfPoints = glyph->endPoint[glyph->numContoursInGlyph - 1] + 1; // excluding the side bearing points
	
	if ( glyph->componentSize > 0 ) {

		size += 5*sizeof(short) + sizeof(short)*glyph->componentSize; // component version number, bbox
		if (glyfBinSize > 0) size += sizeof(short) + glyfBinSize; // glyfBinSize

	} else {

		size += 5*sizeof(short) + glyph->numContoursInGlyph*sizeof(short) + sizeof(short) + glyfBinSize; // number of contours, bbox, glyfBinSize
	
		// Calculate flags
		for ( x = y = i = 0; i < numberOfPoints; i++ ) {
			bitFlags = glyph->onCurve[i] & ONCURVE;
			delta = glyph->x[i] - x; x = glyph->x[i];
			if ( delta == 0 ) {
				bitFlags |= NEXT_X_IS_ZERO;
			} else if ( delta >= -255 && delta <= 255 ) {
				bitFlags |= XSHORT;
				if ( delta > 0 )
					bitFlags |= SHORT_X_IS_POS;
			}
	
			delta = glyph->y[i] - y; y = glyph->y[i];
			if ( delta == 0 ) {
				bitFlags |= NEXT_Y_IS_ZERO;
			} else if ( delta >= -255 && delta <= 255 ) {
				bitFlags |= YSHORT;
				if ( delta > 0 )
					bitFlags |= SHORT_Y_IS_POS;
			}
			this->tmpFlags[i] = bitFlags;
		}
	
		// Write out bitFlags
		for ( i = 0; i < numberOfPoints;) {
			unsigned char repeat = 0;
			for (j = i+1; j < numberOfPoints && this->tmpFlags[i] == this->tmpFlags[j] && repeat < 255; j++)
				repeat++;
			if (repeat > 1) {
				size += 2;
				i = j;
			} else {
				size++;
				i++;
			}
		}
	
		// Write out X
		for (x = i = 0; i < numberOfPoints; i++) {
			delta = glyph->x[i] - x; x = glyph->x[i];
			if (this->tmpFlags[i] & XSHORT) {
				if (!(this->tmpFlags[i] & SHORT_X_IS_POS)) delta = -delta;
				size++;
			} else if (!(this->tmpFlags[i] & NEXT_X_IS_ZERO))
				size += 2;
		}
	
		// Write out Y
		for (y = i = 0; i < numberOfPoints; i++) {
			delta = glyph->y[i] - y; y = glyph->y[i];
			if (this->tmpFlags[i] & YSHORT) {
				if (!(this->tmpFlags[i] & SHORT_Y_IS_POS)) delta = -delta;
				size++;
			} else if (!(this->tmpFlags[i] & NEXT_Y_IS_ZERO))
				size += 2;
		}
	}
	
	return size; // by now
} // TrueTypeFont::GetPackedGlyphSize

uint32_t TrueTypeFont::PackGlyphs(StripCommand strip, TrueTypeGlyph *glyph, int32_t glyphIndex, uint32_t *oldIndexToLoc, uint32_t *newIndexToLoc, unsigned char *dst) {
	uint32_t length,size;
	int32_t i,numEntries; //numContours;
	unsigned char *src;	
	
	numEntries = this->NumberOfGlyphs();

	src = this->GetTablePointer(tag_GlyphData);
 
	size = 0;
	for (i = 0; i < numEntries; i++) {
  		newIndexToLoc[i] = size;
 		
		if (i == glyphIndex) {
			length = this->PackGlyph(&dst[size],glyphIndex,glyph,this->binSize[asmGLYF],this->binData[asmGLYF],&this->horMetric[glyphIndex]);
		} else {
 		  	length = oldIndexToLoc[i+1] - oldIndexToLoc[i];
			if (length) {
			if (strip < stripHints)
 		  			memcpy(&dst[size],&src[oldIndexToLoc[i]],length);
 		  		else
 		  			length = this->StripGlyphBinary(&dst[size],&src[oldIndexToLoc[i]],length);
			}
		}
		
		size += length;
		if (size & 1) dst[(size)++] = 0; // zero-pad word align
 	}
	newIndexToLoc[i] = size;
 	
	this->outShortIndexToLocTable = size < 65535*2;
	
	return size;
} // TrueTypeFont::PackGlyphs

uint32_t TrueTypeFont::PackGlyph(unsigned char *dst, int32_t glyphIndex, TrueTypeGlyph *glyph, int32_t glyfBinSize, unsigned char *glyfInstruction, sfnt_HorizontalMetrics *hmtx) {
	unsigned char *pStart;
	uint32_t size;
	short i, numberOfPoints, x, y, delta, j, whoseMetrics;
	unsigned char bitFlags;
	bool composite,useMyMetrics;
	
	size = 0;
	
//	if (glyph->numContoursInGlyph == 0) { // fake empty glyph with 1 knot; must be off-curve or else we don't get back the side-bearing points from the apple rasterizer...
//		for (i = PHANTOMPOINTS; i > 0; i--) {
//			glyph->x[i] = glyph->x[i-1];
//			glyph->y[i] = glyph->y[i-1];
//			glyph->onCurve[i] = glyph->onCurve[i-1];
//		}
//		glyph->startPoint[0] = glyph->endPoint[0] = glyph->x[0] = glyph->y[0] = glyph->onCurve[0] = false;
//		glyph->numContoursInGlyph++;
//	}

	if (glyph->numContoursInGlyph == 0 || (glyph->numContoursInGlyph == 1 && glyph->startPoint[0] == 0 && glyph->endPoint[0] == 0 && glyph->x[0] == 0 && glyph->y[0] == 0 && !glyph->onCurve[0] && glyfBinSize == 0)) {
		hmtx->leftSideBearing = 0;
		hmtx->advanceWidth = glyph->x[glyph->numContoursInGlyph + 1] - glyph->x[glyph->numContoursInGlyph];
		return 0; // assume this is a dummy point that is neither instructed nor serves as an anchor, hence turn into an empty glyph by leaving with size == 0...
	}
	
	// by now, we have at least one control point
	numberOfPoints = glyph->endPoint[glyph->numContoursInGlyph - 1] + 1; // excluding the side bearing points
	
	pStart = dst;
	
	composite = useMyMetrics = false;
	if ( glyph->componentSize > 0 ) {
		int32_t len = 0;
		short * flags = nullptr;	

		composite = true;
		
		WRITEALIGNEDWORD( dst, SWAPW(glyph->ComponentVersionNumber) );
		
		/* bbox */
		WRITEALIGNEDWORD( dst, SWAPW(glyph->xmin) );
		WRITEALIGNEDWORD( dst, SWAPW(glyph->ymin) );
		WRITEALIGNEDWORD( dst, SWAPW(glyph->xmax) );
		WRITEALIGNEDWORD( dst, SWAPW(glyph->ymax) );

		i = 0;
		do {
			flags = &glyph->componentData[i++];
			if (*flags & CSWAPW(USE_MY_METRICS)) {
				useMyMetrics = true;
				whoseMetrics = glyph->componentData[i++];

				whoseMetrics  = SWAPW(whoseMetrics);
			} else
				i++; // skip glyphIndex
			if (*flags & CSWAPW(ARG_1_AND_2_ARE_WORDS)) i+=2; else i++;

			if (*flags & CSWAPW(WE_HAVE_A_TWO_BY_TWO)) i+=4;
			else if (*flags & CSWAPW(WE_HAVE_A_SCALE)) i++;
			else if (*flags & CSWAPW(WE_HAVE_AN_X_AND_Y_SCALE)) i+=2;
		} while (*flags & CSWAPW(MORE_COMPONENTS));
		
		// glyfBinSize may be binary of plain TrueType code or binary of instructions added to composites
		// for the purpose of stripping instructions, glyfBinSize is set to 0, after which the flag in the flags
		// byte (that is interspersed in the composite code) is inconsistent, hence the above "parsing" and the check below...
		if  (*flags & CSWAPW(WE_HAVE_INSTRUCTIONS) && glyfBinSize == 0) *flags &= ~CSWAPW(WE_HAVE_INSTRUCTIONS);

		len = glyph->componentSize + glyph->componentSize;
		memcpy( (char *) dst,(char *)glyph->componentData, len );
		
		dst += len;
		if (glyfBinSize > 0) {
			
			WRITEALIGNEDWORD( dst, (short)SWAPW(glyfBinSize) );
			memcpy((char*)dst,(char*)glyfInstruction,glyfBinSize); // this is already big-endian
			dst += glyfBinSize;
		}
	} else {
		short xmin,ymin,xmax,ymax;
		
		/* bbox and sidebearings */
		xmin = 0x7fff;
		ymin = 0x7fff;
		xmax = (short)0x8000;
		ymax = (short)0x8000;
		for ( i = 0; i < numberOfPoints; i++ ) {
			xmin = Min(xmin,glyph->x[i]);
			xmax = Max(xmax,glyph->x[i]);
			ymin = Min(ymin,glyph->y[i]);
			ymax = Max(ymax,glyph->y[i]);
		}
		glyph->xmin = xmin;
		glyph->ymin = ymin;
		glyph->xmax = xmax;
		glyph->ymax = ymax;
		
		WRITEALIGNEDWORD( dst, SWAPW(glyph->numContoursInGlyph) );
		
		WRITEALIGNEDWORD( dst, SWAPW(glyph->xmin) );
		WRITEALIGNEDWORD( dst, SWAPW(glyph->ymin) );
		WRITEALIGNEDWORD( dst, SWAPW(glyph->xmax) );
		WRITEALIGNEDWORD( dst, SWAPW(glyph->ymax) );
	
		for ( i = 0; i < glyph->numContoursInGlyph; i++ ) {
			WRITEALIGNEDWORD( dst, SWAPW(glyph->endPoint[i]) );
		}
	
		WRITEALIGNEDWORD( dst, (short)SWAPW(glyfBinSize) );
		memcpy((char*)dst,(char*)glyfInstruction,glyfBinSize);
		dst += glyfBinSize;
		
		/* Calculate flags */
		for ( x = y = i = 0; i < numberOfPoints; i++ ) {
			bitFlags = glyph->onCurve[i] & ONCURVE;
			delta = glyph->x[i] - x; x = glyph->x[i];
			if ( delta == 0 ) {
				bitFlags |= NEXT_X_IS_ZERO;
			} else if ( delta >= -255 && delta <= 255 ) {
				bitFlags |= XSHORT;
				if ( delta > 0 )
					bitFlags |= SHORT_X_IS_POS;
			}
	
			delta = glyph->y[i] - y; y = glyph->y[i];
			if ( delta == 0 ) {
				bitFlags |= NEXT_Y_IS_ZERO;
			} else if ( delta >= -255 && delta <= 255 ) {
				bitFlags |= YSHORT;
				if ( delta > 0 )
					bitFlags |= SHORT_Y_IS_POS;
			}
			this->tmpFlags[i] = bitFlags;
		}
	
		/* Write out bitFlags */
		for ( i = 0; i < numberOfPoints;) {
			unsigned char repeat = 0;
			for ( j = i+1; j < numberOfPoints && this->tmpFlags[i] == this->tmpFlags[j] && repeat < 255; j++ )
				repeat++;
			if ( repeat > 1 ) {  
				*dst++ = this->tmpFlags[i] | REPEAT_FLAGS;
				*dst++ = repeat;
				i = j;
			} else {
				*dst++ = this->tmpFlags[i++];
			}
		}
	
		/* Write out X */
		for ( x = i = 0; i < numberOfPoints; i++ ) {
			delta = glyph->x[i] - x; x = glyph->x[i];
			if ( this->tmpFlags[i] & XSHORT ) {
				if ( !(this->tmpFlags[i] & SHORT_X_IS_POS) ) delta = -delta;
				*dst++ = (unsigned char) delta;
			} else if ( !(this->tmpFlags[i] & NEXT_X_IS_ZERO ) )
				WRITENONALIGNEDWORD( dst, delta );
		}
	
		/* Write out Y */
		for ( y = i = 0; i < numberOfPoints; i++ ) {
			delta = glyph->y[i] - y; y = glyph->y[i];
			if ( this->tmpFlags[i] & YSHORT ) {
				if ( !(this->tmpFlags[i] & SHORT_Y_IS_POS) ) delta = -delta;
				*dst++ = (unsigned char) delta;
			} else if ( !(this->tmpFlags[i] & NEXT_Y_IS_ZERO ) )
				WRITENONALIGNEDWORD( dst, delta );
		}
	}
	
	hmtx->leftSideBearing = glyph->xmin - glyph->x[numberOfPoints]; // some rather oldish fonts have x[LSB] != 0
	hmtx->advanceWidth = useMyMetrics ? this->horMetric[whoseMetrics].advanceWidth : glyph->x[numberOfPoints + 1] - glyph->x[numberOfPoints];

	size = (short)(ptrdiff_t)(dst - pStart);
	return size;
} // TrueTypeFont::PackGlyph

uint32_t TrueTypeFont::StripGlyphBinary(unsigned char *dst, unsigned char *src, uint32_t srcLen) {
	unsigned char *glyfPos, *flagPos;
	short numContours,i;
	unsigned short flags, binLen;
	
	if (!srcLen) return 0; // nothing else we could squeeze out of it...
	
	glyfPos = src;
	
	numContours = COPYALIGNEDWORD(dst,src);

	numContours = SWAPW(numContours);
	COPYALIGNEDWORD(dst,src);
	COPYALIGNEDWORD(dst,src);
	COPYALIGNEDWORD(dst,src);
	COPYALIGNEDWORD(dst,src); // skip bounding box
	
	if (numContours < 0) {
 		binLen = 0; // assume so far
 		do {
 			flagPos = dst;
 			flags = COPYALIGNEDWORD(dst,src);

			flags = SWAPW(flags);
 			i = 1; // skip glyphIndex
 			if (flags & ARG_1_AND_2_ARE_WORDS) i += 2; else i++;
 			if (flags & WE_HAVE_A_SCALE) i++;
 			else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) i += 2;
 			else if (flags & WE_HAVE_A_TWO_BY_TWO) i += 4;
 			while (i > 0) {
				COPYALIGNEDWORD(dst,src); i--; 
			}
 		} while (flags & MORE_COMPONENTS);

 		if (flags & WE_HAVE_INSTRUCTIONS) {
 			binLen = READALIGNEDWORD(src);

			binLen = SWAPW(binLen);
 			flags &= ~WE_HAVE_INSTRUCTIONS;
 			WRITEALIGNEDWORD(flagPos,SWAPW(flags)); // reset flag, cf. PackGlyph
 		}
 	} else {
		while (numContours > 0) { COPYALIGNEDWORD(dst,src); numContours--; } // skip end points array
		binLen = READALIGNEDWORD(src); 

		binLen = SWAPW(binLen);

		WRITEALIGNEDWORD(dst,0);
		memcpy((char*)dst,(char*)(src + binLen),glyfPos + srcLen - src - binLen);
	}
	return srcLen - binLen;
} // TrueTypeFont::StripGlyphBinary

uint32_t TrueTypeFont::GetPackedGlyphSourceSize(TextBuffer *glyfText, TextBuffer *prepText, TextBuffer *cvtText, TextBuffer *talkText, TextBuffer *fpgmText,
							  				short type, int32_t glyphIndex, int32_t glitIndex, sfnt_MemDataEntry *memGlit) {
	uint32_t size;
	
	size = 0;
	
	if (type == 1) {
		if		(memGlit[glitIndex].glyphCode == PRE_PGM_GLYPH_INDEX)  size = prepText->TheLengthInUTF8();
		else if (memGlit[glitIndex].glyphCode == CVT_GLYPH_INDEX)	   size = cvtText ->TheLengthInUTF8();
		else if (memGlit[glitIndex].glyphCode == HL_GLYPH_INDEX)	   size = 0;
		else if (memGlit[glitIndex].glyphCode == FONT_PGM_GLYPH_INDEX) size = fpgmText->TheLengthInUTF8();
		else if (memGlit[glitIndex].glyphCode == glyphIndex)		   size = glyfText->TheLengthInUTF8();
		else if (glitIndex < this->maxGlyphs)                          size = memGlit[glitIndex].length;
	} else { // type == 2
		if		(memGlit[glitIndex].glyphCode == glyphIndex)		   size = talkText->TheLengthInUTF8();
		else if (glitIndex < this->maxGlyphs)                          size = memGlit[glitIndex].length;
	}
	
	if (size & 1) size++; // word align

	return size;
} // TrueTypeFont::GetPackedGlyphSourceSize

uint32_t TrueTypeFont::GetPackedGlyphSourcesSize(TextBuffer *glyfText, TextBuffer *prepText, TextBuffer *cvtText, TextBuffer *talkText, TextBuffer *fpgmText,
							   				 short type, int32_t glyphIndex, sfnt_MemDataEntry *memGlit) {
	uint32_t size;
	int32_t glitIndex;
	
	size = 0;
	
	for (glitIndex = 0; glitIndex < this->maxGlyphs; glitIndex++)
		size += this->GetPackedGlyphSourceSize(glyfText,prepText,cvtText,talkText,fpgmText,type,glyphIndex,glitIndex,memGlit);
	
	size += this->GetPackedGlyphSourceSize(glyfText,prepText,cvtText,talkText,fpgmText,type,PRE_PGM_GLYPH_INDEX, this->maxGlyphs+1,memGlit);
	size += this->GetPackedGlyphSourceSize(glyfText,prepText,cvtText,talkText,fpgmText,type,CVT_GLYPH_INDEX,     this->maxGlyphs+2,memGlit);
	size += this->GetPackedGlyphSourceSize(glyfText,prepText,cvtText,talkText,fpgmText,type,HL_GLYPH_INDEX,      this->maxGlyphs+3,memGlit);
	size += this->GetPackedGlyphSourceSize(glyfText,prepText,cvtText,talkText,fpgmText,type,FONT_PGM_GLYPH_INDEX,this->maxGlyphs+4,memGlit);

	return size;
} // TrueTypeFont::GetPackedGlyphSourcesSize

void TrueTypeFont::PackGlyphSource(TextBuffer *glyfText, TextBuffer *prepText, TextBuffer *cvtText, TextBuffer *talkText, TextBuffer *fpgmText,
							  		short type, int32_t glyphIndex, int32_t glitIndex, sfnt_FileDataEntry *fileGlit, sfnt_MemDataEntry *memGlit,
									uint32_t *dstPos, unsigned char *dst) {

//	B.St.: Notice that PackGlyph, PackGlyphs and probably various such methods do not check against overflowing the sfnt being built in a timely manner,
//	i.e. not until it is too late in BuildNewSfnt. This needs some improvements I didn't get around to doing when re-engineering TypeMan/DovMan/TTED.
	
	if (type == 1) {
		if		(memGlit[glitIndex].glyphCode == PRE_PGM_GLYPH_INDEX)  prepText->GetText((size_t*)&memGlit[glitIndex].length,(char *)&dst[*dstPos]);
		else if (memGlit[glitIndex].glyphCode == CVT_GLYPH_INDEX)	   cvtText ->GetText((size_t*)&memGlit[glitIndex].length,(char *)&dst[*dstPos]);
		else if (memGlit[glitIndex].glyphCode == HL_GLYPH_INDEX)	   memGlit[glitIndex].length = 0;
		else if (memGlit[glitIndex].glyphCode == FONT_PGM_GLYPH_INDEX) fpgmText->GetText((size_t*)&memGlit[glitIndex].length,(char *)&dst[*dstPos]);
		else if (memGlit[glitIndex].glyphCode == glyphIndex)		   glyfText->GetText((size_t*)&memGlit[glitIndex].length,(char *)&dst[*dstPos]);
		else if (glitIndex < this->maxGlyphs) memcpy((char *)&dst[*dstPos], (char *)(this->GetTablePointer(PRIVATE_PGM1) + memGlit[glitIndex].offset), memGlit[glitIndex].length); // fetch binary
	} else { // type == 2
		if		(memGlit[glitIndex].glyphCode == glyphIndex)		   talkText->GetText((size_t*)&memGlit[glitIndex].length,(char *)&dst[*dstPos]);
		else if (glitIndex < this->maxGlyphs) memcpy((char *)&dst[*dstPos], (char *)(this->GetTablePointer(PRIVATE_PGM2) + memGlit[glitIndex].offset), memGlit[glitIndex].length);
	}
	fileGlit[glitIndex].offset = SWAPL(*dstPos);

	{
		int32_t length = memGlit[glitIndex].length > 0x8000 ? 0x8000 : (unsigned short)memGlit[glitIndex].length;
		fileGlit[glitIndex].length = SWAPW(length);
	}
	*dstPos += memGlit[glitIndex].length;
	if (*dstPos & 1) dst[(*dstPos)++] = '\x0D'; // CR-pad (in case length is derived from two consecutive (even!) offsets) word align
} // TrueTypeFont::PackGlyphSource

void TrueTypeFont::PackGlyphSources(TextBuffer *glyfText, TextBuffer *prepText, TextBuffer *cvtText, TextBuffer *talkText, TextBuffer *fpgmText,
							   		short type, int32_t glyphIndex, sfnt_FileDataEntry *fileGlit, sfnt_MemDataEntry *memGlit,
									uint32_t *dstLen, unsigned char *dst) {
	int32_t glitIndex;
	
	*dstLen = 0;
	for (glitIndex = 0; glitIndex < this->maxGlyphs; glitIndex++)
	{
		this->PackGlyphSource(glyfText,prepText,cvtText,talkText,fpgmText,type,glyphIndex,glitIndex,fileGlit,memGlit,dstLen,dst);
	}
	this->PackGlyphSource(glyfText,prepText,cvtText,talkText,fpgmText,type,PRE_PGM_GLYPH_INDEX, this->maxGlyphs+1,fileGlit,memGlit,dstLen,dst);
	this->PackGlyphSource(glyfText,prepText,cvtText,talkText,fpgmText,type,CVT_GLYPH_INDEX,	 this->maxGlyphs+2,fileGlit,memGlit,dstLen,dst);
	this->PackGlyphSource(glyfText,prepText,cvtText,talkText,fpgmText,type,HL_GLYPH_INDEX,		 this->maxGlyphs+3,fileGlit,memGlit,dstLen,dst);
	this->PackGlyphSource(glyfText,prepText,cvtText,talkText,fpgmText,type,FONT_PGM_GLYPH_INDEX,this->maxGlyphs+4,fileGlit,memGlit,dstLen,dst);
} // TrueTypeFont::PackGlyphSources

/*
 * retuns contours and points
 */
bool TrueTypeFont::SubGetNumberOfPointsAndContours(int32_t glyphIndex, short *contours, short *points, short *ComponentDepth, sfnt_glyphbbox *bbox) {
	short i, cont;
	unsigned char *sp = 0 <= glyphIndex && glyphIndex < this->numLocaEntries ? (unsigned char *)this->GetTablePointer(tag_GlyphData) + this->IndexToLoc[glyphIndex] : NULL;
	short tmp;
 	unsigned short flags;
 	sfnt_glyphbbox Sub_bbox;
 	short NbOfPoints = 0;
 	
	if (sp) {
		cont = READALIGNEDWORD( sp );

		cont = SWAPW(cont);
		
		bbox->xmin = READALIGNEDWORD( sp );

		bbox->xmin = SWAPW(bbox->xmin);
		bbox->ymin = READALIGNEDWORD( sp );

		bbox->ymin = SWAPW(bbox->ymin);
		bbox->xmax = READALIGNEDWORD( sp );

		bbox->xmax = SWAPW(bbox->xmax);
		bbox->ymax = READALIGNEDWORD( sp );

		bbox->ymax = SWAPW(bbox->ymax);
	
		if ( cont < 0 ) {
			* ComponentDepth += 1;
 			if ( * ComponentDepth >= MAXCOMPONENTSIZE ) return false;
 			do {
 			
 				flags = READALIGNEDWORD( sp );

				flags = SWAPW(flags);
 			
 				glyphIndex = READALIGNEDWORD( sp );

				glyphIndex = SWAPW(glyphIndex);
				if (!this->SubGetNumberOfPointsAndContours(glyphIndex,contours, points, ComponentDepth, &Sub_bbox)) return false;
 				if ( flags & ARG_1_AND_2_ARE_WORDS ) {
 					/* arg 1 and 2 */
	 				tmp = READALIGNEDWORD( sp );
	 				tmp = READALIGNEDWORD( sp );
 				} else {
 					/* arg 1 and 2 as bytes */
 	 				tmp = READALIGNEDWORD( sp );
 				}
 			
 				if ( flags & WE_HAVE_A_SCALE ) {
 					/* scale */
  	 				tmp = READALIGNEDWORD( sp );
 				} else if ( flags & WE_HAVE_AN_X_AND_Y_SCALE ) {
 					/* xscale, yscale */
   	 				tmp = READALIGNEDWORD( sp );
  	 				tmp = READALIGNEDWORD( sp );
 				} else if ( flags & WE_HAVE_A_TWO_BY_TWO ) {
 					/* xscale, scale01, scale10, yscale */
   	 				tmp = READALIGNEDWORD( sp );
  	 				tmp = READALIGNEDWORD( sp );
   	 				tmp = READALIGNEDWORD( sp );
  	 				tmp = READALIGNEDWORD( sp );
 				}
 			} while ( flags & MORE_COMPONENTS );

		} else {
			*contours += cont;
			for ( i = 0; i < cont; i++ ) {
// claudebe the info is an array of last point number

				tmp = READALIGNEDWORD( sp );
				NbOfPoints = SWAPW(tmp) + 1; /* read endpoints */
			}
			if ((int32_t)(*points) + (int32_t)NbOfPoints >= MAXPOINTS) return false;
			*points += NbOfPoints;
		}
	}
	return true;
} // TrueTypeFont::SubGetNumberOfPointsAndContours

bool TrueTypeFont::GetNumberOfPointsAndContours(int32_t glyphIndex, short *contours, short *points, short *ComponentDepth, sfnt_glyphbbox *bbox) {
	*contours = 0;
	*points = 0;
	*ComponentDepth = 0;
	/* claudebe call recursively this->SubGetNumberOfPointsAndContours and add the number of contours and points
	   of all the components and the subcomponents of composite ghyphs */
	return this->SubGetNumberOfPointsAndContours(glyphIndex,contours, points, ComponentDepth, bbox);
} // TrueTypeFont::GetNumberOfPointsAndContours



