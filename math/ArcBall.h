#pragma once
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

typedef	struct {float x, y, z, w;} Quat;
enum QuatPart {X, Y, Z,	W, QuatLen};
typedef	Quat HVect;
typedef	float HMatrix[QuatLen][QuatLen];

Quat qOne = {0,	0, 0, 1};

int ArcBall_mouseButton = -1 ;
int ArcBall_PrevZoomCoord = 0 ;


typedef	enum AxisSet{NoAxes, CameraAxes, BodyAxes, OtherAxes, NSets} AxisSet;
typedef	float *ConstraintSet;
typedef	struct {
	HVect center;
	double radius;
	Quat qNow, qDown, qDrag;
	HVect vNow,	vDown, vFrom, vTo, vrFrom, vrTo;
	HMatrix mNow, mDown;
	int	showResult, dragging;
	ConstraintSet sets[NSets];
	int	setSizes[NSets];
	AxisSet axisSet;
	int	axisIndex;
	HMatrix userAxes;
} BallData;

HMatrix	mId = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
float otherAxis[][4] = {{(float)-0.48, (float)0.80, (float)0.36, (float)1.0}};


/* Return quaternion product qL	* qR.  Note: order is important!
 * To combine rotations, use the product Mul(qSecond, qFirst),
 * which gives the effect of rotating by qFirst	then qSecond. */
Quat Qt_Mul(Quat qL, Quat qR)
{
    Quat qq;
    qq.w = qL.w*qR.w - qL.x*qR.x - qL.y*qR.y - qL.z*qR.z;
    qq.x = qL.w*qR.x + qL.x*qR.w + qL.y*qR.z - qL.z*qR.y;
    qq.y = qL.w*qR.y + qL.y*qR.w + qL.z*qR.x - qL.x*qR.z;
    qq.z = qL.w*qR.z + qL.z*qR.w + qL.x*qR.y - qL.y*qR.x;
    return (qq);
}

Quat *Qt_ToQuat(HMatrix m, Quat *out)
{
	double e[4];
    double tr,s;
    int	i,j,k;

    tr = m[0][0]+m[1][1]+m[2][2];
    if (tr > 0.0) {
	s = sqrt(tr+1.0);
	e[3] = s*0.5;
	s = 0.5/s;

	e[0] = (m[1][2]-m[2][1])*s;
	e[1] = (m[2][0]-m[0][2])*s;
	e[2] = (m[0][1]-m[1][0])*s;
    }
    else {
	i = 0;
	if (m[1][1] > m[0][0]) i = 1;
	if (m[2][2] > m[i][i]) i = 2;
	j = (i+1) % 3; k = (j+1) % 3;

	s = sqrt((m[i][i]-(m[j][j]+m[k][k])) + 1.0);
	e[i] = s*0.5;
	s = 0.5/s;
	e[j] = (m[i][j]	+ m[j][i])*s;
	e[k] = (m[i][k]	+ m[k][i])*s;
	e[3] = (m[j][k]	- m[k][j])*s;
    }

	out->x = (float)e[0];
	out->y = (float)e[1];
	out->z = (float)e[2];
	out->w = (float)e[3];

    return (out);

}

/* Construct rotation matrix from (possibly non-unit) quaternion.
 * Assumes matrix is used to multiply column vector on the left:
 * vnew	= mat vold.  Works correctly for right-handed coordinate system
 * and right-handed rotations. */
void Qt_ToMatrix(Quat q, HMatrix out)
{
    double Nq =	q.x*q.x	+ q.y*q.y + q.z*q.z + q.w*q.w;
    double s = (Nq > 0.0) ? (2.0 / Nq) : 0.0;
    double xs =	q.x*s,	      ys = q.y*s,	  zs = q.z*s;
    double wx =	q.w*xs,	      wy = q.w*ys,	  wz = q.w*zs;
    double xx =	q.x*xs,	      xy = q.x*ys,	  xz = q.x*zs;
    double yy =	q.y*ys,	      yz = q.y*zs,	  zz = q.z*zs;
    out[X][X] =	(float)1.0 - (float)(yy	+ zz); out[Y][X] = (float)(xy +	wz); out[Z][X] = (float)(xz - wy);
    out[X][Y] =	(float)(xy - wz); out[Y][Y] = (float)1.0 - (float)(xx +	zz); out[Z][Y] = (float)(yz + wx);
    out[X][Z] =	(float)(xz + wy); out[Y][Z] = (float)(yz - wx);	out[Z][Z] = (float)1.0 - (float)(xx + yy);
    out[X][W] =	out[Y][W] = out[Z][W] =	out[W][X] = out[W][Y] =	out[W][Z] = 0.0;
    out[W][W] =	1.0;
}

/* Return conjugate of quaternion. */
Quat Qt_Conj(Quat q)
{
    Quat qq;
    qq.x = -q.x; qq.y =	-q.y; qq.z = -q.z; qq.w	= q.w;
    return (qq);
}

/* Return vector formed	from components	*/
HVect V3_(float	x, float y, float z)
{
    HVect v;
    v.x	= x; v.y = y; v.z = z; v.w = 0;
    return (v);
}

/* Return norm of v, defined as	sum of squares of components */
float V3_Norm(HVect v)
{
    return ( v.x*v.x + v.y*v.y + v.z*v.z );
}

/* Return unit magnitude vector	in direction of	v */
HVect V3_Unit(HVect v)
{
    static HVect u = {0, 0, 0, 0};
    float vlen = (float)sqrt(V3_Norm(v));
    if (vlen !=	0.0) {
	u.x = v.x/vlen;	u.y = v.y/vlen;	u.z = v.z/vlen;
    }
    return (u);
}

/* Return version of v scaled by s */
HVect V3_Scale(HVect v,	float s)
{
    HVect u;
    u.x	= s*v.x; u.y = s*v.y; u.z = s*v.z; u.w = v.w;
    return (u);
}

/* Return negative of v	*/
HVect V3_Negate(HVect v)
{
    static HVect u = {0, 0, 0, 0};
    u.x	= -v.x;	u.y = -v.y; u.z	= -v.z;
    return (u);
}

/* Return sum of v1 and	v2 */
HVect V3_Add(HVect v1, HVect v2)
{
    static HVect v = {0, 0, 0, 0};
    v.x	= v1.x+v2.x; v.y = v1.y+v2.y; v.z = v1.z+v2.z;
    return (v);
}

/* Return difference of	v1 minus v2 */
HVect V3_Sub(HVect v1, HVect v2)
{
    static HVect v = {0, 0, 0, 0};
    v.x	= v1.x-v2.x; v.y = v1.y-v2.y; v.z = v1.z-v2.z;
    return (v);
}

/* Halve arc between unit vectors v0 and v1. */
HVect V3_Bisect(HVect v0, HVect	v1)
{
    HVect v = {0, 0, 0,	0};
    float Nv;
    v =	V3_Add(v0, v1);
    Nv = V3_Norm(v);
    if (Nv < 1.0e-5) {
	v = V3_(0, 0, 1);
    } else {
	v = V3_Scale(v,	(float)1/(float)sqrt(Nv));
    }
    return (v);
}

/* Return dot product of v1 and	v2 */
float V3_Dot(HVect v1, HVect v2)
{
    return (v1.x*v2.x +	v1.y*v2.y + v1.z*v2.z);
}


/* Return cross	product, v1 x v2 */
HVect V3_Cross(HVect v1, HVect v2)
{
    static HVect v = {0, 0, 0, 0};
    v.x	= v1.y*v2.z-v1.z*v2.y;
    v.y	= v1.z*v2.x-v1.x*v2.z;
    v.z	= v1.x*v2.y-v1.y*v2.x;
    return (v);
}




/* Convert window coordinates to sphere	coordinates. */
HVect MouseOnSphere(HVect mouse, HVect ballCenter, double ballRadius)
{
    HVect ballMouse;
    register double mag;
    ballMouse.x	= (mouse.x - ballCenter.x) / (float)ballRadius;
    ballMouse.y	= (mouse.y - ballCenter.y) / (float)ballRadius;
    mag	= ballMouse.x*ballMouse.x + ballMouse.y*ballMouse.y;
    if (mag > 1.0) {
	register double	scale =	1.0/sqrt(mag);
	ballMouse.x *= (float)scale; ballMouse.y *= (float)scale;
	ballMouse.z = 0.0;
    } else {
	ballMouse.z = (float)sqrt(1 - mag);
    }
    ballMouse.w	= 0.0;
    return (ballMouse);
}

/* Construct a unit quaternion from two	points on unit sphere */
Quat Qt_FromBallPoints(HVect from, HVect to)
{
    Quat qu;
    qu.x = from.y*to.z - from.z*to.y;
    qu.y = from.z*to.x - from.x*to.z;
    qu.z = from.x*to.y - from.y*to.x;
    qu.w = from.x*to.x + from.y*to.y + from.z*to.z;
    return (qu);
}

/* Convert a unit quaternion to	two points on unit sphere */
void Qt_ToBallPoints(Quat q, HVect *arcFrom, HVect *arcTo)
{
    double s;
    s =	sqrt(q.x*q.x + q.y*q.y);
    if (s == 0.0) {
	*arcFrom = V3_(0.0, 1.0, 0.0);
    } else {
	*arcFrom = V3_((float)(-q.y/s),	(float)(q.x/s),	(float)0.0);
    }
    arcTo->x = q.w*arcFrom->x -	q.z*arcFrom->y;
    arcTo->y = q.w*arcFrom->y +	q.z*arcFrom->x;
    arcTo->z = q.x*arcFrom->y -	q.y*arcFrom->x;
    if (q.w < 0.0) *arcFrom = V3_(-arcFrom->x, -arcFrom->y, 0.0);
}

/* Force sphere	point to be perpendicular to axis. */
HVect ConstrainToAxis(HVect loose, HVect axis)
{
    HVect onPlane;
    register float norm;
    onPlane = V3_Sub(loose, V3_Scale(axis, V3_Dot(axis,	loose)));
    norm = V3_Norm(onPlane);
    if (norm > 0.0) {
	if (onPlane.z <	0.0) onPlane = V3_Negate(onPlane);
	return ( V3_Scale(onPlane, 1/(float)sqrt(norm))	);
    } /* else drop through */
    if (axis.z == 1) {
	onPlane	= V3_(1.0, 0.0,	0.0);
    } else {
	onPlane	= V3_Unit(V3_(-axis.y, axis.x, 0.0));
    }
    return (onPlane);
}

/* Find	the index of nearest arc of axis set. */
int NearestConstraintAxis(HVect	loose, HVect *axes, int	nAxes)
{
    HVect onPlane;
    register float max,	dot;
    register int i, nearest;
    max	= -1; nearest =	0;
    for	(i=0; i<nAxes; i++) {
	onPlane	= ConstrainToAxis(loose, axes[i]);
	dot = V3_Dot(onPlane, loose);
	if (dot>max) {
	    max	= dot; nearest = i;
	}
    }
    return (nearest);
}


/* Establish reasonable	initial	values for controller. */
void Ball_Init(BallData	*ball, float *init_matrix=NULL )
{
    int	i;
    ball->center = qOne;
    ball->radius = 1.0;
    ball->vDown	= ball->vNow = qOne;
    ball->qDown	= ball->qNow = qOne;
    for	(i=15; i>=0; i--) {
		if (init_matrix	== NULL) 
			((float *)ball->mNow)[i] = ((float *)ball->mDown)[i]	= ((float *)mId)[i];
		else 
			((float *)ball->mNow)[i] = ((float *)ball->mDown)[i]	= init_matrix[i];
		
	}

	// We must also set the qNow quaternion to correspond with the 
	// given initial rotation matrix.
	if (init_matrix) {
		Qt_ToQuat(ball->mNow,&(ball->qNow));
		ball->qDown = ball->qNow;
	}

	ball->showResult = ball->dragging =	false;
    ball->axisSet = NoAxes;
    ball->sets[CameraAxes] = mId[X]; ball->setSizes[CameraAxes]	= 3;
    ball->sets[BodyAxes] = ball->mDown[X]; ball->setSizes[BodyAxes] = 3;
    ball->sets[OtherAxes] = otherAxis[X]; ball->setSizes[OtherAxes] = 1;
}

/* Set the center and size of the controller. */
void Ball_Place(BallData *ball,	HVect center, double radius)
{
    ball->center = center;
    ball->radius = radius;
}

/* Incorporate new mouse position. */
void Ball_Mouse(BallData *ball,	HVect vNow)
{
    ball->vNow = vNow;
}

/* Choose a constraint set, or none. */
void Ball_UseSet(BallData *ball, AxisSet axisSet)
{
    if (!ball->dragging) ball->axisSet = axisSet;
}

/* Set the OtherAxes for constraints. */
void Ball_SetOtherAxes(BallData	*ball, HMatrix conAxis)
{
     for (int i=0; i < 4; i++)
	 for (int j=0; j < 4; j++)
		ball->userAxes[i][j] = conAxis[i][j];
     ball->sets[OtherAxes] = ball->userAxes[X];
     ball->setSizes[OtherAxes] = 3;
}

/* Begin drawing arc for all drags combined. */
void Ball_ShowResult(BallData *ball)
{
    ball->showResult = true;
}

/* Stop	drawing	arc for	all drags combined. */
void Ball_HideResult(BallData *ball)
{
    ball->showResult = false;
}

/* Using vDown,	vNow, dragging,	and axisSet, compute rotation etc. */
void Ball_Update(BallData *ball)
{
    int	setSize	= ball->setSizes[ball->axisSet];
    HVect *set = (HVect	*)(ball->sets[ball->axisSet]);
    ball->vFrom	= MouseOnSphere(ball->vDown, ball->center, ball->radius);
    ball->vTo =	MouseOnSphere(ball->vNow, ball->center,	ball->radius);
    if (ball->dragging)	{
	if (ball->axisSet!=NoAxes) {
	    ball->vFrom	= ConstrainToAxis(ball->vFrom, set[ball->axisIndex]);
	    ball->vTo =	ConstrainToAxis(ball->vTo, set[ball->axisIndex]);
	}
	ball->qDrag = Qt_FromBallPoints(ball->vFrom, ball->vTo);
	ball->qNow = Qt_Mul(ball->qDrag, ball->qDown);
    } else {
	if (ball->axisSet!=NoAxes) {
	    ball->axisIndex = NearestConstraintAxis(ball->vTo, set, setSize);
	}
    }
    Qt_ToBallPoints(ball->qDown, &ball->vrFrom,	&ball->vrTo);
    Qt_ToMatrix(Qt_Conj(ball->qNow), ball->mNow); /* Gives transpose for GL. */
}

/* Return rotation matrix defined by controller	use. */
void Ball_Value(BallData *ball,	HMatrix	mNow)
{
    int	i;
    for	(i=15; i>=0; i--) ((float *)mNow)[i] = ((float *)ball->mNow)[i];
}

/* Return quaternion defined by controller use. */
void Ball_Quat(BallData *ball, float qNow[4])
{
	qNow[0] = ball->qNow.x;
	qNow[1] = ball->qNow.y;
	qNow[2] = ball->qNow.z;
	qNow[3] = ball->qNow.w;
}

/* Begin drag sequence.	*/
void Ball_BeginDrag(BallData *ball)
{
    ball->dragging = true;
    ball->vDown	= ball->vNow;
}

/* Begin drag sequence.	*/
void Ball_BeginDragReset(BallData *ball)
{
    int	i;

    ball->dragging = true;
    ball->vDown	= ball->vNow;

    // Set to Identity
    ball->qDown	= ball->qNow = qOne;
    for	(i=15; i>=0; i--)
       ((float *)ball->mNow)[i]	= ((float *)ball->mDown)[i] = ((float *)mId)[i];
}

/* Stop	drag sequence. */
void Ball_EndDrag(BallData *ball)
{
    int	i;
    ball->dragging = false;
    ball->qDown	= ball->qNow;
    for	(i=15; i>=0; i--)
	((float	*)ball->mDown)[i] = ((float *)ball->mNow)[i];
}

//#define	LG_NSEGS 4
//#define	NSEGS (1<<LG_NSEGS)
//#define	RIMCOLOR()    glColor3f(0.0f, 1.0f, 1.0f);
//#define	FARCOLOR()    glColor3f(0.8f, 0.5f, 0.0f);
//#define	NEARCOLOR()   glColor3f(1.0f, 0.8f, 0.0f);
//#define	DRAGCOLOR()   glColor3f(0.0f, 1.0f, 1.0f);
//#define	RESCOLOR()    glColor3f(0.8f, 0.0f, 0.0f);
//void Ball_Draw(BallData	*ball)		//Draw	the controller with all	its arcs.
//{
//    float r = (float)ball->radius;
//    float x = ball->center.x;
//    float y = ball->center.y;
//
//	glPushAttrib(GL_ENABLE_BIT);
//	glDisable(GL_LIGHTING);
//
//    glMatrixMode(GL_PROJECTION);
//    glPushMatrix();
//    glLoadIdentity();
//
//    // Must use	near and far clip planes at -1.0 and 1.0 or
//    // risk clipping out the circle and	arcs drawn.
//    glOrtho(-1.0,1.0,-1.0,1.0,-1.0,1.0);
//
//    glMatrixMode(GL_MODELVIEW);
//    glPushMatrix();
//    glLoadIdentity();
//    glScalef(r,r,r);
//    glTranslatef(x/r,y/r,0.0); // Scale	for radius adjustment
//
//    // Allows arcball constraints to be	shown properly over arcball
//    glDisable(GL_DEPTH_TEST);
//
//    RIMCOLOR();
//    glBegin(GL_LINE_LOOP);
//    for	(int i=0; i < 36; i++)
//       glVertex3f(cosf((float)i*(float)(2.0*M_PI/36.0)),
//		  sinf((float)i*(float)(2.0*M_PI/36.0)),0.0);
//    glEnd();
//
//    Ball_DrawResultArc(ball);
//    Ball_DrawConstraints(ball);
//    Ball_DrawDragArc(ball);
//
//    glEnable(GL_DEPTH_TEST);
//
//    glPopMatrix();
//    glMatrixMode(GL_PROJECTION);
//    glPopMatrix();
//	glPopAttrib();
//    glMatrixMode(GL_MODELVIEW);
//}
//
///* Draw	an arc defined by its ends. */
//void DrawAnyArc(HVect vFrom, HVect vTo)
//{
//    int	i;
//    HVect pts[NSEGS+1];
//    double dot;
//    pts[0] = vFrom;
//    pts[1] = pts[NSEGS]	= vTo;
//    for	(i=0; i<LG_NSEGS; i++) pts[1] =	V3_Bisect(pts[0], pts[1]);
//    dot	= 2.0*V3_Dot(pts[0], pts[1]);
//    for	(i=2; i<NSEGS; i++) {
//	pts[i] = V3_Sub(V3_Scale(pts[i-1], (float)dot),	pts[i-2]);
//    }
//    glBegin(GL_LINE_STRIP);
//    for	(i=0; i<=NSEGS;	i++) {
//	   glColor3f((float)i/NSEGS,(float)i/NSEGS,(float)i/NSEGS);
//	   glVertex3fv((float *)&pts[i]);
//	}
//    glEnd();
//}
//
///* Draw	the arc	of a semi-circle defined by its	axis. */
//void DrawHalfArc(HVect n)
//{
//    HVect p, m;
//    p.z	= 0;
//    if (n.z != 1.0) {
//	p.x = n.y; p.y = -n.x;
//	p = V3_Unit(p);
//    } else {
//	p.x = 0; p.y = 1;
//    }
//    m =	V3_Cross(p, n);
//    DrawAnyArc(p, m);
//    DrawAnyArc(m, V3_Negate(p));
//}
//
///* Draw	all constraint arcs. */
//void Ball_DrawConstraints(BallData *ball)
//{
//    ConstraintSet set;
//    HVect axis;
//    int	i, axisI, setSize = ball->setSizes[ball->axisSet];
//    if (ball->axisSet==NoAxes) return;
//    set	= ball->sets[ball->axisSet];
//    for	(axisI=0; axisI<setSize; axisI++) {
//	if (ball->axisIndex!=axisI) {
//	    if (ball->dragging)	continue;
//	    FARCOLOR();
//	} else NEARCOLOR();
//	axis = *(HVect *)&set[4*axisI];
//	if (axis.z==1.0) {
//	   glBegin(GL_LINE_LOOP);
//	   for (i=0; i < 36; i++)
//	      glVertex3f(cosf((float)i*float(2.0*M_PI/36.0)),sinf((float)i*(float)(2.0*M_PI/36.0)),0.0);
//	   glEnd();
//	} else {
//	    DrawHalfArc(axis);
//	}
//    }
//}
//
///* Draw	"rubber	band" arc during dragging. */
//void Ball_DrawDragArc(BallData *ball)
//{
//    DRAGCOLOR();
//    if (ball->dragging)	DrawAnyArc(ball->vFrom,	ball->vTo);
//}
//
///* Draw	arc for	result of all drags. */
//void Ball_DrawResultArc(BallData *ball)
//{
//    RESCOLOR();
//    if (ball->showResult) DrawAnyArc(ball->vrFrom, ball->vrTo);
//}