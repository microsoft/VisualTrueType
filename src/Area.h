// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace Areas
{
	typedef struct {
		long left, top, right, bottom;
	} Area;	

	Area SetArea(long left, long top, long right, long bottom);
	Area SetMarquee(long left, long top, long right, long bottom);
	void OffsetArea(Area *a, long dx, long dy);
	void ScaleArea(Area *a, long scale);
	Area InsetArea(const Area *area, long dx, long dy);
	Area UniteArea(const Area *a, const Area *b);
	Area IntersectArea(const Area *a, const Area *b);
	long SubtractArea(const Area *a, const Area *b, Area part[]); // returns #parts (0..4)
	bool SameArea(const Area *a, const Area *b);
	bool AreaIncludesPoint(const Area *a, long x, long y);
	bool AreaIncludesArea(const Area *a, const Area *b);
	bool AreaIntersectsArea(const Area *a, const Area *b);
}
