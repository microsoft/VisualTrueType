// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once 

typedef enum {
	plat_Unicode,
	plat_Macintosh,
	plat_ISO,
	plat_MS
} sfnt_PlatformEnum;

#define maxBooleans 2

#define VTTVersionString	L"6.35"

typedef enum { set, inquire, reset } Trinary;

typedef uint32_t UChar32; 

#ifndef F26Dot6
#define F26Dot6 int32_t
#endif
#define places6	6L
#define two6	0x80L
#define one6	0x40L
#define half6	0x20L
#define Round6(x) (((x) + half6) >> places6)

#define F16Dot16 int32_t

#define PixelOfPt(pt,dpi) (((pt)*(dpi) + 36)/72)

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
	#define Len(a) ((int32_t)(sizeof(a)/sizeof(a[0])))
#endif

#define Cap(ch) (L'a' <= (ch) && (ch) <= L'z' ? ((wchar_t)((short)(ch) - ((short)'a' - (short)'A'))) : (ch))

#ifndef DivRound
	#define DivRound(x,y)  ((x) < 0 ? (-((-(x) + (y)/2)/(y))) : (((x) + (y)/2)/(y)))
#endif

#ifndef DivFloor
	#define DivFloor(x,y)  ((x) < 0 ? (((x) - (y - 1))/(y)) : ((x)/(y))) // y > 0
#endif

#ifndef DivCeil
	#define DivCeil(x,y)   ((x) < 0 ? ((x)/(y)) : (((x) + (y - 1))/(y))) // y > 0
#endif

#ifndef pi
	#define pi (3.1415926535897932384626433832795) // (3.1415926535)
#endif

#ifndef Rad
	#define Rad(deg) ((deg)*pi/180)
#endif

#ifndef Deg
	#define Deg(rad) ((rad)*180/pi)
#endif

#define BULLET L"*"
#define BRK L"\n"
#define IntelOrMac(x,y) x
#define maxStdEditBufSize 65531 // don't push the limits, or else the standard edit control will choke on Win9x

#define MapSystemColor(color)	(Shade)((int32_t)(color) + systemColor)

#define LF L'\n'
#define CR L'\r'

typedef enum { modeCopy = 0, modeOr, modeXor, modeBic } Mode;
typedef enum { black = 0, blue, green, cyan, red, magenta, yellow, white, darkBlack, darkBlue, darkGreen, darkCyan, darkRed, darkMagenta, darkYellow, darkWhite, custom } Color;
typedef enum { solid = 0, darkGrey, grey, lightGrey, empty, horizontal, vertical, darkDiagonal, lightDiagonal, darkarrow, midarrow, lightarrow, systemColor } Shade;

#define iconWidth 32
#define iconHeight 32

#define	maxMenus 16
#define maxIcons 64

#define maxLineSize 0x100



#define PtoCStr CtoPStr
#define CtoPStr(str1, str2)	wcscpy(str2, str1)

long long DateTime(void);
void DateTimeStrg(wchar_t strg[]);






