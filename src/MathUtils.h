// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef MathUtils_Dot_h_Defined
#define MathUtils_Dot_h_Defined

// naming convention: functions operating on Vectors  have a name ending in V
//											 FVectors						FV
//											 RVectors						RV

#include <math.h> // floor
//#include "Platform.h" // F26Dot6 etc.

#define Round(x) ((long)floor(x + 1.0/2.0))

typedef struct { long x,y; } Vector;
typedef struct { F26Dot6 x,y; } F26Dot6Vector;
typedef struct { double x,y; } RVector;

typedef enum { collinear = 0, notOutside, inside } Collinearity;

Vector RoundFV(const F26Dot6Vector v);

Vector AddV(const Vector a, const Vector b);
Vector SubV(const Vector a, const Vector b);
F26Dot6Vector AddFV(const F26Dot6Vector a, const F26Dot6Vector b);
F26Dot6Vector SubFV(const F26Dot6Vector a, const F26Dot6Vector b);

F26Dot6Vector ScaleFV(const F26Dot6Vector v, long scale);
F26Dot6Vector ScaleFVF(const F26Dot6Vector v, F26Dot6 scale);

Vector ShlV(const Vector a, long by);
Vector ShrV(const Vector a, long by);
F26Dot6Vector ShlFV(const F26Dot6Vector a, long by);
F26Dot6Vector ShrFV(const F26Dot6Vector a, long by);

long DistV(const Vector a, const Vector b);
F26Dot6 DistFV(const F26Dot6Vector a, const F26Dot6Vector b);
double DistRV(const RVector a, const RVector b);

double LengthR(const RVector a);

long SqrDistV(const Vector a, const Vector b);
F26Dot6 SqrDistFV(const F26Dot6Vector a, const F26Dot6Vector b);
double SqrDistRV(const RVector a, const RVector b);

F26Dot6Vector DirectionV(const Vector a, const Vector b);
RVector RDirectionV(const Vector a, const Vector b);
RVector RAvgDirectionV(const RVector a, const RVector b);
F26Dot6Vector DirectionFV(const F26Dot6Vector a, const F26Dot6Vector b);

long ScalProdV(const Vector a, const Vector b);
long ScalProdP(const Vector a, const Vector A, const Vector b, const Vector B); // 4-point form
long VectProdV(const Vector a, const Vector b); // z-component, 2-vector form
long VectProdP(const Vector a, const Vector A, const Vector b, const Vector B); // z-component, 4-point form

F26Dot6 ScalProdFV(const F26Dot6Vector a, const F26Dot6Vector b);
F26Dot6 ScalProdFP(const F26Dot6Vector a, const F26Dot6Vector A, const F26Dot6Vector b, const F26Dot6Vector B); // 4-point form
F26Dot6 VectProdFV(const F26Dot6Vector a, const F26Dot6Vector b); // z-component, 2-vector form
F26Dot6 VectProdFP(const F26Dot6Vector a, const F26Dot6Vector A, const F26Dot6Vector b, const F26Dot6Vector B); // z-component, 4-point form

double ScalProdRV(const RVector a, const RVector b);
double ScalProdRP(const RVector a, const RVector A, const RVector b, const RVector B); // 4-point form
double VectProdRV(const RVector a, const RVector b); // z-component, 2-vector form
double VectProdRP(const RVector a, const RVector A, const RVector b, const RVector B); // z-component, 4-point form

bool Collinear(const Vector v, const Vector p, const Vector V, Collinearity c); // p on line through v and V?

bool IntersectI(long ax, long ay, long Ax, long Ay, long bx, long by, long Bx, long By, long* x, long* y);
bool IntersectF(F26Dot6 ax, F26Dot6 ay, F26Dot6 Ax, F26Dot6 Ay, F26Dot6 bx, F26Dot6 by, F26Dot6 Bx, F26Dot6 By, F26Dot6* x, F26Dot6* y);

void QuadraticEqn(double a, double b, double c, long* solutions, double* t1, double* t2);

void DropPerpToLineV(const Vector from, const Vector V0, const Vector V1, long* solutions, Vector foot[], F26Dot6Vector tangent[]);
void DropPerpToLineFV(const F26Dot6Vector from, const F26Dot6Vector V0, const F26Dot6Vector V1, long* solutions, F26Dot6Vector foot[], F26Dot6Vector tangent[]);
void DropPerpToBezierV(const Vector from, const Vector V0, const Vector V1, const Vector V2, long* solutions, Vector foot[], F26Dot6Vector tangent[]);
void DropPerpToBezierFV(const F26Dot6Vector from, const F26Dot6Vector V0, const F26Dot6Vector V1, const F26Dot6Vector V2, long* solutions, F26Dot6Vector foot[], F26Dot6Vector tangent[]);

#endif // MathUtils_Dot_h_Defined