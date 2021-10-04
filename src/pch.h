// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#define maxLineSize 0x100

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <assert.h>
#include <stdint.h>

#include <string>
#include <map>
#include <algorithm>
#include <limits>
#include <list>
#include <vector>
#include <deque>
#include <climits>
#include <codecvt>
#include <span>
#include <cmath>

#include "opentypedefs.h"
#include "Platform.h"
#include "FixedMath.h"
#include "MathUtils.h"
#include "List.h"
#include "Memory.h"
#include "File.h"
#include "TextBuffer.h"
#include "Variation.h"
#include "VariationModels.h"
#include "VariationInstance.h"
#include "TTFont.h"
#include "GUIDecorations.h"
#include "TTAssembler.h"
#include "TTEngine.h"
#include "TTGenerator.h"
#include "CvtManager.h"
#include "TMTParser.h"


#define STRCPYW wcscpy
#define STRCATW wcscat
#define STRLENW	wcslen
#define STRNCPYW wcsncpy
#define STRSTRW wcsstr
#define STRCHARW wcschr
#define STRCMPW wcscmp

#ifndef _WIN32
#define swprintf(wcs, ...) swprintf(wcs, 1024, __VA_ARGS__)
#define wprintf_s wprintf
#endif
