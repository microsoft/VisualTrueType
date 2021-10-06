// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#define maxLineSize 0x100

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <codecvt>
#include <cstdint>
#include <deque>
#include <limits>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "opentypedefs.h"

#include "CvtManager.h"
#include "File.h"
#include "FixedMath.h"
#include "GUIDecorations.h"
#include "List.h"
#include "MathUtils.h"
#include "Memory.h"
#include "Platform.h"
#include "TMTParser.h"
#include "TTAssembler.h"
#include "TTEngine.h"
#include "TTFont.h"
#include "TTGenerator.h"
#include "TextBuffer.h"
#include "Variation.h"
#include "VariationInstance.h"
#include "VariationModels.h"

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
