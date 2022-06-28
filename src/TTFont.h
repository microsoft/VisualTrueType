/// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef TTFont_dot_h
#define TTFont_dot_h

#define trialNumGlyphs	0x0400
#define trialSfntSize	0x200000 // bytes

#include <algorithm>
#include <vector>
#include <memory>
#include <deque>
#include <string>
#include <limits>

#include "opentypedefs.h"
#include "CvtManager.h"

#define MAXCONTOURS 	0x2000
#define MAXPOINTS 		0x4000	// rasterizer limit, needs extra two bits for line transition information in drop-out control algorithm

 //#define PHANTOMPOINTS 4
#define PHANTOMPOINTS 2

#define LEFTSIDEBEARING     0
#define RIGHTSIDEBEARING    1

//#define TOPSIDEBEARING      2
//#define BOTTOMSIDEBEARING   3

#define INVALID_GLYPH_INDEX	0xffff

#define maxNumCharCodes			0x10000

#define GASP_VERSION_DEFAULT	0x0000
#define GASP_VERSION_CT_YAA		0x0001
#define MAX_GASP_ENTRIES		6
#define GASP_GRIDFIT			0x0001
#define GASP_DOGRAY				0x0002
#define GASP_CLEARTYPEGRIDFIT	0x0004
#define GASP_DONONCTDIRAA		0x0008

namespace TtFont
{
	typedef struct {
		unsigned short rangeMaxPPEM;
		unsigned short rangeGaspBehavior;
	} GaspRange;

	typedef struct {
		unsigned short version;
		unsigned short numRanges;
		std::vector<GaspRange> gaspRange;
	} GaspTable;

	typedef struct {
		unsigned short version;
		unsigned short numRanges;
		GaspRange gaspRange[1];
	} GaspTableRaw;
}

typedef struct {
    short xmin, ymin, xmax, ymax;
} sfnt_glyphbbox;

typedef struct {
        unsigned short CharCode;
        CharGroup type;
} GlyphGroupType;

// 'glit' (glyph index) table
typedef struct {
    unsigned short	glyphCode;
    unsigned short	length;
    int32_t			offset;
} sfnt_FileDataEntry;

typedef struct {
    unsigned short	glyphCode;
    uint32_t	length;
    int32_t			offset;
} sfnt_MemDataEntry;
/****** what we have here are two pairs of private tables TS00 through TS03, the first pair for TT sources,
the second pair for TMT sources. Within each pair, the first is merely an index into the second, i.e. TT of
glyph n starts at TS01[TS00[n].offset] and TMT of glyph n starts at TS03[TS02[n].offset], assuming that we
are talking byte offsets here, and TS01 and TS03 are just arrays of bytes. The fact that we have two dif-
ferent structs here could be hidden in the sfnt module: in the font files we may have a short form of the
'glit' table, wherein all the offsets are short (sfnt_FileDataEntry), restricting the size, but in the rest
of the application we want to work only with the long offsets (sfnt_MemDataEntry) *****/

typedef struct {
	short xMin, yMin, xMax, yMax; // rectilinear convex hull of all glyphs (union of all glyphs' bounding boxes)
	short advanceWidthMax;
	short minLeftSideBearing;
	short minRightSideBearing;
	short xMaxExtent;
} FontMetricProfile;

struct VariationAxisRecord
{
	uint32_t axisTag;
	Fixed16_16 minValue;
	Fixed16_16 defaultValue;
	Fixed16_16 maxValue;
	uint16_t flags;
	uint16_t nameID;
};

struct VariationInstanceRecord
{
	uint16_t nameID;
	uint16_t flags;
	std::vector< Fixed16_16> coordinates;
	//uint16_t postScriptNameID;
};

struct FVarTableHeader
{
	uint16_t majorVersion;
	uint16_t minorVersion;
	uint16_t offsetToAxesArray;
	uint16_t countSizePairs;
	uint16_t axisCount;
	uint16_t axisSize;
	uint16_t instanceCount;
	uint16_t instanceSize;
	std::vector<VariationAxisRecord> axisRecords;
	std::vector< VariationInstanceRecord> instanceRecords;
};

struct ShortFracCorrespondence
{
	Fixed2_14 fromCoord; //A normalized coordinate value obtained using default normalization.
	Fixed2_14 toCoord;  //The modified, normalized coordinate value.
};

struct ShortFracSegment
{
	uint16_t pairCount; // The number of correspondence pairs for this axis.
	std::vector<ShortFracCorrespondence> axisValueMaps; // The array of axis value map records for this axis.
};

struct AxisVariationHeader
{
	uint16_t majorVersion;
	uint16_t minorVersion;
	uint16_t reserved;
	uint16_t axisCount;
	std::vector<ShortFracSegment> axisSegmentMaps; // The segment maps array - one segment map for each axis
};

typedef struct {
	uint16_t majorVersion;
	uint16_t minorVersion;
	uint16_t tupleVariationCount;
	uint16_t offsetToData;
} CvarHeader;

struct TSICRecord {
	uint16_t flags = 0;
	uint16_t numCVTEntries = 0;
	uint16_t nameLength = 0;
	std::wstring name; // uint16_t NameArray[nameLength];
	std::vector<uint16_t> cvts; // uint16_t CVTArray[numCVTEntries];
	std::vector<int16_t> cvtValues; // FWORD CVTValueArray[numCVTEntries];
};

struct TSICHeader {
	uint16_t majorVersion = 0;
	uint16_t minorVersion = 0;
	uint16_t flags = 0;
	uint16_t axisCount = 0;
	uint16_t recordCount = 0;
	uint16_t reserved = 0;
	std::vector<uint32_t> axes; //uint32_t AxisArray[axisCount];
	std::vector<std::vector<Fixed2_14>> locations; //uint16_t RecordLocations[recordCount][axisCount];
	std::vector<TSICRecord> tsicRecords; //TSICRecord TSICRecord[recordCount];
};

typedef struct {
	uint16_t cvt;
	int16_t delta;
} CvtElement;

#define MAXCVT maxCvtNum

#define MAXBINSIZE			(0x10000L - 4) // don't push the limit

#define MAXCOMPONENTSIZE 256
#define MAXNUMCOMPONENTS (MAXCOMPONENTSIZE/3) // flags, glyphIndex, pair of bytes for offset = minimal size used by each component

// these values to assert 'maxp' ok with templates (possibly generated by auto-hinter)
#define MIN_STORAGE	47
#define MIN_TWILIGHTPOINTS 16
#define MIN_ELEMENTS 2

// This is the magic cookie for the preferences.
#define MAGIC 0xA2340987 // note that this is a different magic number than SFNT_MAGIC...

typedef struct {
	uint32_t unicode;
	unsigned short glyphIndex;
} UniGlyphMap;

typedef struct {
	uint32_t unicode;
	unsigned short glyphIndex;
	unsigned short numContours;
	bool rounded;
	bool anchor;
	short offsetX,offsetY;      // fUnits, only if !anchor
	short parent,child;         // only if anchor
	bool identity;
	ShortFract transform[2][2]; // 2.14
} TrueTypeComponent;

typedef struct {
	unsigned short numComponents;
	unsigned short useMyMetrics; // or INVALID_GLYPH_INDEX if nobody's metrics
	TrueTypeComponent component[MAXNUMCOMPONENTS];
} TrueTypeBluePrint;

struct TrueTypeCompositeComponent {
	unsigned short flags = 0;
	unsigned short glyphIndex = 0;
	short arg1 = 0; // or arg1 | arg2
	short arg2 = 0;
	ShortFract xscale = 0; // or scale
	ShortFract yscale = 0;
	ShortFract scale01 = 0;
	ShortFract scale10 = 0;
};

#define ONCURVE             0x01

class TrueTypeGlyph { // serves mainly as an interface to new user interface
public:
	TrueTypeGlyph(void);
	virtual ~TrueTypeGlyph(void);
	LinkColor TheColor(short from, short to); // such that everybody can use this, not only the compiler
	bool Misoriented(short contour);

	short xmin, ymin, xmax, ymax; 				// bounding box; xmin corresponds to left side bearing
	short realLeftSideBearing, realRightSideBearing, blackBodyWidth; // as obtained from specifying left and right point by GrabHereInX, to do with auto-hinter???

	// contour, knot data
	int32_t numContoursInGlyph;
	short startPoint[MAXCONTOURS];
	short endPoint[MAXCONTOURS];

	short x[MAXPOINTS];							// these seem to be the (coordinates of the) control points
	short y[MAXPOINTS];							// Use start|endPoint arrays for contour identification
	bool onCurve[MAXPOINTS];					// on curve?
	F26Dot6 xx[MAXPOINTS];						// used to get coordinates back from the rasterizer
	F26Dot6 yy[MAXPOINTS];

	// composite
	bool composite,useMyMetrics;
	short componentData[MAXCOMPONENTSIZE];				// binary of TT composite
	short componentSize;								// size of above data
	short ComponentVersionNumber;						// sort of a magic number, tends to be -1, I'd prefer this to disappear
	TrueTypeBluePrint bluePrint;
private:
	short dirChange[MAXPOINTS];							// used during TheColor
};

#define CO_CompInstrFollow	1
#define CO_StdInstrFollow	2
#define CO_NothingFollows	0

typedef struct {
	short GlobalNON_OVERLAPPING, GlobalMORE_COMPONENTS, numberOfCompositeElements;
	short anchorPrevPoints, nextExitOffset;
	short GlobalUSEMYMETRICS;
	short GlobalSCALEDCOMPONENTOFFSET;
	short GlobalUNSCALEDCOMPONENTOFFSET;
	short numberOfCompositeContours, numberOfCompositePoints;
} TTCompositeProfile;

typedef enum { stripNothing = 0, stripSource, stripHints, stripBinary, stripEverything } StripCommand;

typedef enum { asmGLYF = 0, asmPREP, asmFPGM } ASMType;
#define firstASMType   asmGLYF
#define lastASMType    asmFPGM
#define numASMTypes    (lastASMType - firstASMType + 1)

#define firstTTASMType asmGLYF
#define lastTTASMType  asmFPGM
#define numTTASMTypes  (lastTTASMType - firstTTASMType + 1)


class TrueTypeFont {
public:
	TrueTypeFont(void);
	bool Create();
	virtual ~TrueTypeFont(void);
	void AssertMaxSfntSize(uint32_t minSfntSize, bool assertMainHandle, bool assertTempHandle);
	void AssertMaxGlyphs(int32_t minGlyphs);
	bool Read(File *file, TrueTypeGlyph *glyph, short *platformID, short *encodingID, wchar_t errMsg[], size_t errMsgLen);
	bool Write(File *file, wchar_t errMsg[], size_t errMsgLen);
	ControlValueTable *TheCvt(void);
	bool GetCvt (TextBuffer *cvtText,  wchar_t errMsg[], size_t errMsgLen);
	bool GetPrep(TextBuffer *prepText, wchar_t errMsg[], size_t errMsgLen);
	int32_t PrepBinSize(void);
	bool GetFpgm(TextBuffer *fpgmText, wchar_t errMsg[], size_t errMsgLen);
	int32_t FpgmBinSize(void);
	bool GetGlyf(int32_t glyphIndex, TextBuffer *glyfText, wchar_t errMsg[], size_t errMsgLen);
	bool GetTalk(int32_t glyphIndex, TextBuffer *talkText, wchar_t errMsg[], size_t errMsgLen);
	bool GetGlyph(int32_t glyphIndex, TrueTypeGlyph *glyph, wchar_t errMsg[], size_t errMsgLen);
	int32_t GlyfBinSize(void);
	unsigned char* GlyfBin(void);
	void GetHeights(int32_t* emHeight);
	bool GetHMTXEntry(int32_t glyphIndex, int32_t *leftSideBearing, int32_t *advanceWidth);
	int32_t NumberOfGlyphs(void);
	int32_t GlyphIndexOf(uint32_t charCode);
	bool GlyphIndecesOf(wchar_t textString[], int32_t maxNumGlyphIndeces, int32_t glyphIndeces[], int32_t *numGlyphIndeces, wchar_t errMsg[], size_t errMsgLen);
	uint32_t CharCodeOf(int32_t glyphIndex);
	uint32_t AdjacentChar(uint32_t charCode, bool forward);
	uint32_t FirstChar();
	CharGroup CharGroupOf(int32_t glyphIndex);
	bool CMapExists(short platformID, short encodingID);
	bool DefaultCMap(short *platformID, short *encodingID, wchar_t errMsg[], size_t errMsgLen);
	bool UnpackCMap(short platformID, short encodingID, wchar_t errMsg[], size_t errMsgLen);
	bool IsCvarTupleData();
	int32_t EstimatePrivateCvar();
	int32_t UpdatePrivateCvar(int32_t *size, unsigned char data[]);
	bool HasPrivateCvar();
	bool GetPrivateCvar(TSICHeader &header);
	bool MergePrivateCvarWithInstanceManager(const TSICHeader &header);
	int32_t EstimateCvar();
	int32_t UpdateCvar(int32_t *size, unsigned char data[]);
	void UpdateAdvanceWidthFlag(bool linear);
	bool UpdateBinData(ASMType asmType, int32_t binSize, unsigned char *binData);
	bool BuildNewSfnt(StripCommand strip, CharGroup group, int32_t glyphIndex, TrueTypeGlyph *glyph,
					  TextBuffer *glyfText, TextBuffer *prepText, TextBuffer *cvtText,  TextBuffer *talkText, TextBuffer *fpgmText,
					  wchar_t errMsg[], size_t errMsgLen);

	bool InitIncrBuildSfnt(bool binaryOnly, wchar_t errMsg[], size_t errMsgLen);
	bool AddGlyphToNewSfnt(CharGroup group, int32_t glyphIndex, TrueTypeGlyph *glyph, int32_t glyfBinSize, unsigned char *glyfBin, TextBuffer *glyfText, TextBuffer *talkText, wchar_t errMsg[], size_t errMsgLen);

	bool TermIncrBuildSfnt(bool disposeOnly, TextBuffer *prepText, TextBuffer *cvtText, TextBuffer *fpgmText, wchar_t errMsg[], size_t errMsgLen);

	void InitNewProfiles(void);
	void InheritProfiles(void);
	void UseNewProfiles(void);
	sfnt_maxProfileTable GetProfile(void);
	void UpdateGlyphProfile(TrueTypeGlyph *glyph); // used not only in BuildNewSfnt, but also in calculation of maxp and other odd places...
	void UpdateAssemblerProfile(ASMType asmType, short maxFunctionDefs, short maxStackElements, short maxSizeOfInstructions);
	void UpdateCompositeProfile(TrueTypeGlyph *glyph, TTCompositeProfile *compositeProfile, short context, short RoundingCode, short InstructionIndex, short *args, short argc, sfnt_glyphbbox *Newbbox, short *error);
	bool GetNumberOfPointsAndContours(int32_t glyphIndex, short *contours, short *points, short *ComponentDepth, sfnt_glyphbbox *bbox);
	int32_t GetUnitsPerEm(void);							// FUnits Per EM (2048 is typical)
	void UpdateAutohinterProfile(short maxElements, short maxTwilightPoints, short maxStorage);
	bool HasSource();
	bool IsMakeTupleName(const std::wstring &name) const;

	unsigned char* GetSfntPtr(int32_t offset)
	{
		return (sfntHandle + offset);
	}

	std::shared_ptr<Variation::InstanceManager> GetInstanceManager()
	{
		return instanceManager_;
	}

	std::shared_ptr<Variation::CVTVariationInterpolator1> GetCvtVariationInterpolator()
	{
		return cvtVariationInterpolator_;
	}

	template <typename T>
	bool GetDefaultCvts(std::vector<T> & defaultCvts)
	{
		ControlValueTable *cvt = this->TheCvt();
		int32_t highestCvtNum = cvt->HighestCvtNum();
		defaultCvts.resize(highestCvtNum + 1, 0);

		// Get the default cvts.
		for (int32_t i = 0; i <= highestCvtNum; i++)
		{
			short cvtValue;
			if (cvt->GetCvtValue(i, &cvtValue))
				defaultCvts[i] = cvtValue;
		}

		return true;
	}

	bool ReverseInterpolateCvarTuples();

	bool HasFvar() const
	{
		return fvar_.axisRecords.size() > 0;
	}

	bool HasAvar() const
	{
		return avar_.axisSegmentMaps.size() > 0;
	}

	bool IsVariationFont() const
	{
		return bVariationTypeface_;
	}

	uint16_t GetVariationAxisCount() const
	{
		return axisCount_;
	}

	std::shared_ptr <std::vector<uint32_t>> GetVariationAxisTags()
	{
		return variationAxisTags_;
	}

private:
	void UpdateMetricProfile(TrueTypeGlyph *glyph);
	bool SubGetNumberOfPointsAndContours(int32_t glyphIndex, short *contours, short *points, short *ComponentDepth, sfnt_glyphbbox *bbox);
	bool TableExists(sfnt_TableTag tag);
	int32_t GetTableOffset(sfnt_TableTag tag);
	int32_t GetTableLength(sfnt_TableTag tag);
	unsigned char *GetTablePointer(sfnt_TableTag tag);
	bool UnpackHeadHheaMaxpHmtx(wchar_t errMsg[], size_t errMsgLen);
	bool UnpackGlitsLoca(wchar_t errMsg[], size_t errMsgLen);
	bool UpdateMaxPointsAndContours(wchar_t errMsg[], size_t errMsgLen);
	void EnterChar(int32_t glyphIndex, uint32_t charCode);
	void SortGlyphMap();
	void GetFmt0(sfnt_mappingTable *map);
	void GetFmt4(sfnt_mappingTable *map);
	void GetFmt6(sfnt_mappingTable *map);
	void GetFmt12(sfnt_mappingTable *map);
	bool UnpackCharGroup(wchar_t errMsg[], size_t errMsgLen);
	bool GetSource(bool lowLevel, int32_t glyphIndex, TextBuffer *source, wchar_t errMsg[], size_t errMsgLen);
	bool GetTTOTable(sfnt_TableTag srcTag, TextBuffer *src, sfnt_TableTag binTag, ASMType asmType);
	void CalculateNewCheckSums(void);
	void CalculateCheckSumAdjustment(void);
	void SortTableDirectory(void);
	void PackMaxpHeadHhea(void);
	uint32_t GetPackedGlyphsSizeEstimate(TrueTypeGlyph *glyph, int32_t glyphIndex, uint32_t *oldIndexToLoc);
	uint32_t GetPackedGlyphSize(int32_t glyphIndex, TrueTypeGlyph *glyph, int32_t glyfBinSize);
	uint32_t PackGlyphs(StripCommand strip, TrueTypeGlyph *glyph, int32_t glyphIndex, uint32_t *oldIndexToLoc, uint32_t *newIndexToLoc, unsigned char *dst);
	uint32_t PackGlyph(unsigned char *dst, int32_t glyphIndex, TrueTypeGlyph *glyph, int32_t glyfBinSize, unsigned char *glyfInstruction, sfnt_HorizontalMetrics *hmtx);
	uint32_t StripGlyphBinary(unsigned char *dst, unsigned char *src, uint32_t srcLen);
	uint32_t GetPackedGlyphSourceSize(TextBuffer *glyfText, TextBuffer *prepText, TextBuffer *cvtText, TextBuffer *talkText, TextBuffer *fpgmText,
							  		   short type, int32_t glyphIndex, int32_t glitIndex, sfnt_MemDataEntry *memGlit);
	uint32_t GetPackedGlyphSourcesSize(TextBuffer *glyfText, TextBuffer *prepText, TextBuffer *cvtText, TextBuffer *talkText, TextBuffer *fpgmText,
							   			short type, int32_t glyphIndex, sfnt_MemDataEntry *memGlit);
	void PackGlyphSource(TextBuffer *glyfText, TextBuffer *prepText, TextBuffer *cvtText, TextBuffer *talkText, TextBuffer *fpgmText,
							  short type, int32_t glyphIndex, int32_t glitIndex, sfnt_FileDataEntry *fileGlit, sfnt_MemDataEntry *memGlit,
							  uint32_t *dstPos, unsigned char *dst);
	void PackGlyphSources(TextBuffer *glyfText, TextBuffer *prepText, TextBuffer *cvtText, TextBuffer *talkText, TextBuffer *fpgmText,
							   short type, int32_t glyphIndex, sfnt_FileDataEntry *fileGlit, sfnt_MemDataEntry *memGlit,
							   uint32_t *dstPos, unsigned char *dst);
	bool GetNumPointsAndContours(int32_t glyphIndex, int32_t *numKnots, int32_t *numContours, int32_t *componentDepth);
	bool IncrBuildNewSfnt(wchar_t errMsg[], size_t errMsgLen);
	bool SetSfnt(short platformID, short encodingID, wchar_t errMsg[], size_t errMsgLen);

	void UnpackFvar(void);
	void UnpackAvar(void);

	// sfnt
	unsigned char *sfntHandle = nullptr; 		// handle to the sfnt file layout
	uint32_t sfntSize = 0;				    // actual sfnt data size (in bytes)
	uint32_t maxSfntSize = 0;				// memory (in bytes) allocated for above handle
	unsigned char *sfntTmpHandle = nullptr;		// claudebe 1/26/94 temp sfnt Handle for use in BuildNewSfnt to avoid memory fragmentation
	uint32_t maxTmpSfntSize = 0;			// memory (in bytes) allocated for above handle

	sfnt_OffsetTable *offsetTable;				// tmp for use in BuildNewSfnt to avoid memory fragmentation
	sfnt_OffsetTable *tmpOffsetTable;			// tmp for use in BuildNewSfnt to avoid memory fragmentation

	void *incrBuildSfntData;

	// 'cvt'
	ControlValueTable *cvt = nullptr;

	// 'maxp' (maximum profile) table
	sfnt_maxProfileTable profile;
	sfnt_maxProfileTable newProfile; 			// used for 'maxp' computation
	uint16 maxStackElements[numTTASMTypes];		// used for new heuristic in computing newProfile.maxStackElements

	// 'loca' (index to location) table
	bool shortIndexToLocTable;				// short or long loca table
	bool outShortIndexToLocTable;			// indicate if we want to write in long or short format
	uint32_t *IndexToLoc;					// modif to be able to convert the format loca table store in long format in the glyph informations rather than recomputing all the time
	uint32_t *tmpIndexToLoc;				// tmp for use in BuildNewSfnt to avoid memory fragmentation
	int32_t numLocaEntries;

	// 'head', 'hhea' tables
	FontMetricProfile metricProfile;
	FontMetricProfile newMetricProfile;			// used for 'maxp' computation

	//bool useIntegerScaling;
	unsigned short macStyle;					// passed from ReadHeader to WriteHeader as is


	// 'GLIT' (glyph index) table
	int32_t maxGlitEntries;
	int32_t maxGlyphs;
	sfnt_MemDataEntry *glit1;
	int32_t glit1Entries;
	sfnt_MemDataEntry *glit2;
	int32_t glit2Entries;

	int32_t numberOfChars, numberOfGlyphs;			// numberOfChars in *currently* unpacked cmap
	uint32_t *charCodeOf;					// glyph index
	unsigned char *charGroupOf;					// glyph index

	std::vector<UniGlyphMap> *glyphIndexMap;

	//	TT Asm Tables
	int32_t binSize[numASMTypes];
	unsigned char *binData[numASMTypes];

	unsigned char *tmpFlags;					// tmp for use in BuildNewSfnt to avoid memory fragmentation

	char *devMetricsPtr;
	int32_t hdmxBinSize;

	char *ltshPtr;
	int32_t ltshBinSize;

	char *vdmxPtr;
	int32_t vdmxBinSize;

	// 'gasp' table
	TtFont::GaspTable gaspTable;
    int32_t gaspBinSize;
	bool gaspLoaded = false;

	// 'TSIC' table
	int32_t tsicBinSize;
    // 'cvar' table
    int32_t cvarBinSize;
	bool tsicError = false;

    // vertical metrics: Quick & dirty "guess" for drawing the height lines in the main window.
    int32_t unitsPerEm,                            // FUnits Per EM (2048 is typical)
         capHeight,                             // glyph->ymax of 'H'...
         xHeight,                               // glyph->ymax of 'x'...
         descenderHeight;                       // glyph->ymin of 'p'

    // horizontal metrics: left side-bearing + bounding box width + right side-bearing = advance width (with the right side-bearing being the bound variable)
    sfnt_HorizontalMetrics *horMetric;			// this contains left side-bearing and advance width of all glyphs

    std::vector<std::string> *postScriptNames = nullptr;

    std::shared_ptr<Variation::CVTVariationInterpolator1> cvtVariationInterpolator_ = nullptr;

    bool bVariationTypeface_ = false;
    uint16_t axisCount_ = 0;

	FVarTableHeader fvar_;
	AxisVariationHeader avar_;

	std::shared_ptr<Variation::InstanceManager> instanceManager_ = nullptr;

	std::shared_ptr<std::vector<uint32_t>> variationAxisTags_ = nullptr; // cache the axis tags
};

enum class CheckCompositeResult {Success, Tolerance, Fail};
CheckCompositeResult CheckCompositeVariationCompatible(const short* pFirst, short firstSize, const short* pSecond, short secondSize);


#endif // TTFont_dot_h
