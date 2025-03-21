// Copyright (C) 1999-2000 Id Software, Inc.
//
// q_math.c -- stateless support routines that are included in each code module
#include "q_shared.h"


vec3_t	vec3_origin = {0,0,0};
vec3_t	axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };


vec4_t	colorBlack		=	{0, 0, 0, 1};
vec4_t	colorRed		=	{1, 0, 0, 1};
vec4_t	colorGreen		=	{0, 1, 0, 1};
vec4_t	colorBlue		=	{0, 0, 1, 1};
vec4_t	colorYellow		=	{1, 1, 0, 1};
vec4_t	colorOrange		=	{1, 0.5f, 0, 1};
vec4_t	colorMagenta	=	{1, 0, 1, 1};
vec4_t	colorCyan		=	{0, 1, 1, 1};
vec4_t	colorWhite		=	{1, 1, 1, 1};
vec4_t	colorLtGrey		=	{0.75f, 0.75f, 0.75f, 1};
vec4_t	colorMdGrey		=	{0.5f, 0.5f, 0.5f, 1};
vec4_t	colorDkGrey		=	{0.25f, 0.25f, 0.25f, 1};
vec4_t	colorMdRed		=	{0.5f, 0, 0, 1};
vec4_t	colorMdGreen	=	{0, 0.5f, 0, 1};
vec4_t	colorDkGreen	=	{0, 0.20f, 0, 1};
vec4_t	colorMdCyan		=	{0, 0.5f, 0.5f, 1};
vec4_t	colorMdYellow	=	{0.5f, 0.5f, 0, 1};
vec4_t	colorMdOrange	=	{0.5f, 0.25f, 0, 1};
vec4_t	colorMdBlue		=	{0, 0, 0.5f, 1};
// jet Pilot - a few new colors
vec4_t	colorPink		=   {1, 0,	 1,   1};
vec4_t	colorCthulhu	=	{0, .25f, .25f, 1};


vec4_t		clrBrown =			{0.68f,			0.68f,			0.56f,			1.f};
vec4_t		clrBrownDk =		{0.58f * 0.75f,	0.58f * 0.75f,	0.46f * 0.75f,	1.f};
vec4_t		clrBrownLine =		{0.0525f,		0.05f,			0.025f,			0.2f};
vec4_t		clrBrownLineFull =	{0.0525f,		0.05f,			0.025f,			1.f};

vec4_t		clrBrownTextLt2 =	{108*1.8/255.f,		88*1.8/255.f,	62*1.8/255.f,	1.f};
vec4_t		clrBrownTextLt =	{108*1.3/255.f,		88*1.3/255.f,	62*1.3/255.f,	1.f};
vec4_t		clrBrownText =		{108/255.f,			88/255.f,		62/255.f,		1.f};
vec4_t		clrBrownTextDk =	{20/255.f,			2/255.f,		0/255.f,		1.f};
vec4_t		clrBrownTextDk2 =	{108*0.75/255.f,	88*0.75/255.f,	62*0.75/255.f,	1.f};

vec4_t	g_color_table[32] =
	{
		{ 0.0f,	0.0f,	0.0f,	1.0f },	// 0 - black		0
		{ 1.0f,	0.0f,	0.0f,	1.0f },	// 1 - red			1
		{ 0.0f,	1.0f,	0.0f,	1.0f },	// 2 - green		2
		{ 1.0f,	1.0f,	0.0f,	1.0f },	// 3 - yellow		3
		{ 0.0f,	0.0f,	1.0f,	1.0f },	// 4 - blue			4
		{ 0.0f,	1.0f,	1.0f,	1.0f },	// 5 - cyan			5
		{ 1.0f,	0.0f,	1.0f,	1.0f },	// 6 - purple		6
		{ 1.0f,	1.0f,	1.0f,	1.0f },	// 7 - white		7
		{ 1.0f,	0.5f,	0.0f,	1.0f },	// 8 - orange		8
		{ 0.5f,	0.5f,	0.5f,	1.0f },	// 9 - md.grey		9
		{ 0.75f, 0.75f,	0.75f,	1.0f },	// : - lt.grey		10		// lt grey for names
		{ 0.75f, 0.75f,	0.75f,	1.0f },	// ; - lt.grey		11
		{ 0.0f,	0.5f,	0.0f,	1.0f },	// < - md.green		12
		{ 0.5f,	0.5f,	0.0f,	1.0f },	// = - md.yellow	13
		{ 0.0f,	0.0f,	0.5f,	1.0f },	// > - md.blue		14
		{ 0.5f,	0.0f,	0.0f,	1.0f },	// ? - md.red		15
		{ 0.5f,	0.25f,	0.0f,	1.0f },	// @ - md.orange	16
		{ 1.0f,	0.6f,	0.1f,	1.0f },	// A - lt.orange	17
		{ 0.0f,	0.5f,	0.5f,	1.0f },	// B - md.cyan		18
		{ 0.5f,	0.0f,	0.5f,	1.0f },	// C - md.purple	19
		{ 0.0f,	0.5f,	1.0f,	1.0f },	// D				20
		{ 0.5f,	0.0f,	1.0f,	1.0f },	// E				21
		{ 0.2f,	0.6f,	0.8f,	1.0f },	// F				22
		{ 0.8f,	1.0f,	0.8f,	1.0f },	// G				23
		{ 0.0f,	0.4f,	0.2f,	1.0f },	// H				24
		{ 1.0f,	0.0f,	0.2f,	1.0f },	// I				25
		{ 0.7f,	0.1f,	0.1f,	1.0f },	// J				26
		{ 0.6f,	0.2f,	0.0f,	1.0f },	// K				27
		{ 0.8f,	0.6f,	0.2f,	1.0f },	// L				28
		{ 0.6f,	0.6f,	0.2f,	1.0f },	// M				29
		{ 1.0f,	1.0f,	0.75f,	1.0f },	// N				30
		{ 1.0f,	1.0f,	0.5f,	1.0f },	// O				31
	};



vec3_t	bytedirs[NUMVERTEXNORMALS] =
{
{-0.525731, 0.000000, 0.850651}, {-0.442863, 0.238856, 0.864188},
{-0.295242, 0.000000, 0.955423}, {-0.309017, 0.500000, 0.809017},
{-0.162460, 0.262866, 0.951056}, {0.000000, 0.000000, 1.000000},
{0.000000, 0.850651, 0.525731}, {-0.147621, 0.716567, 0.681718},
{0.147621, 0.716567, 0.681718}, {0.000000, 0.525731, 0.850651},
{0.309017, 0.500000, 0.809017}, {0.525731, 0.000000, 0.850651},
{0.295242, 0.000000, 0.955423}, {0.442863, 0.238856, 0.864188},
{0.162460, 0.262866, 0.951056}, {-0.681718, 0.147621, 0.716567},
{-0.809017, 0.309017, 0.500000},{-0.587785, 0.425325, 0.688191},
{-0.850651, 0.525731, 0.000000},{-0.864188, 0.442863, 0.238856},
{-0.716567, 0.681718, 0.147621},{-0.688191, 0.587785, 0.425325},
{-0.500000, 0.809017, 0.309017}, {-0.238856, 0.864188, 0.442863},
{-0.425325, 0.688191, 0.587785}, {-0.716567, 0.681718, -0.147621},
{-0.500000, 0.809017, -0.309017}, {-0.525731, 0.850651, 0.000000},
{0.000000, 0.850651, -0.525731}, {-0.238856, 0.864188, -0.442863},
{0.000000, 0.955423, -0.295242}, {-0.262866, 0.951056, -0.162460},
{0.000000, 1.000000, 0.000000}, {0.000000, 0.955423, 0.295242},
{-0.262866, 0.951056, 0.162460}, {0.238856, 0.864188, 0.442863},
{0.262866, 0.951056, 0.162460}, {0.500000, 0.809017, 0.309017},
{0.238856, 0.864188, -0.442863},{0.262866, 0.951056, -0.162460},
{0.500000, 0.809017, -0.309017},{0.850651, 0.525731, 0.000000},
{0.716567, 0.681718, 0.147621}, {0.716567, 0.681718, -0.147621},
{0.525731, 0.850651, 0.000000}, {0.425325, 0.688191, 0.587785},
{0.864188, 0.442863, 0.238856}, {0.688191, 0.587785, 0.425325},
{0.809017, 0.309017, 0.500000}, {0.681718, 0.147621, 0.716567},
{0.587785, 0.425325, 0.688191}, {0.955423, 0.295242, 0.000000},
{1.000000, 0.000000, 0.000000}, {0.951056, 0.162460, 0.262866},
{0.850651, -0.525731, 0.000000},{0.955423, -0.295242, 0.000000},
{0.864188, -0.442863, 0.238856}, {0.951056, -0.162460, 0.262866},
{0.809017, -0.309017, 0.500000}, {0.681718, -0.147621, 0.716567},
{0.850651, 0.000000, 0.525731}, {0.864188, 0.442863, -0.238856},
{0.809017, 0.309017, -0.500000}, {0.951056, 0.162460, -0.262866},
{0.525731, 0.000000, -0.850651}, {0.681718, 0.147621, -0.716567},
{0.681718, -0.147621, -0.716567},{0.850651, 0.000000, -0.525731},
{0.809017, -0.309017, -0.500000}, {0.864188, -0.442863, -0.238856},
{0.951056, -0.162460, -0.262866}, {0.147621, 0.716567, -0.681718},
{0.309017, 0.500000, -0.809017}, {0.425325, 0.688191, -0.587785},
{0.442863, 0.238856, -0.864188}, {0.587785, 0.425325, -0.688191},
{0.688191, 0.587785, -0.425325}, {-0.147621, 0.716567, -0.681718},
{-0.309017, 0.500000, -0.809017}, {0.000000, 0.525731, -0.850651},
{-0.525731, 0.000000, -0.850651}, {-0.442863, 0.238856, -0.864188},
{-0.295242, 0.000000, -0.955423}, {-0.162460, 0.262866, -0.951056},
{0.000000, 0.000000, -1.000000}, {0.295242, 0.000000, -0.955423},
{0.162460, 0.262866, -0.951056}, {-0.442863, -0.238856, -0.864188},
{-0.309017, -0.500000, -0.809017}, {-0.162460, -0.262866, -0.951056},
{0.000000, -0.850651, -0.525731}, {-0.147621, -0.716567, -0.681718},
{0.147621, -0.716567, -0.681718}, {0.000000, -0.525731, -0.850651},
{0.309017, -0.500000, -0.809017}, {0.442863, -0.238856, -0.864188},
{0.162460, -0.262866, -0.951056}, {0.238856, -0.864188, -0.442863},
{0.500000, -0.809017, -0.309017}, {0.425325, -0.688191, -0.587785},
{0.716567, -0.681718, -0.147621}, {0.688191, -0.587785, -0.425325},
{0.587785, -0.425325, -0.688191}, {0.000000, -0.955423, -0.295242},
{0.000000, -1.000000, 0.000000}, {0.262866, -0.951056, -0.162460},
{0.000000, -0.850651, 0.525731}, {0.000000, -0.955423, 0.295242},
{0.238856, -0.864188, 0.442863}, {0.262866, -0.951056, 0.162460},
{0.500000, -0.809017, 0.309017}, {0.716567, -0.681718, 0.147621},
{0.525731, -0.850651, 0.000000}, {-0.238856, -0.864188, -0.442863},
{-0.500000, -0.809017, -0.309017}, {-0.262866, -0.951056, -0.162460},
{-0.850651, -0.525731, 0.000000}, {-0.716567, -0.681718, -0.147621},
{-0.716567, -0.681718, 0.147621}, {-0.525731, -0.850651, 0.000000},
{-0.500000, -0.809017, 0.309017}, {-0.238856, -0.864188, 0.442863},
{-0.262866, -0.951056, 0.162460}, {-0.864188, -0.442863, 0.238856},
{-0.809017, -0.309017, 0.500000}, {-0.688191, -0.587785, 0.425325},
{-0.681718, -0.147621, 0.716567}, {-0.442863, -0.238856, 0.864188},
{-0.587785, -0.425325, 0.688191}, {-0.309017, -0.500000, 0.809017},
{-0.147621, -0.716567, 0.681718}, {-0.425325, -0.688191, 0.587785},
{-0.162460, -0.262866, 0.951056}, {0.442863, -0.238856, 0.864188},
{0.162460, -0.262866, 0.951056}, {0.309017, -0.500000, 0.809017},
{0.147621, -0.716567, 0.681718}, {0.000000, -0.525731, 0.850651},
{0.425325, -0.688191, 0.587785}, {0.587785, -0.425325, 0.688191},
{0.688191, -0.587785, 0.425325}, {-0.955423, 0.295242, 0.000000},
{-0.951056, 0.162460, 0.262866}, {-1.000000, 0.000000, 0.000000},
{-0.850651, 0.000000, 0.525731}, {-0.955423, -0.295242, 0.000000},
{-0.951056, -0.162460, 0.262866}, {-0.864188, 0.442863, -0.238856},
{-0.951056, 0.162460, -0.262866}, {-0.809017, 0.309017, -0.500000},
{-0.864188, -0.442863, -0.238856}, {-0.951056, -0.162460, -0.262866},
{-0.809017, -0.309017, -0.500000}, {-0.681718, 0.147621, -0.716567},
{-0.681718, -0.147621, -0.716567}, {-0.850651, 0.000000, -0.525731},
{-0.688191, 0.587785, -0.425325}, {-0.587785, 0.425325, -0.688191},
{-0.425325, 0.688191, -0.587785}, {-0.425325, -0.688191, -0.587785},
{-0.587785, -0.425325, -0.688191}, {-0.688191, -0.587785, -0.425325}
};

//==============================================================

int		Q_rand( int *seed ) {
	*seed = (69069 * *seed + 1);
	return *seed;
}

float	Q_random( int *seed ) {
	return ( Q_rand( seed ) & 0xffff ) / (float)0x10000;
}

float	Q_crandom( int *seed ) {
	return 2.0 * ( Q_random( seed ) - 0.5 );
}


//=======================================================

signed char ClampChar( int i ) {
	if ( i < -128 ) {
		return -128;
	}
	if ( i > 127 ) {
		return 127;
	}
	return i;
}

signed short ClampShort( int i ) {
	if ( i < -32768 ) {
		return -32768;
	}
	if ( i > 0x7fff ) {
		return 0x7fff;
	}
	return i;
}


int DirToByte( vec3_t dir ) {
	// core: fit the vector in 1 byte
	return ( ((int)(dir[0]/360*8) << 5) + ((int)(dir[1]/360*4) << 3) + ((int)(dir[2]/360*8)) );
}

void ByteToDir( int b, vec3_t dir ) {
	// core: get a vector from the byte
	dir[0] = (float)((b>>5 & 7)*(360/8));
	dir[1] = (float)((b>>3 & 3)*(360/4));
	dir[2] = (float)((b & 7)*(360/8));
	VectorNormalize(dir);
}

// this isn't a real cheap function to call!
int DirToByte2( vec3_t dir ) {
	int		i, best;
	float	d, bestd;

	if ( !dir ) {
		return 0;
	}

	bestd = 0;
	best = 0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		d = DotProduct (dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir2( int b, vec3_t dir ) {
	if ( b < 0 || b >= NUMVERTEXNORMALS ) {
		VectorCopy( vec3_origin, dir );
		return;
	}
	VectorCopy (bytedirs[b], dir);
}


unsigned ColorBytes3 (float r, float g, float b) {
	unsigned	i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;

	return i;
}

unsigned ColorBytes4 (float r, float g, float b, float a) {
	unsigned	i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;
	( (byte *)&i )[3] = a * 255;

	return i;
}

float NormalizeColor( const vec3_t in, vec3_t out ) {
	float	max = in[0];

	if ( in[1] > max ) {
		max = in[1];
	}
	if ( in[2] > max ) {
		max = in[2];
	}

	if ( !max ) {
		VectorClear( out );
	} else {
		out[0] = in[0] / max;
		out[1] = in[1] / max;
		out[2] = in[2] / max;
	}
	return max;
}


/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c ) {
	vec3_t	d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );
	if ( VectorNormalize( plane ) == 0 ) {
		return qfalse;
	}

	plane[3] = DotProduct( a, plane );
	return qtrue;
}

/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point,
							 float degrees ) {
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	vec3_t	vr, vup, vf = {dir[0], dir[1], dir[2]};
	float	rad;
	float	s = 0.0f, c = 0.0f;

	// fix from TheDushan/gimhael from openwolf
	// passing a 0-vector the function returns wrong results
	// in that case (and also when degrees is 0.0) the original vertex is returned
	if( degrees == 0.0f || VectorNormalize(vf) == 0.0f  ) {
		// degenerate case
		VectorCopy(point, dst);
		return;
	}
	PerpendicularVector( vr, dir );
	CrossProduct( vr, vf, vup );

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy( im, m, sizeof( im ) );

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	rad = DEG2RAD( degrees );
	SinCos(rad,s,c);

	zrot[0][0] = c;
	zrot[0][1] = s;
	zrot[1][0] = -s;
	zrot[1][1] = c;

	MatrixMultiply( m, zrot, tmpmat );
	MatrixMultiply( tmpmat, im, rot );

	dst[0] = rot[0][0] * point[0] + rot[0][1] * point[1] + rot[0][2] * point[2];
	dst[1] = rot[1][0] * point[0] + rot[1][1] * point[1] + rot[1][2] * point[2];
	dst[2] = rot[2][0] * point[0] + rot[2][1] * point[1] + rot[2][2] * point[2];
}

/*
===============
RotatePointArountVertex

Rotate a point around a vertex

IRATA: unused - however if you ever use this function check 1st line behind '// rotate point' 
see Left handed XYZ - http://en.wikipedia.org/wiki/Euler_angles#Matrix_orientation
===============
*/
void RotatePointAroundVertex ( vec3_t pnt, float rot_x, float rot_y, float rot_z, const vec3_t origin ) {
	float tmp[11];

	// move pnt to rel{0,0,0}
	VectorSubtract( pnt, origin, pnt );

	// init temp values
	tmp[0] = sin( rot_x );
	tmp[1] = cos( rot_x );
	tmp[2] = sin( rot_y );
	tmp[3] = cos( rot_y );
	tmp[4] = sin( rot_z );
	tmp[5] = cos( rot_z );
	tmp[6] = pnt[1] * tmp[5];
	tmp[7] = pnt[0] * tmp[4];
	tmp[8] = pnt[0] * tmp[5];
	tmp[9] = pnt[1] * tmp[4];
	tmp[10] = pnt[2] * tmp[3];

	// rotate point
	// pnt[0] = ( tmp[3] * ( tmp[8] - tmp[9] ) + /* pnt[3] or pnt[2] ? */* tmp[2] );
	pnt[0] = ( tmp[3] * ( tmp[8] - tmp[9] ) + tmp[3] * tmp[2] );
	pnt[1] = ( tmp[0] * ( tmp[2] * tmp[8] - tmp[2] * tmp[9] - tmp[10] ) + tmp[1] * ( tmp[7] + tmp[6] ) );
	pnt[2] = ( tmp[1] * ( -tmp[2] * tmp[8] + tmp[2] * tmp[9] + tmp[10] ) + tmp[0] * ( tmp[7] + tmp[6] ) );

	// move pnt back
	VectorAdd( pnt, origin, pnt );
}

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection( vec3_t axis[3], float yaw ) {

	// create an arbitrary axis[1]
	PerpendicularVector( axis[1], axis[0] );

	// rotate it around axis[0] by yaw
	if ( yaw ) {
		vec3_t	temp;

		VectorCopy( axis[1], temp );
		RotatePointAroundVector( axis[1], axis[0], temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( axis[0], axis[1], axis[2] );
}



void vectoangles( const vec3_t value1, vec3_t angles ) {
	float	yaw = 0;
	float	pitch;

	if ( value1[1] == 0 && value1[0] == 0 ) {
		if ( value1[2] > 0 ) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		float	forward;

		if ( value1[0] ) {
			yaw = RAD2DEG( atan2(value1[1], value1[0]) );
			if ( yaw < 0 ) {
				yaw += 360;
			}
		}
		else if ( value1[1] > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}

		// original code..
//		forward = sqrt( value1[0]*value1[0] + value1[1]*value1[1] );
//		pitch = RAD2DEG( atan2(value1[2], forward) );
#if asmSSE
		// core: using SSE assembler
		// forward = 1/sqrt(x)
		forward = value1[0]*value1[0] + value1[1]*value1[1];
		__asm {
			movss xmm0, forward
			rsqrtss xmm0, xmm0
			movss forward, xmm0
		}
		pitch = RAD2DEG( atan(value1[2]*forward) );
#else
		forward = Q_rsqrt( value1[0]*value1[0] + value1[1]*value1[1] );
		pitch = RAD2DEG( atan(value1[2]*forward) );
#endif
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}


/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] ) {
	vec3_t	right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, axis[0], right, axis[2] );
	VectorSubtract( vec3_origin, right, axis[1] );
}

void AxisClear( vec3_t axis[3] ) {
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void AxisCopy( vec3_t in[3], vec3_t out[3] ) {
	VectorCopy( in[0], out[0] );
	VectorCopy( in[1], out[1] );
	VectorCopy( in[2], out[2] );
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	d = DotProduct( normal, normal );
	inv_denom = (d)? 1.0F/d : 0.0f;

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up) {
	float		d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right);
	CrossProduct (right, forward, up);
}


void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out )
{
	out[0] = DotProduct( in, matrix[0] );
	out[1] = DotProduct( in, matrix[1] );
	out[2] = DotProduct( in, matrix[2] );
}

//============================================================================

/*
** float q_rsqrt( float number )
*/
float Q_rsqrt( float number )
{
	const float	threehalfs = 1.5F;
	float		x2 = number * 0.5F;
	float		y = number;
	long		i = * ( long * ) &y;			// evil floating point bit level hacking

	i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

float Q_fabs( float f ) {
	int tmp = (*(int*)&f) & 0x7FFFFFFF;
	return *(float*)&tmp;
}

#if id386 && !( (defined __linux__ || defined __FreeBSD__ || defined __GNUC__ ) && (defined __i386__ ) ) // rb010123
long myftol( float f ) {
	static int tmp;
	__asm fld f
	__asm fistp tmp
	__asm mov eax, tmp
}
#endif

//============================================================

/*
===============
LerpAngle

===============
*/
float LerpAngle (float from, float to, float frac)
{
	if ( to - from > 180 ) {
		to -= 360;
	}
	if ( to - from < -180 ) {
		to += 360;
	}

	return(from + frac * (to - from));
}

/*
=================
LerpPosition

=================
*/

void LerpPosition( vec3_t start, vec3_t end, float frac, vec3_t out) {
	vec3_t dist;

	VectorSubtract( end, start, dist );
	VectorMA(start, frac, dist, out);
}

/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float AngleSubtract( float a1, float a2 )
{
	float a = a1 - a2;

	while ( a > 180 ) {
		a -= 360;
	}
	while ( a < -180 ) {
		a += 360;
	}
	return a;
}


void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 )
{
	v3[0] = AngleSubtract( v1[0], v2[0] );
	v3[1] = AngleSubtract( v1[1], v2[1] );
	v3[2] = AngleSubtract( v1[2], v2[2] );
}


float AngleMod(float a)
{
	return((360.0 / 65536) * ((int)(a * (65536 / 360.0)) & 65535));
}

/*
=================
AngleNormalize2Pi

returns angle normalized to the range [0 <= angle < 2*M_PI]
=================
*/
float AngleNormalize2Pi ( float angle ) {
	return DEG2RAD( AngleNormalize360( RAD2DEG( angle ) ) );
}

/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
float AngleNormalize360 ( float angle ) {
	return (360.0 / 65536) * ((int)(angle * (65536 / 360.0)) & 65535);
}

// tjw: integer angles used for at least usercmd.angles[3]
//
// returns angle normalized to the range [0 <= angle < 65536]
unsigned int AngleNormalizeInt(int angle)
{
	if(angle < 0)
		angle = (65536 + (angle % 65536));
	return (angle % 65536);

}


/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
float AngleNormalize180 ( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0 ) {
		angle -= 360.0;
	}
	return angle;
}


/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
float AngleDelta ( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}


//============================================================


/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits (cplane_t *out) {
	int	bits  = 0, j;

	// for fast box on planeside test
	for (j=0 ; j<3 ; j++) {
		if (out->normal[j] < 0) {
			bits |= 1<<j;
		}
	}
	out->signbits = bits;
}


/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2

// this is the slow, general version
int BoxOnPlaneSide2 (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	int		i;
	float	dist1, dist2;
	int		sides;
	vec3_t	corners[2];

	for (i=0 ; i<3 ; i++)
	{
		if (p->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist1 = DotProduct (p->normal, corners[0]) - p->dist;
	dist2 = DotProduct (p->normal, corners[1]) - p->dist;
	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

	return sides;
}

==================
*/
#if !(defined __linux__ && defined __i386__ && !defined C_ONLY)
#if defined __LCC__ || defined C_ONLY || !id386 || __GNUC__
int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float	dist1, dist2;
	int		sides;

// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 1:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 2:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 3:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 4:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	return sides;
}
#else
#pragma warning( disable: 4035 )

__inline __declspec( naked ) int BoxOnPlaneSide_fast (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	static int bops_initialized;
	static int Ljmptab[8];

	__asm {

		push ebx

		cmp bops_initialized, 1
		je  initialized
		mov bops_initialized, 1

		mov Ljmptab[0*4], offset Lcase0
		mov Ljmptab[1*4], offset Lcase1
		mov Ljmptab[2*4], offset Lcase2
		mov Ljmptab[3*4], offset Lcase3
		mov Ljmptab[4*4], offset Lcase4
		mov Ljmptab[5*4], offset Lcase5
		mov Ljmptab[6*4], offset Lcase6
		mov Ljmptab[7*4], offset Lcase7

initialized:

		mov edx,dword ptr[4+12+esp]
		mov ecx,dword ptr[4+4+esp]
		xor eax,eax
		mov ebx,dword ptr[4+8+esp]
		mov al,byte ptr[17+edx]
		cmp al,8
		jge Lerror
		fld dword ptr[0+edx]
		fld st(0)
		jmp dword ptr[Ljmptab+eax*4]
Lcase0:
		fmul dword ptr[ebx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ebx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase1:
		fmul dword ptr[ecx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ebx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase2:
		fmul dword ptr[ebx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ecx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase3:
		fmul dword ptr[ecx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ecx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase4:
		fmul dword ptr[ebx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ebx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase5:
		fmul dword ptr[ecx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ebx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase6:
		fmul dword ptr[ebx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ecx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase7:
		fmul dword ptr[ecx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ecx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
LSetSides:
		faddp st(2),st(0)
		fcomp dword ptr[12+edx]
		xor ecx,ecx
		fnstsw ax
		fcomp dword ptr[12+edx]
		and ah,1
		xor ah,1
		add cl,ah
		fnstsw ax
		and ah,1
		add ah,ah
		add cl,ah
		pop ebx
		mov eax,ecx
		ret
Lerror:
		int 3
	}
}

int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct cplane_s *p) {
	// fast axial cases

	if (p->type < 3) {
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

	return BoxOnPlaneSide_fast( emins, emaxs, p );
}

#pragma warning( default: 4035 )

#endif
#endif

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs ) {
	int		i;
	vec3_t	corner;
	float	a, b;

	for (i=0 ; i<3 ; i++) {
		a = Q_fabs( mins[i] );
		b = Q_fabs( maxs[i] );
		corner[i] = a > b ? a : b;
	}

	return VectorLength (corner);
}


void ClearBounds( vec3_t mins, vec3_t maxs ) {
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs ) {
	if ( v[0] < mins[0] ) {
		mins[0] = v[0];
	}
	if ( v[0] > maxs[0]) {
		maxs[0] = v[0];
	}

	if ( v[1] < mins[1] ) {
		mins[1] = v[1];
	}
	if ( v[1] > maxs[1]) {
		maxs[1] = v[1];
	}

	if ( v[2] < mins[2] ) {
		mins[2] = v[2];
	}
	if ( v[2] > maxs[2]) {
		maxs[2] = v[2];
	}
}

qboolean PointInBounds( const vec3_t v, const vec3_t mins, const vec3_t maxs ) {
	if ( v[0] < mins[0] ) {
		return qfalse;
	}
	if ( v[0] > maxs[0]) {
		return qfalse;
	}

	if ( v[1] < mins[1] ) {
		return qfalse;
	}
	if ( v[1] > maxs[1]) {
		return qfalse;
	}

	if ( v[2] < mins[2] ) {
		return qfalse;
	}
	if ( v[2] > maxs[2]) {
		return qfalse;
	}

	return qtrue;
}


int VectorCompare( const vec3_t v1, const vec3_t v2 ) {
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2]) {
		return 0;
	}

	return 1;
}


vec_t VectorNormalize( vec3_t v ) {
	float	len = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	float	ilength;

#if asmSSE
	// core: using SSE assembler
	// ilength = 1/sqrt(len)
	// len = 1/ilength = sqrt(len)
	__asm {
		movss xmm0, len
		rsqrtss xmm0, xmm0
		movss ilength, xmm0
		rcpss xmm0, xmm0
		movss len, xmm0
	}
#else
	ilength = Q_rsqrt(len);
	len = 1/ilength;
#endif

	if ( len ) {
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return len;
}


//
// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length
//
void VectorNormalizeFast( vec3_t v )
{
	float	len = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	float	ilength;

#if asmSSE
	// core: using SSE assembler
	// ilength = 1/sqrt(len)
	__asm {
		movss xmm0, len
		rsqrtss xmm0, xmm0
		movss ilength, xmm0
	}
#else
	ilength = Q_rsqrt(len);
#endif

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}


vec_t VectorNormalize2( const vec3_t v, vec3_t out) {
	float	len = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	float	ilength;

#if asmSSE
	// core: using SSE assembler
	// ilength = 1/sqrt(len)
	// len = 1/ilength = sqrt(len)
	__asm {
		movss xmm0, len
		rsqrtss xmm0, xmm0
		movss ilength, xmm0
		rcpss xmm0, xmm0
		movss len, xmm0
	}
#else
	len = sqrt(len);
	ilength = 1/len;
#endif

	if ( len ) {
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	} else {
		VectorClear( out );
	}

	return len;
}


void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc) {
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


vec_t _DotProduct( const vec3_t v1, const vec3_t v2 ) {
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy( const vec3_t in, vec3_t out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void _VectorScale( const vec3_t in, vec_t scale, vec3_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

void _MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}


void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross ) {
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

vec_t VectorLength( const vec3_t v ) {
	return sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

vec_t VectorLengthSquared( const vec3_t v ) {
	return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

vec_t Distance( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract (p2, p1, v);
	return VectorLength( v );
}

vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract (p2, p1, v);
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}


void VectorInverse( vec3_t v ){
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
	out[3] = in[3]*scale;
}


int Q_log2( int val ) {
	int answer = 0;

	while ( ( val>>=1 ) != 0 ) {
		answer++;
	}
	return answer;
}

void MatrixTranspose(vec3_t in[3], vec3_t out[3]) {
	out[0][0] = in[0][0];
	out[0][1] = in[1][0];
	out[0][2] = in[2][0];
	out[1][0] = in[0][1];
	out[1][1] = in[1][1];
	out[1][2] = in[2][1];
	out[2][0] = in[0][2];
	out[2][1] = in[1][2];
	out[2][2] = in[2][2];
}

void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
	static float	sr=0.0f, ssp=0.0f, sy=0.0f, cr=0.0f, cp=0.0f, cy=0.0f;	// static to help MS compiler fp bugs
	float angle = DEG2RAD( angles[YAW] );

	SinCos(angle,sy,cy);

	angle = DEG2RAD( angles[PITCH] );
	SinCos(angle,ssp,cp);

	angle = DEG2RAD( angles[ROLL] );
	SinCos(angle,sr,cr);

	if (forward) {
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -ssp;
	}
	if (right) {
		float msrsp = -sr*ssp;
		right[0] = msrsp*cy + cr*sy;	// (-1*sr*sp*cy+-1*cr*-sy)
		right[1] = msrsp*sy - cr*cy;	// (-1*sr*sp*sy+-1*cr*cy)
		right[2] = -sr*cp;				// -1*sr*cp;
	}
	if (up) {
		float crsp = cr*ssp;
		up[0] = crsp*cy + sr*sy;		// (cr*sp*cy+-sr*-sy)
		up[1] = crsp*sy - sr*cy;		// (cr*sp*sy+-sr*cy)
		up[2] = cr*cp;
	}
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int		pos = 0;
	float	minelem = 1.0F;
	vec3_t	tempvec;
	float	abs0 = Q_fabs( src[0] );
	float	abs1 = Q_fabs( src[1] );
	float	abs2 = Q_fabs( src[2] );

	/*
	** find the smallest magnitude axially aligned vector
	*/

	if ( abs0 < minelem ) {
		pos = 0;
		minelem = abs0;
	}

	if ( abs1 < minelem ) {
		pos = 1;
		minelem = abs1;
	}

	if ( abs2 < minelem ) {
		pos = 2;
		minelem = abs2;
	}

	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

// Ridah
/*
=================
GetPerpendicularViewVector

  Used to find an "up" vector for drawing a sprite so that it always faces the view as best as possible
=================
*/
void GetPerpendicularViewVector( const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up )
{
	vec3_t	v1, v2;

	VectorSubtract( point, p1, v1 );
	VectorNormalize( v1 );

	VectorSubtract( point, p2, v2 );
	VectorNormalize( v2 );

	CrossProduct( v1, v2, up );
	VectorNormalize( up );
}

/*
================
ProjectPointOntoVector
================
*/
void ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj )
{
	vec3_t pVec, vec;
	float s;

	VectorSubtract( point, vStart, pVec );
	VectorSubtract( vEnd, vStart, vec );
	VectorNormalize( vec );
	// project onto the directional vector for this segment
	s = DotProduct( pVec, vec );
	VectorMA( vStart, s, vec, vProj );
}

/*
================
ProjectPointOntoVectorBounded
================
*/
void ProjectPointOntoVectorBounded( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj )
{
	vec3_t pVec, vec;
	int j;
	float s;

	VectorSubtract( point, vStart, pVec );
	VectorSubtract( vEnd, vStart, vec );
	VectorNormalize( vec );
	// project onto the directional vector for this segment
	s = DotProduct( pVec, vec );
	VectorMA( vStart, s, vec, vProj );
	// check bounds
	for (j = 0; j < 3; j++)
		if ((vProj[j] > vStart[j] && vProj[j] > vEnd[j]) ||
			(vProj[j] < vStart[j] && vProj[j] < vEnd[j]))
			break;
	if (j < 3) {
		if (Q_fabs(vProj[j] - vStart[j]) < Q_fabs(vProj[j] - vEnd[j]))
			VectorCopy(vStart, vProj);
		else
			VectorCopy(vEnd, vProj);
	}
}

/*
================
DistanceFromLineSquared
================
*/
float DistanceFromLineSquared(vec3_t p, vec3_t lp1, vec3_t lp2) {
	vec3_t proj, t;
	int j;

	ProjectPointOntoVector(p, lp1, lp2, proj);
	for (j = 0; j < 3; j++)
		if ((proj[j] > lp1[j] && proj[j] > lp2[j]) ||
			(proj[j] < lp1[j] && proj[j] < lp2[j]))
			break;
	if (j < 3) {
		if (Q_fabs(proj[j] - lp1[j]) < Q_fabs(proj[j] - lp2[j]))
			VectorSubtract(p, lp1, t);
		else
			VectorSubtract(p, lp2, t);
		return VectorLengthSquared(t);
	}
	VectorSubtract(p, proj, t);
	return VectorLengthSquared(t);
}

/*
================
DistanceFromVectorSquared
================
*/
float DistanceFromVectorSquared(vec3_t p, vec3_t lp1, vec3_t lp2) {
	vec3_t proj, t;

	ProjectPointOntoVector(p, lp1, lp2, proj);
	VectorSubtract(p, proj, t);
	return VectorLengthSquared(t);
}

float vectoyaw( const vec3_t vec ) {
	float	yaw;

	if (vec[YAW] == 0 && vec[PITCH] == 0) {
		yaw = 0;
	} else {
		if (vec[PITCH]) {
			yaw = ( atan2( vec[YAW], vec[PITCH]) * 180 / M_PI );
		} else if (vec[YAW] > 0) {
			yaw = 90;
		} else {
			yaw = 270;
		}
		if (yaw < 0) {
			yaw += 360;
		}
	}

	return yaw;
}

/*
=================
AxisToAngles

  Used to convert the MD3 tag axis to MDC tag angles, which are much smaller

  This doesn't have to be fast, since it's only used for conversion in utils, try to avoid
  using this during gameplay
=================
*/
void AxisToAngles( vec3_t axis[3], vec3_t angles ) {
	vec3_t right, roll_angles, tvec;

	// first get the pitch and yaw from the forward vector
	vectoangles( axis[0], angles );

	// now get the roll from the right vector
	VectorCopy( axis[1], right );
	// get the angle difference between the tmpAxis[2] and axis[2] after they have been reverse-rotated
	RotatePointAroundVector( tvec, axisDefault[2], right, -angles[YAW] );
	RotatePointAroundVector( right, axisDefault[1], tvec, -angles[PITCH] );
	// now find the angles, the PITCH is effectively our ROLL
	vectoangles( right, roll_angles );
	roll_angles[PITCH] = AngleNormalize180( roll_angles[PITCH] );
	// if the yaw is more than 90 degrees difference, we should adjust the pitch
	if (DotProduct( right, axisDefault[1] ) < 0) {
		if (roll_angles[PITCH] < 0)
			roll_angles[PITCH] = -90 + (-90 - roll_angles[PITCH]);
		else
			roll_angles[PITCH] =  90 + ( 90 - roll_angles[PITCH]);
	}

	angles[ROLL] = -roll_angles[PITCH];
}

float VectorDistance(vec3_t v1, vec3_t v2)
{
	vec3_t dir;

	VectorSubtract(v2, v1, dir);
	return VectorLength(dir);
}

float VectorDistanceSquared(vec3_t v1, vec3_t v2) {
	vec3_t dir;

	VectorSubtract(v2, v1, dir);
	return VectorLengthSquared(dir);
}

#ifdef _MSC_VER

	//int rint (double x) {

	//	int i;

	//	__asm {
	//		fld x;
	//		fistp i;
	//	};

	//	return i;
	//}

#endif
// done.
