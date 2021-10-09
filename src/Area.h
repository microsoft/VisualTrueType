// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace Areas
{
	typedef struct {
		int left, top, right, bottom;
	} Area;	

	Area SetArea(int left, int top, int right, int bottom);
	Area SetMarquee(int left, int top, int right, int bottom);
	void OffsetArea(Area *a, int dx, int dy);
	void ScaleArea(Area *a, int scale);
	Area InsetArea(const Area *area, int dx, int dy);
	Area UniteArea(const Area *a, const Area *b);
	Area IntersectArea(const Area *a, const Area *b);
	int SubtractArea(const Area *a, const Area *b, Area part[]); // returns #parts (0..4)
	bool SameArea(const Area *a, const Area *b);
	bool AreaIncludesPoint(const Area *a, int x, int y);
	bool AreaIncludesArea(const Area *a, const Area *b);
	bool AreaIntersectsArea(const Area *a, const Area *b);
}
