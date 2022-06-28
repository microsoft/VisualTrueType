// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <cstdint>

namespace Areas
{
	typedef struct {
		int32_t left, top, right, bottom;
	} Area;	

	Area SetArea(int32_t left, int32_t top, int32_t right, int32_t bottom);
	Area SetMarquee(int32_t left, int32_t top, int32_t right, int32_t bottom);
	void OffsetArea(Area *a, int32_t dx, int32_t dy);
	void ScaleArea(Area *a, int32_t scale);
	Area InsetArea(const Area *area, int32_t dx, int32_t dy);
	Area UniteArea(const Area *a, const Area *b);
	Area IntersectArea(const Area *a, const Area *b);
	int32_t SubtractArea(const Area *a, const Area *b, Area part[]); // returns #parts (0..4)
	bool SameArea(const Area *a, const Area *b);
	bool AreaIncludesPoint(const Area *a, int32_t x, int32_t y);
	bool AreaIncludesArea(const Area *a, const Area *b);
	bool AreaIntersectsArea(const Area *a, const Area *b);
}
