// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#define tag_AxisVariations      0x61766172        /* 'avar' */
#define tag_CharToIndexMap      0x636d6170        /* 'cmap' */
#define tag_CVTVariations       0x63766172        /* 'cvar' */
#define tag_ControlValue        0x63767420        /* 'cvt ' */
#define tag_DigitalSignature    0x44534947        /* 'DSIG' */
#define tag_BitmapData          0x45424454        /* 'EBDT' */
#define tag_BitmapLocation      0x45424c43        /* 'EBLC' */
#define tag_BitmapScale         0x45425343        /* 'EBSC' */
#define tag_Editor0             0x65647430        /* 'edt0' */
#define tag_Editor1             0x65647431        /* 'edt1' */
#define tag_Encryption          0x63727970        /* 'cryp' */
#define tag_FontHeader          0x68656164        /* 'head' */
#define tag_FontProgram         0x6670676d        /* 'fpgm' */
#define tag_FontVariations      0x66766172        /* 'fvar' */
#define tag_GridfitAndScanProc  0x67617370        /* 'gasp' */
#define tag_GlyphDirectory      0x67646972        /* 'gdir' */
#define tag_GlyphData           0x676c7966        /* 'glyf' */
#define tag_GlyphVariations     0x67766172        /* 'gvar' */
#define tag_HoriDeviceMetrics   0x68646d78        /* 'hdmx' */
#define tag_HoriHeader          0x68686561        /* 'hhea' */
#define tag_HorizontalMetrics   0x686d7478        /* 'hmtx' */
#define tag_IndexToLoc          0x6c6f6361        /* 'loca' */
#define tag_Kerning             0x6b65726e        /* 'kern' */
#define tag_LinearThreshold     0x4c545348        /* 'LTSH' */
#define tag_MATH                0x4d415448        /* 'MATH' */
#define tag_MaxProfile          0x6d617870        /* 'maxp' */
#define tag_Metamorphosis       0x6d6f7274        /* 'mort' */
#define tag_ExtMetamorphosis    0x6d6f7278        /* 'morx' */
#define tag_NamingTable         0x6e616d65        /* 'name' */
#define tag_OS_2                0x4f532f32        /* 'OS/2' */
#define tag_PCLT                0x50434c54        /* 'PCLT' */
#define tag_Postscript          0x706f7374        /* 'post' */
#define tag_PreProgram          0x70726570        /* 'prep' */
#define tag_VertDeviceMetrics   0x56444d58        /* 'VDMX' */
#define tag_VertHeader          0x76686561        /* 'vhea' */
#define tag_VerticalMetrics     0x766d7478        /* 'vmtx' */

#define tag_TTO_GSUB             0x47535542          /* 'GSUB' */
#define tag_TTO_GPOS             0x47504F53          /* 'GPOS' */
#define tag_TTO_GDEF             0x47444546          /* 'GDEF' */
#define tag_TTO_BASE             0x42415345          /* 'BASE' */
#define tag_TTO_JSTF             0x4A535446          /* 'JSTF' */

#define PHANTOMPOINTS 2	

typedef signed char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;

typedef long long int64;
typedef unsigned long long uint64;

typedef unsigned char   BYTE;
typedef unsigned int    UINT;
typedef unsigned short  USHORT;

typedef short FUnit;
typedef unsigned short uFUnit;

typedef short ShortFract;

typedef int Fixed;

typedef uint32 sfnt_TableTag;

/* Portable code to extract a short or a int from a 2- or 4-byte buffer */
/* which was encoded using Motorola 68000 (TrueType "native") byte order. */
#define FS_2BYTE(p) ( ((unsigned short)((p)[0]) << 8) |  (p)[1])
#define FS_4BYTE(p) ( FS_2BYTE((p)+2) | ( (FS_2BYTE(p)+0L) << 16) )
#define SWAPW(a)    ((int16) FS_2BYTE( (unsigned char const*)(&a) ))
#define CSWAPW(num) (((((num) & 0xff) << 8) & 0xff00) + (((num) >> 8) & 0xff)) // use this variant or else cannot apply to constants due to FS_2BYTE and FS_4BYTE
#define SWAPL(a)    ((int32) FS_4BYTE( (unsigned char const*)(&a) ))
#define CSWAPL(num) ((CSWAPW((num) & 0xffff) << 16) + CSWAPW((num) >> 16)) // use this variant or else cannot apply to constants due to FS_2BYTE and FS_4BYTE
#define SWAPWINC(a) SWAPW(*(a)); a++    /* Do NOT parenthesize! */


#ifndef F26Dot6
#define F26Dot6 int
#endif
#define places6	6L
#define two6	0x80L
#define one6	0x40L
#define half6	0x20L
#define Round6(x) (((x) + half6) >> places6)

#define F16Dot16 int

#ifndef	Min
#define Min(a,b)	((a) < (b) ? (a) : (b))
#endif
#ifndef	Max
#define Max(a,b)	((a) > (b) ? (a) : (b))
#endif
#ifndef	Sgn
#define Sgn(a)		((a) < 0 ? -1 : ((a) > 0 ? 1 : 0))
#endif
#ifndef Sgn3
#define Sgn3(x)		(x == 0 ? 0 : Sgn(x))
#endif
#ifndef	Abs
#define Abs(x)		((x) < 0 ? (-(x)) : (x))
#endif
#ifndef Len
#define Len(a) ((signed int)(sizeof(a)/sizeof(a[0])))
#endif



#pragma pack(1)


typedef struct {
    uint32 bc;
    uint32 ad;
} BigDate;

typedef struct {
    sfnt_TableTag   tag;
    uint32          checkSum;
    uint32          offset;
    uint32          length;
} sfnt_DirectoryEntry;

/*
 *  The search fields limits numOffsets to 4096.
 */
typedef struct {
    int32 version;                  /* 0x10000 (1.0) */
    uint16 numOffsets;              /* number of tables */
    uint16 searchRange;             /* (max2 <= numOffsets)*16 */
    uint16 entrySelector;           /* log2 (max2 <= numOffsets) */
    uint16 rangeShift;              /* numOffsets*16-searchRange*/
    sfnt_DirectoryEntry table[1];   /* table[numOffsets] */
} sfnt_OffsetTable;
#define OFFSETTABLESIZE     12  /* not including any entries */

/*
 *  for the flags field
 */
#define Y_POS_SPECS_BASELINE            0x0001
#define X_POS_SPECS_LSB                 0x0002
#define HINTS_USE_POINTSIZE             0x0004
#define USE_INTEGER_SCALING             0x0008
#define INSTRUCTED_ADVANCE_WIDTH        0x0010

 /* flags 5-10 defined by Apple */
#define APPLE_VERTICAL_LAYOUT           0x0020
#define APPLE_RESERVED                  0x0040
#define APPLE_LINGUISTIC_LAYOUT         0x0080
#define APPLE_GX_METAMORPHOSIS          0x0100
#define APPLE_STRONG_RIGHT_TO_LEFT      0x0200
#define APPLE_INDIC_EFFECT              0x0400

#define FONT_COMPRESSED                 0x0800
#define FONT_CONVERTED                  0x1000

#define OUTLINE_CORRECT_ORIENTATION     0x4000
#define SFNT_MAGIC 0x5F0F3CF5

#define SHORT_INDEX_TO_LOC_FORMAT       0
#define LONG_INDEX_TO_LOC_FORMAT        1
#define GLYPH_DATA_FORMAT               0

typedef struct {
    Fixed       version;            /* for this table, set to 1.0 */
    Fixed       fontRevision;       /* For Font Manufacturer */
    uint32      checkSumAdjustment;
    uint32      magicNumber;        /* signature, should always be 0x5F0F3CF5  == MAGIC */
    uint16      flags;
    uint16      unitsPerEm;         /* Specifies how many in Font Units we have per EM */

    BigDate     created;
    BigDate     modified;

    /** This is the font wide bounding box in ideal space
 (baselines and metrics are NOT worked into these numbers) **/
    FUnit       xMin;
    FUnit       yMin;
    FUnit       xMax;
    FUnit       yMax;

    uint16      macStyle;               /* macintosh style word */
    uint16      lowestRecPPEM;          /* lowest recommended pixels per Em */

    /* 0: fully mixed directional glyphs, 1: only strongly L->R or T->B glyphs,
       -1: only strongly R->L or B->T glyphs, 2: like 1 but also contains neutrals,
       -2: like -1 but also contains neutrals */
    int16       fontDirectionHint;

    int16       indexToLocFormat;
    int16       glyphDataFormat;
} sfnt_FontHeader;

typedef struct {
    Fixed       version;                /* for this table, set to 1.0 */

    FUnit       yAscender;
    FUnit       yDescender;
    FUnit       yLineGap;       /* Recommended linespacing = ascender - descender + linegap */
    uFUnit      advanceWidthMax;
    FUnit       minLeftSideBearing;
    FUnit       minRightSideBearing;
    FUnit       xMaxExtent; /* Max of (LSBi + (XMAXi - XMINi)), i loops through all glyphs */

    int16       horizontalCaretSlopeNumerator;
    int16       horizontalCaretSlopeDenominator;

    uint16      reserved0;
    uint16      reserved1;
    uint16      reserved2;
    uint16      reserved3;
    uint16      reserved4;

    int16       metricDataFormat;           /* set to 0 for current format */
    uint16      numberOf_LongHorMetrics;    /* if format == 0 */
} sfnt_HorizontalHeader;

typedef struct {
    uint16      advanceWidth;
    int16       leftSideBearing;
} sfnt_HorizontalMetrics;

typedef struct {
    uint16      advanceHeight;
    int16       topSideBearing;
} sfnt_VerticalMetrics;

/*
 *  CVT is just a bunch of int16s
 */
typedef int16 sfnt_ControlValue;

/*
 *  Char2Index structures, including platform IDs
 */
typedef struct {
    uint16  format;
    uint16  length;
    uint16  version;
} sfnt_mappingTable;

typedef struct {
    uint16  platformID;
    uint16  specificID;
    uint32  offset;
} sfnt_platformEntry;

typedef struct {
    uint16  version;
    uint16  numTables;
    sfnt_platformEntry platform[1]; /* platform[numTables] */
} sfnt_char2IndexDirectory;
#define SIZEOFCHAR2INDEXDIR     4

typedef struct {
    uint16  firstCode;
    uint16  entryCount;
    int16   idDelta;
    uint16  idRangeOffset;
} sfnt_subHeader2;

typedef struct {
    uint16            subHeadersKeys[256];
    sfnt_subHeader2   subHeaders[1];
} sfnt_mappingTable2;

typedef struct {
    uint16  segCountX2;
    uint16  searchRange;
    uint16  entrySelector;
    uint16  rangeShift;
    uint16  endCount[1];
} sfnt_mappingTable4;

typedef struct {
    uint16  firstCode;
    uint16  entryCount;
    uint16  glyphIdArray[1];
} sfnt_mappingTable6;

typedef struct {
    uint16 format;
    uint16 reserved;
    uint32 length;
    uint32 language;
    uint32 nGroups;
} sfnt_mappingTable12;

typedef struct
{
    uint32 startCharCode;
    uint32 endCharCode;
    uint32 startGlyphCode;
} sfnt_mappingTable12Record;

typedef struct {
    uint16 platformID;
    uint16 specificID;
    uint16 languageID;
    uint16 nameID;
    uint16 length;
    uint16 offset;
} sfnt_NameRecord;

typedef struct {
    uint16 format;
    uint16 count;
    uint16 stringOffset;
    /*  sfnt_NameRecord[count]  */
} sfnt_NamingTable;

struct sfnt_maxProfileTable
{
    Fixed       version;                /* for this table, set to 1.0 */
    uint16      numGlyphs;
    uint16      maxPoints;              /* in an individual glyph */
    uint16      maxContours;            /* in an individual glyph */
    uint16      maxCompositePoints;     /* in an composite glyph */
    uint16      maxCompositeContours;   /* in an composite glyph */
    uint16      maxElements;            /* set to 2, or 1 if no twilightzone points */
    uint16      maxTwilightPoints;      /* max points in element zero */
    uint16      maxStorage;             /* max number of storage locations */
    uint16      maxFunctionDefs;        /* max number of FDEFs in any preprogram */
    uint16      maxInstructionDefs;     /* max number of IDEFs in any preprogram */
    uint16      maxStackElements;       /* max number of stack elements for any individual glyph */
    uint16      maxSizeOfInstructions;  /* max size in bytes for any individual glyph */
    uint16      maxComponentElements;   /* number of glyphs referenced at top level */
    uint16      maxComponentDepth;      /* levels of recursion, 1 for simple components */
};

#define DEVEXTRA    2   /* size + max */
/*
 *  Each record is n+2 bytes, padded to long word alignment.
 *  First byte is ppem, second is maxWidth, rest are widths for each glyph
 */
typedef struct {
    int16               version;
    int16               numRecords;
    int32               recordSize;
    /* Byte widths[numGlyphs+2] * numRecords */
} sfnt_DeviceMetrics;

#define POST_VERSION_2 0x00020000

struct sfnt_PostScriptInfo
{
    Fixed   version;                /* 1.0 */
    Fixed   italicAngle;
    FUnit   underlinePosition;
    FUnit   underlineThickness;
    uint32  isFixedPitch;
    uint32  minMemType42;
    uint32  maxMemType42;
    uint32  minMemType1;
    uint32  maxMemType1;

    uint16  numberGlyphs;
    union
    {
        uint16  glyphNameIndex[1];   /* version == 2.0 */
        int8    glyphNameIndex25[1]; /* version == 2.5 */
    } postScriptNameIndices;
};

typedef struct {
    uint16  Version;
    int16   xAvgCharWidth;
    uint16  usWeightClass;
    uint16  usWidthClass;
    int16   fsType;
    int16   ySubscriptXSize;
    int16   ySubscriptYSize;
    int16   ySubscriptXOffset;
    int16   ySubscriptYOffset;
    int16   ySuperScriptXSize;
    int16   ySuperScriptYSize;
    int16   ySuperScriptXOffset;
    int16   ySuperScriptYOffset;
    int16   yStrikeOutSize;
    int16   yStrikeOutPosition;
    int16   sFamilyClass;
    uint8   Panose[10];
    uint32  ulCharRange[4];
    char    achVendID[4];
    uint16  usSelection;
    uint16  usFirstChar;
    uint16  usLastChar;
    int16   sTypoAscender;
    int16  sTypoDescender;
    int16   sTypoLineGap;
    int16   sWinAscent;
    int16   sWinDescent;
    uint32  ulCodePageRange[2];
} sfnt_OS2;

typedef struct
{
    uint8   bEmY;
    uint8   bEmX;
    uint8   abInc[1];
} sfnt_hdmxRecord;

typedef struct
{
    uint16          Version;
    int16           sNumRecords;
    int32           lSizeRecord;
    sfnt_hdmxRecord HdmxTable;
} sfnt_hdmx;

typedef struct
{
    uint16    Version;
    uint16    usNumGlyphs;
    uint8     ubyPelsHeight;
} sfnt_LTSH;

typedef struct
{
    uint16          rangeMaxPPEM;
    uint16          rangeGaspBehavior;
} sfnt_gaspRange;

typedef struct
{
    uint16          version;
    uint16          numRanges;
    sfnt_gaspRange  gaspRange[1];
} sfnt_gasp;

/* various typedef to access to the sfnt data */

typedef sfnt_OffsetTable* sfnt_OffsetTablePtr;
typedef sfnt_FontHeader* sfnt_FontHeaderPtr;
typedef sfnt_HorizontalHeader* sfnt_HorizontalHeaderPtr;
typedef sfnt_maxProfileTable* sfnt_maxProfileTablePtr;
typedef sfnt_ControlValue* sfnt_ControlValuePtr;
typedef sfnt_char2IndexDirectory* sfnt_char2IndexDirectoryPtr;
typedef sfnt_HorizontalMetrics* sfnt_HorizontalMetricsPtr;
typedef sfnt_VerticalMetrics* sfnt_VerticalMetricsPtr;
typedef sfnt_platformEntry* sfnt_platformEntryPtr;
typedef sfnt_NamingTable* sfnt_NamingTablePtr;
typedef sfnt_OS2* sfnt_OS2Ptr;
typedef sfnt_DirectoryEntry* sfnt_DirectoryEntryPtr;
typedef sfnt_gasp* sfnt_gaspPtr;

/*
 * 'gasp' Table Constants
*/

#define GASP_GRIDFIT    0x0001
#define GASP_DOGRAY     0x0002


/*
 * UNPACKING Constants
*/
/*define ONCURVE                 0x01   defined in FSCDEFS.H    */
#define XSHORT              0x02
#define YSHORT              0x04
#define REPEAT_FLAGS        0x08 /* repeat flag n times */
/* IF XSHORT */
#define SHORT_X_IS_POS      0x10 /* the short vector is positive */
/* ELSE */
#define NEXT_X_IS_ZERO      0x10 /* the relative x coordinate is zero */
/* ENDIF */
/* IF YSHORT */
#define SHORT_Y_IS_POS      0x20 /* the short vector is positive */
/* ELSE */
#define NEXT_Y_IS_ZERO      0x20 /* the relative y coordinate is zero */
/* ENDIF */
/* 0x40 & 0x80              RESERVED
** Set to Zero
**
*/

/*
 * Composite glyph constants
 */
#define COMPONENTCTRCOUNT           -1      /* ctrCount == -1 for composite */
#define ARG_1_AND_2_ARE_WORDS       0x0001  /* if set args are words otherwise they are bytes */
#define ARGS_ARE_XY_VALUES          0x0002  /* if set args are xy values, otherwise they are points */
#define ROUND_XY_TO_GRID            0x0004  /* for the xy values if above is true */
#define WE_HAVE_A_SCALE             0x0008  /* Sx = Sy, otherwise scale == 1.0 */
#define NON_OVERLAPPING             0x0010  /* set to same value for all components */
#define MORE_COMPONENTS             0x0020  /* indicates at least one more glyph after this one */
#define WE_HAVE_AN_X_AND_Y_SCALE    0x0040  /* Sx, Sy */
#define WE_HAVE_A_TWO_BY_TWO        0x0080  /* t00, t01, t10, t11 */
#define WE_HAVE_INSTRUCTIONS        0x0100  /* instructions follow */
#define USE_MY_METRICS              0x0200  /* apply these metrics to parent glyph */
#define OVERLAP_COMPOUND            0x0400  /* used by Apple in GX fonts */
#define SCALED_COMPONENT_OFFSET     0x0800  /* composite designed to have the component offset scaled (designed for Apple) */
#define UNSCALED_COMPONENT_OFFSET   0x1000  /* composite designed not to have the component offset scaled (designed for MS) */

#pragma pack()


