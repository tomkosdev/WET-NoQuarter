/*
 * name:	cg_atmospheric.c
 *
 * desc:	Add atmospheric effects (e.g. rain, snow etc.) to view.
 * 			Current supported effects are rain and snow.
 *
 * NQQS:
 *
 */
#define ATM_NEW

#include "cg_local.h"

#define	MAX_ATMOSPHERIC_HEIGHT			MAX_MAP_SIZE	// maximum world height
// #define	MIN_ATMOSPHERIC_HEIGHT			-MAX_MAP_SIZE	// minimum world height

#define	MAX_ATMOSPHERIC_PARTICLES		4000	// maximum # of particles
#define	MAX_ATMOSPHERIC_DISTANCE		1000	// maximum distance from refdef origin that particles are visible
#define MAX_ATMOSPHERIC_EFFECTSHADERS	6		// maximum different effectshaders for an atmospheric effect
#define	ATMOSPHERIC_DROPDELAY			1000
// #define	ATMOSPHERIC_CUTHEIGHT			800

#define	ATMOSPHERIC_RAIN_SPEED		(1.1f * DEFAULT_GRAVITY)
#define	ATMOSPHERIC_RAIN_HEIGHT		150

#define	ATMOSPHERIC_SNOW_SPEED		(0.1f * DEFAULT_GRAVITY)
#define	ATMOSPHERIC_SNOW_HEIGHT		3

typedef enum {
	ATM_NONE,
	ATM_RAIN,
	ATM_SNOW
} atmFXType_t;

#ifndef ATM_NEW
/*
** Atmospheric Particles PolyPool
*/
static polyVert_t atmPolyPool[MAX_ATMOSPHERIC_PARTICLES*3];
static int numParticlesInFrame;
static qhandle_t atmPolyShader;

static void CG_ClearPolyPool( void )
{
	numParticlesInFrame = 0;
	atmPolyShader = 0;
}

static void CG_RenderPolyPool( void )
{
	if( numParticlesInFrame ) {
		trap_R_AddPolysToScene( atmPolyShader, 3, atmPolyPool, numParticlesInFrame );
		CG_ClearPolyPool();
	}
}
#endif // ATM_NEW

/*
==============
CG_AddPolyToPool
==============
*/
static void CG_AddPolyToPool( qhandle_t shader, const polyVert_t *verts )
{
#ifndef ATM_NEW
		if( atmPolyShader && atmPolyShader != shader ) {
			CG_RenderPolyPool();
		}

		if( numParticlesInFrame == MAX_ATMOSPHERIC_PARTICLES ) {
			CG_RenderPolyPool();
		}

		atmPolyShader = shader;
		memcpy( &atmPolyPool[numParticlesInFrame*3], verts, 3 * sizeof( polyVert_t ) );
		numParticlesInFrame++;
#else
		int firstIndex;
		int firstVertex;
		int i;

		polyBuffer_t* pPolyBuffer = CG_PB_FindFreePolyBuffer( shader, 3, 3 );
		if(!pPolyBuffer) {
			return;
		}

		firstIndex = pPolyBuffer->numIndicies;
		firstVertex = pPolyBuffer->numVerts;

		for( i = 0; i < 3; i++ ) {
			VectorCopy( verts[i].xyz, pPolyBuffer->xyz[firstVertex+i] );

			pPolyBuffer->st[firstVertex + i][0] = verts[i].st[0];
			pPolyBuffer->st[firstVertex + i][1] = verts[i].st[1];
			pPolyBuffer->color[firstVertex + i][0] = verts[i].modulate[0];
			pPolyBuffer->color[firstVertex + i][1] = verts[i].modulate[1];
			pPolyBuffer->color[firstVertex + i][2] = verts[i].modulate[2];
			pPolyBuffer->color[firstVertex + i][3] = verts[i].modulate[3];

			pPolyBuffer->indicies[firstIndex + i] = firstVertex + i;

		}

		pPolyBuffer->numIndicies += 3;
		pPolyBuffer->numVerts += 3;
#endif // ATM_NEW
}

/*
==============
CG_AtmosphericKludge
==============
*/
static qboolean kludgeChecked, kludgeResult;
qboolean CG_AtmosphericKludge()
{
	// Activate rain for specified kludge maps that don't
	// have it specified for them.

	if( kludgeChecked ) {
		return( kludgeResult );
	}
	kludgeChecked = qtrue;
	kludgeResult = qfalse;

	return( kludgeResult = qfalse );
}

typedef enum {
	ACT_NOT,
	ACT_FALLING
} active_t;

typedef struct cg_atmosphericParticle_s {
	vec3_t pos, delta, deltaNormalized, colour;
	float height, weight;
	active_t active;
	int nextDropTime;
	qhandle_t *effectshader;
} cg_atmosphericParticle_t;

typedef struct cg_atmosphericEffect_s {
	cg_atmosphericParticle_t particles[MAX_ATMOSPHERIC_PARTICLES];
	qhandle_t effectshaders[MAX_ATMOSPHERIC_EFFECTSHADERS];
	int lastRainTime, numDrops;
	int gustStartTime, gustEndTime;
	int baseStartTime, baseEndTime;
	int gustMinTime, gustMaxTime;
	int changeMinTime, changeMaxTime;
	int baseMinTime, baseMaxTime;
	float baseWeight, gustWeight;
	int baseDrops, gustDrops;
	int baseHeightOffset;
	int numEffectShaders;
	vec3_t baseVec, gustVec;

	vec3_t viewDir;

	qboolean (*ParticleCheckVisible)( cg_atmosphericParticle_t *particle );
	qboolean (*ParticleGenerate)( cg_atmosphericParticle_t *particle, vec3_t currvec, float currweight );
	void (*ParticleRender)( cg_atmosphericParticle_t *particle );

	int dropsActive, oldDropsActive;
	int dropsRendered, dropsCreated, dropsSkipped;
} cg_atmosphericEffect_t;

static cg_atmosphericEffect_t cg_atmFx;

/*
==============
CG_SetParticleActive
==============
*/
static qboolean CG_SetParticleActive( cg_atmosphericParticle_t *particle, active_t active )
{
	particle->active = active;
	return active ? qtrue : qfalse;
}

//	Raindrop management functions

/*
==============
CG_RainParticleGenerate
==============
*/
static qboolean CG_RainParticleGenerate( cg_atmosphericParticle_t *particle, vec3_t currvec, float currweight )
{
	// Attempt to 'spot' a raindrop somewhere below a sky texture.
	float angle = random() * 2*M_PI;
	float distance = 20 + MAX_ATMOSPHERIC_DISTANCE * random();
	float groundHeight, skyHeight;
//	int msec = trap_Milliseconds();

//	n_generatetime++;

	particle->pos[0] = cg.refdef_current->vieworg[0] + sin(angle) * distance;
	particle->pos[1] = cg.refdef_current->vieworg[1] + cos(angle) * distance;

	// ydnar: choose a spawn point randomly between sky and ground
	skyHeight = BG_GetSkyHeightAtPoint( particle->pos );
	if( skyHeight == MAX_ATMOSPHERIC_HEIGHT ) {
		return qfalse;
	}
	groundHeight = BG_GetSkyGroundHeightAtPoint( particle->pos );
	if( groundHeight >= skyHeight ) {
		return qfalse;
	}
	particle->pos[2] = groundHeight + random() * (skyHeight - groundHeight);

	// make sure it doesn't fall from too far cause it then will go over our heads ('lower the ceiling')
	if( cg_atmFx.baseHeightOffset > 0 ) {
		if( particle->pos[2] - cg.refdef_current->vieworg[2] > cg_atmFx.baseHeightOffset ) {
			particle->pos[2] = cg.refdef_current->vieworg[2] + cg_atmFx.baseHeightOffset;

			if( particle->pos[2] < groundHeight ) {
				return qfalse;
			}
		}
	}

	// ydnar: rain goes in bursts - every 10 seconds allow max raindrops
	// float maxActiveDrops = 0.50 * cg_atmFx.numDrops + 0.001 * cg_atmFx.numDrops * (10000 - (cg.time % 10000));
	if( cg_atmFx.oldDropsActive > ( 0.50 * cg_atmFx.numDrops + 0.001 * cg_atmFx.numDrops * (10000 - (cg.time % 10000)) ) ) {
		return qfalse;
	}

	CG_SetParticleActive( particle, ACT_FALLING );
	particle->colour[0] = 0.6 + 0.2 * random() * 0xFF;
	particle->colour[1] = 0.6 + 0.2 * random() * 0xFF;
	particle->colour[2] = 0.6 + 0.2 * random() * 0xFF;
	VectorCopy( currvec, particle->delta );
	particle->delta[2] += crandom() * 100;
	VectorCopy( particle->delta, particle->deltaNormalized );
	VectorNormalizeFast( particle->deltaNormalized );
	particle->height = ATMOSPHERIC_RAIN_HEIGHT + crandom() * 100;
	particle->weight = currweight;
	particle->effectshader = &cg_atmFx.effectshaders[0];
	//	particle->effectshader = &cg_atmFx.effectshaders[ (int) (random() * ( cg_atmFx.numEffectShaders - 1 )) ];

//	generatetime += trap_Milliseconds() - msec;
	return( qtrue );
}

/*
==============
CG_RainParticleCheckVisible
==============
*/
static qboolean CG_RainParticleCheckVisible( cg_atmosphericParticle_t *particle )
{
	// Check the raindrop is visible and still going, wrapping if necessary.

	//	int msec = trap_Milliseconds();

	if( !particle || particle->active == ACT_NOT ) {
		//		checkvisibletime += trap_Milliseconds() - msec;
		return( qfalse );
	}

	{
		float moved = (cg.time - cg_atmFx.lastRainTime) * 0.001;	// Units moved since last frame
		vec2_t distance;

		VectorMA( particle->pos, moved, particle->delta, particle->pos );

		if( particle->pos[2] + particle->height < BG_GetSkyGroundHeightAtPoint( particle->pos ) ) {
			//		checkvisibletime += trap_Milliseconds() - msec;
			return CG_SetParticleActive( particle, ACT_NOT );
		}

		distance[0] = particle->pos[0] - cg.refdef_current->vieworg[0];
		distance[1] = particle->pos[1] - cg.refdef_current->vieworg[1];
		if( (distance[0] * distance[0] + distance[1] * distance[1]) > Square( MAX_ATMOSPHERIC_DISTANCE ) ) {
			// ydnar: just nuke this particle, let it respawn
			return CG_SetParticleActive( particle, ACT_NOT );

		}
	}
 	//	checkvisibletime += trap_Milliseconds() - msec;
	return( qtrue );
}

/*
==============
CG_RainParticleRender
==============
*/
static void CG_RainParticleRender( cg_atmosphericParticle_t *particle )
{
	// Draw a raindrop
	vec3_t		forward, right;
	polyVert_t	verts[3];
	vec2_t		line;
	float		len, dist;
	vec3_t		start, finish;
	float		groundHeight;

//	int			msec = trap_Milliseconds();

//	n_rendertime++;

	if( particle->active == ACT_NOT ) {
//		rendertime += trap_Milliseconds() - msec;
		return;
	}

	if( CG_CullPoint( particle->pos ) ) {
		return;
	}

	VectorCopy( particle->pos, start );

	dist = DistanceSquared( particle->pos, cg.refdef_current->vieworg );

	// Make sure it doesn't clip through surfaces
	groundHeight = BG_GetSkyGroundHeightAtPoint( start );
	len = particle->height;
	if( start[2] <= groundHeight ) {
		float		s;
		// Stop snow going through surfaces.
		len = particle->height - groundHeight + start[2];
		s = len - particle->height;
		VectorMA( start, s, particle->deltaNormalized, start );
	}

	if( len <= 0 ) {
//		rendertime += trap_Milliseconds() - msec;
		return;
	}

	// fade nearby rain particles
	if( dist < Square( 128.f ) ) {
		dist = .25f + .75f * ( dist / Square( 128.f ) );
	}
	else {
		dist = 1.0f;
	}

	VectorCopy( particle->deltaNormalized, forward );
	VectorMA( start, -len, forward, finish );

	line[0] = DotProduct( forward, cg.refdef_current->viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef_current->viewaxis[2] );

	VectorScale( cg.refdef_current->viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef_current->viewaxis[2], right );
	VectorNormalize( right );

	// dist = 1.0;

	VectorCopy( finish, verts[0].xyz );
	verts[0].st[0] = 0.5f;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = particle->colour[0];
	verts[0].modulate[1] = particle->colour[1];
	verts[0].modulate[2] = particle->colour[2];
	verts[0].modulate[3] = 100 * dist;

	VectorMA( start, -particle->weight, right, verts[1].xyz );
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = particle->colour[0];
	verts[1].modulate[1] = particle->colour[1];
	verts[2].modulate[2] = particle->colour[2];
	verts[1].modulate[3] = 200 * dist;

	VectorMA( start, particle->weight, right, verts[2].xyz );
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = particle->colour[0];
	verts[2].modulate[1] = particle->colour[1];
	verts[2].modulate[2] = particle->colour[2];
	verts[2].modulate[3] = 200 * dist;

	CG_AddPolyToPool( *particle->effectshader, verts );

//	rendertime += trap_Milliseconds() - msec;
}

//	Snow management functions

/*
==============
CG_SnowParticleCheckVisible
==============
*/
static qboolean CG_SnowParticleGenerate( cg_atmosphericParticle_t *particle, vec3_t currvec, float currweight )
{
	// Attempt to 'spot' a snowflake somewhere below a sky texture.
	float angle = random() * 2*M_PI;
	float distance = 20 + MAX_ATMOSPHERIC_DISTANCE * random();
	float groundHeight, skyHeight;
	//	int msec = trap_Milliseconds();
	//	n_generatetime++;

	particle->pos[0] = cg.refdef_current->vieworg[0] + sin(angle) * distance;
	particle->pos[1] = cg.refdef_current->vieworg[1] + cos(angle) * distance;

	// ydnar: choose a spawn point randomly between sky and ground
	skyHeight = BG_GetSkyHeightAtPoint( particle->pos );
	if( skyHeight == MAX_ATMOSPHERIC_HEIGHT )
		return qfalse;
	groundHeight = BG_GetSkyGroundHeightAtPoint( particle->pos );
	if( groundHeight >= skyHeight )
		return qfalse;
	particle->pos[2] = groundHeight + random() * (skyHeight - groundHeight);

	// make sure it doesn't fall from too far cause it then will go over our heads ('lower the ceiling')
	if( cg_atmFx.baseHeightOffset > 0 ) {
		if( particle->pos[2] - cg.refdef_current->vieworg[2] > cg_atmFx.baseHeightOffset ) {
			particle->pos[2] = cg.refdef_current->vieworg[2] + cg_atmFx.baseHeightOffset;
			if( particle->pos[2] < groundHeight )
				return qfalse;
		}
	}

	CG_SetParticleActive( particle, ACT_FALLING );
	VectorCopy( currvec, particle->delta );
	particle->delta[2] += crandom() * 25;
	VectorCopy( particle->delta, particle->deltaNormalized );
	VectorNormalizeFast( particle->deltaNormalized );
	particle->height = ATMOSPHERIC_SNOW_HEIGHT + random() * 2;
	particle->weight = particle->height * 0.5f;
	particle->effectshader = &cg_atmFx.effectshaders[0];
	//	particle->effectshader = &cg_atmFx.effectshaders[ (int) (random() * ( cg_atmFx.numEffectShaders - 1 )) ];

	//	generatetime += trap_Milliseconds() - msec;
	return( qtrue );
}

/*
==============
CG_SnowParticleCheckVisible
==============
*/
static qboolean CG_SnowParticleCheckVisible( cg_atmosphericParticle_t *particle )
{
	// Check the snowflake is visible and still going, wrapping if necessary.
	//	int msec = trap_Milliseconds();

	//	n_checkvisibletime++;

	if( !particle || particle->active == ACT_NOT ) {
		//		checkvisibletime += trap_Milliseconds() - msec;
		return( qfalse );
	}

	{
		float moved = (cg.time - cg_atmFx.lastRainTime) * 0.001;	// Units moved since last frame
		vec2_t distance;

		VectorMA( particle->pos, moved, particle->delta, particle->pos );
		if( particle->pos[2] < BG_GetSkyGroundHeightAtPoint( particle->pos ) ) {
			//		checkvisibletime += trap_Milliseconds() - msec;
			return CG_SetParticleActive( particle, ACT_NOT );
		}

		distance[0] = particle->pos[0] - cg.refdef_current->vieworg[0];
		distance[1] = particle->pos[1] - cg.refdef_current->vieworg[1];
		if( (distance[0] * distance[0] + distance[1] * distance[1]) > Square( MAX_ATMOSPHERIC_DISTANCE ) ) {
			// ydnar: just nuke this particle, let it respawn
			return CG_SetParticleActive( particle, ACT_NOT );
		}
	}
 	//	checkvisibletime += trap_Milliseconds() - msec;
	return( qtrue );
}

/*
==============
CG_SnowParticleRender
==============
*/
static void CG_SnowParticleRender( cg_atmosphericParticle_t *particle )
{
	// Draw a snowflake
	vec3_t		forward, right;
	polyVert_t	verts[3];
	vec2_t		line;
	float		len, sinTumbling, cosTumbling, particleWidth, dist;
	vec3_t		start, finish;
	float		groundHeight;

	//	int			msec = trap_Milliseconds();
	//	n_rendertime++;

	if( particle->active == ACT_NOT ) {
		//		rendertime += trap_Milliseconds() - msec;
		return;
	}

	if( CG_CullPoint( particle->pos ) ) {
		return;
	}

	VectorCopy( particle->pos, start );

	sinTumbling = sin( particle->pos[2] * 0.03125f * ( 0.5f * particle->weight ) );
	cosTumbling = cos( ( particle->pos[2] + particle->pos[1] ) * 0.03125f * ( 0.5f * particle->weight ) );
	start[0] += 24 * ( 1 - particle->deltaNormalized[2] ) * sinTumbling;
	start[1] += 24 * ( 1 - particle->deltaNormalized[2] ) * cosTumbling;

	// Make sure it doesn't clip through surfaces
	groundHeight = BG_GetSkyGroundHeightAtPoint( start );
	len = particle->height;
	if( start[2] <= groundHeight ) {
		float		s;
		// Stop snow going through surfaces.
		len = particle->height - groundHeight + start[2];
		s = len - particle->height;
		VectorMA( start, s, particle->deltaNormalized, start );
	}

	if( len <= 0 ) {
		//		rendertime += trap_Milliseconds() - msec;
		return;
	}

	line[0] = particle->pos[0] - cg.refdef_current->vieworg[0];
	line[1] = particle->pos[1] - cg.refdef_current->vieworg[1];

	dist = DistanceSquared( particle->pos, cg.refdef_current->vieworg );
	// dist becomes scale
	if( dist > Square( 500.f ) ) {
        dist = 1.f + ( ( dist - Square( 500.f ) ) * ( 10.f / Square( 2000.f ) ) );
	}
	else {
		dist = 1.f;
	}

	len *= dist;

	VectorCopy( particle->deltaNormalized, forward );
	VectorMA( start, -( len /** sinTumbling*/ ), forward, finish );

	line[0] = DotProduct( forward, cg.refdef_current->viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef_current->viewaxis[2] );

	VectorScale( cg.refdef_current->viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef_current->viewaxis[2], right );
	VectorNormalize( right );

	particleWidth = dist * (/*cosTumbling **/ particle->weight);

	VectorMA( finish, -particleWidth, right, verts[0].xyz );
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA( start, -particleWidth, right, verts[1].xyz );
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA( start, particleWidth, right, verts[2].xyz );
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	CG_AddPolyToPool( *particle->effectshader, verts );
	//	rendertime += trap_Milliseconds() - msec;
}

/*
==============
CG_EffectGust

Set up gust parameters.
==============
*/
static void CG_EffectGust()
{
	// Generate random values for the next gust
	int diff;

	cg_atmFx.baseEndTime		= cg.time					+ cg_atmFx.baseMinTime		+ (rand() % (cg_atmFx.baseMaxTime - cg_atmFx.baseMinTime));
	diff						= cg_atmFx.changeMaxTime	- cg_atmFx.changeMinTime;
	cg_atmFx.gustStartTime		= cg_atmFx.baseEndTime		+ cg_atmFx.changeMinTime	+ (diff ? (rand() % diff) : 0);
	diff						= cg_atmFx.gustMaxTime		- cg_atmFx.gustMinTime;
	cg_atmFx.gustEndTime		= cg_atmFx.gustStartTime	+ cg_atmFx.gustMinTime		+ (diff ? (rand() % diff) : 0);
	diff						= cg_atmFx.changeMaxTime	- cg_atmFx.changeMinTime;
	cg_atmFx.baseStartTime		= cg_atmFx.gustEndTime		+ cg_atmFx.changeMinTime	+ (diff ? (rand() % diff) : 0);
}

/*
==============
CG_EffectGustCurrent
==============
*/
static qboolean CG_EffectGustCurrent( vec3_t curr, float *weight, int *num )
{
	// Calculate direction for new drops.
	if( cg.time < cg_atmFx.baseEndTime ) {
		VectorCopy( cg_atmFx.baseVec, curr );
		*weight = cg_atmFx.baseWeight;
		*num = cg_atmFx.baseDrops;
	}
	else {
		vec3_t temp;

		VectorSubtract( cg_atmFx.gustVec, cg_atmFx.baseVec, temp );
		if( cg.time < cg_atmFx.gustStartTime ) {
			float frac = ((float)(cg.time - cg_atmFx.baseEndTime))/((float)(cg_atmFx.gustStartTime - cg_atmFx.baseEndTime));

			VectorMA( cg_atmFx.baseVec, frac, temp, curr );
			*weight = cg_atmFx.baseWeight + (cg_atmFx.gustWeight - cg_atmFx.baseWeight) * frac;
			*num = cg_atmFx.baseDrops + ((float)(cg_atmFx.gustDrops - cg_atmFx.baseDrops)) * frac;
		}
		else if( cg.time < cg_atmFx.gustEndTime ) {
			VectorCopy( cg_atmFx.gustVec, curr );
			*weight = cg_atmFx.gustWeight;
			*num = cg_atmFx.gustDrops;
		}
		else {
			float frac = 1.0 - ((float)(cg.time - cg_atmFx.gustEndTime))/((float)(cg_atmFx.baseStartTime - cg_atmFx.gustEndTime));

			VectorMA( cg_atmFx.baseVec, frac, temp, curr );
			*weight = cg_atmFx.baseWeight + (cg_atmFx.gustWeight - cg_atmFx.baseWeight) * frac;
			*num = cg_atmFx.baseDrops + ((float)(cg_atmFx.gustDrops - cg_atmFx.baseDrops)) * frac;
			if( cg.time >= cg_atmFx.baseStartTime )
				return( qtrue );
		}
	}
	return( qfalse );
}

/*
==============
CG_EP_ParseFloats
==============
*/
static void CG_EP_ParseFloats( char *floatstr, float *f1, float *f2 )
{
	// Parse the float or floats
	char *middleptr;
	char buff[64];

	Q_strncpyz( buff, floatstr, sizeof(buff) );
	for( middleptr = buff; *middleptr && *middleptr != ' '; middleptr++ );
	if( *middleptr ) {
		*middleptr++ = 0;
		*f1 = atof( floatstr );
		*f2 = atof( middleptr );
	}
	else {
		*f1 = *f2 = atof( floatstr );
	}
}

/*
==============
CG_EP_ParseInts
==============
*/
static void CG_EP_ParseInts( char *intstr, int *i1, int *i2 )
{
	// Parse the int or ints
	char *middleptr;
	char buff[64];

	Q_strncpyz( buff, intstr, sizeof(buff) );
	for( middleptr = buff; *middleptr && *middleptr != ' '; middleptr++ );
	if( *middleptr ) {
		*middleptr++ = 0;
		*i1 = atof( intstr );
		*i2 = atof( middleptr );
	}
	else {
		*i1 = *i2 = atof( intstr );
	}
}

/*
==============
CG_EffectParse
==============
*/
void CG_EffectParse( const char *effectstr )
{
	// Split the string into it's component parts.
	float bmin, bmax, cmin, cmax, gmin, gmax, bdrop, gdrop/*, wsplash, lsplash*/;
	int count, bheight;
	char *startptr, *eqptr, *endptr;
	char workbuff[128];
	atmFXType_t atmFXType = ATM_NONE;

	if( CG_AtmosphericKludge() ) {
		return;
	}

	// Set up some default values
	cg_atmFx.baseVec[0] = cg_atmFx.baseVec[1] = 0;
	cg_atmFx.gustVec[0] = cg_atmFx.gustVec[1] = 100;
	bmin = 5;
	bmax = 10;
	cmin = 1;
	cmax = 1;
	gmin = 0;
	gmax = 2;
	bdrop = gdrop = 300;
	cg_atmFx.baseWeight = 0.7f;
	cg_atmFx.gustWeight = 1.5f;
	bheight = 0;

		// Parse the parameter string
	Q_strncpyz( workbuff, effectstr, sizeof(workbuff) );
	for( startptr = workbuff; *startptr; ) {
		for( eqptr = startptr; *eqptr && *eqptr != '=' && *eqptr != ','; eqptr++ );
		if( !*eqptr )
			break;			// No more string
		if( *eqptr == ',' ) {
			startptr = eqptr + 1;	// Bad argument, continue
			continue;
		}
		*eqptr++ = 0;
		for( endptr = eqptr; *endptr && *endptr != ','; endptr++ );
		if( *endptr )
			*endptr++ = 0;

		if( atmFXType == ATM_NONE ) {
			if( Q_stricmp( startptr, "T" ) ) {
				cg_atmFx.numDrops = 0;
				CG_Printf( "Atmospheric effect must start with a type.\n" );
				return;
			}
			if( !Q_stricmp( eqptr, "RAIN" ) ) {
				atmFXType = ATM_RAIN;
				cg_atmFx.ParticleCheckVisible = &CG_RainParticleCheckVisible;
				cg_atmFx.ParticleGenerate = &CG_RainParticleGenerate;
				cg_atmFx.ParticleRender = &CG_RainParticleRender;

				cg_atmFx.baseVec[2] = cg_atmFx.gustVec[2] = - ATMOSPHERIC_RAIN_SPEED;
			}
			else if( !Q_stricmp( eqptr, "SNOW" ) ) {
				atmFXType = ATM_SNOW;
				cg_atmFx.ParticleCheckVisible = &CG_SnowParticleCheckVisible;
				cg_atmFx.ParticleGenerate = &CG_SnowParticleGenerate;
				cg_atmFx.ParticleRender = &CG_SnowParticleRender;

				cg_atmFx.baseVec[2] = cg_atmFx.gustVec[2] = - ATMOSPHERIC_SNOW_SPEED;
			}
			else {
				cg_atmFx.numDrops = 0;
				CG_Printf( "Only effect type 'rain' and 'snow' are supported.\n" );
				return;
			}
		}
		else {
			if( !Q_stricmp( startptr, "B" ) )
				CG_EP_ParseFloats( eqptr, &bmin, &bmax );
			else if( !Q_stricmp( startptr, "C" ) )
				CG_EP_ParseFloats( eqptr, &cmin, &cmax );
			else if( !Q_stricmp( startptr, "G" ) )
				CG_EP_ParseFloats( eqptr, &gmin, &gmax );
			else if( !Q_stricmp( startptr, "BV" ) )
				CG_EP_ParseFloats( eqptr, &cg_atmFx.baseVec[0], &cg_atmFx.baseVec[1] );
			else if( !Q_stricmp( startptr, "GV" ) )
				CG_EP_ParseFloats( eqptr, &cg_atmFx.gustVec[0], &cg_atmFx.gustVec[1] );
			else if( !Q_stricmp( startptr, "W" ) )
				CG_EP_ParseFloats( eqptr, &cg_atmFx.baseWeight, &cg_atmFx.gustWeight );
			else if( !Q_stricmp( startptr, "D" ) )
				CG_EP_ParseFloats( eqptr, &bdrop, &gdrop );
			else if( !Q_stricmp( startptr, "H" ) )
				CG_EP_ParseInts( eqptr, &bheight, &bheight );
			else CG_Printf( "Unknown effect key '%s'.\n", startptr );
		}
		startptr = endptr;
	}

	if( atmFXType == ATM_NONE || !BG_LoadTraceMap( cgs.rawmapname, cg.mapcoordsMins, cg.mapcoordsMaxs ) ) {
		// No effects

		cg_atmFx.numDrops = -1;
		return;
	}

	cg_atmFx.baseHeightOffset = bheight;
	if( cg_atmFx.baseHeightOffset < 0 ) {
		cg_atmFx.baseHeightOffset = 0;
	}
	cg_atmFx.baseMinTime = SECONDS_1 * bmin;
	cg_atmFx.baseMaxTime = SECONDS_1 * bmax;
	cg_atmFx.changeMinTime = SECONDS_1 * cmin;
	cg_atmFx.changeMaxTime = SECONDS_1 * cmax;
	cg_atmFx.gustMinTime = SECONDS_1 * gmin;
	cg_atmFx.gustMaxTime = SECONDS_1 * gmax;
	cg_atmFx.baseDrops = bdrop;
	cg_atmFx.gustDrops = gdrop;

	cg_atmFx.numDrops = (cg_atmFx.baseDrops > cg_atmFx.gustDrops) ? cg_atmFx.baseDrops : cg_atmFx.gustDrops;
	if( cg_atmFx.numDrops > MAX_ATMOSPHERIC_PARTICLES ) {
		cg_atmFx.numDrops = MAX_ATMOSPHERIC_PARTICLES;
	}
	// Load graphics

	// Rain
	if( atmFXType == ATM_RAIN ) {
		cg_atmFx.numEffectShaders = 1;
		cg_atmFx.effectshaders[0] = trap_R_RegisterShader( "gfx/misc/raindrop" );
		if( !(cg_atmFx.effectshaders[0]) ) {
			cg_atmFx.effectshaders[0] = -1;
			cg_atmFx.numEffectShaders = 0;
		}

	// Snow
	}
	else if( atmFXType == ATM_SNOW ) {
		cg_atmFx.numEffectShaders = 1;
		cg_atmFx.effectshaders[0] = trap_R_RegisterShader( "gfx/misc/snow" );

	// This really should never happen
	}
	else {
		cg_atmFx.numEffectShaders = 0;
	}

		// Initialise atmospheric effect to prevent all particles falling at the start
	for( count = 0; count < cg_atmFx.numDrops; count++ ) {
		cg_atmFx.particles[count].nextDropTime = ATMOSPHERIC_DROPDELAY + (rand() % ATMOSPHERIC_DROPDELAY);
	}
	CG_EffectGust();
}

/*
==============
CG_AddAtmosphericEffects

Main render loop
==============
*/
void CG_AddAtmosphericEffects()
{
	// Add atmospheric effects (e.g. rain, snow etc.) to view
	int curr, max, currnum;
	cg_atmosphericParticle_t *particle;
	vec3_t currvec;
	float currweight;

	if( cg_atmFx.numDrops <= 0 || cg_atmFx.numEffectShaders == 0 || cg_atmosphericEffects.value <= 0 ) {
		return;
	}

#ifndef ATM_NEW
	CG_ClearPolyPool();
#endif // ATM_NEW

	max = cg_atmosphericEffects.value < 1 ? cg_atmosphericEffects.value * cg_atmFx.numDrops : cg_atmFx.numDrops;
	if( CG_EffectGustCurrent( currvec, &currweight, &currnum ) ) {
		CG_EffectGust();			// Recalculate gust parameters
	}

	// ydnar: allow parametric management of drop count for swelling/waning precip
	cg_atmFx.oldDropsActive = cg_atmFx.dropsActive;
	cg_atmFx.dropsActive = 0;

	cg_atmFx.dropsRendered = cg_atmFx.dropsCreated = cg_atmFx.dropsSkipped = 0;

	//	getgroundtime = getskytime = rendertime = checkvisibletime = generatetime = 0;
	//	n_getgroundtime = n_getskytime = n_rendertime = n_checkvisibletime = n_generatetime = 0;

	VectorSet( cg_atmFx.viewDir, cg.refdef_current->viewaxis[0][0], cg.refdef_current->viewaxis[0][1], 0.f );

	for( curr = 0; curr < max; curr++ ) {
		particle = &cg_atmFx.particles[curr];

		if( !cg_atmFx.ParticleCheckVisible( particle ) ) {

			if( !cg_atmFx.ParticleGenerate( particle, currvec, currweight ) ) {
				// Ensure it doesn't attempt to generate every frame, to prevent
				// 'clumping' when there's only a small sky area available.
				particle->nextDropTime = cg.time + ATMOSPHERIC_DROPDELAY;
				continue;
			}
			else {
				cg_atmFx.dropsCreated++;
			}
		}

		cg_atmFx.ParticleRender( particle );
		cg_atmFx.dropsActive++;
	}

	cg_atmFx.lastRainTime = cg.time;

	//	CG_Printf( "Active: %d Generated: %d Rendered: %d Skipped: %d\n", cg_atmFx.dropsActive, cg_atmFx.dropsCreated, cg_atmFx.dropsRendered, cg_atmFx.dropsSkipped );
	//	CG_Printf( "gg: %i gs: %i rt: %i cv: %i ge: %i\n", getgroundtime, getskytime, rendertime, checkvisibletime, generatetime );
	//	CG_Printf( "\\-> %i \\-> %i \\-> %i \\-> %i \\-> %i\n", n_getgroundtime, n_getskytime, n_rendertime, n_checkvisibletime, n_generatetime );
}
