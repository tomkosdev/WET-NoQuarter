/*
 * name:		g_match.c
 *
 * desc:		Match handling
 *
 * NQQS:
 *
 */
#include "g_local.h"
#include "../../etmain/ui/menudef.h"

void G_initMatch(void) {
	int i;

	for(i=TEAM_AXIS; i<=TEAM_ALLIES; i++) {
		G_teamReset(i, qfalse);
	}
}

// Setting initialization
void G_loadMatchGame(void) {
	unsigned int i, dwBlueOffset, dwRedOffset;
	unsigned int aRandomValues[MAX_REINFSEEDS];
	char strReinfSeeds[MAX_STRING_CHARS];

	if(server_autoconfig.integer > 0 && (!(z_serverflags.integer & ZSF_COMP) || level.newSession)) {
		G_configSet(g_gametype.integer, (server_autoconfig.integer == 1));

		trap_Cvar_Set("z_serverflags", va("%d", z_serverflags.integer | ZSF_COMP));
	}

	G_Printf("Setting MOTD...\n");
	trap_SetConfigstring(CS_CUSTMOTD + 0, server_motd0.string);
	trap_SetConfigstring(CS_CUSTMOTD + 1, server_motd1.string);
	trap_SetConfigstring(CS_CUSTMOTD + 2, server_motd2.string);
	trap_SetConfigstring(CS_CUSTMOTD + 3, server_motd3.string);
	trap_SetConfigstring(CS_CUSTMOTD + 4, server_motd4.string);
	trap_SetConfigstring(CS_CUSTMOTD + 5, server_motd5.string);

	// Voting flags
	G_voteFlags();

	// Set up the random reinforcement seeds for both teams and send to clients
	dwBlueOffset = rand() % MAX_REINFSEEDS;
	dwRedOffset = rand() % MAX_REINFSEEDS;
	strcpy(strReinfSeeds, va("%d %d", (dwBlueOffset << REINF_BLUEDELT) + (rand() % (1 << REINF_BLUEDELT)),
									  (dwRedOffset << REINF_REDDELT)  + (rand() % (1 << REINF_REDDELT))));

	for(i=0; i<MAX_REINFSEEDS; i++) {
		aRandomValues[i] = (rand() % REINF_RANGE) * aReinfSeeds[i];
		strcat(strReinfSeeds, va(" %d", aRandomValues[i]));
	}

	level.dwBlueReinfOffset = 1000 * aRandomValues[dwBlueOffset] / aReinfSeeds[dwBlueOffset];
	level.dwRedReinfOffset  = 1000 * aRandomValues[dwRedOffset] / aReinfSeeds[dwRedOffset];

	trap_SetConfigstring(CS_REINFSEEDS, strReinfSeeds);
}

// Simple alias for sure-fire print :)
void G_printFull(char *str, gentity_t *ent)
{
	if(ent != NULL) {
		CP(va("print \"%s\n\"", str));
		CP(va("cp \"%s\n\"", str));
	}
	else {
		AP(va("print \"%s\n\"", str));
		AP(va("cp \"%s\n\"", str));
	}
}

// Plays specified sound globally.
void G_globalSound(char *sound)
{
	gentity_t *te = G_TempEntityNotLinked( EV_GLOBAL_SOUND );

	te->s.eventParm = G_SoundIndex(sound);
	te->r.svFlags |= SVF_BROADCAST;
}

// Can I mangle name in C?
void G_globalSoundEnum(int sound)
{
    gentity_t *te = G_TempEntityNotLinked( EV_GLOBAL_SOUND );

    te->s.eventParm = sound;
    te->r.svFlags |= SVF_BROADCAST;
}

void G_delayPrint(gentity_t *dpent)
{
	int think_next = 0;
	qboolean fFree = qtrue;

	switch(dpent->spawnflags) {
		case DP_PAUSEINFO:
		{
			if(level.match_pause > PAUSE_UNPAUSING) {
				int cSeconds = match_timeoutlength.integer * SECONDS_1 - (level.time - dpent->timestamp);
				if(match_timeoutlength.integer == -1) return;
				if(cSeconds > SECONDS_1) {
					AP(va("cp \"^3Match resuming in ^1%d^3 seconds!\n\"", cSeconds/SECONDS_1));
					think_next = level.time + SECONDS_15;
					fFree = qfalse;
				}
				else {
					level.match_pause = PAUSE_UNPAUSING;
					AP("print \"^3Match resuming in 10 seconds!\n\"");
					trap_SendServerCommand( -1, "announcer 1");
					G_spawnPrintf(DP_UNPAUSING, level.time + 10, NULL);
				}
			}
			break;
		}

		case DP_UNPAUSING:
		{
			if(level.match_pause == PAUSE_UNPAUSING) {
				int cSeconds = 11*1000 - (level.time - dpent->timestamp);

				if(cSeconds > 1000) {
					AP(va("cp \"^3Match resuming in ^1%d^3 seconds!\n\"", cSeconds/1000));
					think_next = level.time + SECONDS_1;
					fFree = qfalse;
				}
				else {
					level.match_pause = PAUSE_NONE;
					trap_SendServerCommand( -1, "announcer 0"); // !FIGHT announce
					G_printFull("^1FIGHT!", NULL);
					trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime + level.timeDelta));
					level.server_settings &= ~CV_SVS_PAUSE;
					trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
				}
			}
			break;
		}
#ifdef MV_SUPPORT
		case DP_MVSPAWN:
		{
			int i;
			gentity_t *ent;

			for(i=0; i<level.numConnectedClients; i++) {
				ent = g_entities + level.sortedClients[i];

				if(ent->client->pers.mvReferenceList == 0) continue;
				if(ent->client->sess.sessionTeam != TEAM_SPECTATOR) continue;
				G_smvRegenerateClients(ent, ent->client->pers.mvReferenceList);
			}

			break;
		}
#endif
		default:
			break;
	}

	dpent->nextthink = think_next;
	if(fFree) {
		dpent->think = 0;
		G_FreeEntity(dpent);
	}
}

static char *pszDPInfo[] =
{
	"DPRINTF_PAUSEINFO",
	"DPRINTF_UNPAUSING",
	"DPRINTF_CONNECTINFO",
	"DPRINTF_MVSPAWN",
	"DPRINTF_UNK1",
	"DPRINTF_UNK2",
	"DPRINTF_UNK3",
	"DPRINTF_UNK4",
	"DPRINTF_UNK5"
};

void G_spawnPrintf(int print_type, int print_time, gentity_t *owner)
{
	gentity_t	*ent = G_Spawn();

	ent->classname	= pszDPInfo[print_type];
	ent->classnamehash = DPRINTF_HASH;
	ent->clipmask	= 0;
	ent->parent		= owner;
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eFlags  |= EF_NODRAW;
	ent->s.eType	= ET_ITEM;

	ent->spawnflags = print_type;		// Tunnel in DP enum
	ent->timestamp	= level.time;		// Time entity was created

	ent->nextthink	= print_time;
	ent->think		= G_delayPrint;
}

// Records accuracy, damage, and kill/death stats.
void G_addStats(gentity_t *targ, gentity_t *attacker, int dmg_ref, int mod)
{
	int			dmg;
	int			ref;
	qboolean	onSameTeam = qfalse;

	if(!targ || !targ->client) {
		return;
	}

	// Special hack for intentional gibbage
	if(targ->health <= 0 && targ->client->ps.pm_type == PM_DEAD && mod != MOD_SHOTGUN) {
		if( attacker && attacker->client) {
			int x;

			ref = G_weapStatIndex_MOD(mod);
			x = attacker->client->sess.aWeaponStats[ref].atts--;
			if (x < 1) attacker->client->sess.aWeaponStats[ref].atts = 1;
		}
		return;
	}

	if(g_gamestate.integer != GS_PLAYING) {
		return;
	}

	if(mod == MOD_SWITCHTEAM) {
		return;
	}

	if((targ->client->ps.pm_flags & PMF_LIMBO)) {
		return;
	}

	if((targ->client->ps.eFlags & EF_DEAD) && !(targ->client->ps.eFlags & EF_PLAYDEAD) && attacker->client) {
		attacker->client->sess.aWeaponStats[G_weapStatIndex_MOD(mod)].hits++;
		return;
	}

//	G_Printf("mod: %d, Index: %d, dmg: %d\n", mod, G_weapStatIndex_MOD(mod), dmg_ref);

	// Suicides only affect the player specifically
	if(targ == attacker || !attacker || !attacker->client || mod == MOD_SUICIDE) {
		if(targ->health <= 0) targ->client->sess.suicides++;
#ifdef DEBUG_STATS
		if(!attacker || !attacker->client)
#endif
			return;
	}

	// Telefrags only add 100 points.. not 100k!!
	if (mod == MOD_TELEFRAG)
		dmg = 100;
	else
		dmg = dmg_ref;

	if(g_gametype.integer < GT_WOLF)
		return;

	onSameTeam = ( targ->client->sess.sessionTeam == attacker->client->sess.sessionTeam );

	if( onSameTeam ) {
		if ( !G_FRIENDLYFIRE )
			return;

		switch(mod) {
			case MOD_SYRINGE:
				break;
			case MOD_FLAMETHROWER:
				attacker->client->sess.team_damage += dmg;
				attacker->client->sess.team_hits += 0.1f;
				attacker->client->sess.hits += 0.1f;
				break;
			case MOD_LANDMINE:
				attacker->client->sess.team_damage += dmg;
				attacker->client->sess.team_hits += 0.5f;
				attacker->client->sess.hits += 0.5f;
				break;
			default:
				attacker->client->sess.team_damage += dmg;
				attacker->client->sess.team_hits++;
				attacker->client->sess.hits++;
		}

		if(targ->health <= 0) {
			attacker->client->sess.team_kills++;
		}
	}
	else {
		attacker->client->sess.damage_given += dmg;
		switch(mod) {
			case MOD_POISON:
				// only count initial syringe hit
				// not subsequent MOD_POISON G_Damage calls
				if(!(targ->client->ps.eFlags & EF_POISONED))
					attacker->client->sess.hits++;
				break;
			case MOD_FLAMETHROWER:
				attacker->client->sess.hits += 0.1f;
				break;
			// jet Pilot - each pellet only counts as a fraction of a hit, to prevent super accuracy
			case MOD_SHOTGUN:
				attacker->client->sess.hits += (float)(1.0f/(float)MAX_SHOTGUN_PELLETS); //0.1f;
				break;
			default:
				attacker->client->sess.hits++;
		}

		targ->client->sess.damage_received += dmg;

		// if the target is killed now
		if(targ->health <= 0 && !(targ->client->ps.pm_type == PM_DEAD ) )	{ // and not already dead..
			attacker->client->sess.kills++;
			targ->client->sess.deaths++;

			// credits
			attacker->client->sess.credits += CREDITS_KILLBONUS;
			if ( attacker->client->sess.currentBetTarget == targ ) {
				// transfer the credits
				attacker->client->sess.credits += attacker->client->sess.currentBetAmount;
				targ->client->sess.credits -= targ->client->sess.currentBetAmount;
				// clear the bet
				G_ClearBet(attacker);
			}
		}
	}

	// Player weapon stats
	ref = G_weapStatIndex_MOD(mod);

	// tjw: only record syringe hits for the initial hit
	if(dmg > 0 && mod != MOD_POISON) {
		attacker->client->sess.aWeaponStats[ref].hits++;
	}

	// matt: strict shrub implementation doesn't count weapon kills/deaths
	// if done to a teammate
	if(targ->health <= 0 && !onSameTeam && !(targ->client->ps.pm_type == PM_DEAD ) ) {	// and not already dead..
		attacker->client->sess.aWeaponStats[ref].kills++;
		targ->client->sess.aWeaponStats[ref].deaths++;
	}

	// Autokick excessive team bleeders
	// IRATA don't kick bots for this issue
	if ( g_teamDamageRestriction.integer > 0 && !G_shrubbot_permission(attacker, SBF_IMMUNITY) && !(attacker->r.svFlags & SVF_BOT) ) {
		float		teamHitPct = 0;

		if ( attacker->client->sess.hits > 0 ) {
			teamHitPct = (attacker->client->sess.team_hits / attacker->client->sess.hits)*(100);
		}

		if ( attacker->client->sess.hits >= g_minHits.integer && teamHitPct > g_teamDamageRestriction.integer ) {
			int banTime = 0;

			if ( g_autoTempBan.integer && g_autoTempBanTime.integer ) {
				banTime = g_autoTempBanTime.integer;
			}
			trap_DropClient(attacker-g_entities, va("Kicked for %d seconds for excessive team damage", banTime), banTime);
		}
	}
}

// Records weapon headshots
void G_addStatsHeadShot(gentity_t *attacker, int mod)
{
#ifndef DEBUG_STATS
	if(g_gamestate.integer != GS_PLAYING) {
		return;
	}
#endif
	if(!attacker || !attacker->client) return;

	attacker->client->sess.aWeaponStats[G_weapStatIndex_MOD(mod)].headshots++;
}

//	--> MOD_* to WS_* conversion
//
// WS_MAX = no equivalent/not used
static const weap_ws_convert_t aWeapMOD[MOD_NUM_MODS] = {
	// MOD_										// WS_					// MOD_ index
	{ MOD_UNKNOWN,								WS_MAX },				// 0
	{ MOD_MACHINEGUN,							WS_MG42 },
	{ MOD_BROWNING,								WS_MG42 },
	{ MOD_MG42,									WS_MG42 },
	{ MOD_GRENADE,								WS_GRENADE },
	{ MOD_KNIFE,								WS_KNIFE },				// 5
	{ MOD_LUGER,								WS_LUGER },
	{ MOD_COLT,									WS_COLT },
	{ MOD_MP40,									WS_MP40 },
	{ MOD_THOMPSON,								WS_THOMPSON },
	{ MOD_STEN,									WS_STEN },				// 10
	{ MOD_GARAND,								WS_GARAND },
	{ MOD_SILENCER,								WS_LUGER },
	{ MOD_FG42,									WS_FG42 },
	{ MOD_FG42_SCOPE,							WS_FG42 },
	{ MOD_PANZERFAUST,							WS_PANZERFAUST },		// 15
	{ MOD_GRENADE_LAUNCHER,						WS_GRENADE },
	{ MOD_FLAMETHROWER,							WS_FLAMETHROWER },
	{ MOD_GRENADE_PINEAPPLE,					WS_GRENADE },
	{ MOD_MAPMORTAR,							WS_MORTAR },
	{ MOD_MAPMORTAR_SPLASH,						WS_MORTAR },			// 20
	{ MOD_KICKED,								WS_FOOTKICK },
	{ MOD_DYNAMITE,								WS_DYNAMITE },
	{ MOD_AIRSTRIKE,							WS_AIRSTRIKE },
	{ MOD_SYRINGE,								WS_MAX },
	{ MOD_AMMO,									WS_MAX },				// 25
	{ MOD_ARTY,									WS_ARTILLERY },
	{ MOD_WATER,								WS_MAX },
	{ MOD_SLIME,								WS_MAX },
	{ MOD_LAVA,									WS_MAX },
	{ MOD_CRUSH,								WS_MAX },				// 30
	{ MOD_TELEFRAG,								WS_MAX },
	{ MOD_FALLING,								WS_MAX },
	{ MOD_SUICIDE,								WS_MAX },
	{ MOD_TARGET_LASER,							WS_MAX },
	{ MOD_TRIGGER_HURT,							WS_MAX },				// 35
	{ MOD_EXPLOSIVE,							WS_MAX },
	{ MOD_CARBINE,								WS_GARAND },
	{ MOD_KAR98,								WS_K43 },
	{ MOD_GPG40,								WS_GRENADELAUNCHER },
	{ MOD_M7,									WS_GRENADELAUNCHER },	// 40
	{ MOD_LANDMINE,								WS_LANDMINE },
	{ MOD_SATCHEL,								WS_SATCHEL },
	{ MOD_SMOKEBOMB,							WS_MAX },
	{ MOD_MOBILE_MG42,							WS_MG42},
	{ MOD_SILENCED_COLT,						WS_COLT },				// 45
	{ MOD_GARAND_SCOPE,							WS_GARAND },
	{ MOD_CRUSH_CONSTRUCTION,					WS_MAX },
	{ MOD_CRUSH_CONSTRUCTIONDEATH,				WS_MAX },
	{ MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER,	WS_MAX },
	{ MOD_K43,									WS_K43 },				// 50
	{ MOD_K43_SCOPE,							WS_K43 },
	{ MOD_MORTAR,								WS_MORTAR },
	{ MOD_AKIMBO_COLT,							WS_COLT },
	{ MOD_AKIMBO_LUGER,							WS_LUGER },
	{ MOD_AKIMBO_SILENCEDCOLT,					WS_COLT },				// 55
	{ MOD_AKIMBO_SILENCEDLUGER,					WS_LUGER },
	{ MOD_SMOKEGRENADE,							WS_AIRSTRIKE },
	{ MOD_SWAP_PLACES,							WS_MAX },
	{ MOD_SWITCHTEAM,							WS_MAX },
	{ MOD_GOOMBA,								WS_FOOTKICK },			// 60
	{ MOD_POISON,								WS_POISON },
	{ MOD_FEAR,									WS_MAX },
	{ MOD_CENSORED,								WS_MAX },
	{ MOD_SHOTGUN,								WS_SHOTGUN },
	{ MOD_BACKSTAB,								WS_KNIFE },				// 65
	{ MOD_MOBILE_BROWNING,						WS_BROWNING },
	{ MOD_BAR,									WS_BAR },
	{ MOD_STG44,								WS_STG44 },
	{ MOD_BAZOOKA,								WS_BAZOOKA },
	{ MOD_JOHNSON,								WS_JOHNSON },				// 70
	{ MOD_MP34,									WS_MP34 },
	{ MOD_VENOM,								WS_VENOM },
	{ MOD_SHOVE,								WS_MAX },
	{ MOD_THROWKNIFE,							WS_KNIFE },				// 74
	{ MOD_JOHNSON_SCOPE,						WS_JOHNSON },			// 75

};

// Get right stats index based on weapon mod
unsigned int G_weapStatIndex_MOD(unsigned int iWeaponMOD)
{
	if ( iWeaponMOD > MOD_UNKNOWN && iWeaponMOD < MOD_NUM_MODS ) {
		return aWeapMOD[iWeaponMOD].iWS;
	}
	return WS_MAX;
}

// Generates weapon stat info for given ent
char *G_createStats(gentity_t *refEnt)
{
	unsigned int i, dwWeaponMask = 0, dwSkillPointMask = 0;
	char strWeapInfo[MAX_STRING_CHARS] = {0};
	char strSkillInfo[MAX_STRING_CHARS] = {0};
	char strHitRegionInfo[MAX_STRING_CHARS] = {0};

	if(!refEnt) return(NULL);

	// Add weapon stats as necessary
	// CHRUKER: b015 - The client also expects stats when kills are above 0
	for(i = WS_KNIFE; i < WS_MAX; i++) {
		if(refEnt->client->sess.aWeaponStats[i].atts	||
		  refEnt->client->sess.aWeaponStats[i].hits	||
		  refEnt->client->sess.aWeaponStats[i].deaths	||
		  refEnt->client->sess.aWeaponStats[i].kills) {

			dwWeaponMask |= (1 << i);

			Q_strcat(strWeapInfo, sizeof(strWeapInfo), va(" %d %d %d %d %d",
					refEnt->client->sess.aWeaponStats[i].hits, refEnt->client->sess.aWeaponStats[i].atts,
					refEnt->client->sess.aWeaponStats[i].kills, refEnt->client->sess.aWeaponStats[i].deaths,
					refEnt->client->sess.aWeaponStats[i].headshots));
		}
	}

	// Additional info
	// CHRUKER: b015 - Only send these when there are some weaponstats. This is what the client expects.
	if(dwWeaponMask != 0) {
		Q_strcat(strWeapInfo, sizeof(strWeapInfo), va(" %d %d %d",
			refEnt->client->sess.damage_given,
			refEnt->client->sess.damage_received,
			refEnt->client->sess.team_damage
			));
	}

	// core: Additional HitRegion info
	// Only send these when there are some weaponstats. This is what the client expects.
	if(dwWeaponMask != 0) {
		Q_strcat(strHitRegionInfo, sizeof(strHitRegionInfo), va(" %d %d %d %d",
				refEnt->client->pers.playerStats.hitRegions[HR_HEAD],
				refEnt->client->pers.playerStats.hitRegions[HR_ARMS],
				refEnt->client->pers.playerStats.hitRegions[HR_BODY],
				refEnt->client->pers.playerStats.hitRegions[HR_LEGS]) );
	}

	// Add skillpoints as necessary
	for(i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++) {
		// CHRUKER: b037 - Need to add negative skillpoints too.
		if(refEnt->client->sess.skillpoints[i] != 0) {
			dwSkillPointMask |= (1 << i);
			Q_strcat(strSkillInfo, sizeof(strSkillInfo), va(" %d", (int)refEnt->client->sess.skillpoints[i]));
		}
	}

#if defined(__x86_64__)
	return( va("%d %d %d%s%s %d%s", (int)(refEnt - g_entities),
									refEnt->client->sess.rounds,
									dwWeaponMask,
									strHitRegionInfo,
									strWeapInfo,
									dwSkillPointMask,
									strSkillInfo) );
#else
	return( va("%d %d %d%s%s %d%s", refEnt - g_entities,
									refEnt->client->sess.rounds,
									dwWeaponMask,
									strHitRegionInfo,
									strWeapInfo,
									dwSkillPointMask,
									strSkillInfo) );
#endif
}

// Resets player's current stats
void G_deleteStats(int nClient)
{
	int	j;
	gclient_t *cl = &level.clients[nClient];

	cl->sess.damage_given		= 0;
	cl->sess.damage_received	= 0;
	cl->sess.deaths				= 0;
	cl->sess.game_points		= 0;
	cl->sess.rounds				= 0;
	cl->sess.kills				= 0;
	cl->sess.suicides			= 0;
	cl->sess.team_damage		= 0;
	cl->sess.team_kills			= 0;
	cl->sess.team_hits			= 0.f;
	cl->sess.hits				= 0.f;
	cl->sess.newton				= 0;
	cl->sess.executions			= 0;
	cl->sess.killingSpree		= 0;
	cl->sess.poisoneer			= 0;
	cl->sess.panzernoob			= 0;
	cl->sess.kickass			= 0;
	cl->sess.missionair			= 0;
	cl->sess.resuscetations		= 0;
	cl->sess.darwin				= 0;
	cl->sess.goomba				= 0;
	cl->sess.dogtags			= 0;

	for ( j=0; j<SK_NUM_SKILLS; j++ ) {
		cl->sess.mapstartSkillpoints[j] = cl->sess.skillpoints[j];
	}

	memset(&cl->sess.aWeaponStats, 0, sizeof(cl->sess.aWeaponStats));
	trap_Cvar_Set(va("wstats%i", nClient), va("%d", nClient));
}

// Parses weapon stat info for given ent
//	---> The given string must be space delimited and contain only integers
void G_parseStats(char *pszStatsInfo)
{
	gclient_t *cl;
	const char *tmp = pszStatsInfo;
	unsigned int i, dwWeaponMask, dwClientID = atoi(pszStatsInfo);

	if(dwClientID < 0 || dwClientID > MAX_CLIENTS) return;

	cl = &level.clients[dwClientID];

#define GETVAL(x) if((tmp = strchr(tmp, ' ')) == NULL) return; x = atoi(++tmp);

	GETVAL(cl->sess.rounds);
	GETVAL(dwWeaponMask);
	for(i=WS_KNIFE; i<WS_MAX; i++) {
		if(dwWeaponMask & (1 << i)) {
			GETVAL(cl->sess.aWeaponStats[i].hits);
			GETVAL(cl->sess.aWeaponStats[i].atts);
			GETVAL(cl->sess.aWeaponStats[i].kills);
			GETVAL(cl->sess.aWeaponStats[i].deaths);
			GETVAL(cl->sess.aWeaponStats[i].headshots);
		}
	}

	// CHRUKER: b015 - These only gets generated when there are some
	//          weaponstats. This is what the client expects.
	if (dwWeaponMask != 0) {
		GETVAL(cl->sess.damage_given);
		GETVAL(cl->sess.damage_received);
		GETVAL(cl->sess.team_damage);
	}
}


// Prints current player match info.
//	--> FIXME: put the pretty print on the client
void G_printMatchInfo(gentity_t *ent)
{
	int i, j, cnt = 0, eff;
	int tot_kills, tot_deaths, tot_gp, tot_sui, tot_tk, tot_dg, tot_dr, tot_td;
	gclient_t *cl;
	char *ref;
	char n2[MAX_STRING_CHARS];

	for(i=TEAM_AXIS; i<=TEAM_ALLIES; i++) {
		if(!TeamCount(-1, i)) continue;

		tot_kills = 0;
		tot_deaths = 0;
		tot_sui = 0;
		tot_tk = 0;
		tot_dg = 0;
		tot_dr = 0;
		tot_td = 0;
		tot_gp = 0;

		CP("sc \"\n^7TEAM   Player          Kll Dth Sui TK Eff  ^3GP^7    ^2DG    ^1DR   ^6TD  ^3Score\n^7---------------------------------------------------------------------\n\"");

		for(j=0; j<level.numPlayingClients; j++) {
			cl = level.clients + level.sortedClients[j];

			if(cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam != i) continue;

			SanitizeString(cl->pers.netname, n2, qfalse);
			n2[15] = 0;

			ref = "^7";
			tot_kills  += cl->sess.kills;
			tot_deaths += cl->sess.deaths;
			tot_sui    += cl->sess.suicides;
			tot_tk	   += cl->sess.team_kills;
			tot_dg	   += cl->sess.damage_given;
			tot_dr	   += cl->sess.damage_received;
			tot_td	   += cl->sess.team_damage;
			tot_gp	   += cl->sess.game_points;

			eff = (cl->sess.deaths + cl->sess.kills == 0) ? 0 : 100 * cl->sess.kills / (cl->sess.deaths + cl->sess.kills);
			if(eff < 0) eff = 0;

			if(ent->client == cl ||
			  (ent->client->sess.sessionTeam == TEAM_SPECTATOR &&
			   ent->client->sess.spectatorState == SPECTATOR_FOLLOW &&
			   ent->client->sess.spectatorClient == level.sortedClients[j])) {
				ref = "^3";
			}

			cnt++;
			CP(va("sc \"%-10s %s%-15s^3%4d%4d%4d%3d%s%4d^3%4d^2%6d^1%6d^6%5d^3%7d\n\"",
											aTeams[i],
											ref,
											n2,
											cl->sess.kills,
											cl->sess.deaths,
											cl->sess.suicides,
											cl->sess.team_kills,
											ref,
											eff,
											cl->sess.game_points - (cl->sess.kills * WOLF_FRAG_BONUS),
											cl->sess.damage_given,
											cl->sess.damage_received,
											cl->sess.team_damage,
											cl->ps.persistant[PERS_SCORE]));
		}

		eff = (tot_kills + tot_deaths == 0) ? 0 : 100 * tot_kills / (tot_kills + tot_deaths);
		if(eff < 0) eff = 0;

		CP(va("sc \"^7---------------------------------------------------------------------\n%-10s ^5%-15s%4d%4d%4d%3d^5%4d^3%4d^2%6d^1%6d^6%5d^3%7d\n\"",
															aTeams[i],
															"Totals",
															tot_kills,
															tot_deaths,
															tot_sui,
															tot_tk,
															eff,
															tot_gp - (tot_kills * WOLF_FRAG_BONUS),
															tot_dg,
															tot_dr,
															tot_td,
															tot_gp));
	}

	CP(va("sc \"%s\n\n\" 0", ((!cnt) ? "^3\nNo scores to report." : "")));
}

// Dumps end-of-match info
void G_matchInfoDump(unsigned int dwDumpType)
{
	int i, ref;
	gentity_t *ent;
	gclient_t *cl;

	for(i=0; i<level.numConnectedClients; i++ ) {
		ref = level.sortedClients[i];
		ent = &g_entities[ref];
		cl = ent->client;

		if(cl->pers.connected != CON_CONNECTED) continue;

		if(dwDumpType == EOM_WEAPONSTATS) {
			// If client wants to write stats to a file, don't auto send this stuff
			if(!(cl->pers.clientFlags & CGF_STATSDUMP)) {
				if((cl->pers.autoaction & AA_STATSALL)
#ifdef MV_SUPPORT
					|| cl->pers.mvCount > 0
#endif
				) {
					G_statsall_cmd(ent, 0, qfalse);
				}
				else if(cl->sess.sessionTeam != TEAM_SPECTATOR) {
					if(cl->pers.autoaction & AA_STATSTEAM) G_statsall_cmd(ent, cl->sess.sessionTeam, qfalse);	// Currently broken.. need to support the overloading of dwCommandID
					else CP(va("ws %s\n", G_createStats(ent)));

				}
				else if(cl->sess.spectatorState != SPECTATOR_FREE) {
					int pid = cl->sess.spectatorClient;

					if((cl->pers.autoaction & AA_STATSTEAM)) G_statsall_cmd(ent, level.clients[pid].sess.sessionTeam, qfalse);	// Currently broken.. need to support the overloading of dwCommandID
					else CP(va("ws %s\n", G_createStats(g_entities + pid)));
				}
			}

			// Log it
			if(cl->sess.sessionTeam != TEAM_SPECTATOR) {
				G_LogPrintf("WeaponStats: %s\n", G_createStats(ent));
			}

		}
		else if(dwDumpType == EOM_MATCHINFO) {
			if(!(cl->pers.clientFlags & CGF_STATSDUMP)) G_printMatchInfo(ent);
			if(g_gametype.integer == GT_WOLF_STOPWATCH) {
				if(g_currentRound.integer == 1) {	// We've already missed the switch
					CP(va("print \">>> ^3Clock set to: %d:%02d\n\n\n\"",
												g_nextTimeLimit.integer,
												(int)(60.0 * (float)(g_nextTimeLimit.value - g_nextTimeLimit.integer))));
				}
				else {
					float val = (float)((level.timeCurrent - (level.startTime + level.time - level.intermissiontime)) / 60000.0);
					if(val < g_timelimit.value) {
						CP(va("print \">>> ^3Objective reached at %d:%02d (original: %d:%02d)\n\n\n\"",
												(int)val,
												(int)(60.0 * (val - (int)val)),
												g_timelimit.integer,
												(int)(60.0 * (float)(g_timelimit.value - g_timelimit.integer))));
					}
					else {
						CP(va("print \">>> ^3Objective NOT reached in time (%d:%02d)\n\n\n\"",
												g_timelimit.integer,
												(int)(60.0 * (float)(g_timelimit.value - g_timelimit.integer))));
					}
				}
			}
		}
	}
}

// Update configstring for vote info
int G_checkServerToggle(vmCvar_t *cv)
{
	int nFlag;

	if(cv == &g_friendlyFire) {
		if(G_FRIENDLYFIRE) level.server_settings |= CV_SVS_FRIENDLYFIRE;
		else level.server_settings &= ~CV_SVS_FRIENDLYFIRE;
		return(qtrue);
	}
	else if(cv == &match_mutespecs) nFlag = CV_SVS_MUTESPECS;
	else if(cv == &g_antilag) nFlag = CV_SVS_ANTILAG;
	else if(cv == &g_balancedteams) nFlag = CV_SVS_BALANCEDTEAMS;
	// special case for 2 bits
	else if(cv == &match_warmupDamage) {
		if(cv->integer > 0) {
			level.server_settings &= ~CV_SVS_WARMUPDMG;
			nFlag = (cv->integer > 2) ? 2 : cv->integer;
			nFlag = nFlag << 2;
		}
		else {
			nFlag = CV_SVS_WARMUPDMG;
		}
	}
	else if(cv == &g_nextmap && g_gametype.integer != GT_WOLF_CAMPAIGN) {
		if( *cv->string ) {
			level.server_settings |= CV_SVS_NEXTMAP;
		}
		else {
			level.server_settings &= ~ CV_SVS_NEXTMAP;
		}
		return( qtrue );
	}
	else if(cv == &g_nextcampaign && g_gametype.integer == GT_WOLF_CAMPAIGN) {
		if( *cv->string ) {
			level.server_settings |= CV_SVS_NEXTMAP;
		}
		else {
			level.server_settings &= ~ CV_SVS_NEXTMAP;
		}
		return( qtrue );
	}
	else return(qfalse);

	if( cv->integer > 0 )
		level.server_settings |= nFlag;
	else
		level.server_settings &= ~nFlag;

	return(qtrue);
}

// Sends a player's stats to the requesting client.
void G_statsPrint(gentity_t *ent, int nType)
{
	char *cmd = (nType == 0) ? "ws" : ((nType == 1) ? "wws" : "gstats");	// Yes, not the cleanest
	char arg[MAX_TOKEN_CHARS];

	if(!ent || (ent->r.svFlags & SVF_BOT)) return;

	// If requesting stats for self, its easy.
	if(trap_Argc() < 2) {
		if(ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
			CP(va("%s %s\n", cmd, G_createStats(ent)));
		// Specs default to players they are chasing
		}
		else if(ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
			CP(va("%s %s\n", cmd, G_createStats(g_entities + ent->client->sess.spectatorClient)));
		}
		else {
			CP("cpm \"Type ^3\\stats <player_id>^7 to see stats on an active player.\n\"");
			return;
		}
	}
	else {
		int pid;

		// Find the player to poll stats.
		trap_Argv(1, arg, sizeof(arg));
		if((pid = ClientNumberFromString(ent, arg)) == -1) return;

		CP(va("%s %s\n", cmd, G_createStats(g_entities + pid)));
	}
}

void G_resetRoundState(void)
{
	if(g_gametype.integer == GT_WOLF_STOPWATCH) {
		trap_Cvar_Set( "g_currentRound", "0" );
	}
	else if( g_gametype.integer == GT_WOLF_LMS ) {
		trap_Cvar_Set( "g_currentRound", "0" );
		trap_Cvar_Set( "g_lms_currentMatch", "0" );
	}
}

void G_resetModeState(void)
{
	if ( g_gametype.integer == GT_WOLF_STOPWATCH ) {
		trap_Cvar_Set( "g_nextTimeLimit", "0" );
	}
	else if( g_gametype.integer == GT_WOLF_LMS ) {
		trap_Cvar_Set( "g_axiswins", "0" );
		trap_Cvar_Set( "g_alliedwins", "0" );
	}
}
