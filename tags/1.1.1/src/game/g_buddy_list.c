#include "g_local.h"

//
// Waypoints
//
/*
#define WAYPOINTSET_POSTDELAY_TIME	3000	// can only set a waypoint once every 3 seconds

void waypoint_think( gentity_t *ent ) {

	ent->nextthink = level.time + FRAMETIME;
}

void G_SetWayPoint( gentity_t *ent, wayPointType_t wayPointType, vec3_t loc ) {
	gclient_t	*client;
	gentity_t	*wp;

	if( !ent || !ent->client ) {
		return;	// something went horribly wrong here
	}

	if( !wayPointType || wayPointType >= NUM_WAYPOINTTYPES ) {
		trap_SendServerCommand( ent-g_entities, "print \"Bad waypoint type.\n\"" );
		//G_Printf( "^3WARNING: G_SetWayPoint, bad waypoint type %i\n", wayPointType );
		return;
	}

	client = ent->client;

	if( !client->pers.wayPoint ) {
		wp = G_Spawn();

		wp->r.svFlags = SVF_BROADCAST;
		wp->classname = "waypoint";
		wp->s.eType = ET_WAYPOINT;
		wp->s.pos.trType = TR_STATIONARY;

		wp->r.ownerNum = ent->s.number;
		wp->s.clientNum = ent->s.number;

		wp->think = waypoint_think;
		wp->nextthink = level.time + FRAMETIME;

		// Set location
		VectorCopy( loc, wp->s.pos.trBase );

		// Set type
		wp->s.frame = wayPointType;

		// Can't set for a while - hijack this to save some memory
		wp->lastHintCheckTime = level.time;

		trap_LinkEntity( wp );

		client->pers.wayPoint = wp;
		// spint model
		VectorClear(wp->s.apos.trBase);
		VectorCopy (tv(0, 25, 0 ), wp->s.apos.trDelta );
		wp->s.apos.trType = TR_LINEAR;
		wp->s.apos.trTime = level.time;

	} else {

		wp = client->pers.wayPoint;
		
		// Set location
		VectorCopy( loc, wp->s.pos.trBase );

		// Set type
		wp->s.frame = wayPointType;

		// Can't set for a while
		wp->lastHintCheckTime = level.time;

		trap_LinkEntity( wp );
	}
}

void G_RemoveWayPoint( gclient_t *client ) {
	if( client->pers.wayPoint ) {
		G_FreeEntity( client->pers.wayPoint );
		client->pers.wayPoint = NULL;
	}
}*/

void G_RemoveFromAllIgnoreLists( int clientNum ) {
	int i;

	for( i = 0; i < MAX_CLIENTS; i++) {
		COM_BitClear( level.clients[i].sess.ignoreClients, clientNum );
	}
}
