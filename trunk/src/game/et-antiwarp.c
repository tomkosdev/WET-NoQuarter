/*
 * name:	et-antiwarp.c
 *
 * desc:	Antiwarp code from etpro thanks to zinx
 *
 * NQQS:
 *
 */
#include "g_local.h"

// Dens: fixes spectator bugs
qboolean G_DoAntiwarp( gentity_t *ent ) {
	// only antiwarp if requested
	// IRATA: - don't warp in intermission (fixes g_trueping 1 in intermission scoreboard)
	// IlDuca - now antiwarp works with pmove too
	if ( !g_antiwarp.integer || g_gamestate.integer == GS_INTERMISSION) {
        return qfalse;
	}

	if ( ent && ent->client ) {
        // don't antiwarp spectators or local clients ( bots, and player on listen server )
        if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
             ent->client->ps.pm_flags & PMF_LIMBO ||
             // IRATA: don't do antiwarp for players not moving self controlled (dead, tank)
             // EF_MG42_ACTIVE is used for any mg42 (also mobile one)
             // ent->client->ps.eFlags & ( EF_DEAD|EF_MOUNTEDTANK ) ||
			 ent->client->pers.localClient )
                return qfalse;

        // don't antiwarp if they haven't been connected for 5 seconds
        // note: this check is generally only triggered during mid-map
        // connects, because clients connect before loading the map.
        if ( (level.time - ent->client->pers.connectTime) < SECONDS_5 )
                return qfalse;
	}

	return qtrue;
}

void etpro_AddUsercmd( int clientNum, usercmd_t *cmd ) {
	gentity_t *ent = g_entities + clientNum;

	int idx = (ent->client->cmdhead + ent->client->cmdcount) % LAG_MAX_COMMANDS;
	ent->client->cmds[idx] = *cmd;

	if (ent->client->cmdcount < LAG_MAX_COMMANDS) {
		ent->client->cmdcount++;
	}
	else {
		ent->client->cmdhead = (ent->client->cmdhead + 1) % LAG_MAX_COMMANDS;
	}
}

// zinx - G_CmdScale is a hack :x
// IlDuca - updated according the changes made by NoQuarter
static float G_CmdScale( gentity_t *ent, usercmd_t *cmd ) {
	float	scale = abs( cmd->forwardmove );

	if ( abs( cmd->rightmove ) > scale ) {
		scale = abs( cmd->rightmove );
	}
	// zinx - don't count crouch/jump; just count moving in water
	if ( ent->waterlevel && abs( cmd->upmove ) > scale ) {
		scale = abs( cmd->upmove );
	}

	scale /= 127.f;
	if ( g_antiwarp.integer & 2 ) {
		// core: we do this like done in bg_pmove: PM_CmdScale()
		scale *= Q_rsqrt( cmd->forwardmove*cmd->forwardmove + cmd->rightmove*cmd->rightmove + cmd->upmove*cmd->upmove );
	}

	return scale;
}

void DoClientThinks( gentity_t *ent ) {
	int lastCmd, lastTime, latestTime, serverTime, totalDelta, timeDelta, savedTime;
	// This sets the max LAG the system can manage
	int drop_threshold = LAG_MAX_DROP_THRESHOLD;
#ifdef DEBUG
	// How much packets we have in the queeue
	int startPackets = ent->client->cmdcount;
#endif
	usercmd_t *cmd;
	qboolean deltahax;
	float speed, delta, scale;

	// If we don't have packets in the queeue, exit
	if ( ent->client->cmdcount <= 0 ) {
		return;
	}

	// Returns the number of milliseconds that have elapsed since ET was executed
	// allow some more movement if time has passed
	latestTime = trap_Milliseconds();

	// ent->client->lastCmdRealTime stores the number of milliseconds that have
	// elapsed since ET was executed when DoClientThinks was called the last time

	// If lastCmdRealTime is higher then latestTime is an impossible case
	// since should be the server is going backward in the time...
	if ( ent->client->lastCmdRealTime > latestTime ) {
		// zinx - stoopid server went backwards in time, reset the delta
		// instead of giving them even -less- movement ability
		ent->client->cmddelta = 0;
	}
	// latestTime - ent->client->lastCmdRealTime are the milliseconds between the last
	// time DoClientThinks was called and now, so the time passed from the last antiwarped
	// command. Reduce ent->client->cmddelta of this time.
	else {
		ent->client->cmddelta -= (latestTime - ent->client->lastCmdRealTime);
	}

	// If we have 1 or less then 1 packets in the queeue and cmddelta is
	// negative, then set cmddelta to 0
	if ( ent->client->cmdcount <= 1 && ent->client->cmddelta < 0 ) {
		ent->client->cmddelta = 0;
	}

	// Update ent->client->lastCmdRealTime with the current number of milliseconds that have
	// elapsed since ET was executed, in this way we can use it the next time
	// DoClientThinks is called
	ent->client->lastCmdRealTime = latestTime;

	// This is the last command in the queeue
	lastCmd = (ent->client->cmdhead + ent->client->cmdcount - 1) % LAG_MAX_COMMANDS;

	// ent->client->ps.commandTime stores the time when whe last command was
	// executed. Store this time in lastTime
	lastTime = ent->client->ps.commandTime;

	// Store in latestTime the time of the last command in the queeue
	latestTime = ent->client->cmds[lastCmd].serverTime;

	// While there are commands in queeue, exec this cycle
	while ( ent->client->cmdcount > 0 ) {
		// Store in cmd the first command in our queeue
		cmd = &ent->client->cmds[ent->client->cmdhead];

		// This is the difference between the time of the first command in the queeue
		// and the time of the last command in the queeue
		// IlDuca - tested : this must stay without pmove correction
		totalDelta = latestTime - cmd->serverTime;

		deltahax = qfalse;

		// Save in serverTime the time of cmd, so the time of
		// the first command in the queeue
		serverTime = cmd->serverTime;

		// If the client or the server is using pmove_fixed
		// correct the time of cmd, so the first command in the queeue,
		// according with the pmove effect.
		if ( ent->client->pers.pmoveFixed || pmove_fixed.integer ) {
			// Store the corrected time in serverTime
			// IlDuca - tested : this must stay division and multiply
			serverTime = ((serverTime + ent->client->pers.pmoveMsec - 1) /
				ent->client->pers.pmoveMsec) * ent->client->pers.pmoveMsec;
		}

		// This is the difference of time between the first command in the queeue
		// and the last command executed
		timeDelta = serverTime - lastTime;

		// If the time between the fist and the last command in the queeue
		// is too high, then there is too much lag.
		if ( totalDelta >= drop_threshold ) {
			drop_threshold = LAG_MIN_DROP_THRESHOLD;
			lastTime = ent->client->ps.commandTime = cmd->serverTime;

			goto drop_packet;
		}

		// If the time between the fist and the last command in the queeue
		// is negative, means the first command in the queeue arrived after the
		// last packet in the queeue, so there is a problem...
		if ( totalDelta < 0 ) {
			goto drop_packet;
		}

		// If the difference of time between the first command in the queeue
		// and the last command executed is zero or negative, this means the
		// current command comes from the past, so is not needed since we already
		// used newer commands
		if ( timeDelta <= 0 ) {
			// zinx - packet from the past
			goto drop_packet;
		}

		scale = 1.f / LAG_DECAY;

		speed = G_CmdScale( ent , cmd );

		// IlDuca - why sets delta twice if timeDelta is higher then
		// 50 ? Added the sets inside and if / else check
		if ( timeDelta > SERVER_FRAMETIME ) {
			timeDelta = SERVER_FRAMETIME;
			delta = (speed * (float)timeDelta);
			delta *= scale;
			deltahax = qtrue;
		}
		else {
			delta = (speed * (float)timeDelta);
			delta *= scale;
		}

		if ( (ent->client->cmddelta + delta) >= LAG_MAX_DELTA ) {
			// too many commands this server frame

			// if it'll fit in the next frame, just wait until then.
			if ( delta < LAG_MAX_DELTA && (totalDelta + delta) < LAG_MIN_DROP_THRESHOLD ) {
				break;
			}

			// try to split it up in to smaller commands
			delta = ((float)LAG_MAX_DELTA - ent->client->cmddelta);
			timeDelta = ceil(delta / speed); // prefer speedup
			delta = (float)timeDelta * speed;

			if ( timeDelta < 1 ) {
				break;
			}

			delta *= scale;
			deltahax = qtrue;
		}

		ent->client->cmddelta += delta;

		if ( deltahax ) {
			savedTime = cmd->serverTime;
			cmd->serverTime = lastTime + timeDelta;
		}
		else {
			savedTime = 0;	// zinx - shut up compiler
		}

		// zinx - erh.  hack, really. make it run for the proper amount of time.
		ent->client->ps.commandTime = lastTime;
		ClientThink_cmd( ent , cmd );
		lastTime = ent->client->ps.commandTime;

		if ( deltahax ) {
			cmd->serverTime = savedTime;

			if ( delta <= 0.1f ) {
				break;
			}

			continue;
		}

	  drop_packet:
		if ( ent->client->cmdcount <= 0 ) {
			// ent->client was cleared...
			break;
		}

		ent->client->cmdhead = (ent->client->cmdhead + 1) % LAG_MAX_COMMANDS;
		ent->client->cmdcount--;
		continue;
	}

// no pop corn for the release
#ifdef DEBUG
	// zinx - added ping, packets processed this frame
	// warning: eats bandwidth like popcorn
	if ( g_antiwarp.integer & 32 ) {
		trap_SendServerCommand( ent - g_entities,
			va( "cp \"%d %d\n\"", latestTime - lastTime, startPackets - ent->client->cmdcount));
	}
#endif

	// IlDuca - The code down here isn't used
	/*
	// zinx - debug; size is added lag (amount above player's network lag)
	// rotation is time
	if ( (g_antiwarp.integer & 16) && ent->client->cmdcount ) {
		vec3_t org, parms;

		VectorCopy( ent->client->ps.origin, org );
		SnapVector( org );

		parms[0] = 3;
		parms[1] = (float)(latestTime - ent->client->ps.commandTime) / 10.f;
		if (parms[1] < 1.f) parms[1] = 1.f;
		parms[2] = (ent->client->ps.commandTime * 180.f) / 1000.f;

		//etpro_AddDebugLine( org, parms, ((ent - g_entities) % 32), LINEMODE_SPOKES, LINESHADER_RAILCORE, 0, qfalse );
	}*/
}
