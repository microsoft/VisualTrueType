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

F26Dot6Vector ScaleFV(const F26Dot6Vector v, int scale) {
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

Vector ShlV(const Vector a, int by) {
	Vector b;
	
	b.x = a.x << by;
	b.y = a.y << by;
	return b;
} // ShlV

Vector ShrV(const Vector a, int by) {
	Vector b;
	
	b.x = a.x >> by;
	b.y = a.y >> by;
	return b;
} // ShrV


F26Dot6Vector ShlFV(const F26Dot6Vector a, int by) {
	F26Dot6Vector b;
	
	b.x = a.x << by;
	b.y = a.y << by;
	return b;
} // ShlFV

F26Dot6Vector ShrFV(const F26Dot6Vector a, int by) {
	F26Dot6Vector b;
	
	b.x = a.x >> by;
	b.y = a.y >> by;
	return b;
} // ShrFV


int DistV(const Vector a, const Vector b) {
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

int SqrDistV(const Vector a, const Vector b) {
	int dx,dy;
	
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

int ScalProdV(const Vector a, const Vector b) {
	return a.x*b.x + a.y*b.y;
} // ScalProdV

int ScalProdP(const Vector a, const Vector A, const Vector b, const Vector B) { // 4-point form
	return (A.x-a.x)*(B.x-b.x) + (A.y-a.y)*(B.y-b.y);
} // ScalProdP

int VectProdV(const Vector a, const Vector b) { // z-component, 2-vector form
	return a.x*b.y - b.x*a.y;
} // VectProdV

int VectProdP(const Vector a, const Vector A, const Vector b, const Vector B) { // z-component, 4-point form
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
	int n = p.x-v.x, N = Abs(n), d = V.x-v.x, D = Abs(d); // t = n/d
	
	if (n*(V.y-v.y) != d*(p.y-v.y)) return false; // not collinear
	switch (c) {
        case notOutside: return (Sgn(n) == Sgn(d) && 0 < N && N < D) || (p.x == v.x && p.y == v.y) || (p.x == V.x && p.y == V.y); // 0 ≤ t ≤ 1
		case inside:	 return Sgn(n) == Sgn(d) && 0 < N && N < D; // 0 < t < 1
		default:		 return true;
	}
} // Collinear

/*****
	Intersection routines all work as follows:
	
	the lines a and b, respectively, go from (ax,ay) to (Ax,Ay) and from (bx,by) to (Bx,By)
	
	if they are to intersect, there must be parameters t1 and t2 such that
	
	ax + t1*(Ax - ax) = bx + t2*(Bx - bx)
	ay + t1*(Ay - ay) = by + t2*(By - by)
	
	rearranging terms, this yields a system of two linear equations with two unknowns t1 and t2
	
	ax - bx = t1*(ax - Ax) + t2*(Bx - bx)
	ay - by = t1*(ay - Ay) + t2*(By - by)
	
	or
	
	b1 = a11*t1 + a12*t2
	b2 = a21*t1 + a22*t2
	
	the inverse of the matrix
	
	a11 a12
	a21 a22
	
	is
	
	 a22/d -a12/d
	-a21/d  a11/d
	
	with the determinant d = a11*a22 - a12*a21
	
	if the determinant is 0, then the two lines are parallel or anti-parallel, in which case there is
	no solution (other than taking the average of the two lines' start points), else
	
	t1 = ( a22*b1 - a12*b2)/d;
	t2 = (-a21*b1 + a11*b2)/d;
	
	substituting t1 and t2 into the above equation yields the desired intersection point (taking the
	average of the two solutions, in hopes that this evens out numerical effects)
*****/

bool IntersectI(int ax, int ay, int Ax, int Ay, int bx, int by, int Bx, int By, int* x, int* y) {
	int a11,a12,a21,a22,b1,b2,t1,t2,det;
	
	b1 = ax - bx; a11 = ax - Ax; a12 = Bx - bx;
	b2 = ay - by; a21 = ay - Ay; a22 = By - by;
	det = a11*a22 - a12*a21;
	
	if (det == 0) {
		*x = (ax + bx + 1)/2;
		*y = (ay + by + 1)/2;
		return false;
	} else {
		t1 = a22*b1 - a12*b2;
		t2 = a11*b2 - a21*b1;
	//	*x = (ax*det - t1*a11 + bx*det + t2*a12 + det)/(2*det);
	//	*y = (ay*det - t1*a21 + by*det + t2*a22 + det)/(2*det);
		if (ax == Ax) *x = ax; else if (bx == Bx) *x = bx; else *x = (ax*det - t1*a11 + bx*det + t2*a12 + det)/(2*det); // try to fight the
		if (ay == Ay) *y = ay; else if (by == By) *y = by; else *y = (ay*det - t1*a21 + by*det + t2*a22 + det)/(2*det); // rounding beast...
		return true;
	}
} // IntersectI

bool IntersectF(F26Dot6 ax, F26Dot6 ay, F26Dot6 Ax, F26Dot6 Ay, F26Dot6 bx, F26Dot6 by, F26Dot6 Bx, F26Dot6 By, F26Dot6* x, F26Dot6* y) {
// cf comments above
/*****
	F26Dot6 a11,a12,a21,a22,b1,b2,t1,t2,det;
	
	b1 = ax - bx; a11 = ax - Ax; a12 = Bx - bx;
	b2 = ay - by; a21 = ay - Ay; a22 = By - by;
	det = a11*a22 - a12*a21 + half6 >> places6;
	
	if (det == 0) {
		*x = ax + bx + 1 >> 1;
		*y = ay + by + 1 >> 1;
	} else {
		t1 = a22*b1 - a12*b2 + half6 >> places6;
		t2 = a11*b2 - a21*b1 + half6 >> places6;
		*x = (ax*det - t1*a11 + bx*det + t2*a12 + det)/(2*det);
		*y = (ay*det - t1*a21 + by*det + t2*a22 + det)/(2*det);
	};
*****/
	double a11,a12,a21,a22,b1,b2,t1,t2,det;
	
	b1 = ax - bx; a11 = ax - Ax; a12 = Bx - bx;
	b2 = ay - by; a21 = ay - Ay; a22 = By - by;
	det = a11*a22 - a12*a21;
	
	if (det == 0) {
		*x = Round((ax + bx)/2);
		*y = Round((ay + by)/2);
		return false;
	} else {
		t1 = a22*b1 - a12*b2;
		t2 = a11*b2 - a21*b1;
		*x = Round((ax*det - t1*a11 + bx*det + t2*a12)/(2*det));
		*y = Round((ay*det - t1*a21 + by*det + t2*a22)/(2*det));
		return true;
	}
} // IntersectF

void QuadraticEqn(double a, double b, double c, int* solutions, double* t1, double* t2) {
	// the usual method for solving quadratic equations
	double radicand,root;
	
	if (a == 0) {
		if (b == 0) {
			*solutions = 0;
		} else {
			*solutions = 1;
			*t1 = -c/b;
		}
	} else {
		radicand = b*b - 4*a*c;
		if (radicand < 0)  {
			*solutions = 0;
		} else if (radicand == 0) {
			*solutions = 1;
			*t1 = -b/(2*a);
		} else {
			*solutions = 2;
			root = sqrt(radicand);
			*t1 = (-b+root)/(2*a);
			*t2 = -(b+root)/(2*a);
		}
	}
} // QuadraticEqn

#define epsilon 1.0e-12

void CubicEqn(double a, double b, double c, double d, int* solutions, double* t1, double* t2, double* t3); // not general enough that I'd want to make it public
void CubicEqn(double a, double b, double c, double d, int* solutions, double* t1, double* t2, double* t3) {
	// a cubic equation always has at least one solution, Newton was unstable, therefore find solution by interval halfing
    double f = 0,low = 0,mid = 0,high = 0;
	
	if (a < 0) {
		a = -a; b = -b; c = -c; d = -d;
	}
	
	low = -1;
	f = ((a*low + b)*low + c)*low + d;
	while (f > 0) {
		low = low*2;
		f = ((a*low + b)*low + c)*low + d;
	}
	high = 1;
	f = ((a*high + b)*high + c)*high + d;
	while (f < 0) {
		high = high*2;
		f = ((a*high + b)*high + c)*high + d;
	}
	
	// now f(high) >= 0 && f(low) <= 0, hence there must be a zero inbetween
	while (high - low > epsilon) {
		mid = (low + high)/2;
		f = ((a*mid + b)*mid + c)*mid + d;
		if (f <= 0) low = mid;
		if (f >= 0) high = mid;
	}
	*t1 = mid;
	
	//	now there are possibly upto two solutions left. To find them, divide the
	//	cubic polynomial by t - t1:
	//	
	//	a*t^3 + b*t^2 + c*t + d : t - t1 = a*t^2 + (a*t1 + b)*t + (a*t1^2 + b*t1 + c)
	
	QuadraticEqn(a,a*(*t1) + b,(a*(*t1) + b)*(*t1) + c,solutions,t2,t3);
	
	if (*solutions == 2) {
		if (*t2 != *t1 && *t3 != *t1) (*solutions)++;
		else if (*t2 == *t1) *t2 = *t3;
	} else if (*solutions == 1) {
		if (*t2 != *t1) (*solutions)++;
	} else {
		(*solutions)++;
	}
}

void DropPerpToLineV(const Vector from, const Vector V0, const Vector V1, int* solutions, Vector foot[], F26Dot6Vector tangent[]) {
	Vector V,W;
	int vw,ww;
	double len,zero;
	
	*solutions = 0;
	if (V0.x != V1.x || V0.y != V1.y) { // there is at most one solution
		V.x = from.x - V0.x; W.x = V1.x - V0.x;
		V.y = from.y - V0.y; W.y = V1.y - V0.y;
		vw = ScalProdV(V,W);
		ww = ScalProdV(W,W);
		if (0 <= vw && vw <= ww) {
			len = sqrt((double)ww);
			zero = (double)vw/(double)ww;
			foot[*solutions].x = Round(V0.x + zero*W.x);
			foot[*solutions].y = Round(V0.y + zero*W.y);
			tangent[*solutions].x = Round(one6*W.x/len);
			tangent[*solutions].y = Round(one6*W.y/len);
			(*solutions)++;
		}
	}
} // DropPerpToLineV

void DropPerpToLineFV(const F26Dot6Vector from, const F26Dot6Vector V0, const F26Dot6Vector V1, int* solutions, F26Dot6Vector foot[], F26Dot6Vector tangent[]) {
	RVector V,W;
	double vw,ww,len,zero;
	
	*solutions = 0;
	if (V0.x != V1.x || V0.y != V1.y) { // there is at most one solution
		V.x = from.x - V0.x; W.x = V1.x - V0.x;
		V.y = from.y - V0.y; W.y = V1.y - V0.y;
		vw = V.x*W.x + V.y*W.y;
		ww = W.x*W.x + W.y*W.y;
		if (0 <= vw && vw <= ww) {
			len = sqrt(ww);
			zero = vw/ww;
			foot[*solutions].x = Round(V0.x + zero*W.x);
			foot[*solutions].y = Round(V0.y + zero*W.y);
			tangent[*solutions].x = Round(one6*W.x/len);
			tangent[*solutions].y = Round(one6*W.y/len);
			(*solutions)++;
		}
	}
} // DropPerpToLineFV

void DropPerpToBezierV(const Vector from, const Vector V0, const Vector V1, const Vector V2, int* solutions, Vector foot[], F26Dot6Vector tangent[]) {
	RVector A,B,C,D,E;
	int i,zeroes;
	double t[3],zero,dx,dy,len;
	
	C.x = V0.x - 2*V1.x + V2.x; D.x = 2*(V1.x - V0.x); E.x = V0.x - from.x; A.x = 2*C.x; B.x = D.x;
	C.y = V0.y - 2*V1.y + V2.y; D.y = 2*(V1.y - V0.y); E.y = V0.y - from.y; A.y = 2*C.y; B.y = D.y;
	CubicEqn(A.x*C.x + A.y*C.y,
			 A.x*D.x + B.x*C.x + A.y*D.y + B.y*C.y,
			 A.x*E.x + B.x*D.x + A.y*E.y + B.y*D.y,
			 B.x*E.x + B.y*E.y,
			 &zeroes,&t[0],&t[1],&t[2]);
	*solutions = 0;
	for (i = 0; i < zeroes; i++) {
		zero = t[i];
		if (0 <= zero && zero <= 1) {
			foot[*solutions].x = Round((C.x*zero + D.x)*zero + V0.x);
			foot[*solutions].y = Round((C.y*zero + D.y)*zero + V0.y);
			dx = 2*C.x*zero + D.x;
			dy = 2*C.y*zero + D.y;
			len = sqrt(dx*dx + dy*dy);
			tangent[*solutions].x = Round(one6*dx/len);
			tangent[*solutions].y = Round(one6*dy/len);
			(*solutions)++;
		}
	}
} // DropPerpToBezierV

void DropPerpToBezierFV(const F26Dot6Vector from, const F26Dot6Vector V0, const F26Dot6Vector V1, const F26Dot6Vector V2, int* solutions, F26Dot6Vector foot[], F26Dot6Vector tangent[]) {
	RVector A,B,C,D,E;
	int i,zeroes;
	double t[3],zero,dx,dy,len;
	
	C.x = V0.x - 2*V1.x + V2.x; D.x = 2*(V1.x - V0.x); E.x = V0.x - from.x; A.x = 2*C.x; B.x = D.x;
	C.y = V0.y - 2*V1.y + V2.y; D.y = 2*(V1.y - V0.y); E.y = V0.y - from.y; A.y = 2*C.y; B.y = D.y;
	CubicEqn(A.x*C.x + A.y*C.y,
			 A.x*D.x + B.x*C.x + A.y*D.y + B.y*C.y,
			 A.x*E.x + B.x*D.x + A.y*E.y + B.y*D.y,
			 B.x*E.x + B.y*E.y,
			 &zeroes,&t[0],&t[1],&t[2]);
	*solutions = 0;
	for (i = 0; i < zeroes; i++) {
		zero = t[i];
		if (0 <= zero && zero <= 1) {
			foot[*solutions].x = Round((C.x*zero + D.x)*zero + V0.x);
			foot[*solutions].y = Round((C.y*zero + D.y)*zero + V0.y);
			dx = 2*C.x*zero + D.x;
			dy = 2*C.y*zero + D.y;
			len = sqrt(dx*dx + dy*dy);
			tangent[*solutions].x = Round(one6*dx/len);
			tangent[*solutions].y = Round(one6*dy/len);
			(*solutions)++;
		}
	}
} // DropPerpToBezierFV

