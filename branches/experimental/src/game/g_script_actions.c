//===========================================================================
//
// Name:			g_script_actions.c
// Function:		Wolfenstein Entity Scripting
// Programmer:		Ridah
// Tab Size:		4 (real tabs)
//===========================================================================

#include "../game/g_local.h"
#include "../game/q_shared.h"

#ifdef OMNIBOTS
#include "../game/g_etbot_interface.h"
#endif

/*
Contains the code to handle the various commands available with an event script.

These functions will return true if the action has been performed, and the script
should proceed to the next item on the list.
*/

void script_linkentity(gentity_t *ent);

/*
===============
G_ScriptAction_SetModelFromBrushmodel
===============
*/
qboolean G_ScriptAction_SetModelFromBrushmodel( gentity_t *ent, char *params )
{
	char	*pString = params;
	char    *token = COM_ParseExt( &pString, qfalse );
	char	modelname[MAX_QPATH];
	int 	i;
	qboolean solid = qtrue;


	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetModelFromBrushmodel: setmodelfrombrushmodel must have an targetname\n" );
	}
	Q_strncpyz( modelname, token, MAX_QPATH );

	ent->r.svFlags &= ~SVF_IGNOREBMODELEXTENTS;

	while (*token) {
		if(!Q_stricmp( token, "useoriginforpvs" )) {
			ent->r.svFlags |= SVF_IGNOREBMODELEXTENTS;
		}
		else if(!Q_stricmp( token, "nonsolid" )) {
			solid = qfalse;
		}

		token = COM_ParseExt( &pString, qfalse );
	}

	if ( modelname[0] == '*'){
		trap_SetBrushModel( ent, modelname );

		if(!solid) {
			ent->s.eFlags |= EF_NONSOLID_BMODEL;
			ent->clipmask = 0;
			ent->r.contents = 0;
			trap_LinkEntity( ent );
		}

		return qtrue;
	}
	else for(i = 0; i < level.numBrushModels; ++i) {
		if(!Q_stricmp( level.brushModelInfo[i].modelname, modelname )) {
			trap_SetBrushModel( ent, va( "*%i", level.brushModelInfo[i].model ) );

			if(!solid) {
				ent->s.eFlags |= EF_NONSOLID_BMODEL;
				ent->clipmask = 0;
				ent->r.contents = 0;

				trap_LinkEntity( ent );
			}

			return qtrue;
		}
	}

	G_Error(S_COLOR_RED "G_ScriptAction_SetModelFromBrushmodel(): setmodelfrombrushmodel target not found %s\n", modelname );

	return qtrue;
}

/*
===============
G_ScriptAction_SetPosition
===============
*/
qboolean G_ScriptAction_SetPosition( gentity_t *ent, char *params )
{
	pathCorner_t* pPathCorner;
	char	*pString = params;
	char *token = COM_ParseExt( &pString, qfalse );

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_Scripting(): setposition must have an targetname\n" );
	}

	if ((pPathCorner = BG_Find_PathCorner( token ))) {
		G_SetOrigin( ent, pPathCorner->origin );
	}
	else {
		// find the entity with the given "targetname"
		gentity_t *target = G_FindByTargetname( NULL, token );
		if (!target) {
			G_Error(S_COLOR_RED "G_Scripting(): can't find entity with \"targetname\" = \"%s\"\n", token );
		}

		G_SetOrigin( ent, target->r.currentOrigin );
		if(ent->client) {
			VectorCopy( target->r.currentOrigin, ent->client->ps.origin );
		}
	}

	return qtrue;
}

void SetPlayerSpawn( gentity_t* ent, int spawn, qboolean update );

/*
===============
G_ScriptAction_SetAutoSpawn
===============
*/
qboolean G_ScriptAction_SetAutoSpawn( gentity_t* ent, char *params )
{
	char		*pString = params;
	char 		*token	= COM_ParseExt( &pString, qfalse );
	char		spawnname[MAX_QPATH];
	team_t		team;
	int*		pTeamAutoSpawn;
	gentity_t	*tent;

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetAutoSpawn(): setautospawn must have a target spawn\n" );
	}

	Q_strncpyz( spawnname, token, MAX_QPATH );

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetAutoSpawn(): setautospawn must have a target team\n" );
	}

	team = atoi(token);
	pTeamAutoSpawn = team == 0 ? &(level.axisAutoSpawn) : &(level.alliesAutoSpawn);

	tent = G_Find( NULL, FOFS(message), spawnname );
	if(!tent) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetAutoSpawn(): setautospawn, couldn't find target\n" );
	}

	if( !tent->count ) {
		return qfalse;
	}

	G_Printf( "Setting %s autospawn to %s\n", team == 0 ? "Axis" : "Allied", spawnname );

	*pTeamAutoSpawn = tent->count - CS_MULTI_SPAWNTARGETS;

	G_UpdateSpawnCounts();

	return qtrue;
}

/*
===============
G_ScriptAction_ChangeModel
===============
*/
qboolean G_ScriptAction_ChangeModel( gentity_t* ent, char *params )
{
	char	*pString = params;
	char 	*token = COM_ParseExt( &pString, qfalse );
	char	tagname[MAX_QPATH];

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_ChangeModel(): changemodel must have a target model name\n" );
	}

	COM_StripExtensionSafe( token, tagname , sizeof(tagname));
	Q_strcat( tagname, MAX_QPATH, ".tag" );
	ent->tagNumber = trap_LoadTag( tagname );

	ent->s.modelindex2 = G_ModelIndex( token );

	return qtrue;
}

/*
===============
G_ScriptAction_ShaderRemap
===============
*/
qboolean G_ScriptAction_ShaderRemap( gentity_t* ent, char *params ) {
	char *pString = params;
	char *token = COM_ParseExt( &pString, qfalse );
	float f = level.time * 0.001;
	char oldShader[256];
	char newShader[256];

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_ShaderRemap(): remapshader must have a target shader name\n" );
	}
	Q_strncpyz( oldShader, token, 256 );

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_ShaderRemap(): remapshader must have a new shader name\n" );
	}
	Q_strncpyz( newShader, token, 256 );

	AddRemap(oldShader, newShader, f);

	return qtrue;
}

/*
===============
G_ScriptAction_ShaderRemapFlush
===============
*/
qboolean G_ScriptAction_ShaderRemapFlush( gentity_t* ent, char *params ) {
	trap_SetConfigstring( CS_SHADERSTATE, BuildShaderStateConfig() );
	return qtrue;
}

/*
===============
G_ScriptAction_FollowPath
===============
*/
qboolean G_ScriptAction_FollowPath( gentity_t* ent, char *params ) {
	if (params && (ent->scriptStatus.scriptFlags & SCFL_GOING_TO_MARKER)) {
		// we can't process a new movement until the last one has finished
		return qfalse;
	}

	if (!params || ent->scriptStatus.scriptStackChangeTime < level.time) {			// we are waiting for it to reach destination
		if (ent->s.pos.trTime + ent->s.pos.trDuration <= level.time) {	// we made it
			ent->scriptStatus.scriptFlags &= ~SCFL_GOING_TO_MARKER;

			// set the angles at the destination
			BG_EvaluateTrajectory( &ent->s.apos, ent->s.apos.trTime + ent->s.apos.trDuration, ent->s.angles, qtrue, ent->s.effect2Time );
			VectorCopy( ent->s.angles, ent->s.apos.trBase );
			VectorCopy( ent->s.angles, ent->r.currentAngles );
			ent->s.apos.trTime = level.time;
			ent->s.apos.trDuration = 0;
			ent->s.apos.trType = TR_STATIONARY;
			VectorClear( ent->s.apos.trDelta );

			// stop moving
			BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin, qfalse, ent->s.effect2Time );
			VectorCopy( ent->s.origin, ent->s.pos.trBase );
			VectorCopy( ent->s.origin, ent->r.currentOrigin );
			ent->s.pos.trTime = level.time;
			ent->s.pos.trDuration = 0;
			ent->s.pos.trType = TR_STATIONARY;
			VectorClear( ent->s.pos.trDelta );

			script_linkentity(ent);

			return qtrue;
		}
	}
	else {	// we have just started this command
		char	*pString = params;
		char	*token = COM_ParseExt( &pString, qfalse );
		float	speed;
		qboolean wait = qfalse;
		int backward;
		float length = 0;
		float dist;
		int i;

		splinePath_t* pSpline;

		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_FollowPath(): followpath must have a direction\n" );
		}

		backward = atoi(token);

		token = COM_ParseExt( &pString, qfalse );
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_FollowPath(): followpath must have an targetname\n" );
		}

		if(!(pSpline = BG_Find_Spline( token ))) {
			G_Error(S_COLOR_RED "G_ScriptAction_FollowPath(): can't find spline with \"targetname\" = \"%s\"\n", token );
		}

		token = COM_ParseExt( &pString, qfalse );
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_FollowPath(): followpath must have a speed\n" );
		}

		speed = atof(token);

		while (token[0]) {
			token = COM_ParseExt( &pString, qfalse );
			if (token[0]) {
				if (!Q_stricmp( token, "wait" )) {
					wait = qtrue;
				}

				if (!Q_stricmp( token, "length" )) {
					token = COM_ParseExt( &pString, qfalse );
					if (!token[0]) {
						G_Error(S_COLOR_RED "G_ScriptAction_FollowPath(): length must have a value\n" );
					}

					length = atoi(token);
				}
			}
		}

		// calculate the trajectory
		ent->s.apos.trType = ent->s.pos.trType = TR_LINEAR_PATH;
		ent->s.apos.trTime = ent->s.pos.trTime = level.time;

		ent->s.apos.trBase[0] = length;

		ent->s.effect2Time = backward ? -(pSpline - splinePaths + 1) : pSpline - splinePaths + 1;

		VectorClear( ent->s.pos.trDelta );

		dist = 0;
		for( i = 0; i < MAX_SPLINE_SEGMENTS; ++i ) {
			dist += pSpline->segments[i].length;
		}

		ent->s.apos.trDuration = ent->s.pos.trDuration = 1000 * (dist / speed);

		if (!wait) {
			// round the duration to the next 50ms
			if (ent->s.pos.trDuration%50) {
				float frac;

				frac = (float)(((ent->s.pos.trDuration/50)*50 + 50) - ent->s.pos.trDuration) / (float)(ent->s.pos.trDuration);
				if (frac < 1) {
					ent->s.apos.trDuration = ent->s.pos.trDuration = (ent->s.pos.trDuration/50)*50 + 50;
				}
			}

			// set the goto flag, so we can keep processing the move until we reach the destination
			ent->scriptStatus.scriptFlags |= SCFL_GOING_TO_MARKER;
			return qtrue;	// continue to next command
		}

	}

	BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->r.currentOrigin, qfalse, ent->s.effect2Time  );
	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles, qtrue, ent->s.effect2Time  );
	script_linkentity(ent);

	return qfalse;
}

/*
===============
G_ScriptAction_AttatchToTrain
===============
*/
qboolean G_ScriptAction_AttatchToTrain( gentity_t* ent, char *params ) {
	char* pString = params;
	char* token = COM_ParseExt( &pString, qfalse );
	gentity_t* target;

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_AttatchToTrain(): attatchtotrain must have a target\n" );
	}

	target = G_FindByTargetname( NULL, token );
	if (!target) {
		G_Error(S_COLOR_RED "G_ScriptAction_AttatchToTrain(): can't find entity with \"targetname\" = \"%s\"\n", token );
	}

	ent->s.torsoAnim = target->s.number;

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_AttatchToTrain(): attatchtotrain must have a length\n" );
	}

	ent->s.angles2[0] = atoi(token);
	ent->s.eFlags |= EF_PATH_LINK;

	return qtrue;
}

/*
===============
G_ScriptAction_FreezeAnimation
===============
*/
qboolean G_ScriptAction_FreezeAnimation( gentity_t* ent, char *params ) {
	ent->s.loopSound = 1;

	return qtrue;
}

/*
===============
G_ScriptAction_UnFreezeAnimation
===============
*/
qboolean G_ScriptAction_UnFreezeAnimation( gentity_t* ent, char *params ) {
	ent->s.loopSound = 0;
	return qtrue;
}

/*
===============
G_ScriptAction_StartAnimation
===============
*/
qboolean G_ScriptAction_StartAnimation( gentity_t* ent, char *params ) {
	char* pString = params;
	char* token = COM_ParseExt( &pString, qfalse );
	qboolean norandom = qfalse;
	qboolean nolerp = qfalse;
	qboolean noloop =	qfalse;

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_StartAnimation(): startanimation must have a start frame\n" );
	}

	ent->s.legsAnim = atoi(token);

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_StartAnimation(): startanimation must have a frame count\n" );
	}

	ent->s.torsoAnim = atoi(token);

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_StartAnimation(): startanimation must have an fps count\n" );
	}

	ent->s.weapon = 1000.f / atoi(token);

	while (token[0]) {
		token = COM_ParseExt( &pString, qfalse );
		if (token[0]) {
			if (!Q_stricmp( token, "norandom" )) {
				norandom = qtrue;
			}

			if (!Q_stricmp( token, "nolerp" )) {
				nolerp = qtrue;
			}

			if (!Q_stricmp( token, "noloop" )) {
				noloop = qtrue;
			}
		}
	}

	if(norandom) {
		ent->s.frame = 0;
	}
	else {
		ent->s.frame = rand() % ent->s.torsoAnim;
	}

	if(noloop) {
		ent->s.clientNum = 1;
	}
	else {
		ent->s.clientNum = 0;
	}

	if(nolerp) {
		ent->s.animMovetype++;
	}

	return qtrue;
}

/*
===============
G_ScriptAction_SetSpeed
===============
*/
qboolean G_ScriptAction_SetSpeed( gentity_t* ent, char *params ) {
	vec3_t speed;
	char* pString = params;
	int i;
	char* token;
	qboolean gravity = qfalse;
	qboolean lowgravity = qfalse;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->r.currentOrigin, qtrue, ent->s.effect2Time  );
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );

	for(i = 0; i < 3; ++i) {
		token = COM_Parse(&pString);
		if (!token || !*token) {
			G_Error(S_COLOR_RED "G_ScriptAction_SetSpeed(): syntax: setspeed <x> <y> <z> [gravity|lowgravity]\n" );
		}
		speed[i] = atoi(token);
	}

	while((token = COM_Parse(&pString)) && *token) {
		if(!Q_stricmp( token, "gravity" )) {
			gravity = qtrue;
		}
		else if(!Q_stricmp( token, "lowgravity" )) {
			lowgravity = qtrue;
		}
	}

	if(gravity) {
		ent->s.pos.trType = TR_GRAVITY;
	}
	else if(lowgravity) {
		ent->s.pos.trType = TR_GRAVITY_LOW;
	}
	else {
		ent->s.pos.trType = TR_LINEAR;
	}
	ent->s.pos.trTime = level.time;

	VectorCopy( speed, ent->s.pos.trDelta );

	script_linkentity(ent);

	return qtrue;
}

/*
===============
G_ScriptAction_SetRotation
===============
*/
qboolean G_ScriptAction_SetRotation( gentity_t* ent, char *params ) {
	vec3_t angles;
	char* pString = params;
	int i;
	char* token;

	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles, qtrue, ent->s.effect2Time  );
	VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );

	ent->s.apos.trType = TR_LINEAR;
	ent->s.apos.trTime = level.time;

	for (i=0; i<3; ++i) {
		token = COM_Parse(&pString);
		if (!token || !token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_SetRotation(): syntax: setrotation <pitchspeed> <yawspeed> <rollspeed>\n" );
		}
		angles[i] = atoi(token);
	}

	VectorCopy( angles, ent->s.apos.trDelta );
	script_linkentity(ent);

	return qtrue;
}

/*
===============
G_ScriptAction_StopRotation
===============
*/
qboolean G_ScriptAction_StopRotation( gentity_t* ent, char *params ) {
	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles, qtrue, ent->s.effect2Time  );
	VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
	ent->s.apos.trTime = level.time;
	ent->s.apos.trDuration = 0;
	ent->s.apos.trType = TR_STATIONARY;
	VectorClear( ent->s.apos.trDelta );

	return qtrue;
}

/*
===============
G_ScriptAction_FollowSpline

  syntax: followspline <targetname> <speed> [wait]

  NOTE: speed may be modified to round the duration to the next 50ms for smooth
  transitions
===============
*/
qboolean G_ScriptAction_FollowSpline( gentity_t* ent, char *params ) {
	if (params && (ent->scriptStatus.scriptFlags & SCFL_GOING_TO_MARKER)) {
		// we can't process a new movement until the last one has finished
		return qfalse;
	}

	if (!params || ent->scriptStatus.scriptStackChangeTime < level.time) {			// we are waiting for it to reach destination
		if (ent->s.pos.trTime + ent->s.pos.trDuration <= level.time) {	// we made it
			ent->scriptStatus.scriptFlags &= ~SCFL_GOING_TO_MARKER;

			// set the angles at the destination
			BG_EvaluateTrajectory( &ent->s.apos, ent->s.apos.trTime + ent->s.apos.trDuration, ent->s.angles, qtrue, ent->s.effect2Time );
			VectorCopy( ent->s.angles, ent->s.apos.trBase );
			VectorCopy( ent->s.angles, ent->r.currentAngles );
			ent->s.apos.trTime = level.time;
			ent->s.apos.trDuration = 0;
			ent->s.apos.trType = TR_STATIONARY;
			VectorClear( ent->s.apos.trDelta );

			// stop moving
			BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin, qfalse, ent->s.effect2Time );
			VectorCopy( ent->s.origin, ent->s.pos.trBase );
			VectorCopy( ent->s.origin, ent->r.currentOrigin );
			ent->s.pos.trTime = level.time;
			ent->s.pos.trDuration = 0;
			ent->s.pos.trType = TR_STATIONARY;
			VectorClear( ent->s.pos.trDelta );

			script_linkentity(ent);

			return qtrue;
		}
	}
	else {	// we have just started this command
		char	*pString	= params;
		char 	*token 		= COM_ParseExt( &pString, qfalse );
		vec3_t 	vec;
		float	speed;
		qboolean wait = qfalse;
		int 	backward;
		float 	length = 0;
		float 	roll[2] = { 0, 0 };

		splinePath_t* pSpline;

		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_FollowSpline(): followspline must have a direction\n" );
		}

		if (!Q_stricmp( token, "accum" )) {
			int bufferIndex;

			token = COM_ParseExt( &pString, qfalse );
			if (!token[0]) {
				G_Error(S_COLOR_RED "G_ScriptAction_FollowSpline(): accum without a buffer index\n" );
			}

			bufferIndex = atoi(token);
			if (bufferIndex < 0 || bufferIndex >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
				G_Error(S_COLOR_RED "G_ScriptAction_FollowSpline(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
			}

			backward = ent->scriptAccumBuffer[bufferIndex] != 0 ? qtrue : qfalse;
		}
		else if (!Q_stricmp( token, "globalaccum" )) {
			int bufferIndex;

			token = COM_ParseExt( &pString, qfalse );
			if (!token[0]) {
				// CHRUKER: b056 - Help text said accum
				G_Error(S_COLOR_RED "G_ScriptAction_FollowSpline(): globalaccum without a buffer index\n" );
			}

			bufferIndex = atoi(token);
			// CHRUKER: b055 - Was using G_MAX_SCRIPT_ACCUM_BUFFERS, which can result in a wrong index
			if (bufferIndex < 0 || bufferIndex >= MAX_SCRIPT_ACCUM_BUFFERS) {
				// CHURKER: b056 - Help text said accum
				G_Error(S_COLOR_RED "G_ScriptAction_FollowSpline(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
			}

			backward = level.globalAccumBuffer[bufferIndex] != 0 ? qtrue : qfalse;
		}
		else {
			backward = atoi(token);
		}

		token = COM_ParseExt( &pString, qfalse );
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_FollowSpline(): followspline must have an targetname\n" );
		}

		if(!(pSpline = BG_Find_Spline( token ))) {
			G_Error(S_COLOR_RED "G_ScriptAction_FollowSpline(): can't find spline with \"targetname\" = \"%s\"\n", token );
		}
		VectorSubtract( pSpline->point.origin, ent->r.currentOrigin, vec );

		token = COM_ParseExt( &pString, qfalse );
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_FollowSpline(): followspline must have a speed\n" );
		}

		speed = atof(token) * (float)g_moverScale.value;

		while (token[0]) {
			token = COM_ParseExt( &pString, qfalse );
			if (token[0]) {
				if (!Q_stricmp( token, "wait" )) {
					wait = qtrue;
				}

				if (!Q_stricmp( token, "length" )) {
					token = COM_ParseExt( &pString, qfalse );
					if (!token[0]) {
						G_Error(S_COLOR_RED "G_ScriptAction_FollowSpline(): length must have a value\n" );
					}

					length = atoi(token);
				}

				if (!Q_stricmp( token, "roll" )) {
					token = COM_ParseExt( &pString, qfalse );
					if (!token[0]) {
						G_Error(S_COLOR_RED "G_ScriptAction_FollowSpline(): roll must have a start angle\n" );
					}

					roll[0] = atoi(token);

					token = COM_ParseExt( &pString, qfalse );
					if (!token[0]) {
						G_Error(S_COLOR_RED "G_ScriptAction_FollowSpline(): roll must have an end angle\n" );
					}

					roll[1] = atoi(token);
				}

				if (!Q_stricmp( token, "dampin" )) {
					if(roll[0] >= 0) {
						roll[0] += 1000;
					}
					else {
						roll[0] -= 1000;
					}
				}

				if (!Q_stricmp( token, "dampout" )) {
					if(roll[0] >= 0) {
						roll[0] += 10000;
					}
					else {
						roll[0] -= 10000;
					}
				}
			}
		}

		// calculate the trajectory
		ent->s.apos.trType = ent->s.pos.trType = TR_SPLINE;
		ent->s.apos.trTime = ent->s.pos.trTime = level.time;

		ent->s.apos.trBase[0] = length;
		ent->s.apos.trBase[1] = roll[0];
		ent->s.apos.trBase[2] = roll[1];

		ent->s.effect2Time = backward ? -(pSpline - splinePaths + 1) : pSpline - splinePaths + 1;

		VectorClear( ent->s.pos.trDelta );
		ent->s.apos.trDuration = ent->s.pos.trDuration = 1000 * (pSpline->length / speed);

		if (!wait) {
			// round the duration to the next 50ms
			if (ent->s.pos.trDuration%50) {
				float frac;

				frac = (float)(((ent->s.pos.trDuration/50)*50 + 50) - ent->s.pos.trDuration) / (float)(ent->s.pos.trDuration);
				if (frac < 1) {
					ent->s.apos.trDuration = ent->s.pos.trDuration = (ent->s.pos.trDuration/50)*50 + 50;
				}
			}

			// set the goto flag, so we can keep processing the move until we reach the destination
			ent->scriptStatus.scriptFlags |= SCFL_GOING_TO_MARKER;
			return qtrue;	// continue to next command
		}

	}

	BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->r.currentOrigin, qfalse, ent->s.effect2Time  );
	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles, qtrue, ent->s.effect2Time  );
	script_linkentity(ent);

	return qfalse;
}

/*
===============
G_ScriptAction_AbortMove

  syntax: abortmove
===============
*/
qboolean G_ScriptAction_AbortMove( gentity_t *ent, char *params ) {

	ent->scriptStatus.scriptFlags &= ~SCFL_GOING_TO_MARKER;

	// set the angles at the destination
	BG_EvaluateTrajectory( &ent->s.apos, ent->s.apos.trTime + ent->s.apos.trDuration, ent->s.angles, qtrue, ent->s.effect2Time );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	VectorCopy( ent->s.angles, ent->r.currentAngles );
	ent->s.apos.trTime = level.time;
	ent->s.apos.trDuration = 0;
	ent->s.apos.trType = TR_STATIONARY;
	VectorClear( ent->s.apos.trDelta );

	// stop moving
	BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin, qfalse, ent->s.effect2Time );
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
	ent->s.pos.trTime = level.time;
	ent->s.pos.trDuration = 0;
	ent->s.pos.trType = TR_STATIONARY;
	VectorClear( ent->s.pos.trDelta );

	script_linkentity(ent);

	return qtrue;
}

/*
===============
G_ScriptAction_SetChargeTimeFactor

  syntax: setchargetimefactor <team> <class> <factor>

  team: 0 = axis, 1 = allies
===============
*/
qboolean G_ScriptAction_SetChargeTimeFactor( gentity_t* ent, char *params ) {
	char	*pString = params;
	char  	*token = COM_ParseExt( &pString, qfalse );
	float	factor;
	team_t	team;
	char	playerclass[64];

	if( !token[0] ) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetChargeTimeFactor(): setchargetimefactor must have a team\n" );
	}

	team = atoi(token);

	token = COM_ParseExt( &pString, qfalse );
	if( !token[0] ) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetChargeTimeFactor(): setchargetimefactor must have a class name\n" );
	}

	Q_strncpyz( playerclass, token, sizeof(playerclass) );

	token = COM_ParseExt( &pString, qfalse );

	if( !token[0] ) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetChargeTimeFactor(): setchargetimefactor must have a factor\n" );
	}

	factor = atof(token);

	if( factor < 0.f ) {
		G_Printf( S_COLOR_YELLOW "WARNING: setchargetimefactor with factor < 0, clamped to 0\n" );
		factor = 0;
	}
	else if( factor > 1.f ) {
		G_Printf( S_COLOR_YELLOW "WARNING: setchargetimefactor with factor > 1, clamped to 1\n" );
		factor = 1.f;
	}

	if( !Q_stricmp( playerclass, "soldier" ) ) {
		level.soldierChargeTimeModifier[team] = factor;
		level.soldierChargeTime[team] = g_soldierChargeTime.integer * factor;
	}
	else if( !Q_stricmp( playerclass, "medic" ) ) {
		level.medicChargeTimeModifier[team] = factor;
		level.medicChargeTime[team] = g_medicChargeTime.integer * factor;
	}
	else if( !Q_stricmp( playerclass, "engineer" ) ) {
		level.engineerChargeTimeModifier[team] = factor;
		level.engineerChargeTime[team] = g_engineerChargeTime.integer * factor;
	}
	else if( !Q_stricmp( playerclass, "fieldops" ) ) {
		level.lieutenantChargeTimeModifier[team] = factor;
		level.lieutenantChargeTime[team] = g_LTChargeTime.integer * factor;
	}
	else if( !Q_stricmp( playerclass, "lieutenant" ) ) { // FIXME: remove from missionpack
		level.lieutenantChargeTimeModifier[team] = factor;
		level.lieutenantChargeTime[team] = g_LTChargeTime.integer * factor;
	}
	else if( !Q_stricmp( playerclass, "covertops" ) ) {
		level.covertopsChargeTimeModifier[team] = factor;
		level.covertopsChargeTime[team] = g_covertopsChargeTime.integer * factor;
	}

	G_UpdateChargeTimes();
	return qtrue;
}

/*
=================
G_ScriptAction_SpawnRubble

  syntax: spawnrubble <targetname>
  Gordon: debris test
=================
*/
qboolean G_ScriptAction_SpawnRubble( gentity_t *ent, char *params ) {
	int i;

	for( i = 0; i < MAX_DEBRISCHUNKS; ++i ) {
		if(!Q_stricmp( level.debrisChunks[i].targetname, params )) {
			gentity_t* temp = G_TempEntity( level.debrisChunks[i].origin, EV_DEBRIS );
			VectorCopy( level.debrisChunks[i].velocity, temp->s.origin2 );
			temp->s.modelindex = level.debrisChunks[i].model;
		}
	}

	return qtrue;
}

/*
=================
G_ScriptAction_AllowTankExit

  syntax: allowtankexit <yes|on|no|off>
=================
*/
qboolean G_ScriptAction_AllowTankExit( gentity_t *ent, char *params ) {
	char	*pString = params;
	char 	*token = COM_ParseExt( &pString, qfalse );

	if( !*token ) {
		G_Error(S_COLOR_RED "G_ScriptAction_AllowTankExit(): allowtankexit must have a enable value\n" );
	}

	if( !Q_stricmp( token, "yes" ) || !Q_stricmp( token, "on" ) || atoi(token) ) {
		level.disableTankExit = qfalse;
	}
	else {
		level.disableTankExit = qtrue;
	}

	return qtrue;
}

/*
=================
G_ScriptAction_AllowTankEnter
=================
*/
qboolean G_ScriptAction_AllowTankEnter( gentity_t *ent, char *params ) {
	char	*pString = params;
	char 	*token = COM_ParseExt( &pString, qfalse );

	if( !*token ) {
		G_Error(S_COLOR_RED "G_ScriptAction_AllowTankEnter(): allowtankenter must have a enable value\n" );
	}

	if( !Q_stricmp( token, "yes" ) || !Q_stricmp( token, "on" ) || atoi(token) ) {
		level.disableTankEnter = qfalse;
	}
	else {
		level.disableTankEnter = qtrue;
	}

	return qtrue;
}

/*
=================
G_ScriptAction_SetTankAmmo
=================
*/
qboolean G_ScriptAction_SetTankAmmo( gentity_t *ent, char *params ) {
	char	*pString = params;
	char	*token = COM_ParseExt( &pString, qfalse );
	gentity_t* tank;

	if( !*token ) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetTankAmmo(): settankammo must have a target\n" );
	}

	tank = G_FindByTargetname( NULL, token );
	if(!tank) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetTankAmmo(): settankammo, failed to find target (%s)\n", token );
	}

	if(tank->s.eType != ET_MOVER) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetTankAmmo(): settankammo, must target a mover\n" );
	}

	token = COM_ParseExt( &pString, qfalse );
	if( !*token ) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetTankAmmo(): settankammo must have an amount\n" );
	}

	tank->s.effect1Time = atoi(token);

	return qtrue;
}

/*
=================
G_ScriptAction_AddTankAmmo
=================
*/
qboolean G_ScriptAction_AddTankAmmo( gentity_t *ent, char *params ) {
	char	*pString = params;
	char 	 *token = COM_ParseExt( &pString, qfalse );
	gentity_t* tank;

	if( !*token ) {
		G_Error(S_COLOR_RED "G_ScriptAction_AddTankAmmo(): addtankammo must have a target\n" );
	}

	tank = G_FindByTargetname( NULL, token );
	if(!tank) {
		G_Error(S_COLOR_RED "G_ScriptAction_AddTankAmmo(): addtankammo, failed to find target (%s)\n", token );
	}

	if(tank->s.eType != ET_MOVER) {
		G_Error(S_COLOR_RED "G_ScriptAction_AddTankAmmo(): addtankammo, must target a mover\n" );
	}

	token = COM_ParseExt( &pString, qfalse );
	if( !*token ) {
		G_Error(S_COLOR_RED "G_ScriptAction_AddTankAmmo(): addtankammo must have an amount\n" );
	}

	tank->s.effect1Time += atoi(token);

	token = COM_ParseExt( &pString, qfalse );
	if( *token ) {
		if(tank->s.effect1Time > atoi(token)) {
			tank->s.effect1Time = atoi(token);
		}
	}

	return qtrue;
}

/*
=================
G_ScriptAction_DisableMessage
=================
*/
qboolean G_ScriptAction_DisableMessage( gentity_t *ent, char *params ) {
	char *pString = params;
	char *token = COM_ParseExt( &pString, qfalse );
	gentity_t* target = NULL;

	if (!token[0]) {
		// CHRUKER: b056 - Said setposition instead of disablemessage
		G_Error(S_COLOR_RED "G_ScriptAction_DisableMessage(): disablemessage must have an targetname\n" );
	}

	// find the entity with the given "targetname"
	while ((target = G_FindByTargetname( target, token ))) {
		target->s.aiState = 1;
	}

	return qtrue;
}

/*
=================
G_ScriptAction_Kill
=================
*/
qboolean G_ScriptAction_Kill( gentity_t *ent, char *params ) {
	char *pString = params;
	char *token = COM_ParseExt( &pString, qfalse );

	if( !*token ) {
		G_Error(S_COLOR_RED "G_ScriptAction_Kill(): kill must have a target\n" );
	}

	G_KillEnts( token, NULL, ent->enemy, ent->deathType );

	return qtrue;
}

/*
=================
G_ScriptAction_SetGlobalFog

  syntax: setglobalfog <bool:restore> <int:duration> [float:r] [float:g] [float:b] [float:depthForOpaque]
=================
*/
qboolean G_ScriptAction_SetGlobalFog( gentity_t *ent, char *params ) {
	char *pString = params;
	char *token = COM_ParseExt( &pString, qfalse );
	qboolean	restore;
	int			duration;
	vec3_t		color;
	float		depthForOpaque;

	if( !token[0] ) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetGlobalFog(): setglobalfog must have a restore value\n" );
	}

	restore = atoi(token);

	token = COM_ParseExt( &pString, qfalse );
	if( !token[0] ) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetGlobalFog(): setglobalfog must have a duration value\n" );
	}

	duration = atoi(token);

	if( restore ) {
		trap_SetConfigstring( CS_GLOBALFOGVARS, va("1 %i 0 0 0 0", duration ) );
		return qtrue;
	}

	Parse1DMatrix( &pString, 3, color );

	token = COM_ParseExt( &pString, qfalse );
	if( !token[0] ) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetGlobalFog(): setglobalfog without restore flag must have a depth for opaque value\n" );
	}

	depthForOpaque = atof(token);

	trap_SetConfigstring( CS_GLOBALFOGVARS, va("0 %i %f %f %f %f", duration, color[0], color[1], color[2], depthForOpaque ) );

	return qtrue;
}

// ===================

/*
===============
G_ScriptAction_GotoMarker

  syntax: gotomarker <targetname> <speed> [accel/deccel] [turntotarget] [wait] [relative <position>]

  NOTE: speed may be modified to round the duration to the next 50ms for smooth
  transitions
===============
*/
qboolean G_ScriptAction_GotoMarker( gentity_t *ent, char *params )
{
	if (params && (ent->scriptStatus.scriptFlags & SCFL_GOING_TO_MARKER)) {
		// we can't process a new movement until the last one has finished
		return qfalse;
	}

	if (!params || ent->scriptStatus.scriptStackChangeTime < level.time) {			// we are waiting for it to reach destination
		if (ent->s.pos.trTime + ent->s.pos.trDuration <= level.time) {	// we made it
			ent->scriptStatus.scriptFlags &= ~SCFL_GOING_TO_MARKER;

			// set the angles at the destination
			BG_EvaluateTrajectory( &ent->s.apos, ent->s.apos.trTime + ent->s.apos.trDuration, ent->s.angles, qtrue, ent->s.effect2Time );
			VectorCopy( ent->s.angles, ent->s.apos.trBase );
			VectorCopy( ent->s.angles, ent->r.currentAngles );
			ent->s.apos.trTime = level.time;
			ent->s.apos.trDuration = 0;
			ent->s.apos.trType = TR_STATIONARY;
			VectorClear( ent->s.apos.trDelta );

			// stop moving
			BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin, qfalse, ent->s.effect2Time );
			VectorCopy( ent->s.origin, ent->s.pos.trBase );
			VectorCopy( ent->s.origin, ent->r.currentOrigin );
			ent->s.pos.trTime = level.time;
			ent->s.pos.trDuration = 0;
			ent->s.pos.trType = TR_STATIONARY;
			VectorClear( ent->s.pos.trDelta );

			script_linkentity(ent);

			return qtrue;
		}
	}
	else {	// we have just started this command
		char	*pString = params;
		char 	*token = COM_ParseExt( &pString, qfalse );
		gentity_t *target = NULL;
		float	speed, dist;
		vec3_t vec;
		qboolean wait=qfalse, turntotarget=qfalse;
		int trType;
		vec3_t	diff;
		vec3_t	angles;

		pathCorner_t* pPathCorner;

		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GotoMarker(): gotomarker must have an targetname\n" );
		}

		if ((pPathCorner = BG_Find_PathCorner( token ))) {
			VectorSubtract( pPathCorner->origin, ent->r.currentOrigin, vec );
		}
		else {
			// find the entity with the given "targetname"
			target = G_FindByTargetname( NULL, token );

			if (!target) {
				G_Error(S_COLOR_RED "G_ScriptAction_GotoMarker(): can't find entity with \"targetname\" = \"%s\"\n", token );
			}

			VectorSubtract( target->r.currentOrigin, ent->r.currentOrigin, vec );
		}

		token = COM_ParseExt( &pString, qfalse );
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GotoMarker(): gotomarker must have a speed\n" );
		}

		speed = atof(token);
		trType = TR_LINEAR_STOP;

		while (token[0]) {
			token = COM_ParseExt( &pString, qfalse );
			if (token[0]) {
				if (!Q_stricmp( token, "accel" )) {
					trType = TR_ACCELERATE;
				}
				else if (!Q_stricmp( token, "deccel" )) {
					trType = TR_DECCELERATE;
				}
				else if (!Q_stricmp( token, "wait" )) {
					wait = qtrue;
				}
				else if (!Q_stricmp( token, "turntotarget" )) {
					turntotarget = qtrue;
				}
				else if (!Q_stricmp( token, "relative" )) {
					gentity_t*		target2;
					pathCorner_t*	pPathCorner2;
					vec3_t vec2;

					token = COM_ParseExt( &pString, qfalse );

					if ((pPathCorner2 = BG_Find_PathCorner( token ))) {
						VectorCopy( pPathCorner2->origin, vec2 );
					}
					else if ((target2 = G_FindByTargetname( NULL, token ))) {
						VectorCopy( target2->r.currentOrigin, vec2 );
					}
					else {
						G_Error(S_COLOR_RED "Target for relative gotomarker not found: %s\n", token );
						return qfalse; // Jaybird - shut compiler up
					}

					VectorAdd( vec, ent->r.currentOrigin, vec );
					VectorSubtract( vec, vec2, vec );
				}
			}
		}

		// start the movement
		if (ent->s.eType == ET_MOVER) {

			VectorCopy( vec, ent->movedir );
			VectorCopy( ent->r.currentOrigin, ent->pos1 );
			VectorAdd( ent->r.currentOrigin, vec, ent->pos2 );
			ent->speed = speed * (float)g_moverScale.value;
			dist = VectorDistance( ent->pos1, ent->pos2 );
			// setup the movement with the new parameters
			InitMover(ent);

			if (ent->s.eType == ET_MOVER && ent->spawnflags & 8) {
				ent->use = script_mover_use;
			}
			// start the movement

			SetMoverState( ent, MOVER_1TO2, level.time );
			if (trType != TR_LINEAR_STOP) {
				// allow for acceleration/decceleration
				ent->s.pos.trDuration = 1000.0 * dist / (speed / 2.0);
				ent->s.pos.trType = trType;
			}
			ent->reached = NULL;

#ifdef	OMNIBOTS
			// Send a trigger to omni-bot
			{
				const char *pName = _GetEntityName(ent);
				Bot_Util_SendTrigger(ent,
					NULL,
					va("%s_goto", pName ? pName : "<unknown>"),
					va("%.2f %.2f %.2f", ent->s.pos.trDelta[0], ent->s.pos.trDelta[1], ent->s.pos.trDelta[2]));
			}
#endif
			if (turntotarget && !pPathCorner) {
				int i;

				int duration = ent->s.pos.trDuration;

				VectorCopy( target->s.angles, angles );

				for (i=0; i<3; ++i) {
					diff[i] = AngleDifference( angles[i], ent->s.angles[i] );
					while (diff[i] > 180)
						diff[i] -= 360;
					while (diff[i] < -180)
						diff[i] += 360;
				}
				VectorCopy( ent->s.angles, ent->s.apos.trBase );
				if (duration)
					VectorScale( diff, 1000.0/(float)duration, ent->s.apos.trDelta );
				else
					VectorClear( ent->s.apos.trDelta );
				ent->s.apos.trDuration = duration;
				ent->s.apos.trTime = level.time;
				ent->s.apos.trType = TR_LINEAR_STOP;
				if (trType != TR_LINEAR_STOP) {	// allow for acceleration/decceleration
					ent->s.pos.trDuration = 1000.0 * dist / (speed / 2.0);
					ent->s.pos.trType = trType;
				}
			}

		}
		else {
			// calculate the trajectory
			ent->s.pos.trType = TR_LINEAR_STOP;
			ent->s.pos.trTime = level.time;
			VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
			dist = VectorNormalize( vec );
			VectorScale( vec, speed, ent->s.pos.trDelta );
			ent->s.pos.trDuration = 1000 * (dist / speed);

			if (turntotarget && !pPathCorner) {
				int i;
				int duration = ent->s.pos.trDuration;

				VectorCopy( target->s.angles, angles );

				for (i=0; i<3; ++i) {
					diff[i] = AngleDifference( angles[i], ent->s.angles[i] );
					while (diff[i] > 180)
						diff[i] -= 360;
					while (diff[i] < -180)
						diff[i] += 360;
				}
				VectorCopy( ent->s.angles, ent->s.apos.trBase );
				if (duration)
					VectorScale( diff, 1000.0/(float)duration, ent->s.apos.trDelta );
				else
					VectorClear( ent->s.apos.trDelta );
				ent->s.apos.trDuration = duration;
				ent->s.apos.trTime = level.time;
				ent->s.apos.trType = TR_LINEAR_STOP;
			}

		}

		if (!wait) {
			// round the duration to the next 50ms
			if (ent->s.pos.trDuration%50) {
				float frac;

				frac = (float)(((ent->s.pos.trDuration/50)*50 + 50) - ent->s.pos.trDuration) / (float)(ent->s.pos.trDuration);
				if (frac < 1) {
					VectorScale( ent->s.pos.trDelta, 1.0 / (1.0 + frac), ent->s.pos.trDelta );
					ent->s.pos.trDuration = (ent->s.pos.trDuration/50)*50 + 50;
				}
			}

			// set the goto flag, so we can keep processing the move until we reach the destination
			ent->scriptStatus.scriptFlags |= SCFL_GOING_TO_MARKER;
			return qtrue;	// continue to next command
		}

	}

	BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->r.currentOrigin, qfalse, ent->s.effect2Time  );
	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles, qtrue, ent->s.effect2Time  );
	script_linkentity(ent);

	return qfalse;
}

/*
=================
G_ScriptAction_Wait

  syntax:	wait <duration>
			wait random <min> <max>
=================
*/
qboolean G_ScriptAction_Wait( gentity_t *ent, char *params )
{
	char 	*pString = params;
	char 	*token = COM_ParseExt( &pString, qfalse );
	int		duration;

	// get the duration
	if (!*token) {
		G_Error(S_COLOR_RED "G_ScriptAction_Wait(): wait must have a duration\n" );
	}

	// Gordon: adding random wait ability
	if(!Q_stricmp( token, "random" )) {
		int min, max;

		token = COM_ParseExt( &pString, qfalse );
		if (!*token) {
			G_Error(S_COLOR_RED "G_ScriptAction_Wait(): wait random must have a min duration\n" );
		}
		min = atoi(token);

		token = COM_ParseExt( &pString, qfalse );
		if (!*token) {
			G_Error(S_COLOR_RED "G_ScriptAction_Wait(): wait random must have a max duration\n" );
		}
		max = atoi(token);

		if(ent->scriptStatus.scriptStackChangeTime + min > level.time) {
			return qfalse;
		}

		if(ent->scriptStatus.scriptStackChangeTime + max < level.time) {
			return qtrue;
		}

		return !(rand() % (int)((max - min) * 0.02f));
	}

	duration = atoi( token );
	return (ent->scriptStatus.scriptStackChangeTime + duration < level.time);
}


/*
=================
G_ScriptAction_Trigger

  syntax: trigger <aiName/scriptName> <trigger>

  Calls the specified trigger for the given ai character or script entity
=================
*/
qboolean G_ScriptAction_Trigger( gentity_t *ent, char *params )
{
	char *pString = params;
	char *token = COM_ParseExt( &pString, qfalse );
	gentity_t *trent;
	char name[MAX_QPATH], trigger[MAX_QPATH];
	int oldId;
	qboolean terminate, found;


	// get the cast name

	Q_strncpyz( name, token, sizeof(name) );
	if (!*name) {
		G_Error(S_COLOR_RED "G_ScriptAction_Trigger(): trigger must have a name and an identifier: %s\n", params );
	}

	token = COM_ParseExt( &pString, qfalse );
	Q_strncpyz( trigger, token, sizeof(trigger) );
	if (!*trigger) {
		G_Error(S_COLOR_RED "G_ScriptAction_Trigger(): trigger must have a name and an identifier: %s\n", params );
	}

	if (!Q_stricmp( name, "self" )) {
		trent = ent;
		oldId = trent->scriptStatus.scriptId;
		G_Script_ScriptEvent( trent, SE_TRIGGER, trigger );
		// if the script changed, return false so we don't muck with it's variables
		return ((trent != ent) || (oldId == trent->scriptStatus.scriptId));
	}
	else if (!Q_stricmp( name, "global" )) {
		int i;
		terminate = qfalse;
		found = qfalse;
		// for all entities/bots with this scriptName
		trent = g_entities;
		for (i=0; i<level.num_entities; ++i, ++trent) {
			if (!trent->inuse) continue;
			if (!trent->scriptName) continue;
			if (!trent->scriptName[0]) continue;
			found = qtrue;
			if (!(trent->r.svFlags & SVF_BOT)) {
				oldId = trent->scriptStatus.scriptId;
				G_Script_ScriptEvent( trent, SE_TRIGGER, trigger );
				// if the script changed, return false so we don't muck with it's variables
				if ((trent == ent) && (oldId != trent->scriptStatus.scriptId)) {
					terminate = qtrue;
				}
			}
		}
		//
		if (terminate) return qfalse;
		if (found) return qtrue;
	}
	else if (!Q_stricmp( name, "player" )) {
		int i;
		for (i = 0; i < g_maxclients.integer; ++i) {
			if (level.clients[i].pers.connected != CON_CONNECTED)
				continue;
			G_Script_ScriptEvent( &g_entities[i], SE_TRIGGER, trigger );
		}
		return qtrue;	// always true, as players aren't always there
	}
	else if (!Q_stricmp( name, "activator" )) {
		return qtrue;	// always true, as players aren't always there
	}
	else {
		// calculate a hash value, for faster string comparison..
		int hash = BG_StringHashValue( (char *)&name );
		int from;
		terminate = qfalse;
		found = qfalse;

		// for all entities/bots with this scriptName
		// core: check the short array first..
		for (from=0; from<g_scripttriggers.count; ++from ) {
			trent = g_scripttriggers.ent[from];
			if ( trent->scriptnamehash != hash ) continue;
			if ( Q_strncmp(trent->scriptName, (char *)&name, MAX_QPATH) ) continue;
			found = qtrue;
			if (!(trent->r.svFlags & SVF_BOT)) {
				oldId = trent->scriptStatus.scriptId;
				G_Script_ScriptEvent( trent, SE_TRIGGER, trigger );
				// if the script changed, return false so we don't muck with it's variables
				if ((trent == ent) && (oldId != trent->scriptStatus.scriptId)) {
					terminate = qtrue;
				}
			}
		}
		// core: if not found in the short array, search through all entities..
		if ( found==qfalse ) {
			trent = NULL;
			while ( (trent = G_FindByScriptnameFast(trent, name, hash)) ) {
				found = qtrue;
				if (!(trent->r.svFlags & SVF_BOT)) {
					oldId = trent->scriptStatus.scriptId;
					G_Script_ScriptEvent( trent, SE_TRIGGER, trigger );
					// if the script changed, return false so we don't muck with it's variables
					if ((trent == ent) && (oldId != trent->scriptStatus.scriptId)) {
						terminate = qtrue;
					}
				}
				// core: add this entity to the short array for quicker searches next time..
				AddToEntityArray( &g_scripttriggers, trent, qtrue );
			}
		}

		if (terminate) return qfalse;
		if (found) return qtrue;
	}


	G_Printf( "G_ScriptAction_Trigger(): trigger has unknown name: %s\n", name);
	return qtrue;	// shutup the compiler
}

/*
================
G_ScriptAction_PlaySound

  syntax: playsound <soundname OR scriptname> [LOOPING]

  Currently only allows playing on the VOICE channel, unless you use a sound script.

  Use the optional LOOPING paramater to attach the sound to the entities looping channel.
================
*/
qboolean G_ScriptAction_PlaySound( gentity_t *ent, char *params )
{
	char *pString, *token;
	char	sound[MAX_QPATH];
	qboolean looping = qfalse;
	int volume = 255;

	if (!params) {
		G_Error(S_COLOR_RED "G_ScriptAction_PlaySound(): syntax error\n\nplaysound <soundname OR scriptname>\n");
	}

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	Q_strncpyz(sound, token, sizeof(sound));

	token = COM_ParseExt( &pString, qfalse );
	while(*token) {
		if (!Q_stricmp(token, "looping")) {
			looping = qtrue;
		}
		else if (!Q_stricmp(token, "volume")) {
			token = COM_ParseExt( &pString, qfalse );
			volume = atoi(token);
			if(!volume) {
				volume = 255;
			}
		}

		token = COM_ParseExt( &pString, qfalse );
	}

	if(!looping) {
		if(volume == 255) {
			G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( sound ) );
		}
		else {
			G_AddEvent( ent, EV_GENERAL_SOUND_VOLUME, G_SoundIndex( sound ) );
			ent->s.onFireStart = volume >> 1;
		}
	}
	else {	// looping channel
		ent->s.loopSound = G_SoundIndex( sound );
		ent->s.onFireStart = volume >> 1;
	}

	return qtrue;
}

/*
================
G_ScriptAction_FadeAllSounds

  syntax: fadeallsounds [UP/DOWN] time

  Fades all sounds up or down, going from the current level to max or zero in 'time' milliseconds

================
*/
qboolean G_ScriptAction_FadeAllSounds( gentity_t *ent, char *params )
{
	char *pString, *token;
	qboolean up = qfalse;
	int time;

	if (!params)
	{
		G_Error(S_COLOR_RED "G_ScriptAction_FadeAllSounds(): usage: FadeAllSounds [up|down] time\n");
	}

	pString = params;
	token = COM_ParseExt( &pString, qfalse );

	if (!Q_stricmp(token, "up")) {
		up = qtrue;
	}
	else if (!Q_stricmp(token, "down")) {
		up = qfalse;
	}
	else {
		G_Error(S_COLOR_RED "G_ScriptAction_FadeAllSounds(): FadeAllSounds found '%s' when expecting [up|down]\n", token);
	}

	token = COM_ParseExt( &pString, qfalse );

	time = atoi(token);
	if (!time)
	{
		G_Error(S_COLOR_RED "G_ScriptAction_FadeAllSounds(): FadeAllSounds found '%s' when expecting 'time'\n", token);
	}

	trap_SendServerCommand(-1, va("snd_fade %f %d %i", (up)?1.0:0.0, time, (up)?0:1));

	return qtrue;
}


/*
==================
G_ScriptAction_MusicStart
==================
*/
qboolean G_ScriptAction_MusicStart( gentity_t *ent, char *params) {
	char	*pString 	= params;
	char 	*token 		= COM_ParseExt( &pString, qfalse );
	char	cvarName[MAX_QPATH];
	int		fadeupTime = 0;

	if (!token[0]) {
		G_Error("G_Scripting: syntax: mu_start <musicfile> <fadeuptime>");
	}
	Q_strncpyz( cvarName, token, sizeof(cvarName) );

	token = COM_ParseExt( &pString, qfalse );
	if (token[0]) {
		fadeupTime = atoi(token);
	}

	trap_SendServerCommand(-1, va("mu_start %s %d", cvarName, fadeupTime));

	return qtrue;
}

/*
==================
G_ScriptAction_MusicPlay
==================
*/
qboolean G_ScriptAction_MusicPlay( gentity_t *ent, char *params) {
	char	*pString 	= params;
	char 	*token 		= COM_ParseExt( &pString, qfalse );
	char	cvarName[MAX_QPATH];
	int		fadeupTime = 0;

	if (!token[0]) {
		G_Error("G_Scripting: syntax: mu_play <musicfile> [fadeup time]");
	}
	Q_strncpyz( cvarName, token, sizeof(cvarName) );

	trap_SendServerCommand(-1, va("mu_play %s %d", cvarName, fadeupTime));

	return qtrue;
}


/*
==================
G_ScriptAction_MusicStop
==================
*/
qboolean G_ScriptAction_MusicStop( gentity_t *ent, char *params) {
	char	*pString 	= params;
	char 	*token 		= COM_ParseExt( &pString, qfalse );
	int		fadeoutTime = 0;

	if (token[0]) {
		fadeoutTime = atoi(token);
	}

	trap_SendServerCommand(-1, va("mu_stop %i\n", fadeoutTime));

	return qtrue;
}

/*
==================
G_ScriptAction_MusicQueue
==================
*/
qboolean G_ScriptAction_MusicQueue( gentity_t *ent, char *params) {
	char	*pString 	= params;
	char 	*token 		= COM_ParseExt( &pString, qfalse );
	char	cvarName[MAX_QPATH];

	if (!token[0]) {
		G_Error("G_Scripting: syntax: mu_queue <musicfile>");
	}
	Q_strncpyz( cvarName, token, sizeof(cvarName) );

	trap_SetConfigstring( CS_MUSIC_QUEUE, cvarName );

	return qtrue;
}

/*
==================
G_ScriptAction_MusicFade
==================
*/
qboolean G_ScriptAction_MusicFade( gentity_t *ent, char *params) {
	char	*pString 	= params;
	char 	*token 		= COM_ParseExt( &pString, qfalse );
	int		fadeoutTime = 0;
	float	targetVol = 0.0;

	if (!token[0]) {
		G_Error("G_Scripting: syntax: mu_fade <target volume 0.0-1.0> <fadeout time>");
	}
	targetVol = atof(token);
	if (targetVol < 0 || targetVol >= 1)
		G_Error("G_Scripting: syntax: mu_fade <target volume 0.0-1.0> <fadeout time>");

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0] || token[0] < '0' || token[0] > '9') {
		G_Error("G_Scripting: syntax: mu_fade <target volume 0.0-1.0> <fadeout time>");
	}
	fadeoutTime = atoi(token);

	trap_SendServerCommand(-1, va("mu_fade %f %i\n", targetVol, fadeoutTime));

	return qtrue;
}


/*
=================
G_ScriptAction_PlayAnim

  syntax: playanim <startframe> <endframe> [looping <FOREVER/duration>] [rate <FPS>]

  NOTE: all source animations must be at 20fps
=================
*/
qboolean G_ScriptAction_PlayAnim( gentity_t *ent, char *params )
{
	char *pString = params, *token, tokens[2][MAX_QPATH];
	int	i;
	int endtime = 0;
	qboolean looping=qfalse, forever=qfalse;
	int		startframe, endframe, idealframe;
	int		rate = 20;

	if ((ent->scriptStatus.scriptFlags & SCFL_ANIMATING) && (ent->scriptStatus.scriptStackChangeTime == level.time)) {
		// this is a new call, so cancel the previous animation
		ent->scriptStatus.scriptFlags &= ~SCFL_ANIMATING;
	}

	for (i=0; i<2; ++i) {
		token = COM_ParseExt( &pString, qfalse );
		if (!token || !token[0]) {
			G_Printf("G_ScriptAction_PlayAnim(): syntax error\n\nplayanim <startframe> <endframe> [LOOPING <duration>]\n");
			return qtrue;
		}
		else {
			Q_strncpyz( tokens[i], token, sizeof(tokens[i]) );
		}
	}

	startframe = atoi(tokens[0]);
	endframe = atoi(tokens[1]);

	// check for optional parameters
	token = COM_ParseExt( &pString, qfalse );
	if (token[0]) {
		if (!Q_stricmp(token, "looping")) {
			looping = qtrue;

			token = COM_ParseExt( &pString, qfalse );
			if (!token || !token[0]) {
				G_Printf( "G_ScriptAction_PlayAnim(): syntax error\n\nplayanim <startframe> <endframe> [LOOPING <duration>]\n");
				return qtrue;
			}
			if (!Q_stricmp(token, "untilreachmarker")) {
				if (level.time < ent->s.pos.trTime + ent->s.pos.trDuration)
					endtime = level.time + 100;
				else
					endtime = 0;
			}
			else if (!Q_stricmp(token, "forever")) {
				ent->scriptStatus.animatingParams = params;
				ent->scriptStatus.scriptFlags |= SCFL_ANIMATING;
				endtime = level.time + 100;		// we don't care when it ends, since we are going forever!
				forever = qtrue;
			}
			else {
				endtime = ent->scriptStatus.scriptStackChangeTime + atoi(token);
			}

			token = COM_ParseExt( &pString, qfalse );
		}

		if (token[0] && !Q_stricmp(token, "rate")) {
			token = COM_ParseExt( &pString, qfalse );
			if (!token[0])
				G_Error(S_COLOR_RED "G_ScriptAction_PlayAnim(): playanim has RATE parameter without an actual rate specified");
			rate = atoi(token);
		}

		if (!looping) {
			endtime = ent->scriptStatus.scriptStackChangeTime + ((endframe - startframe) * (1000/20));
		}
	}

	idealframe = startframe + (int)floor((float)(level.time - ent->scriptStatus.scriptStackChangeTime) / (1000.0/(float)rate));
	if (looping) {
		ent->s.frame = startframe + (idealframe - startframe)%(endframe - startframe);
	}
	else {
		if (idealframe > endframe)
			ent->s.frame = endframe;
		else
			ent->s.frame = idealframe;
	}

	if (forever) {
		return qtrue;	// continue to the next command
	}

	return (endtime <= level.time);
};

/*
=================
G_ScriptAction_AlertEntity

  syntax: alertentity <targetname>

 Arnout: modified to target multiple entities with the same targetname
=================
*/
qboolean G_ScriptAction_AlertEntity( gentity_t *ent, char *params )
{
	gentity_t	*alertent = NULL;
	qboolean	foundalertent = qfalse;
	int hash;

	if( !params || !*params ) {
		G_Error(S_COLOR_RED "G_ScriptAction_AlertEntity(): alertentity without targetname\n" );
	}
	hash = BG_StringHashValue( params );

	// find this targetname
	while(1) {
		alertent = G_FindByTargetnameFast( alertent, params, hash );
		if (!alertent ) {
			if( !foundalertent ) {
				G_Error(S_COLOR_RED "G_ScriptAction_AlertEntity(): alertentity cannot find targetname \"%s\"\n", params );
			}
			else {
				break;
			}
		}

		foundalertent = qtrue;

		if (alertent->client) {
			// call this entity's AlertEntity function
			if (!alertent->AIScript_AlertEntity) {
				G_Error(S_COLOR_RED "G_ScriptAction_AlertEntity(): alertentity \"%s\" (classname = %s) doesn't have an \"AIScript_AlertEntity\" function\n", params, alertent->classname );
			}
			alertent->AIScript_AlertEntity (alertent);
		}
		else {
			if (!alertent->use) {
				G_Error(S_COLOR_RED "G_ScriptAction_AlertEntity(): alertentity \"%s\" (classname = %s) doesn't have a \"use\" function\n", params, alertent->classname );
			}
			G_UseEntity( alertent, NULL, NULL );
		}
	}

	return qtrue;
}

/*
=================
G_ScriptAction_ToggleSpeaker

  syntax: togglespeaker <targetname>
=================
*/
qboolean G_ScriptAction_ToggleSpeaker( gentity_t *ent, char *params )
{
	int				i;
	long			hash;
	gentity_t		*tent;
	bg_speaker_t	*speaker;

	if( !params || !*params ) {
		G_Error(S_COLOR_RED "G_ScriptAction_ToggleSpeaker(): togglespeaker without targetname\n" );
	}

	hash = BG_StringHashValue( params );

	// find this targetname
	for( i = 0; i < BG_NumScriptSpeakers(); ++i ) {
		speaker = BG_GetScriptSpeaker( i );

		if( hash != speaker->targetnamehash && Q_stricmp( params, speaker->targetname ) ) {
			continue;
		}

		tent = G_TempEntity( speaker->origin, EV_ALERT_SPEAKER );
		tent->r.svFlags = SVF_BROADCAST;
		tent->s.otherEntityNum = i;
		tent->s.otherEntityNum2 = 0;
	}

	return qtrue;
}

/*
=================
G_ScriptAction_DisableSpeaker

  syntax: disablespeaker <targetname>
=================
*/
qboolean G_ScriptAction_DisableSpeaker( gentity_t *ent, char *params )
{
	int				i;
	long			hash;
	gentity_t		*tent;
	bg_speaker_t	*speaker;

	if( !params || !*params ) {
		G_Error(S_COLOR_RED "G_ScriptAction_DisableSpeaker(): disablespeaker without targetname\n" );
	}

	hash = BG_StringHashValue( params );

	// find this targetname
	for( i = 0; i < BG_NumScriptSpeakers(); ++i ) {
		speaker = BG_GetScriptSpeaker( i );

		if( hash != speaker->targetnamehash && Q_stricmp( params, speaker->targetname ) ) {
			continue;
		}

		tent = G_TempEntity( speaker->origin, EV_ALERT_SPEAKER );
		tent->r.svFlags = SVF_BROADCAST;
		tent->s.otherEntityNum = i;
		tent->s.otherEntityNum2 = 1;
	}

	return qtrue;
}

/*
=================
G_ScriptAction_EnableSpeaker

  syntax: enablespeaker <targetname>
=================
*/
qboolean G_ScriptAction_EnableSpeaker( gentity_t *ent, char *params )
{
	int				i;
	long			hash;
	gentity_t		*tent;
	bg_speaker_t	*speaker;

	if( !params || !*params ) {
		G_Error(S_COLOR_RED "G_ScriptAction_EnableSpeaker(): enablespeaker without targetname\n" );
	}

	hash = BG_StringHashValue( params );

	// find this targetname
	for( i = 0; i < BG_NumScriptSpeakers(); ++i ) {
		speaker = BG_GetScriptSpeaker( i );

		if( hash != speaker->targetnamehash && Q_stricmp( params, speaker->targetname ) ) {
			continue;
		}

		tent = G_TempEntity( speaker->origin, EV_ALERT_SPEAKER );
		tent->r.svFlags = SVF_BROADCAST;
		tent->s.otherEntityNum = i;
		tent->s.otherEntityNum2 = 2;
	}

	return qtrue;
}

/*
=================
G_ScriptAction_Accum

  syntax: accum <buffer_index> <command> <paramater...>

  Commands:

	accum <n> inc <m>
	accum <n> abort_if_less_than <m>
	accum <n> abort_if_greater_than <m>
	accum <n> abort_if_not_equal <m>
	accum <n> abort_if_equal <m>
	accum <n> set <m>
	accum <n> random <m>
	accum <n> bitset <m>
	accum <n> bitreset <m>
	accum <n> abort_if_bitset <m>
	accum <n> abort_if_not_bitset <m>

// Gordon: added (12/06/02)
	accum <n> trigger_if_equal <m> <s> <t>

// Gordon: added (12/06/02)
	accum <n> wait_while_equal <m>

// core: added (05/25/2009)
	accum <n> inc_accum <a>
	accum <n> abort_if_less_than_accum <a>
	accum <n> abort_if_greater_than_accum <a>
	accum <n> abort_if_not_equal_accum <a>
	accum <n> abort_if_equal_accum <a>
	accum <n> set_accum <a>
	accum <n> random_accum <a>
	accum <n> set_to_playingclients

	accum <n> inc_globalaccum <g>
	accum <n> abort_if_less_than_globalaccum <g>
	accum <n> abort_if_greater_than_globalaccum <g>
	accum <n> abort_if_not_equal_globalaccum <g>
	accum <n> abort_if_equal_globalaccum <g>
	accum <n> set_globalaccum <g>
	accum <n> random_globalaccum <g>

// core: added (05/25/2009)
	accum <n> dec <m>
	accum <n> mul <m>
	accum <n> div <m>
	accum <n> dec_accum <a>
	accum <n> mul_accum <a>
	accum <n> div_accum <a>
	accum <n> dec_globalaccum <g>
	accum <n> mul_globalaccum <g>
	accum <n> div_globalaccum <g>
=================
*/

int BotGetTargetDynamite( int *list, int listSize, gentity_t* target );

qboolean G_ScriptAction_Accum( gentity_t *ent, char *params )
{
	char	*pString 	= params;
	char 	*token 		= COM_ParseExt( &pString, qfalse );
	char 	lastToken[MAX_QPATH];
	int 	bufferIndex;
	// IlDuca - declare bufferIndex2
	int bufferIndex2;


	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum without a buffer index\n" );
	}

	bufferIndex = atoi(token);
	if (bufferIndex >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
		// CHRUKER: b055 - Was printing 10 as the last bufferindex, but its actually 9
		G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
	}

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum without a command\n" );
	}

	Q_strncpyz( lastToken, token, sizeof(lastToken) );
	token = COM_ParseExt( &pString, qfalse );

	if (!Q_stricmp(lastToken, "inc")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		ent->scriptAccumBuffer[bufferIndex] += atoi(token);
	}
	else if (!Q_stricmp(lastToken, "abort_if_less_than")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		if (ent->scriptAccumBuffer[bufferIndex] < atoi(token)) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_greater_than")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		if (ent->scriptAccumBuffer[bufferIndex] > atoi(token)) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_not_equal") || !Q_stricmp(lastToken, "abort_if_not_equals")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		if (ent->scriptAccumBuffer[bufferIndex] != atoi(token)) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_equal")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		if (ent->scriptAccumBuffer[bufferIndex] == atoi(token)) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "bitset")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		ent->scriptAccumBuffer[bufferIndex] |= (1<<atoi(token));
	}
	else if (!Q_stricmp(lastToken, "bitreset")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		ent->scriptAccumBuffer[bufferIndex] &= ~(1<<atoi(token));
	}
	else if (!Q_stricmp(lastToken, "abort_if_bitset")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "Scripting: accum %s requires a parameter\n", lastToken );
		}
		if (ent->scriptAccumBuffer[bufferIndex] & (1<<atoi(token))) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_not_bitset")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		if (!(ent->scriptAccumBuffer[bufferIndex] & (1<<atoi(token)))) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "set")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		ent->scriptAccumBuffer[bufferIndex] = atoi(token);
	}
	else if (!Q_stricmp(lastToken, "random")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		ent->scriptAccumBuffer[bufferIndex] = rand() % atoi(token);
	}
	else if (!Q_stricmp(lastToken, "trigger_if_equal")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		if (ent->scriptAccumBuffer[bufferIndex] == atoi(token)) {
			gentity_t* trent;
			int oldId;
			int hash;
			char name[MAX_QPATH];
			qboolean terminate, found;

			token = COM_ParseExt( &pString, qfalse );
			Q_strncpyz( lastToken, token, sizeof(lastToken) );
			if (!*lastToken) {
				G_Error(S_COLOR_RED "G_ScriptAction_Accum(): trigger must have a name and an identifier: %s\n", params );
			}
			hash = BG_StringHashValue( token );

			token = COM_ParseExt( &pString, qfalse );
			Q_strncpyz( name, token, sizeof(name) );
			if (!*name) {
				G_Error(S_COLOR_RED "G_ScriptAction_Accum(): trigger must have a name and an identifier: %s\n", params );
			}
			//
			terminate = qfalse;
			found = qfalse;
			// for all entities/bots with this scriptName
			trent = NULL;
			while ( (trent = G_FindByScriptnameFast(trent, lastToken, hash)) ) {
				found = qtrue;
				oldId = trent->scriptStatus.scriptId;
				G_Script_ScriptEvent( trent, SE_TRIGGER, name );
				// if the script changed, return false so we don't muck with it's variables
				if ((trent == ent) && (oldId != trent->scriptStatus.scriptId)) {
					terminate = qtrue;
				}
			}

			if (terminate) return qfalse;
			if (found) return qtrue;

			G_Printf(S_COLOR_YELLOW "WARNING G_ScriptAction_Accum(): trigger has unknown name: %s\n", name);
			return qtrue;
		}
	}
	else if( !Q_stricmp(lastToken, "wait_while_equal") ) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		if (ent->scriptAccumBuffer[bufferIndex] == atoi(token)) {
			return qfalse;
		}
	}
	else if( !Q_stricmp(lastToken, "set_to_dynamitecount") ) {
		gentity_t* target;
		if( !*token ) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}

		target = G_FindByTargetname( NULL, token );
		if( !target ) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s could not find target\n", lastToken );
		}

		{
			int num=0, i;
			// sigh, searching..
			for ( i=MAX_CLIENTS ; i< level.num_entities; ++i ){

				if ( !(g_entities[i].etpro_misc_1 & 1))
					continue;

				if ( g_entities[i].etpro_misc_2 != target-g_entities)
					continue;

				num++;
			}

			ent->scriptAccumBuffer[bufferIndex] = num;//BotGetTargetDynamite( NULL, 0, target );

		}

	// added by core: few commands to make life easier for mapscripters
	// accum/constant
	}
	else if (!Q_stricmp(lastToken, "dec")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		ent->scriptAccumBuffer[bufferIndex] -= atoi(token);
	}
	else if (!Q_stricmp(lastToken, "mul")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		ent->scriptAccumBuffer[bufferIndex] *= atoi(token);
	}
	else if (!Q_stricmp(lastToken, "div")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		if (atoi(token)!=0) ent->scriptAccumBuffer[bufferIndex] /= atoi(token);

	// accum/accum
	}
	else if (!Q_stricmp(lastToken, "inc_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		ent->scriptAccumBuffer[bufferIndex] += ent->scriptAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "abort_if_less_than_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (ent->scriptAccumBuffer[bufferIndex] < ent->scriptAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_greater_than_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (ent->scriptAccumBuffer[bufferIndex] > ent->scriptAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_not_equal_accum") || !Q_stricmp(lastToken, "abort_if_not_equals_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_Scripting: accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (ent->scriptAccumBuffer[bufferIndex] != ent->scriptAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_equal_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (ent->scriptAccumBuffer[bufferIndex] == ent->scriptAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "set_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		ent->scriptAccumBuffer[bufferIndex] = ent->scriptAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "random_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		ent->scriptAccumBuffer[bufferIndex] = rand() % ent->scriptAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "dec_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		ent->scriptAccumBuffer[bufferIndex] -= ent->scriptAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "mul_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		ent->scriptAccumBuffer[bufferIndex] *= ent->scriptAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "div_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (ent->scriptAccumBuffer[bufferIndex2]!=0) ent->scriptAccumBuffer[bufferIndex] /= ent->scriptAccumBuffer[bufferIndex2];

	}
	else if (!Q_stricmp(lastToken, "set_to_playingclients")) {
//		if( !*token ) {
//			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
//		}
		ent->scriptAccumBuffer[bufferIndex] = level.numPlayingClients;

	// accum/globalaccum
	}
	else if (!Q_stricmp(lastToken, "inc_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		ent->scriptAccumBuffer[bufferIndex] += level.globalAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "abort_if_less_than_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (ent->scriptAccumBuffer[bufferIndex] < level.globalAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_greater_than_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (ent->scriptAccumBuffer[bufferIndex] > level.globalAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_not_equal_globalaccum") || !Q_stricmp(lastToken, "abort_if_not_equals_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (ent->scriptAccumBuffer[bufferIndex] != level.globalAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_equal_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (ent->scriptAccumBuffer[bufferIndex] == level.globalAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "set_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		ent->scriptAccumBuffer[bufferIndex] = level.globalAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "random_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		ent->scriptAccumBuffer[bufferIndex] = rand() % level.globalAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "dec_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		ent->scriptAccumBuffer[bufferIndex] -= level.globalAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "mul_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		ent->scriptAccumBuffer[bufferIndex] *= level.globalAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "div_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Accum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (level.globalAccumBuffer[bufferIndex2]!=0) ent->scriptAccumBuffer[bufferIndex] /= level.globalAccumBuffer[bufferIndex2];

	}
	else {
		G_Error(S_COLOR_RED "G_ScriptAction_Accum(): accum %s: unknown command\n", params );
	}

	return qtrue;
}

/*
=================
G_ScriptAction_GlobalAccum

  syntax: globalAccum <buffer_index> <command> <paramater...>

  Commands:

	globalAccum <n> inc <m>
	globalAccum <n> abort_if_less_than <m>
	globalAccum <n> abort_if_greater_than <m>
	globalAccum <n> abort_if_not_equal <m>
	globalAccum <n> abort_if_equal <m>
	globalAccum <n> set <m>
	globalAccum <n> random <m>
	globalAccum <n> bitset <m>
	globalAccum <n> bitreset <m>
	globalAccum <n> abort_if_bitset <m>
	globalAccum <n> abort_if_not_bitset <m>

// Gordon: added (12/06/02)
	globalAccum <n> trigger_if_equal <m> <s> <t>

// Gordon: added (12/06/02)
	globalAccum <n> wait_while_equal <m>

// core: added (05/25/2009)
	globalaccum <n> inc_accum <a>
	globalaccum <n> abort_if_less_than_accum <a>
	globalaccum <n> abort_if_greater_than_accum <a>
	globalaccum <n> abort_if_not_equal_accum <a>
	globalaccum <n> abort_if_equal_accum <a>
	globalaccum <n> set_accum <a>
	globalaccum <n> random_accum <a>
	globalaccum <n> set_to_playingclients

	globalaccum <n> inc_globalaccum <g>
	globalaccum <n> abort_if_less_than_globalaccum <g>
	globalaccum <n> abort_if_greater_than_globalaccum <g>
	globalaccum <n> abort_if_not_equal_globalaccum <g>
	globalaccum <n> abort_if_equal_globalaccum <g>
	globalaccum <n> set_globalaccum <g>
	globalaccum <n> random_globalaccum <g>

// core: added (05/29/2009)
	globalaccum <n> dec <m>
	globalaccum <n> mul <m>
	globalaccum <n> div <m>
	globalaccum <n> dec_accum <a>
	globalaccum <n> mul_accum <a>
	globalaccum <n> div_accum <a>
	globalaccum <n> dec_globalaccum <g>
	globalaccum <n> mul_globalaccum <g>
	globalaccum <n> div_globalaccum <g>
=================
*/
qboolean G_ScriptAction_GlobalAccum( gentity_t *ent, char *params )
{
	char *pString = params;
	char *token = COM_ParseExt( &pString, qfalse );
	char lastToken[MAX_QPATH], name[MAX_QPATH];
	int bufferIndex;
	// IlDuca - declare bufferIndex2
	int bufferIndex2;
	qboolean terminate, found;

	// CHRUKER: b056 - Corrected help text, to say "globalaccum" instead of "accum"
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum without a buffer index\n" );
	}

	bufferIndex = atoi(token);
	// CHRUKER: b055 - Was using G_MAX_SCRIPT_ACCUM_BUFFERS, which would result in invalid indexes
	if (bufferIndex >= MAX_SCRIPT_ACCUM_BUFFERS) {
		G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
	}

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum without a command\n" );
	}

	Q_strncpyz( lastToken, token, sizeof(lastToken) );
	token = COM_ParseExt( &pString, qfalse );

	if (!Q_stricmp(lastToken, "inc")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		level.globalAccumBuffer[bufferIndex] += atoi(token);
	}
	else if (!Q_stricmp(lastToken, "abort_if_less_than")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		if (level.globalAccumBuffer[bufferIndex] < atoi(token)) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_greater_than")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		if (level.globalAccumBuffer[bufferIndex] > atoi(token)) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_not_equal") || !Q_stricmp(lastToken, "abort_if_not_equals")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		if (level.globalAccumBuffer[bufferIndex] != atoi(token)) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_equal")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		if (level.globalAccumBuffer[bufferIndex] == atoi(token)) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "bitset")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		level.globalAccumBuffer[bufferIndex] |= (1<<atoi(token));
	}
	else if (!Q_stricmp(lastToken, "bitreset")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		level.globalAccumBuffer[bufferIndex] &= ~(1<<atoi(token));
	}
	else if (!Q_stricmp(lastToken, "abort_if_bitset")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		if (level.globalAccumBuffer[bufferIndex] & (1<<atoi(token))) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_not_bitset")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		if (!(level.globalAccumBuffer[bufferIndex] & (1<<atoi(token)))) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "set")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		level.globalAccumBuffer[bufferIndex] = atoi(token);
	}
	else if (!Q_stricmp(lastToken, "random")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		level.globalAccumBuffer[bufferIndex] = rand() % atoi(token);
	}
	else if (!Q_stricmp(lastToken, "trigger_if_equal")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		if (level.globalAccumBuffer[bufferIndex] == atoi(token)) {
			gentity_t* trent;
			int oldId;
			int hash;

			token = COM_ParseExt( &pString, qfalse );
			Q_strncpyz( lastToken, token, sizeof(lastToken) );
			if (!*lastToken) {
				G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): trigger must have a name and an identifier: %s\n", params );
			}
			hash = BG_StringHashValue( token );

			token = COM_ParseExt( &pString, qfalse );
			Q_strncpyz( name, token, sizeof(name) );
			if (!*name) {
				G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): trigger must have a name and an identifier: %s\n", params );
			}

			terminate = qfalse;
			found = qfalse;
			// for all entities/bots with this scriptName
			trent = NULL;
			while ( (trent = G_FindByScriptnameFast(trent, lastToken, hash)) ) {
				found = qtrue;
				oldId = trent->scriptStatus.scriptId;
				G_Script_ScriptEvent( trent, SE_TRIGGER, name );
				// if the script changed, return false so we don't muck with it's variables
				if ((trent == ent) && (oldId != trent->scriptStatus.scriptId)) {
					terminate = qtrue;
				}
			}

			if (terminate) return qfalse;
			if (found) return qtrue;

			G_Printf(S_COLOR_YELLOW "WARNING G_ScriptAction_GlobalAccum(): trigger has unknown name: %s\n", name);
			return qtrue;
		}
	}
	else if (!Q_stricmp(lastToken, "wait_while_equal")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		if (level.globalAccumBuffer[bufferIndex] == atoi(token)) {
			return qfalse;
		}

	// added by core: few commands to make life easier for mapscripters
	// globalaccum/constant
	}
	else if (!Q_stricmp(lastToken, "dec")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "Scripting: accum %s requires a parameter\n", lastToken );
		}
		level.globalAccumBuffer[bufferIndex] -= atoi(token);
	}
	else if (!Q_stricmp(lastToken, "mul")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum %s requires a parameter\n", lastToken );
		}
		level.globalAccumBuffer[bufferIndex] *= atoi(token);
	}
	else if (!Q_stricmp(lastToken, "div")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum %s requires a parameter\n", lastToken );
		}
		if (atoi(token)!=0) level.globalAccumBuffer[bufferIndex] /= atoi(token);

	// globalaccum/accum
	}
	else if (!Q_stricmp(lastToken, "inc_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		level.globalAccumBuffer[bufferIndex] += ent->scriptAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "abort_if_less_than_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (level.globalAccumBuffer[bufferIndex] < ent->scriptAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_greater_than_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_Scripting: accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (level.globalAccumBuffer[bufferIndex] > ent->scriptAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_not_equal_accum") || !Q_stricmp(lastToken, "abort_if_not_equals_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (level.globalAccumBuffer[bufferIndex] != ent->scriptAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_equal_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (level.globalAccumBuffer[bufferIndex] == ent->scriptAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "set_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		level.globalAccumBuffer[bufferIndex] = ent->scriptAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "random_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		level.globalAccumBuffer[bufferIndex] = rand() % ent->scriptAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "dec_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		level.globalAccumBuffer[bufferIndex] -= ent->scriptAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "mul_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		level.globalAccumBuffer[bufferIndex] *= ent->scriptAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "div_accum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= G_MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (ent->scriptAccumBuffer[bufferIndex2]!=0) level.globalAccumBuffer[bufferIndex] /= ent->scriptAccumBuffer[bufferIndex2];

	} else if (!Q_stricmp(lastToken, "set_to_playingclients")) {
//		if( !*token ) {
//			G_Error(S_COLOR_RED "Scripting: globalaccum %s requires a parameter\n", lastToken );
//		}
		level.globalAccumBuffer[bufferIndex] = level.numPlayingClients;

	// globalaccum/globalaccum
	}
	else if (!Q_stricmp(lastToken, "inc_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		level.globalAccumBuffer[bufferIndex] += level.globalAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "abort_if_less_than_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (level.globalAccumBuffer[bufferIndex] < level.globalAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_greater_than_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (level.globalAccumBuffer[bufferIndex] > level.globalAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_not_equal_globalaccum") || !Q_stricmp(lastToken, "abort_if_not_equals_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (level.globalAccumBuffer[bufferIndex] != level.globalAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_equal_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (level.globalAccumBuffer[bufferIndex] == level.globalAccumBuffer[bufferIndex2]) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "set_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_Scripting: globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		level.globalAccumBuffer[bufferIndex] = level.globalAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "random_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		level.globalAccumBuffer[bufferIndex] = rand() % level.globalAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "dec_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		level.globalAccumBuffer[bufferIndex] -= level.globalAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "mul_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		level.globalAccumBuffer[bufferIndex] *= level.globalAccumBuffer[bufferIndex2];
	}
	else if (!Q_stricmp(lastToken, "div_globalaccum")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum %s requires a parameter\n", lastToken );
		}
		bufferIndex2 = atoi(token);
		if (bufferIndex2 >= MAX_SCRIPT_ACCUM_BUFFERS) {
			G_Error(S_COLOR_RED "G_ScriptAction_GlobalAccum(): globalaccum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS-1 );
		}
		if (level.globalAccumBuffer[bufferIndex2]!=0) level.globalAccumBuffer[bufferIndex] /= level.globalAccumBuffer[bufferIndex2];

	}
	else {
		G_Error(S_COLOR_RED "Scripting: globalaccum %s: unknown command\n", params );
	}

	return qtrue;
}

/*
=================
G_ScriptAction_Print

  syntax: print <text>

  Mostly for debugging purposes
=================
*/
qboolean G_ScriptAction_Print( gentity_t *ent, char *params )
{
	// Default to printing whole string
	char *printThis = params;

	char *pString, *token;
	int printLevel = 0;

	if (!params || !params[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_Print(): print requires some text\n" );
	}

	// Start parsing at the beginning of the string
	pString = params;

	// See if the first parameter is a /N, where N is a number
	if ((token = COM_ParseExt( &pString, qfalse )) && token[0] == '/') {
		// Get the integer version of the print debug level
		printLevel = atoi(&(token[1]));

		// Just print what's left
		printThis = pString;

	}

	// Only print if our debug level is as high as the print level
	if (g_scriptDebugLevel.integer >= printLevel) {
		G_Printf("(G_Script) %s-> %s\n", ent->scriptName, printThis);
	}

	return qtrue;
}

/*
=================
G_ScriptAction_FaceAngles

  syntax: faceangles <pitch> <yaw> <roll> <duration/GOTOTIME> [ACCEL/DECCEL]

  The entity will face the given angles, taking <duration> to get their. If the
  GOTOTIME is given instead of a timed duration, the duration calculated from the
  last gotomarker command will be used instead.
=================
*/
qboolean G_ScriptAction_FaceAngles( gentity_t *ent, char *params )
{
	if (!params || !params[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_FaceAngles(): syntax: faceangles <pitch> <yaw> <roll> <duration/GOTOTIME>\n" );
	}

	if (ent->scriptStatus.scriptStackChangeTime == level.time) {
		char *pString = params, *token;
		int trType = TR_LINEAR_STOP;
		int duration, i;
		vec3_t	diff;
		vec3_t	angles;

		for (i=0; i<3; ++i) {
			token = COM_Parse(&pString);
			if (!token || !token[0]) {
				G_Error(S_COLOR_RED "G_ScriptAction_FaceAngles(): syntax: faceangles <pitch> <yaw> <roll> <duration/GOTOTIME>\n" );
			}
			angles[i] = atoi(token);
		}

		token = COM_Parse(&pString);
		if (!token || !token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_FaceAngles(): faceangles requires a <pitch> <yaw> <roll> <duration/GOTOTIME>\n" );
		}
		if (!Q_stricmp(token, "gototime")) {
			duration = ent->s.pos.trDuration;
		}
		else {
			duration = atoi(token);
		}

		token = COM_Parse(&pString);
		if (token && token[0]) {
			if (!Q_stricmp(token, "accel"))
				trType = TR_ACCELERATE;
			if (!Q_stricmp(token, "deccel"))
				trType = TR_DECCELERATE;
		}

		for (i=0; i<3; ++i) {
			diff[i] = AngleDifference( angles[i], ent->s.angles[i] );
			while (diff[i] > 180)
				diff[i] -= 360;
			while (diff[i] < -180)
				diff[i] += 360;
		}

		VectorCopy( ent->s.angles, ent->s.apos.trBase );
		if (duration)
			VectorScale( diff, 1000.0/(float)duration, ent->s.apos.trDelta );
		else
			VectorClear( ent->s.apos.trDelta );
		ent->s.apos.trDuration = duration;
		ent->s.apos.trTime = level.time;
		ent->s.apos.trType = TR_LINEAR_STOP;

		if (trType != TR_LINEAR_STOP) {	// accel / deccel logic
			// calc the speed from duration and start/end delta
			for (i=0; i<3; ++i) {
				ent->s.apos.trDelta[i] = 2.0 * 1000.0 * diff[i] / (float)duration;
			}
			ent->s.apos.trType = trType;
		}
#ifdef OMNIBOTS
		{
			const char *pName = _GetEntityName(ent);
			Bot_Util_SendTrigger(ent, NULL, va("%s_start", pName ? pName : "<unknown>"),
				va("%.2f %.2f %.2f", ent->s.apos.trDelta[0], ent->s.apos.trDelta[1], ent->s.apos.trDelta[2]));
		}
#endif
	}
	else if (ent->s.apos.trTime + ent->s.apos.trDuration <= level.time) {
		// finished turning
		BG_EvaluateTrajectory( &ent->s.apos, ent->s.apos.trTime + ent->s.apos.trDuration, ent->s.angles, qtrue, ent->s.effect2Time  );
		VectorCopy( ent->s.angles, ent->s.apos.trBase );
		VectorCopy( ent->s.angles, ent->r.currentAngles );
		ent->s.apos.trTime = level.time;
		ent->s.apos.trDuration = 0;
		ent->s.apos.trType = TR_STATIONARY;
		VectorClear( ent->s.apos.trDelta );
#ifdef OMNIBOTS
		{
			const char *pName = _GetEntityName(ent);
			Bot_Util_SendTrigger(ent, NULL, va("%s_stop", pName ? pName : "<unknown>"),
				va("%.2f %.2f %.2f", ent->s.apos.trDelta[0], ent->s.apos.trDelta[1], ent->s.apos.trDelta[2]));
		}
#endif
		script_linkentity( ent );

		return qtrue;
	}

	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles, qtrue, ent->s.effect2Time );
	script_linkentity( ent );

	return qfalse;
}

/*
===================
G_ScriptAction_ResetScript

	causes any currently running scripts to abort, in favour of the current script
===================
*/
qboolean G_ScriptAction_ResetScript( gentity_t *ent, char *params )
{
	if (level.time == ent->scriptStatus.scriptStackChangeTime) {
		return qfalse;
	}

	return qtrue;
}

/*
===================
G_ScriptAction_TagConnect

	syntax: attachtotag <targetname/scriptname> <tagname>

	connect this entity onto the tag of another entity
===================
*/
qboolean G_ScriptAction_TagConnect( gentity_t *ent, char *params )
{
	char *pString = params;
	char *token = COM_Parse(&pString);
	gentity_t *parent;

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_TagConnect(): syntax: attachtotag <targetname> <tagname>\n" );
	}

	parent = G_FindByTargetname( NULL, token );
	if (!parent) {
		int hash = BG_StringHashValue( token );
		parent = G_FindByScriptnameFast(NULL, token, hash);
		if (!parent) {
			G_Error(S_COLOR_RED "G_ScriptAction_TagConnect(): unable to find entity with targetname \"%s\"", token );
		}
	}

	token = COM_Parse(&pString);
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_TagConnect(): syntax: attachtotag <targetname> <tagname>\n" );
	}

	ent->tagParent = parent;
	Q_strncpyz( ent->tagName, token, MAX_QPATH );

	G_ProcessTagConnect( ent, qtrue );

	// clear out the angles so it always starts out facing the tag direction
	VectorClear( ent->s.angles );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	ent->s.apos.trTime = level.time;
	ent->s.apos.trDuration = 0;
	ent->s.apos.trType = TR_STATIONARY;
	VectorClear( ent->s.apos.trDelta );

	return qtrue;
}

/*
====================
G_ScriptAction_Halt

  syntax: halt

  Stop moving.
====================
*/
qboolean G_ScriptAction_Halt( gentity_t *ent, char *params )
{
	if (level.time == ent->scriptStatus.scriptStackChangeTime) {
		ent->scriptStatus.scriptFlags &= ~SCFL_GOING_TO_MARKER;

		// stop the angles
		BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->s.angles, qtrue, ent->s.effect2Time  );
		VectorCopy( ent->s.angles, ent->s.apos.trBase );
		VectorCopy( ent->s.angles, ent->r.currentAngles );
		ent->s.apos.trTime = level.time;
		ent->s.apos.trDuration = 0;
		ent->s.apos.trType = TR_STATIONARY;
		VectorClear( ent->s.apos.trDelta );

		// stop moving
		BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin, qfalse, ent->s.effect2Time  );
		VectorCopy( ent->s.origin, ent->s.pos.trBase );
		VectorCopy( ent->s.origin, ent->r.currentOrigin );
		ent->s.pos.trTime = level.time;
		ent->s.pos.trDuration = 0;
		ent->s.pos.trType = TR_STATIONARY;
		VectorClear( ent->s.pos.trDelta );

		script_linkentity( ent );

		return qfalse;	// kill any currently running script
	}
	else {
		return qtrue;
	}
}

/*
===================
G_ScriptAction_StopSound

  syntax: stopsound

  Stops any looping sounds for this entity.
===================
*/
qboolean G_ScriptAction_StopSound( gentity_t *ent, char *params )
{
	ent->s.loopSound = 0;
	return qtrue;
}

/*
=================
G_ScriptAction_EntityScriptName
=================
*/
qboolean G_ScriptAction_EntityScriptName( gentity_t *ent, char *params) {
	trap_Cvar_Set( "g_scriptName", params );
	return qtrue;
}

// -----------------------------------------------------------------------

/*
=================
core:
	CheckRespawnTimes

	Calculate the respawntime.
	If g_spawntimeMinPlayers is set, spawntimes can be adjusted according to the number of players.
	* g_spawntimeMinPlayers.integer or more playing clients = 100% original spawntime,
	  downto 1 client = 10% of the original spawntime.
=================
*/

// copies of the original map-spawntimes..
static int redlimbotime = 30000;	// initialize to default value for g_redlimbotime
static int bluelimbotime = 30000;	// initialize to default value for g_bluelimbotime

void CheckRespawnTimes( qboolean disabled ) {
	qboolean plentyPlayers = qfalse;

	if ( g_spawntimeMinPlayers.integer > 0 ) {
		if ( level.numPlayingClients > 0 && level.numPlayingClients < g_spawntimeMinPlayers.integer ) {
			float ratio = 0.1f + ((0.9f / (float)(g_spawntimeMinPlayers.integer)) * (float)(level.numPlayingClients-1));
			// axis respawntime
			if ( !g_userAxisRespawnTime.integer ) {
				int newRedTime = floor((float)ratio * (float)(redlimbotime));
				// round up to full seconds, otherwise the reinforcement countdown looks silly..
				newRedTime += 1000 - (newRedTime % 1000);
				// spawntime must be at least 1 second..
				if ( newRedTime < 1000 ) newRedTime = 1000;
				trap_Cvar_Set( "g_redlimbotime", va("%i", newRedTime) );
			}
			// allies respawntime
			if ( !g_userAlliedRespawnTime.integer ) {
				int newBlueTime = floor((float)ratio * (float)(bluelimbotime));
				newBlueTime += 1000 - (newBlueTime % 1000);
				if ( newBlueTime < 1000 ) newBlueTime = 1000;
				trap_Cvar_Set( "g_bluelimbotime", va("%i", newBlueTime) );
			}
		} else {
			plentyPlayers = qtrue;
		}
	}

	// spawntime reduction inactive or disabled?..
	if ( plentyPlayers || disabled ) {
		if ( !g_userAxisRespawnTime.integer ) {
			if ( g_redlimbotime.integer != redlimbotime ) {
				trap_Cvar_Set( "g_redlimbotime", va("%i", redlimbotime) );
			}
		}
		if ( !g_userAlliedRespawnTime.integer ) {
			if ( g_bluelimbotime.integer != bluelimbotime ) {
				trap_Cvar_Set( "g_bluelimbotime", va("%i", bluelimbotime) );
			}
		}
	}

	return;
}


/*
===================
G_ScriptAction_AxisRespawntime

  syntax: wm_axis_respawntime <seconds>
===================
*/
qboolean G_ScriptAction_AxisRespawntime( gentity_t *ent, char *params )
{
	char *pString = params;
	char *token = COM_Parse(&pString);

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_AxisRespawntime(): time parameter required\n" );
	}

	if ( g_userAxisRespawnTime.integer )
		trap_Cvar_Set( "g_redlimbotime", va( "%i", g_userAxisRespawnTime.integer * 1000 ) );
	else {
		trap_Cvar_Set( "g_redlimbotime", va( "%s000", token ) );

		// core: check if spawntimes need to be reduced (for games with few players)
		redlimbotime = g_redlimbotime.integer;	// save a copy of the original spawntime
		CheckRespawnTimes(qfalse);
	}
	return qtrue;
}

/*
===================
G_ScriptAction_AlliedRespawntime

  syntax: wm_allied_respawntime <seconds>
===================
*/
qboolean G_ScriptAction_AlliedRespawntime( gentity_t *ent, char *params )
{
	char *pString = params;
	char *token = COM_Parse(&pString);

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_AlliedRespawntime(): time parameter required\n" );
	}

	if ( g_userAlliedRespawnTime.integer )
		trap_Cvar_Set( "g_bluelimbotime", va( "%i", g_userAlliedRespawnTime.integer * 1000 ) );
	else {
		trap_Cvar_Set( "g_bluelimbotime", va( "%s000", token ) );

		// core: check if spawntimes need to be reduced (for games with few players)
		bluelimbotime = g_bluelimbotime.integer;	// save a copy of the original spawntime
		CheckRespawnTimes(qfalse);
	}
	return qtrue;
}

/*
===================
G_ScriptAction_NumberofObjectives

  syntax: wm_number_of_objectives <number>
===================
*/
qboolean G_ScriptAction_NumberofObjectives( gentity_t *ent, char *params )
{
	char *pString = params;
	char *token = COM_Parse(&pString);
	char cs[MAX_STRING_CHARS];
	int	num;

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_NumberofObjectives(): number parameter required\n" );
	}

	num = atoi( token );
	if ( num < 1 || num > MAX_OBJECTIVES ) {
		G_Error(S_COLOR_RED "G_ScriptAction_NumberofObjectives(): Invalid number of objectives\n" );
	}

	trap_GetConfigstring( CS_MULTI_INFO, cs, sizeof(cs) );
	Info_SetValueForKey( cs, "o", token );		// numobjectives
	trap_SetConfigstring( CS_MULTI_INFO, cs );

	return qtrue;
}

/*
===================
G_ScriptAction_SetMainObjective

  syntax: wm_set_main_objective <objective_number> <team>
===================
*/
qboolean G_ScriptAction_SetMainObjective( gentity_t *ent, char *params ) {
/*	char *pString, *token;
	char	cs[MAX_STRING_CHARS];
	char*	parm;
	int		num, cs_obj;

	pString = params;
	token = COM_Parse(&pString);
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_ObjectiveImage(): number parameter required\n" );
	}

	num = atoi( token );
	if ( num < 1 || num > MAX_OBJECTIVES ) {
		G_Error(S_COLOR_RED "G_ScriptAction_ObjectiveImage(): Invalid objective number\n" );
	}
	parm = va("%i", num);

	token = COM_Parse(&pString);
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_ObjectiveImage(): team parameter required\n" );
	}

	cs_obj = !atoi(token) ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE;
	trap_GetConfigstring( cs_obj, cs, sizeof(cs) );

	if ( Q_stricmp( cs , parm ) ) {
		trap_SetConfigstring( cs_obj, parm );
	}*/

	return qtrue;
}

/*
===================
G_ScriptAction_ObjectiveStatus

  syntax: wm_objective_status <objective_number> <team> <status>
===================
*/
qboolean G_ScriptAction_ObjectiveStatus( gentity_t *ent, char *params ) {
	char *pString = params;
	char *token = COM_Parse(&pString);

	char *pConfigStr;
	char cs[MAX_STRING_CHARS];
	char* parm;

	int	num;

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_ObjectiveImage(): number parameter required\n" );
	}

	num = atoi( token );
	if ( num < 1 || num > MAX_OBJECTIVES ) {
		G_Error(S_COLOR_RED "G_ScriptAction_ObjectiveImage(): Invalid objective number\n" );
	}

	token = COM_Parse(&pString);
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_ObjectiveImage(): team parameter required\n" );
	}
	parm = atoi(token) == 0 ? "x" : "a";

	token = COM_Parse(&pString);
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_ObjectiveImage(): status parameter required\n" );
	}

	if( atoi(token) != 0 && atoi(token) != 1 && atoi(token) != 2) {
		G_Error(S_COLOR_RED "G_ScriptAction_ObjectiveImage(): status parameter must be 0 (default), 1 (complete) or 2 (failed)\n" );
	}

	trap_GetConfigstring( CS_MULTI_OBJECTIVE, cs, sizeof(cs) );
	pConfigStr = va( "%s%i", parm, num );
	Info_SetValueForKey( cs, pConfigStr, token );
	trap_SetConfigstring( CS_MULTI_OBJECTIVE, cs );
#ifdef OMNIBOTS
	{
		const char *pTagName = _GetEntityName(ent);
		switch(atoi(token))
		{
		case 0:
			Bot_Util_SendTrigger(ent, NULL, pTagName, parm[0] == 'x' ? "axis_default" : "allied_default");
			break;
		case 1:
			Bot_Util_SendTrigger(ent, NULL, pTagName, parm[0] == 'x' ? "axis_complete" : "allied_complete");
			break;
		case 2:
			Bot_Util_SendTrigger(ent, NULL, pTagName, parm[0] == 'x' ? "axis_failed" : "allied_failed");
			break;
		}
	}
#endif

	return qtrue;
}

/*
===================
G_ScriptAction_SetDebugLevel
===================
*/
qboolean G_ScriptAction_SetDebugLevel( gentity_t *ent, char *params)
{
	char *pString, *token;

	if (!params || !params[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetDebugLevel(): requires integer level\n" );
	}

	// Start parsing at the beginning of the string
	pString = params;


	// See if the first parameter is a /N, where N is a number
	if ((token = COM_ParseExt( &pString, qfalse )) && token[0]) {
		int debugLevel = 0;
		// Get the integer version of the debug level
		debugLevel = atoi(token);

		// Set the debug level
		trap_Cvar_Set( "g_scriptDebugLevel", va( "%i", debugLevel ) );
	}

	return qtrue;
}

/*
===================
G_ScriptAction_VoiceAnnounce
===================
*/
qboolean G_ScriptAction_VoiceAnnounce( gentity_t *ent, char *params) {
	char *pString, *token;
	int	num, sysmsg;

	if( g_gamestate.integer == GS_INTERMISSION ) {
		return qtrue;
	}

	pString = params;
	token = COM_Parse(&pString);
	if (!token[0]) {
		G_Error(S_COLOR_RED " G_ScriptAction_VoiceAnnounc()e: team parameter required\n" );
	}

	num = atoi( token );
	if ( num != 0 && num != 1 ) {
		G_Error(S_COLOR_RED "G_ScriptAction_VoiceAnnounce(): Invalid team number\n" );
	}

	token = COM_Parse(&pString);
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_VoiceAnnounce(): system message parameter required\n" );
	}

	sysmsg = G_GetSysMessageNumber( token );
	if(sysmsg == -1) {
		G_Error(S_COLOR_RED "G_ScriptAction_VoiceAnnounce(): invalid system message\n" );
	}

	G_SendSystemMessage( sysmsg, !num ? TEAM_AXIS : TEAM_ALLIES );

	return qtrue;
}

/*
===================
G_ScriptAction_SetWinner

  syntax: wm_setwinner <team>

  team: 0==AXIS, 1==ALLIED
===================
*/
qboolean G_ScriptAction_SetWinner( gentity_t *ent, char *params )
{
	char *pString, *token;
	char cs[MAX_STRING_CHARS];
	int	num;

	if( g_gamestate.integer == GS_INTERMISSION ) {
		return qtrue;
	}

	pString = params;
	token = COM_Parse(&pString);
	if (!token[0]) {
		G_Error(S_COLOR_RED " G_ScriptAction_SetWinner(): number parameter required\n" );
	}

	num = atoi( token );
	if ( num < -1 || num > 1 ) {
		G_Error(S_COLOR_RED " G_ScriptAction_SetWinner(): Invalid team number\n" );
	}

	if( g_gametype.integer == GT_WOLF_LMS ) {
		num = -1;
	}

	trap_GetConfigstring( CS_MULTI_MAPWINNER, cs, sizeof(cs) );
	Info_SetValueForKey( cs, "winner", token );
	trap_SetConfigstring( CS_MULTI_MAPWINNER, cs );

	return qtrue;
}

/*
===================
G_ScriptAction_SetDefendingTeam

  syntax: wm_set_objective_status <status>

  status: 0==axis, 1==allies

  NERVE - SMF - sets defending team for stopwatch mode
===================
*/
qboolean G_ScriptAction_SetDefendingTeam( gentity_t *ent, char *params )
{
	char *pString, *token;
	char	cs[MAX_STRING_CHARS];
	int	num;

	if( g_gamestate.integer == GS_INTERMISSION ) {
		return qtrue;
	}

	pString = params;
	token = COM_Parse(&pString);
	if (!token[0]) {
		G_Error(S_COLOR_RED " G_ScriptAction_SetDefendingTeam(): number parameter required\n" );
	}

	num = atoi( token );
	if ( num < 0 || num > 1 ) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetDefendingTeam(): Invalid team number\n" );
	}

	trap_GetConfigstring( CS_MULTI_INFO, cs, sizeof(cs) );
	Info_SetValueForKey( cs, "d", token );		//defender
	trap_SetConfigstring( CS_MULTI_INFO, cs );

	return qtrue;
}

/*
===================
G_ScriptAction_AddTeamVoiceAnnounce

  syntax: wm_addteamvoiceannounce
===================
*/
qboolean G_ScriptAction_AddTeamVoiceAnnounce( gentity_t *ent, char *params ) {
	char *pString, *token;
	int team, i, index;

	if( g_gamestate.integer != GS_PLAYING ) {
		return qtrue;
	}

	pString = params;

	token = COM_Parse(&pString);
	if( !*token ) {
		G_Error(S_COLOR_RED " G_ScriptAction_AddTeamVoiceAnnounce(): team parameter required\n" );
	}

	if( atoi( token ) ) {
		team = 1;
	} else {
		team = 0;
	}

	token = COM_Parse(&pString);
	if( !*token ) {
		G_Error(S_COLOR_RED " G_ScriptAction_AddTeamVoiceAnnounce(): sound parameter required\n" );
	}

	index = G_SoundIndex( token );

	for( i = 0; i < MAX_COMMANDER_TEAM_SOUNDS; ++i ) {
		if( level.commanderSounds[team][i].index == index + 1 ) {
			return qtrue; // already exists
		}
	}

	for( i = 0; i < MAX_COMMANDER_TEAM_SOUNDS; ++i ) {
		if( !level.commanderSounds[team][i].index ) {
			level.commanderSounds[team][i].index = index + 1;
			break;
		}
	}

	return qtrue;
}


/*
===================
G_ScriptAction_RemoveTeamVoiceAnnounce
===================
*/
qboolean G_ScriptAction_RemoveTeamVoiceAnnounce( gentity_t *ent, char *params ) {
	char *pString = params;
	char *token = COM_Parse(&pString);
	int team;
	int index, i;

	if( !*token ) {
		G_Error(S_COLOR_RED " G_ScriptAction_RemoveTeamVoiceAnnounce(): team parameter required\n" );
	}

	if( atoi( token ) ) {
		team = 1;
	}
	else {
		team = 0;
	}

	token = COM_Parse(&pString);
	if( !*token ) {
		G_Error(S_COLOR_RED "G_ScriptAction_RemoveTeamVoiceAnnounce(): sound parameter required\n" );
	}

	index = G_SoundIndex( token );

	for( i = 0; i < MAX_COMMANDER_TEAM_SOUNDS; ++i ) {
		if( index + 1 == level.commanderSounds[team][i].index ) {
			level.commanderSounds[team][i].index = 0;
		}
	}

	return qtrue;
}

/*
===================
G_ScriptAction_TeamVoiceAnnounce

  syntax: wm_teamvoiceannounce <team> <soundname>
===================
*/
qboolean G_ScriptAction_TeamVoiceAnnounce( gentity_t *ent, char *params ) {
	char *pString, *token;
	team_t team;
	gentity_t* tent;

	if( g_gamestate.integer != GS_PLAYING ) {
		return qtrue;
	}

	pString = params;

	token = COM_Parse(&pString);
	if( !*token ) {
		G_Error(S_COLOR_RED " G_ScriptAction_TeamVoiceAnnounce(): team parameter required\n" );
	}

	if( atoi( token ) ) {
		team = TEAM_ALLIES;
	}
	else {
		team = TEAM_AXIS;
	}

	token = COM_Parse(&pString);
	if( !*token ) {
		G_Error(S_COLOR_RED " G_ScriptAction_TeamVoiceAnnounce(): sound parameter required\n" );
	}

	tent = G_TempEntity( vec3_origin, EV_GLOBAL_TEAM_SOUND );
	tent->s.teamNum = team;
	tent->s.eventParm = G_SoundIndex( token );
	tent->r.svFlags = SVF_BROADCAST;

#ifdef OMNIBOTS
	{
		Bot_Util_SendTrigger(ent, NULL, token, "team_announce");
	}
#endif

	return qtrue;
}

/*
===================
G_ScriptAction_Announce

  syntax: wm_announce_icon <icon number> <"text to send to all clients">
===================
*/
qboolean G_ScriptAction_Announce_Icon( gentity_t *ent, char *params ) {
	char *pString, *token;
	int iconnumber;

	if( g_gamestate.integer == GS_INTERMISSION ) {
		return qtrue;
	}

	pString = params;

	token = COM_Parse(&pString);
	if( !*token ) {
		G_Error(S_COLOR_RED " G_ScriptAction_Announce_Icon(): icon index parameter required\n" );
	}
	iconnumber = atoi(token);
	if(iconnumber < 0 || iconnumber >= PM_NUM_TYPES) {
		G_Error(S_COLOR_RED " G_ScriptAction_Announce_Icon(): icon index parameter out of range %i\n", iconnumber );
	}

	token = COM_Parse(&pString);
	if( !*token ) {
		G_Error(S_COLOR_RED " G_ScriptAction_Announce_Icon(): statement parameter required\n" );
	}

	trap_SendServerCommand( -1, va("cpmi %i \"%s\"", iconnumber, token ));

#ifdef OMNIBOTS
	{
		Bot_Util_SendTrigger(ent, NULL, token, "announce_icon");
	}
#endif

	// pheno/IRATA: ETPro behavior
	G_LogPrintf("nq announce: \"%s\"\n", token);

	return qtrue;
}

/*
===================
G_ScriptAction_Announce

  syntax: wm_announce <"text to send to all clients">
===================
*/
qboolean G_ScriptAction_Announce( gentity_t *ent, char *params )
{
	char *pString, *token;

	if( g_gamestate.integer == GS_INTERMISSION ) {
		return qtrue;
	}

	pString = params;
	token = COM_Parse(&pString);
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_Announce: statement parameter required\n" );
	}

	// redeye - use cpm_map for easier filtering of map event messages, it does the same as cpm
	trap_SendServerCommand( -1, va("cpm_map \"%s\"", token ));

#ifdef OMNIBOTS
	{
		Bot_Util_SendTrigger(ent, NULL, token, "announce");
	}
#endif

	// pheno/IRATA: ETPro behavior
	G_LogPrintf("nq announce: \"%s\"\n", token);

	return qtrue;
}



extern void LogExit( const char *string );

/*
===================
G_ScriptAction_EndRound

  syntax: wm_endround <>
===================
*/
qboolean G_ScriptAction_EndRound( gentity_t *ent, char *params )
{
	if( g_gamestate.integer == GS_INTERMISSION ) {
		return qtrue;
	}

	LogExit( "Wolf EndRound." );

	return qtrue;
}

/*
===================
G_ScriptAction_SetRoundTimelimit

  syntax: wm_set_round_timelimit <number>
===================
*/
qboolean G_ScriptAction_SetRoundTimelimit( gentity_t *ent, char *params )
{
	char *pString = params;
	char *token = COM_Parse(&pString);
	float nextTimeLimit;

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetRoundTimelimit(): number parameter required\n" );
	}

	// NERVE - SMF
	nextTimeLimit = g_nextTimeLimit.value;

	if ( g_gametype.integer == GT_WOLF_STOPWATCH && nextTimeLimit ) {
		trap_Cvar_Set( "timelimit", va( "%f", nextTimeLimit ) );
	}
	else if( g_gametype.integer == GT_WOLF_LMS ) {
		if( g_userTimeLimit.integer ) {
			int timelimit = g_userTimeLimit.integer < 3 ? 3 : g_userTimeLimit.integer;
			trap_Cvar_Set( "timelimit", va( "%i", timelimit ) );
		}
		else {
			trap_Cvar_Set( "timelimit", token );
		}
	}
	else {
		if ( g_userTimeLimit.integer )
			trap_Cvar_Set( "timelimit", va( "%i", g_userTimeLimit.integer ) );
		else
			trap_Cvar_Set( "timelimit", token );
	}

	return qtrue;
}

/*
===================
G_ScriptAction_RemoveEntity

  syntax: remove
===================
*/
qboolean G_ScriptAction_RemoveEntity( gentity_t *ent, char *params )
{
	ent->think = G_FreeEntity;
	ent->nextthink = level.time + FRAMETIME;

	return qtrue;
}

/*
===================
G_ScriptAction_SetDamagable
===================
*/
qboolean G_ScriptAction_SetDamagable( gentity_t *ent, char *params )
{
	gentity_t *target;
	char *pString = params;
	char *token = COM_ParseExt( &pString, qfalse );
	char name[MAX_QPATH], state[MAX_QPATH];
	qboolean canDamage;

	Q_strncpyz( name, token, sizeof(name) );
	if (!name[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetDamagable(): setdamagable must have a name and an state\n" );
	}

	token = COM_ParseExt( &pString, qfalse );
	Q_strncpyz( state, token, sizeof(state) );
	if (!state[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetDamagable(): setdamagable must have a name and an state\n" );
	}

	canDamage = atoi(state) == 1 ? qtrue : qfalse;

	// look for entities
	target = &g_entities[MAX_CLIENTS-1];
	while ((target = G_FindByTargetname( target, name ))) {
		target->takedamage = canDamage;
	}

	return qtrue;
}

/*
===================
G_ScriptAction_SetState

  syntax: remove
===================
*/
qboolean G_ScriptAction_SetState( gentity_t *ent, char *params ) {
	gentity_t *target;
	char *pString = params;
	char *token = COM_ParseExt( &pString, qfalse );
	char name[MAX_QPATH], state[MAX_QPATH];
	entState_t entState = STATE_DEFAULT;
	int hash;
	qboolean found = qfalse;

	// get the cast name

	Q_strncpyz( name, token, sizeof(name) );
	if( !*name ) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetState(): setstate must have a name and an state\n" );
	}

	token = COM_ParseExt( &pString, qfalse );
	Q_strncpyz( state, token, sizeof(state) );
	if (!state[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetState(): setstate must have a name and an state\n" );
	}

	if( !Q_stricmp( state, "default" ) ) {
		entState = STATE_DEFAULT;
	}
	else if( !Q_stricmp( state, "invisible" ) ) {
		entState = STATE_INVISIBLE;
	}
	else if( !Q_stricmp( state, "underconstruction" ) ) {
		entState = STATE_UNDERCONSTRUCTION;
	}
	else {
		G_Error(S_COLOR_RED "G_ScriptAction_SetState(): setstate with invalid state '%s'\n", state );
	}

	// look for an entities
	target = &g_entities[MAX_CLIENTS-1];
	hash = BG_StringHashValue( name );
	while( 1 ) {
		target = G_FindByTargetnameFast( target, name, hash );

		if( !target ) {
			if( !found ) {
				G_Printf(S_COLOR_YELLOW "Warning: setstate called and no entity found with name: %s\n", name);
			}
			break;
		}

		found = qtrue;

		G_SetEntState( target, entState );
	}

	return qtrue;
}


/*
===================
G_ScriptAction_RepairMG42

  syntax: repairmg42 <target>
===================
*/
qboolean G_ScriptAction_RepairMG42( gentity_t *ent, char *params ) {
	gentity_t *target;
	char *pString = params;
	char *token = COM_ParseExt( &pString, qfalse );
	char name[MAX_QPATH];
	int i;

	Q_strncpyz( name, token, sizeof(name) );
	if (!name[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_RepairMG42(): repairmg42 must have a target\n" );
	}

	// look for entities
	for ( i=0; i<g_miscMG42s.count; ++i ) {
		target = g_miscMG42s.ent[i];

		if ( Q_stricmp( target->targetname, name ) ) {
			continue;
		}

		if( target->takedamage ) {
			continue;
		}

		if( target->s.eType != ET_MG42_BARREL ) {
			continue;
		}

		target->s.frame = 0;

		if( target->mg42BaseEnt > 0 ) {
			g_entities[ target->mg42BaseEnt ].health = MG42_MULTIPLAYER_HEALTH;
			g_entities[ target->mg42BaseEnt ].takedamage = qtrue;
			target->health = 0;
		}
		else {
			target->health = MG42_MULTIPLAYER_HEALTH;
		}

		target->takedamage = qtrue;
		target->s.eFlags &= ~EF_SMOKING;

#ifdef OMNIBOTS
		{
			Bot_Util_SendTrigger(ent, NULL, name, "repair_mg42");
		}
#endif

	}

	return qtrue;
}

/*
===================
G_ScriptAction_SetHQStatus

  syntax: sethqstatus <team> <status>
===================
*/
qboolean G_ScriptAction_SetHQStatus( gentity_t *ent, char *params ) {
	char *pString = params;
	char *token = COM_ParseExt( &pString, qfalse );
	team_t	team;
	qboolean exists = qfalse;

	if( !token[0] ) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetHQStatus(): sethqstatus must have a team\n" );
	}

	team = atoi(token);

	token = COM_ParseExt( &pString, qfalse );
	if( !token[0] ) {
		G_Error(S_COLOR_RED "G_ScriptAction_SetHQStatus(): sethqstatus must have a status\n" );
	}

	exists = atoi(token);

	// just in case
	if( !level.gameManager )
		return qtrue;

	if( team == 0 )
		level.gameManager->s.modelindex = exists;
	else if( team == 1 )
		level.gameManager->s.modelindex2 = exists;
	else
		G_Error(S_COLOR_RED "G_ScriptAction_SetHQStatus(): sethqstatus with bad team set" );

	return qtrue;
}

/*
===================
G_ScriptAction_PrintAccum

  syntax: printaccum <accumNumber>

  prints out the value of  accum 'accumNumber'
===================
*/
qboolean G_ScriptAction_PrintAccum( gentity_t *ent, char *params )
{
	char *token, *pString;
	int bufferIndex;

	if (!params || !params[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_PrintAccum(): syntax: PrintAccum <accumNumber>\n" );
	}

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_PrintAccum(): syntax: PrintAccum <accumNumber>\n" );
	}


	bufferIndex = atoi(token);
	// CHRUKER: b055 - Was using MAX_SCRIPT_ACCUM_BUFFERS which is a different limit
	if ((bufferIndex < 0) || (bufferIndex >= G_MAX_SCRIPT_ACCUM_BUFFERS) ) {
		G_Error(S_COLOR_RED"G_ScriptAction_PrintAccum: buffer is outside range (0 - %i)", G_MAX_SCRIPT_ACCUM_BUFFERS-1 );
	}

	G_Printf("(G_Script) %s: Accum[%i] = %d\n", ent->scriptName, bufferIndex, ent->scriptAccumBuffer[bufferIndex]);

	return qtrue;
}

/*
===================
G_ScriptAction_PrintGlobalAccum

  syntax: printGlobalAccum <globalaccumnumber>

  prints out the value of global accum 'globalaccumnumber'
===================
*/
qboolean G_ScriptAction_PrintGlobalAccum( gentity_t *ent, char *params )
{
	char *token, *pString;
	int bufferIndex;

	if (!params || !params[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_PrintGlobalAccum(): syntax: PrintGlobalAccum <globalAccumNumber>\n" );
	}

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_PrintGlobalAccum(): syntax: PrintGlobalAccum <globalAccumNumber>\n" );
	}


	bufferIndex = atoi(token);
	if ((bufferIndex < 0) || (bufferIndex >= MAX_SCRIPT_ACCUM_BUFFERS) ) {
		// CHRUKER: b055 - Was printing 8 as the last buffer index, but its actually 7
		G_Error(S_COLOR_RED "G_ScriptAction_PrintGlobalAccum(): buffer is outside range (0 - %i)", MAX_SCRIPT_ACCUM_BUFFERS-1 );
	}

	G_Printf("(G_Script) GlobalAccum[%i] = %d\n", bufferIndex, level.globalAccumBuffer[bufferIndex]);

	return qtrue;
}

/*
=================
G_ScriptAction_SetAASState

IRATA: Removed old ai code
=================
*/
qboolean G_ScriptAction_SetAASState( gentity_t *ent, char *params ) {

	G_Printf("Warning: G_ScriptAction_SetAASState for entity called but not longer implemented. Param:  %s\n", params);

	return qtrue;
}

void AutoBuildConstruction( gentity_t* constructible );

/*
===================
G_ScriptAction_Construct
===================
*/
qboolean G_ScriptAction_Construct(gentity_t *ent, char *params ) {
	char *pString = params, *token;
	gentity_t* constructible;

	if(!(token = COM_ParseExt( &pString, qfalse ))) {
		G_Error(S_COLOR_RED "G_ScriptAction_Construct(): \"construct\" must have a targetname\n" );
	}

	constructible = G_FindByTargetname( NULL, token );
	if(!constructible || !constructible->inuse || constructible->s.eType != ET_CONSTRUCTIBLE) {
		G_Error(S_COLOR_RED "G_ScriptAction_Construct(): \"construct\" could not find entity with targetname: %s\n", token );
	}

	AutoBuildConstruction( constructible );

	return qtrue;
}

/*
===================
G_ScriptAction_ConstructibleClass

  syntax: constructible_class <int:class>
===================
*/
qboolean G_ScriptAction_ConstructibleClass( gentity_t *ent, char *params )
{
	char *pString = params, *token;
	int value;

	if(!(token = COM_ParseExt( &pString, qfalse ))) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleClass(): \"constructible_class\" must have a class value\n" );
	}

	value = atoi(token);

	if( value <= 0 || value > NUM_CONSTRUCTIBLE_CLASSES ) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleClass(): \"constructible_class\" has a bad value %i\n", value );
	}

	value--;

	ent->constructibleStats = g_constructible_classes[value];
	ent->constructibleStats.weaponclass--;
	ent->health = ent->constructibleStats.health;

	return qtrue;
}

/*
===================
G_ScriptAction_ConstructibleChargeBarReq

  syntax: constructible_chargebarreq <float:fraction>
===================
*/
qboolean G_ScriptAction_ConstructibleChargeBarReq( gentity_t *ent, char *params )
{
	char *pString = params, *token;
	float value;

	if(!(token = COM_ParseExt( &pString, qfalse ))) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleChargeBarReq(): \"constructible_chargebarreq\" must have a fraction value\n" );
	}

	value = atof(token);

	if( value < 0 ) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleChargeBarReq(): \"constructible_chargebarreq\" has a bad value %f\n", value );
	}

	ent->constructibleStats.chargebarreq = value;

	return qtrue;
}

/*
===================
G_ScriptAction_ConstructibleConstructXPBonus

  syntax: constructible_constructxpbonus <int:xppoints>
===================
*/
qboolean G_ScriptAction_ConstructibleConstructXPBonus( gentity_t *ent, char *params )
{
	char *pString = params, *token;
	int value;

	if(!(token = COM_ParseExt( &pString, qfalse ))) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleConstructXPBonus(): \"constructible_constructxpbonus\" must have a xppoints value\n" );
	}

	value = atoi(token);

	if( value < 0 ) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleConstructXPBonus(): \"constructible_constructxpbonus\" has a bad value %i\n", value );
	}

	ent->constructibleStats.constructxpbonus = value;

	return qtrue;
}

/*
===================
G_ScriptAction_ConstructibleDestructXPBonus

  syntax: constructible_destructxpbonus <int:xppoints>
===================
*/
qboolean G_ScriptAction_ConstructibleDestructXPBonus( gentity_t *ent, char *params )
{
	char *pString = params, *token;
	int value;

	if(!(token = COM_ParseExt( &pString, qfalse ))) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleDestructXPBonus(): \"constructible_destructxpbonus\" must have a xppoints value\n" );
	}

	value = atoi(token);

	if( value < 0 ) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleDestructXPBonus(): \"constructible_destructxpbonus\" has a bad value %i\n", value );
	}

	ent->constructibleStats.destructxpbonus = value;

	return qtrue;
}

/*
===================
G_ScriptAction_ConstructibleHealth

  syntax: constructible_health <int:health>
===================
*/
qboolean G_ScriptAction_ConstructibleHealth( gentity_t *ent, char *params )
{
	char *pString = params, *token;
	int value;

	if(!(token = COM_ParseExt( &pString, qfalse ))) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleHealth(): \"constructible_health\" must have a health value\n" );
	}

	value = atoi(token);

	if( value <= 0 ) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleHealth(): \"constructible_health\" has a bad value %i\n", value );
	}

	ent->constructibleStats.health = value;
	ent->health = ent->constructibleStats.health;

	return qtrue;
}

/*
===================
G_ScriptAction_ConstructibleWeaponclass

  syntax: constructible_weaponclass <int:class>
===================
*/
qboolean G_ScriptAction_ConstructibleWeaponclass( gentity_t *ent, char *params )
{
	char *pString = params, *token;
	int value;

	if(!(token = COM_ParseExt( &pString, qfalse ))) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleWeaponclass(): \"constructible_weaponclass\" must have a weapon class value\n" );
	}

	value = atoi(token);

	if( value < 1 || value > 3  ) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleWeaponclass(): \"constructible_weaponclass\" has a bad value %i\n", value );
	}

	ent->constructibleStats.weaponclass = value;
	ent->constructibleStats.weaponclass--;

	return qtrue;
}

/*
===================
G_ScriptAction_ConstructibleDuration

  syntax: constructible_duration <int:duration>
===================
*/
qboolean G_ScriptAction_ConstructibleDuration( gentity_t *ent, char *params )
{
	char *pString = params, *token;
	int value;

	if(!(token = COM_ParseExt( &pString, qfalse ))) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleDuration(): \"constructible_duration\" must have a duration value\n" );
	}

	value = atoi(token);

	if( value < 0 ) {
		G_Error(S_COLOR_RED "G_ScriptAction_ConstructibleDuration(): \"constructible_duration\" has a bad value %i\n", value );
	}

	ent->constructibleStats.duration = value;

	return qtrue;
}

/*
===================
G_ScriptAction_Cvar

  syntax: cvar <cvarName> <operation> <value>
===================
*/
qboolean G_ScriptAction_Cvar( gentity_t *ent, char *params )
{
	char *pString = params;
	char *token = COM_ParseExt( &pString, qfalse );
	char lastToken[MAX_QPATH], cvarName[MAX_QPATH];
	int cvarValue;

	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar without a cvar name\n" );
	}
	Q_strncpyz( cvarName, token, sizeof( cvarName ) );

	cvarValue = trap_Cvar_VariableIntegerValue( cvarName );

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar without a command\n" );
	}

	Q_strncpyz( lastToken, token, sizeof(lastToken) );
	token = COM_ParseExt( &pString, qfalse );

	if (!Q_stricmp(lastToken, "inc")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s requires a parameter\n", lastToken );
		}
		trap_Cvar_Set( cvarName, va("%i", cvarValue+1) );
	}
	else if (!Q_stricmp(lastToken, "abort_if_less_than")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s requires a parameter\n", lastToken );
		}
		if (cvarValue < atoi(token)) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_greater_than")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s requires a parameter\n", lastToken );
		}
		if (cvarValue > atoi(token)) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_not_equal") || !Q_stricmp(lastToken, "abort_if_not_equals")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_Scripting: cvar %s requires a parameter\n", lastToken );
		}
		if (cvarValue != atoi(token)) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_equal") || !Q_stricmp(lastToken, "abort_if_equals")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s requires a parameter\n", lastToken );
		}
		if (cvarValue == atoi(token)) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "bitset")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s requires a parameter\n", lastToken );
		}
		cvarValue |= (1<<atoi(token));
	}
	else if (!Q_stricmp(lastToken, "bitreset")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s requires a parameter\n", lastToken );
		}
		cvarValue &= ~(1<<atoi(token));
	}
	else if (!Q_stricmp(lastToken, "abort_if_bitset")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s requires a parameter\n", lastToken );
		}
		if (cvarValue & (1<<atoi(token))) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "abort_if_not_bitset")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s requires a parameter\n", lastToken );
		}
		if (!(cvarValue & (1<<atoi(token)))) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	}
	else if (!Q_stricmp(lastToken, "set")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s requires a parameter\n", lastToken );
		}
		cvarValue = atoi(token);
	}
	else if (!Q_stricmp(lastToken, "random")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s requires a parameter\n", lastToken );
		}
		cvarValue = rand() % atoi(token);
	}
	else if (!Q_stricmp(lastToken, "trigger_if_equal")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s requires a parameter\n", lastToken );
		}
		if (cvarValue == atoi(token)) {
			gentity_t* trent;
			int oldId;
			int hash;
			char name[MAX_QPATH];
			qboolean terminate, found;

			token = COM_ParseExt( &pString, qfalse );
			Q_strncpyz( lastToken, token, sizeof(lastToken) );
			if (!*lastToken) {
				G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): trigger must have a name and an identifier: %s\n", params );
			}
			hash = BG_StringHashValue( token );

			token = COM_ParseExt( &pString, qfalse );
			Q_strncpyz( name, token, sizeof(name) );
			if (!*name) {
				G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): trigger must have a name and an identifier: %s\n", params );
			}

			terminate = qfalse;
			found = qfalse;
			// for all entities/bots with this scriptName
			trent = NULL;
			while ( (trent = G_FindByScriptnameFast(trent, lastToken, hash)) ) {
				found = qtrue;
				oldId = trent->scriptStatus.scriptId;
				G_Script_ScriptEvent( trent, SE_TRIGGER, name );
				// if the script changed, return false so we don't muck with it's variables
				if ((trent == ent) && (oldId != trent->scriptStatus.scriptId)) {
					terminate = qtrue;
				}
			}

			if (terminate) return qfalse;
			if (found) return qtrue;


			G_Printf(S_COLOR_YELLOW "WARNING G_ScriptAction_Cvar(): trigger has unknown name: %s\n", name);
			return qtrue;
		}
	}
	else if (!Q_stricmp(lastToken, "wait_while_equal")) {
		if (!token[0]) {
			G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s requires a parameter\n", lastToken );
		}
		if (cvarValue == atoi(token)) {
			return qfalse;
		}
	}
	else {
		G_Error(S_COLOR_RED "G_ScriptAction_Cvar(): cvar %s: unknown command\n", params );
	}

	return qtrue;
}

/*
=====================
G_ScriptAction_AbortIfWarmup
=====================
*/
qboolean G_ScriptAction_AbortIfWarmup( gentity_t *ent, char *params )
{
	if (level.warmupTime) {
		// abort the current script
		ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
	}
	return qtrue;
}

/*
====================
G_ScriptAction_AbortIfNotSinglePlayer
====================
*/
qboolean G_ScriptAction_AbortIfNotSinglePlayer( gentity_t *ent, char *params )
{
	ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;

	return qtrue;
}


/*
===================
etpro_ScriptAction_SetValues

Ikkyo - change key/value pairs of any entity

syntax (example):
set
{
	origin "3510 -960 1280"
	classname "ookblat"
	etc etc
}
available fields can be found in field_t of g_spawn.c, it is quite simple to add new ones
===================
*/
// jaquboss - FIXME should we just do set stuff BEFORE actual spawning code? or add update code to entities? ( there are certainly more entities requiring update after set{} )
void G_ModifyTOI( gentity_t *ent ) {
	char *scorestring;
	char* customimage;
	int cix, cia;

	if( G_SpawnString( "customimage", "", &customimage ) ) {
		cix = cia = G_ShaderIndex( customimage );
	}
	else {
		if( G_SpawnString( "customaxisimage", "", &customimage ) ) {
			cix = G_ShaderIndex( customimage );
		}
		else {
			cix = 0;
		}

		if( G_SpawnString( "customalliesimage", "", &customimage ) ) {
			cia = G_ShaderIndex( customimage );
		}
		else if(G_SpawnString( "customalliedimage", "", &customimage )) {
			cia = G_ShaderIndex( customimage );
		}
		else {
			cia = 0;
		}
	}

	G_SetConfigStringValue( CS_OID_DATA + ent->s.teamNum, "e",	va( "%i", ent-g_entities ) );

	if ( cix )
		G_SetConfigStringValue( CS_OID_DATA + ent->s.teamNum, "b",	va( "%i", cix )	); // "cix"
	if ( cia )
		G_SetConfigStringValue( CS_OID_DATA + ent->s.teamNum, "a",	va( "%i", cia ) ); // "cia"

	G_SetConfigStringValue( CS_OID_DATA + ent->s.teamNum, "s",	va( "%i", ent->spawnflags )			);
	G_SetConfigStringValue( CS_OID_DATA + ent->s.teamNum, "n",	ent->message ? ent->message : ""	);

	if ( G_SpawnString ("score", "0", &scorestring) )
		ent->accuracy = atof (scorestring);

	trap_SetConfigstring( CS_OID_TRIGGERS + ent->s.teamNum, ent->track );

	if( ent->s.origin[0] || ent->s.origin[1] || ent->s.origin[2] ) {
		G_SetConfigStringValue( CS_OID_DATA + ent->s.teamNum, "x",	va( "%i", (int)ent->s.origin[0] )	);
		G_SetConfigStringValue( CS_OID_DATA + ent->s.teamNum, "y",	va( "%i", (int)ent->s.origin[1] )	);
		G_SetConfigStringValue( CS_OID_DATA + ent->s.teamNum, "z",	va( "%i", (int)ent->s.origin[2] )	);
	}
	else {
		vec3_t mid;
		VectorAdd( ent->r.absmin, ent->r.absmax, mid );
		VectorScale( mid, 0.5f, mid );

		G_SetConfigStringValue( CS_OID_DATA + ent->s.teamNum, "x",	va( "%i", (int)mid[0] )	);
		G_SetConfigStringValue( CS_OID_DATA + ent->s.teamNum, "y",	va( "%i", (int)mid[1] )	);
		G_SetConfigStringValue( CS_OID_DATA + ent->s.teamNum, "z",	va( "%i", (int)mid[2] )	);
	}

	if( !ent->target ) {
		// no target - just link and go
		trap_LinkEntity (ent);
	}
	else {
		// Arnout: finalize spawing on fourth frame to allow for proper linking with targets
		ent->nextthink = level.time + (3*FRAMETIME);
		ent->think = Think_SetupObjectiveInfo;
	}
}

/*
===================
etpro_ScriptAction_SetValues
===================
*/
qboolean etpro_ScriptAction_SetValues( gentity_t *ent, char *params ) {
	char	*token;
	char	*p = params;
	char	key[MAX_TOKEN_CHARS], value[MAX_TOKEN_CHARS];
	int		classchanged = 0;
	qboolean nospawn = qfalse;

	// rain - reset and fill in the spawnVars info so that spawn
	// functions can use them
	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// Get each key/value pair
	while( 1 ) {
		token = COM_ParseExt( &p, qfalse );
		if( !token[0] )
			break;

		strcpy( key, token );

		token = COM_ParseExt( &p, qfalse );
		if( !token[0] ) {
			G_Error(S_COLOR_RED "etpro_ScriptAction_SetValues(): key \"%s\" has no value", key);
			break;
		}

		strcpy(value, token);

		if( g_scriptDebug.integer ) {
			G_Printf("%d : (%s) %s: set [%s] [%s] [%s]\n",
				level.time, ent->scriptName, GAMEVERSION,
				ent->scriptName, key, value);
		}

		if(!Q_stricmp(key, "classname_nospawn")) {
			Q_strncpyz(key, "classname", sizeof(key));
			nospawn = qtrue;
		}
		if(!Q_stricmp(key, "classname")) {
			if (Q_stricmp(value, ent->classname))
				classchanged = 1;
		}

		// rain - add spawn var so that spawn functions can use them
		if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
			G_Error(S_COLOR_RED "etpro_ScriptAction_SetValues(): MAX_SPAWN_VARS" );
		}
		level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( key );
		level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( value );
		level.numSpawnVars++;

		G_ParseField( key, value, ent );

		if( !Q_stricmp( key, "targetname" ) ) {
			//need to hash this ent targetname for setstate script targets...
			ent->targetnamehash = BG_StringHashValue( ent->targetname );
		}

		if( !Q_stricmp( key, "target" ) ) {
			ent->targethash = BG_StringHashValue( ent->target );
		}

		if( !Q_stricmp( key, "scriptname" ) ) {
			ent->scriptnamehash = BG_StringHashValue( ent->scriptName );
		}

		// jaquboss - modify TOI, that will also update some relevant client info, note it also handles dynamites and fixes some crashes
		if(!Q_stricmp(ent->classname, "trigger_objective_info") && !classchanged) {
			G_ModifyTOI( ent );
		}

	}

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	// rain - if the classname was changed, call the spawn func again
	if (classchanged) {
		if(!nospawn)
			G_CallSpawn( ent );

		if ( ent->classname && *ent->classname ) {
			ent->classnamehash = BG_StringHashValue( ent->classname );
		} else {
			ent->classnamehash = -1;
		}

		trap_LinkEntity( ent );
	}

	// relink if once linked
	if( ent->r.linked )
		trap_LinkEntity( ent );

	return qtrue;
}

extern field_t fields[];

/*
=============
	core: Delete entities that match all the criteria provided in "params"
=============
*/
qboolean G_ScriptAction_Delete ( gentity_t *ent, char *params ) {
	gentity_t	*found = NULL;
	char		*token;
	char		*p = params;
	char		key[MAX_TOKEN_CHARS], value[MAX_TOKEN_CHARS];
	int			i;
	int			pass[MAX_GENTITIES];	// number of matching criteria for each entity
	int			count = 0;				// number of key/value pairs in params
	int			deleted = 0;			// number of deleted entities
	qboolean	terminate = qfalse;
	int			valueInt;
	float		valueFloat;
	vec3_t		valueVector;

	// core: params can contain more than 1 key/value pair,
	//       We must check if the entity equals all of these pairs
	//       before we delete it..
	for ( i=MAX_CLIENTS; i<MAX_GENTITIES; ++i) {
		pass[i] = 0;
	}

	while( !terminate ) {
		// strip key/value from the params..
		token = COM_ParseExt( &p, qfalse );

		// are there tokes in the params?..
		if( !token || token[0]==0 )
			break;

		strcpy( key, token );

		token = COM_ParseExt( &p, qfalse );
		if( !token || token[0]==0 ) {
			G_Error(S_COLOR_RED "G_ScriptAction_Delete(): key \"%s\" has no value", key);
			break;
		}

		strcpy(value, token);

		// does the field exist?..
		for ( i=0; fields[i].name; i++ )
			if ( !Q_stricmp( fields[i].name, key ) )
				break;
		if ( !fields[i].name ) {
			G_Error(S_COLOR_RED "G_ScriptAction_Delete(): non-existing key \"%s\"", key);
			break;
		}

		// we have a key/value pair to search for..
		count++;

		// start searching from the first entity..
		found = NULL;

		// check key's datatype..
		switch( fields[i].type ) {
			case F_INT:
				valueInt = atoi(value);
				// find all entities with the given key/value..
				while ((found = G_FindInt(found, fields[i].ofs, valueInt))!=NULL) {
					pass[found->s.number]++;
				}
				break;

			case F_FLOAT:
				valueFloat = atof(value);
				while ((found = G_FindFloat( found, fields[i].ofs, valueFloat )) != NULL) {
					pass[found->s.number]++;
				}
				break;

			case F_LSTRING:
			case F_GSTRING:
				while ((found = G_Find( found, fields[i].ofs, value )) != NULL) {
					pass[found->s.number]++;
				}
				break;

			case F_VECTOR:
				sscanf( value, "%f %f %f", &valueVector[0], &valueVector[1], &valueVector[2] );
				while ((found = G_FindVector( found, fields[i].ofs, valueVector )) != NULL) {
					pass[found->s.number]++;
				}
				break;

			case F_ANGLEHACK:
				valueVector[0] = 0;
				valueVector[1] = atof(value);
				valueVector[2] = 0;
				while ((found = G_FindVector( found, fields[i].ofs, valueVector )) != NULL) {
					pass[found->s.number]++;
				}
				break;

			case F_ENTITY:
			case F_ITEM:
			case F_CLIENT:
			case F_IGNORE:
			default:
				// It's certain the test will fail now, so just abort..
				G_Printf(S_COLOR_YELLOW "WARNING G_ScriptAction_Delete(): invalid key \"%s\"", key);
				terminate = qtrue;
				break;
		}
	}

	// did we find any key/value pairs in the params at all?..
	if ( count == 0 ) return qfalse;

	// now delete the entities that passed all tests..
	for ( i=MAX_GENTITIES-1; i>=MAX_CLIENTS; i--)
		if ( pass[i] == count ) {
			deleted++;
			G_Printf(S_COLOR_YELLOW	"G_ScriptAction_Delete(): \"%s\" entity %i removed (%s)\n", g_entities[i].classname, i, params );
			G_FreeEntity( &g_entities[i] );
		}

	// did we actually delete any entity?..
	if ( deleted > 0 ) {
		return qtrue;
	}
	else {
		G_Printf(S_COLOR_YELLOW	"G_ScriptAction_Delete(): no entities found (%s)\n", params );
	}
	return qfalse;
}

/*
===================
G_ScriptAction_Create


tjw: added for compatability with etpro map scripts.
===================
*/
qboolean G_ScriptAction_Create( gentity_t *ent, char *params ) {
	gentity_t *create;
	char	*token;
	char	*p = params;
	char	key[MAX_TOKEN_CHARS], value[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	while( 1 ) {
		token = COM_ParseExt( &p, qfalse );
		if( !token[0] )
			break;

		strcpy( key, token );

		token = COM_ParseExt( &p, qfalse );
		if( !token[0] ) {
			G_Error(S_COLOR_RED "G_ScriptAction_Create(): key \"%s\" has no value", key);
			break;
		}

		strcpy(value, token);

		if(g_scriptDebug.integer)
			G_Printf( "%d : (%s) %s: set [%s] [%s] [%s]\n",
			level.time, ent->scriptName, GAMEVERSION,
			ent->scriptName, key, value );

		if(level.numSpawnVars == MAX_SPAWN_VARS) {
			G_Error(S_COLOR_RED "G_ScriptAction_Create(): MAX_SPAWN_VARS");
		}

		level.spawnVars[level.numSpawnVars][0] =
			G_AddSpawnVarToken(key);
		level.spawnVars[level.numSpawnVars][1] =
			G_AddSpawnVarToken(value);
		level.numSpawnVars++;
	}
	create = G_SpawnGEntityFromSpawnVars();
	trap_LinkEntity(create);
	return qtrue;
}
