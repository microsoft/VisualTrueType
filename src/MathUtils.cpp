// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "opentypedefs.h" // F26Dot6 etc.
#include "MathUtils.h"

Vector RoundFV(const F26Dot6Vector v) {
	Vector w;
	
	w.x = Round6(v.x);
	w.y = Round6(v.y);
	return w;
} // RoundFV

Vector AddV(const Vector a, const Vector b) {
	Vector c;
	
	c.x = a.x + b.x;
	c.y = a.y + b.y;
	return c;
} // AddV

Vector SubV(const Vector a, const Vector b) {
	Vector c;
	
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	return c;
} // SubV

F26Dot6Vector AddFV(const F26Dot6Vector a, const F26Dot6Vector b) {
	F26Dot6Vector c;
	
	c.x = a.x + b.x;
	c.y = a.y + b.y;
	return c;
} // AddFV

F26Dot6Vector SubFV(const F26Dot6Vector a, const F26Dot6Vector b) {
	F26Dot6Vector c;
	
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	return c;
} // SubFV

F26Dot6Vector ScaleFV(const F26Dot6Vector v, long scale) {
	F26Dot6Vector w;
	
	w.x = scale*v.x;
	w.y = scale*v.y;
	return w;
} // ScaleFV

F26Dot6Vector ScaleFVF(const F26Dot6Vector v, F26Dot6 scale) {
	F26Dot6Vector w;
	
	w.x = (scale*v.x + half6) >> places6;
	w.y = (scale*v.y + half6) >> places6;
	return w;
} // ScaleFVF

Vector ShlV(const Vector a, long by) {
	Vector b;
	
	b.x = a.x << by;
	b.y = a.y << by;
	return b;
} // ShlV

Vector ShrV(const Vector a, long by) {
	Vector b;
	
	b.x = a.x >> by;
	b.y = a.y >> by;
	return b;
} // ShrV


F26Dot6Vector ShlFV(const F26Dot6Vector a, long by) {
	F26Dot6Vector b;
	
	b.x = a.x << by;
	b.y = a.y << by;
	return b;
} // ShlFV

F26Dot6Vector ShrFV(const F26Dot6Vector a, long by) {
	F26Dot6Vector b;
	
	b.x = a.x >> by;
	b.y = a.y >> by;
	return b;
} // ShrFV


long DistV(const Vector a, const Vector b) {
	double dx,dy; // to avoid overflow...
	
	dx = a.x - b.x;
	dy = a.y - b.y;
	return Round(sqrt(dx*dx + dy*dy));
} // DistV

F26Dot6 DistFV(const F26Dot6Vector a, const F26Dot6Vector b) {
	double dx,dy; // to avoid overflow...
	
	dx = a.x - b.x;
	dy = a.y - b.y;
	return Round(sqrt(dx*dx + dy*dy));
} // DistFV

double DistRV(const RVector a, const RVector b) {
	double dx,dy;
	
	dx = a.x - b.x;
	dy = a.y - b.y;
	return sqrt(dx*dx + dy*dy);
} // DistRV

double LengthR(const RVector a) {
	return sqrt(a.x*a.x + a.y*a.y);
} // LengthR

long SqrDistV(const Vector a, const Vector b) {
	long dx,dy;
	
	dx = a.x - b.x;
	dy = a.y - b.y;
	return dx*dx + dy*dy;
} // SqrDistV

F26Dot6 SqrDistFV(const F26Dot6Vector a, const F26Dot6Vector b) {
	double dx,dy,dd;
	
	dx = a.x - b.x;
	dy = a.y - b.y;
	dd = (dx*dx + dy*dy)/one6;
	if (dd > 2147483647) return 2147483647;
	else if (dd < -2147483647) return -2147483647;
	else return Round(dd);
} // SqrDistFV

double SqrDistRV(const RVector a, const RVector b) {
	double dx,dy;
	
	dx = a.x - b.x;
	dy = a.y - b.y;
	return dx*dx + dy*dy;
} // SqrDistRV

F26Dot6Vector DirectionV(const Vector a, const Vector b) {
	double dx,dy,len; // to avoid overflow...
	F26Dot6Vector dir;
	
	dx = a.x - b.x;
	dy = a.y - b.y;
	len = sqrt(dx*dx + dy*dy);
	if (len > 0) {
		dir.x = Round(one6*dx/len);
		dir.y = Round(one6*dy/len);
	} else {
		dir.x = 0;
		dir.y = 0;
	}
	return dir;
} // DirectionV

RVector RDirectionV(const Vector a, const Vector b) {
	double len;
	RVector d;
	
	d.x = a.x - b.x;
	d.y = a.y - b.y;
	len = sqrt(d.x*d.x + d.y*d.y);
	if (len > 0) {
		d.x /= len;
		d.y /= len;
	} else {
		d.x = 0;
		d.y = 0;
	}
	return d;
} // RDirectionV

RVector RAvgDirectionV(const RVector a, const RVector b) {
	double len;
	RVector d;
	
	d.x = a.x + b.x;
	d.y = a.y + b.y;
	len = sqrt(d.x*d.x + d.y*d.y);
	if (len > 0) {
		d.x /= len;
		d.y /= len;
	} else {
		d.x = 0;
		d.y = 0;
	}
	return d;
} // RAvgDirectionV

F26Dot6Vector DirectionFV(const F26Dot6Vector a, const F26Dot6Vector b) {
	double dx,dy,len; // to avoid overflow...
	F26Dot6Vector dir;
	
	dx = a.x - b.x;
	dy = a.y - b.y;
	len = sqrt(dx*dx + dy*dy);
	if (len > 0) {
		dir.x = Round(one6*dx/len);
		dir.y = Round(one6*dy/len);
	} else {
		dir.x = 0;
		dir.y = 0;
	}
	return dir;
} // DirectionFV

long ScalProdV(const Vector a, const Vector b) {
	return a.x*b.x + a.y*b.y;
} // ScalProdV

long ScalProdP(const Vector a, const Vector A, const Vector b, const Vector B) { // 4-point form
	return (A.x-a.x)*(B.x-b.x) + (A.y-a.y)*(B.y-b.y);
} // ScalProdP

long VectProdV(const Vector a, const Vector b) { // z-component, 2-vector form
	return a.x*b.y - b.x*a.y;
} // VectProdV

long VectProdP(const Vector a, const Vector A, const Vector b, const Vector B) { // z-component, 4-point form
	return (A.x-a.x)*(B.y-b.y) - (B.x-b.x)*(A.y-a.y);
} // VectProdP


F26Dot6 ScalProdFV(const F26Dot6Vector a, const F26Dot6Vector b) {
	return (a.x*b.x + a.y*b.y + half6) >> places6;
} // ScalProdFV

F26Dot6 ScalProdFP(const F26Dot6Vector a, const F26Dot6Vector A, const F26Dot6Vector b, const F26Dot6Vector B) { // 4-point form
	return ((A.x-a.x)*(B.x-b.x) + (A.y-a.y)*(B.y-b.y) + half6) >> places6;
} // ScalProdFP

F26Dot6 VectProdFV(const F26Dot6Vector a, const F26Dot6Vector b) { // z-component, 2-vector form
	return (a.x*b.y - b.x*a.y + half6) >> places6;
} // VectProdFV

F26Dot6 VectProdFP(const F26Dot6Vector a, const F26Dot6Vector A, const F26Dot6Vector b, const F26Dot6Vector B) { // z-component, 4-point form
	return ((A.x-a.x)*(B.y-b.y) - (B.x-b.x)*(A.y-a.y) + half6) >> places6;
} // VectProdFP


double ScalProdRV(const RVector a, const RVector b) {
	return a.x*b.x + a.y*b.y;
} // ScalProdRV

double ScalProdRP(const RVector a, const RVector A, const RVector b, const RVector B) { // 4-point form
	return (A.x-a.x)*(B.x-b.x) + (A.y-a.y)*(B.y-b.y);
} // ScalProdRP

double VectProdRV(const RVector a, const RVector b) { // z-component, 2-vector form
	return a.x*b.y - b.x*a.y;
} // VectProdRV

double VectProdRP(const RVector a, const RVector A, const RVector b, const RVector B) { // z-component, 4-point form
	return (A.x-a.x)*(B.y-b.y) - (B.x-b.x)*(A.y-a.y);
} // VectProdRP

bool Collinear(const Vector v, const Vector p, const Vector V, Collinearity c) { // p on line through v and V?
	/***** see wether there is a t such that p = v + t*(V-v) *****/
	long n = p.x-v.x, N = Abs(n), d = V.x-v.x, D = Abs(d); // t = n/d
	
	if (n*(V.y-v.y) != d*(p.y-v.y)) return false; // not collinear
	switch (c) {
        case notOutside: return (Sgn(n) == Sgn(d) && 0 < N && N < D) || (p.x == v.x && p.y == v.y) || (p.x == V.x && p.y == V.y); // 0 ≤ t ≤ 1
		case inside:	 return Sgn(n) == Sgn(d) && 0 < N && N < D; // 0 < t < 1
		default:		 return true;
	}
} // Collinear
