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

#include "Area.h"
#include "opentypedefs.h"
#include "platform.h"
#include "FixedMath.h"
#include "mathutils.h"
#include "list.h"
#include "Memory.h"
#include "file.h"
#include "TextBuffer.h"
#include "Variation.h"
#include "VariationModels.h"
#include "VariationInstance.h"
#include "ttfont.h"
#include "guidecorations.h"
#include "ttassembler.h"
#include "ttengine.h"
#include "TTGenerator.h"
#include "cvtmanager.h"
#include "TMTParser.h"


#define STRCPYW wcscpy
#define STRCATW wcscat
#define STRLENW	wcslen
#define STRNCPYW wcsncpy
#define STRSTRW wcsstr
#define STRCHARW wcschr
#define STRCMPW wcscmp
