// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Area.h"

#ifndef	Min
#define Min(a,b)	((a) < (b) ? (a) : (b))
#endif
#ifndef	Max
#define Max(a,b)	((a) > (b) ? (a) : (b))
#endif

namespace Areas
{
	
	Area SetArea(long left, long top, long right, long bottom) {
		Area a;

		a.left = left;
		a.top = top;
		a.right = right;
		a.bottom = bottom;
		return a;
	} // SetArea

	Area SetMarquee(long left, long top, long right, long bottom) {
		Area a;

		a.left = Min(left, right);
		a.top = Min(top, bottom);
		a.right = Max(left, right);
		a.bottom = Max(top, bottom);
		return a;
	} // SetMarquee

	void OffsetArea(Area *a, long dx, long dy) {
		a->left += dx; a->right += dx;
		a->top += dy; a->bottom += dy;
	} // OffsetArea

	void ScaleArea(Area *a, long scale) {
		a->left *= scale; a->right *= scale;
		a->top *= scale; a->bottom *= scale;
	} // ScaleArea

	Area InsetArea(const Area *area, long dx, long dy) {
		Area inset;

		inset.left = area->left + dx; inset.right = Max(inset.left, area->right - dx);
		inset.top = area->top + dy; inset.bottom = Max(inset.top, area->bottom - dy);
		return inset;
	} // InsetArea

	Area UniteArea(const Area *a, const Area *b) {
		Area c;

		if (b->left >= b->right || b->top >= b->bottom) return *a; // don't include null areas
		if (a->left >= a->right || a->top >= a->bottom) return *b; // don't include null areas

		c.left = Min(a->left, b->left);
		c.top = Min(a->top, b->top);
		c.right = Max(a->right, b->right);
		c.bottom = Max(a->bottom, b->bottom);
		return c;
	} // UniteArea

	Area IntersectArea(const Area *a, const Area *b) {
		Area c;

		c.left = Max(a->left, b->left);
		c.top = Max(a->top, b->top);
		c.right = Min(a->right, b->right);
		c.bottom = Min(a->bottom, b->bottom);
		if (c.right < c.left) c.right = c.left;
		if (c.bottom < c.top) c.bottom = c.top;
		return c;
	} // IntersectArea

	long SubtractArea(const Area *a, const Area *b, Area part[]) { // returns #parts (0..4)
		long left, top, right, bottom, parts;

		left = a->left;  top = a->top;
		right = a->right; bottom = a->bottom;
		parts = 0;
		if (top < b->top) {
			part[parts].top = top;
			part[parts].left = left;
			part[parts].right = right;
			part[parts].bottom = Min(bottom, b->top);
			top = b->top;
			parts++;
		}
		if (bottom > b->bottom) {
			part[parts].bottom = bottom;
			part[parts].left = left;
			part[parts].right = right;
			part[parts].top = Max(top, b->bottom);
			bottom = b->bottom;
			parts++;
		}
		if (top >= bottom) return parts;

		if (left < b->left) {
			part[parts].left = left;
			part[parts].top = top;
			part[parts].bottom = bottom;
			part[parts].right = Min(right, b->left);
			left = b->left;
			parts++;
		}
		if (right > b->right) {
			part[parts].right = right;
			part[parts].top = top;
			part[parts].bottom = bottom;
			part[parts].left = Max(left, b->right);
			right = b->right;
			parts++;
		}
		return parts;
	} // SubtractArea

	bool SameArea(const Area *a, const Area *b) {
		return a->left == b->left && a->top == b->top && a->right == b->right && a->bottom == b->bottom;
	} // SameArea

	bool AreaIncludesPoint(const Area *a, long x, long y) {
		return a->left <= x && x <= a->right && a->top <= y && y <= a->bottom;
	} // AreaIncludesPoint

	bool AreaIntersectsArea(const Area *a, const Area *b) {
		return b->right > a->left && a->right > b->left && b->bottom > a->top && a->bottom > b->top;
	} // AreaIntersectsArea

	bool AreaIncludesArea(const Area *a, const Area *b) {
		return a->left <= b->left && b->right <= a->right && a->top <= b->top && b->bottom <= a->bottom;
	} // AreaIncludesArea

}

