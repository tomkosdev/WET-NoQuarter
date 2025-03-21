


// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"

/*
==============================================================================

PACKET FILTERING
 

You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and you can use '*' to match any value
so you can specify an entire class C network with "addip 192.246.40.*"

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.

TTimo NOTE: GUID functions are copied over from the model of IP banning,
used to enforce max lives independently from server reconnect and team changes (Xian)

TTimo NOTE: for persistence, bans are stored in g_banIPs cvar MAX_CVAR_VALUE_STRING
The size of the cvar string buffer is limiting the banning to around 20 masks
this could be improved by putting some g_banIPs2 g_banIps3 etc. maybe
still, you should rely on PB for banning instead

==============================================================================
*/

typedef struct ipGUID_s
{
	char		compare[33];
} ipGUID_t;

#define	MAX_IPFILTERS	1024

typedef struct ipFilterList_s {
	ipFilter_t	ipFilters[MAX_IPFILTERS];
	int			numIPFilters;
	char		cvarIPList[32];
} ipFilterList_t;

static ipFilterList_t		ipFilters;
static ipFilterList_t		ipMaxLivesFilters;

static ipGUID_t		guidMaxLivesFilters[MAX_IPFILTERS];
static int			numMaxLivesFilters = 0;

/*
=================
StringToFilter
=================
*/
qboolean StringToFilter( const char *s, ipFilter_t *f )
{
	char	num[128];
	int		i, j;
	byte	b[4];
	byte	m[4];
	
	for (i=0 ; i<4 ; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}
	
	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			if (*s == '*') // 'match any'
			{
				// b[i] and m[i] to 0
				s++;
				if (!*s)
					break;
				s++;
				continue;
			}
			G_Printf( "Bad filter address: %s\n", s );
			return qfalse;
		}
		
		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi(num);
		m[i] = 255;

		if (!*s)
			break;
		s++;
	}
	
	f->mask = *(unsigned *)m;
	f->compare = *(unsigned *)b;
	
	return qtrue;
}

/*
=================
UpdateIPBans
=================
*/
static void UpdateIPBans( ipFilterList_t *ipFilterList )
{
	byte	b[4];
	byte	m[4];
	int		i,j;
	char	iplist_final[MAX_CVAR_VALUE_STRING];
	char	ip[64];

	*iplist_final = 0;
	for (i = 0 ; i < ipFilterList->numIPFilters ; i++)
	{
		if( ipFilterList->ipFilters[i].compare == 0xffffffff )
			continue;

		*(unsigned *)b = ipFilterList->ipFilters[i].compare;
		*(unsigned *)m = ipFilterList->ipFilters[i].mask;
		*ip = 0;
		for( j = 0; j < 4 ; j++ ) {
			if( m[j] != 255 )
				Q_strcat(ip, sizeof(ip), "*");
			else
				Q_strcat(ip, sizeof(ip), va("%i", b[j]));
			Q_strcat(ip, sizeof(ip), (j<3) ? "." : " ");
		}		
		if( strlen(iplist_final)+strlen(ip) < MAX_CVAR_VALUE_STRING )
		{
			Q_strcat( iplist_final, sizeof(iplist_final), ip);
		}
		else
		{
			Com_Printf( "%s overflowed at MAX_CVAR_VALUE_STRING\n", ipFilterList->cvarIPList );
			break;
		}
	}

	trap_Cvar_Set( ipFilterList->cvarIPList, iplist_final );
}

void PrintMaxLivesGUID ()
{
	int		i;

	for (i = 0 ; i < numMaxLivesFilters ; i++)
	{
		G_LogPrintf( "%i. %s\n", i, guidMaxLivesFilters[i].compare );
	}
	G_LogPrintf ( "--- End of list\n");
}

/*
=================
G_FilterPacket
=================
*/
qboolean G_FilterPacket( ipFilterList_t *ipFilterList, char *from )
{
	int		i;
	unsigned	in;
	byte m[4];
	char *p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		m[i] = 0;
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i]*10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++, p++;
	}
	
	in = *(unsigned *)m;

	for( i = 0; i < ipFilterList->numIPFilters; i++ )
		if( (in & ipFilterList->ipFilters[i].mask) == ipFilterList->ipFilters[i].compare)
 			return g_filterBan.integer != 0;

	return g_filterBan.integer == 0;
}

qboolean G_FilterIPBanPacket( char *from )
{
	return( G_FilterPacket( &ipFilters, from ) );
}

qboolean G_FilterMaxLivesIPPacket( char *from )
{
	return( G_FilterPacket( &ipMaxLivesFilters, from ) );
}

/*
 Check to see if the user is trying to sneak back in with g_enforcemaxlives enabled
*/
qboolean G_FilterMaxLivesPacket( char *from )
{
	int		i;

	for( i = 0; i < numMaxLivesFilters; i++ )
	{
		if ( !Q_stricmp( guidMaxLivesFilters[i].compare, from ) )
			return 1;
	}
	return 0;
}

/*
=================
AddIP
=================
*/
void AddIP( ipFilterList_t *ipFilterList, const char *str )
{
	int		i;

	for( i = 0; i < ipFilterList->numIPFilters; i++ ) {
		if(  ipFilterList->ipFilters[i].compare == 0xffffffff )
			break;		// free spot
	}

	if( i == ipFilterList->numIPFilters ) {
		if( ipFilterList->numIPFilters == MAX_IPFILTERS ) {
			G_Printf( "IP filter list is full\n" );
			return;
		}
		ipFilterList->numIPFilters++;
	}
	
	if( !StringToFilter( str, &ipFilterList->ipFilters[i] ) )
		ipFilterList->ipFilters[i].compare = 0xffffffffu;

	UpdateIPBans( ipFilterList );
}

void AddIPBan( const char *str )
{
	AddIP( &ipFilters, str );
}

void AddMaxLivesBan( const char *str )
{
	AddIP( &ipMaxLivesFilters, str );
}

/*
=================
AddMaxLivesGUID
Xian - with g_enforcemaxlives enabled, this adds a client GUID to a list
that prevents them from quitting and reconnecting
=================
*/
void AddMaxLivesGUID( char *str )
{
	if( numMaxLivesFilters == MAX_IPFILTERS ) {
		G_Printf( "MaxLives GUID filter list is full\n" );
		return;
	}
	Q_strncpyz( guidMaxLivesFilters[numMaxLivesFilters].compare, str, 33 );
	numMaxLivesFilters++;
}

/*
=================
G_ProcessIPBans
=================
*/
void G_ProcessIPBans(void) 
{
	char *s, *t;
	char str[MAX_CVAR_VALUE_STRING];

	ipFilters.numIPFilters = 0;
	Q_strncpyz( ipFilters.cvarIPList, "g_banIPs", sizeof(ipFilters.cvarIPList) );

	Q_strncpyz( str, g_banIPs.string, sizeof(str) );

	for( t = s = g_banIPs.string; *t; /* */ ) {
		s = strchr(s, ' ');
		if( !s )
			break;
		while( *s == ' ' )
			*s++ = 0;
		if( *t )
			AddIP( &ipFilters, t );
		t = s;
	}
}

/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f (void)
{
	char		str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf( "Usage:  addip <ip-mask>\n" );
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	AddIP( &ipFilters, str );

}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f (void)
{
	ipFilter_t	f;
	int			i;
	char		str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf("Usage:  removeip <ip-mask>\n");
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	if( !StringToFilter( str, &f ) )
		return;

	for (i=0 ; i< ipFilters.numIPFilters ; i++) {
		if( ipFilters.ipFilters[i].mask == f.mask	&&
			ipFilters.ipFilters[i].compare == f.compare ) {
			ipFilters.ipFilters[i].compare = 0xffffffffu;
			G_Printf( "Removed.\n" );

			UpdateIPBans( &ipFilters );
			return;
		}
	}

	G_Printf( "Didn't find %s.\n", str );
}

/*
 Xian - Clears out the entire list maxlives enforcement banlist
*/
void ClearMaxLivesBans ()
{
	int i;
	
	for( i = 0; i < numMaxLivesFilters; i++ ) {
		guidMaxLivesFilters[i].compare[0] = '\0';
	}
	numMaxLivesFilters = 0;

	ipMaxLivesFilters.numIPFilters = 0;
	Q_strncpyz( ipMaxLivesFilters.cvarIPList, "g_maxlivesbanIPs", sizeof(ipMaxLivesFilters.cvarIPList) );
}

/*
===================
Svcmd_EntityList_f
===================
*/
void	Svcmd_EntityList_f (void) {
	int			e;
	gentity_t		*check;

	check = g_entities+1;
	for (e = 1; e < level.num_entities ; e++, check++) {
		if ( !check->inuse ) {
			continue;
		}
		G_Printf("%3i:", e);
		switch ( check->s.eType ) {
		case ET_GENERAL:
			G_Printf("ET_GENERAL          ");
			break;
		case ET_PLAYER:
			G_Printf("ET_PLAYER           ");
			break;
		case ET_ITEM:
			G_Printf("ET_ITEM             ");
			break;
		case ET_MISSILE:
			G_Printf("ET_MISSILE          ");
			break;
		case ET_MOVER:
			G_Printf("ET_MOVER            ");
			break;
		case ET_BEAM:
			G_Printf("ET_BEAM             ");
			break;
		case ET_PORTAL:
			G_Printf("ET_PORTAL           ");
			break;
		case ET_SPEAKER:
			G_Printf("ET_SPEAKER          ");
			break;
		case ET_PUSH_TRIGGER:
			G_Printf("ET_PUSH_TRIGGER     ");
			break;
		case ET_CONCUSSIVE_TRIGGER:
			G_Printf("ET_CONCUSSIVE_TRIGGR");
			break;
		case ET_TELEPORT_TRIGGER:
			G_Printf("ET_TELEPORT_TRIGGER ");
			break;
		case ET_INVISIBLE:
			G_Printf("ET_INVISIBLE        ");
			break;
		case ET_EXPLOSIVE:
			G_Printf("ET_EXPLOSIVE        ");
			break;
		case ET_EF_SPOTLIGHT:
			G_Printf("ET_EF_SPOTLIGHT     ");
			break;
		case ET_ALARMBOX:
			G_Printf("ET_ALARMBOX          ");
			break;
		case ET_MISSILECAM:
			G_Printf("ET_MISSILECAM          ");
			break;
		default:
			G_Printf("%3i                 ", check->s.eType);
			break;
		}

		if ( check->classname ) {
			G_Printf("%s", check->classname);
		}
		G_Printf("\n");
	}
}

// fretn, note: if a player is called '3' and there are only 2 players
// on the server (clientnum 0 and 1)
// this function will say 'client 3 is not connected'
// solution: first check for usernames, if none is found, check for slotnumbers
gclient_t	*ClientForString( const char *s ) {
	gclient_t	*cl;
	int			i;
	int			idnum;

	// check for a name match
	for ( i=0 ; i < level.maxclients ; i++ ) {
		cl = &level.clients[i];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !Q_stricmp( cl->pers.netname, s ) ) {
			return cl;
		}
	}

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			Com_Printf( "Bad client slot: %i\n", idnum );
			return NULL;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			G_Printf( "Client %i is not connected\n", idnum );
			return NULL;
		}
		return cl;
	}

	G_Printf( "User %s is not on the server\n", s );

	return NULL;
}

// fretn

static qboolean G_Is_SV_Running( void ) {

	char		cvar[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "sv_running", cvar, sizeof( cvar ) );
	return (qboolean)atoi( cvar );
}

/*
==================
G_GetPlayerByNum
==================
*/
gclient_t	*G_GetPlayerByNum( int clientNum ) {
	gclient_t	*cl;

	
	// make sure server is running
	if ( !G_Is_SV_Running() ) {
		return NULL;
	}

	if ( trap_Argc() < 2 ) {
		G_Printf( "No player specified.\n" );
		return NULL;
	}

	if ( clientNum < 0 || clientNum >= level.maxclients ) {
		Com_Printf( "Bad client slot: %i\n", clientNum );
		return NULL;
	}

	cl = &level.clients[clientNum];
	if ( cl->pers.connected == CON_DISCONNECTED ) {
		G_Printf( "Client %i is not connected\n", clientNum );
		return NULL;
	}

	if (cl)
		return cl;

	
	G_Printf( "User %d is not on the server\n", clientNum );

	return NULL;
}

/*
==================
G_GetPlayerByName
==================
*/
gclient_t *G_GetPlayerByName( char *name ) {

	int			i;
	gclient_t	*cl;
	char		cleanName[64];

	// make sure server is running
	if ( !G_Is_SV_Running() ) {
		return NULL;
	}

	if ( trap_Argc() < 2 ) {
		G_Printf( "No player specified.\n" );
		return NULL;
	}

	for (i = 0; i < level.numConnectedClients; i++) {
		
		 // CHRUKER: b097 - Kick command is not able to kick some players by name
		cl = &level.clients[level.sortedClients[i]];
		
		if (!Q_stricmp(cl->pers.netname, name)) {
			return cl;
		}

		Q_strncpyz( cleanName, cl->pers.netname, sizeof(cleanName) );
		Q_CleanStr( cleanName );
		if ( !Q_stricmp( cleanName, name ) ) {
			return cl;
		}
	}

	G_Printf( "Player %s is not on the server\n", name );

	return NULL;
}

// -fretn

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/

void Svcmd_ForceTeam_f( void ) {
	gclient_t	*cl;
	char		str[MAX_TOKEN_CHARS];

	// find the player
	trap_Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );
	if ( !cl ) {
		return;
	}

	// set the team
	trap_Argv( 2, str, sizeof( str ) );
	SetTeam( &g_entities[cl - level.clients], str, qfalse, cl->sess.playerWeapon, cl->sess.playerWeapon2, qtrue );
}

/*
============
Svcmd_StartMatch_f

NERVE - SMF - starts match if in tournament mode
============
*/
void Svcmd_StartMatch_f(void)
{
/*	if ( !g_noTeamSwitching.integer ) {
		trap_SendServerCommand( -1, va("print \"g_noTeamSwitching not activated.\n\""));
		return;
	}
*/

	G_refAllReady_cmd(NULL);

/*
	if ( level.numPlayingClients <= 1 ) {
		trap_SendServerCommand( -1, va("print \"Not enough playing clients to start match.\n\""));
		return;
	}

	if ( g_gamestate.integer == GS_PLAYING ) {
		trap_SendServerCommand( -1, va("print \"Match is already in progress.\n\""));
		return;
	}

	if ( g_gamestate.integer == GS_WARMUP ) {
		trap_SendConsoleCommand( EXEC_APPEND, va( "map_restart 0 %i\n", GS_PLAYING ) );
	}
*/
}

/*
==================
Svcmd_ResetMatch_f

OSP - multiuse now for both map restarts and total match resets
==================
*/
void Svcmd_ResetMatch_f(qboolean fDoReset, qboolean fDoRestart)
{
	int i;

	for(i=0; i<level.numConnectedClients; i++) {
		g_entities[level.sortedClients[i]].client->pers.ready = 0;
	}

	if(fDoReset) {
		G_resetRoundState();
		G_resetModeState();
	}

	if(fDoRestart) {
		int mode = GS_RESET;

		switch(g_gamestate.integer) {
		case GS_PLAYING:
		case GS_INTERMISSION:
			mode = GS_WARMUP;
		}
		trap_SendConsoleCommand(EXEC_APPEND,
			va("map_restart 0 %i\n", mode)); 
	}
	// tjw: added so that a test can be done when on the first map
	//      of a campaign to determine if this is a new campaign
	//      or just a re-run of it's first map
	trap_Cvar_Set("g_reset", "1");	
}

/*
============
Svcmd_SwapTeams_f

NERVE - SMF - swaps all clients to opposite team
============
*/
void Svcmd_SwapTeams_f(void)
{
	G_resetRoundState();

	if ( (g_gamestate.integer == GS_INITIALIZE) ||
		 (g_gamestate.integer == GS_WARMUP) ||
		 (g_gamestate.integer == GS_RESET) ) {
		G_swapTeams();
		return;
	}

	G_resetModeState();

	trap_Cvar_Set( "g_swapteams", "1" );
	Svcmd_ResetMatch_f(qfalse, qtrue);
}


/*
====================
Svcmd_ShuffleTeams_f

OSP - randomly places players on teams
====================
*/
void Svcmd_ShuffleTeams_f( qboolean restart)
{
	if ( restart )
	G_resetRoundState();

	G_shuffleTeams();

	if((g_gamestate.integer == GS_INITIALIZE) ||
	  (g_gamestate.integer == GS_WARMUP) ||
	  (g_gamestate.integer == GS_RESET)) {
		return;
	}
	if ( restart ){
		G_resetModeState();
		Svcmd_ResetMatch_f(qfalse, qtrue);
	}
}

void Svcmd_Campaign_f(void) {
	char	str[MAX_TOKEN_CHARS];
	int		i;
	g_campaignInfo_t *campaign = NULL;

	// find the campaign
	trap_Argv( 1, str, sizeof( str ) );
	
	for( i = 0; i < level.campaignCount; i++ ) {
		campaign = &g_campaigns[i];

		if( !Q_stricmp( campaign->shortname, str ) ) {
			break;
		}
	}

	if( i == level.campaignCount || !(campaign->typeBits & (1 << GT_WOLF) ) ) {
		G_Printf( "Can't find campaign '%s'\n", str );
		return;
	}

	trap_Cvar_Set( "g_oldCampaign", g_currentCampaign.string );
	trap_Cvar_Set( "g_currentCampaign", campaign->shortname );
	trap_Cvar_Set( "g_currentCampaignMap", "0" );

	level.newCampaign = qtrue;

	// we got a campaign, start it
	trap_Cvar_Set( "g_gametype", va( "%i", GT_WOLF_CAMPAIGN ) );
#if 0
	if( g_developer.integer )
		trap_SendConsoleCommand( EXEC_APPEND, va( "devmap %s\n", campaign->mapnames[0] ) );
	else
#endif
		trap_SendConsoleCommand( EXEC_APPEND, va( "map %s\n", campaign->mapnames[0] ) );
}

void Svcmd_ListCampaigns_f(void) {
	int i, mpCampaigns;

	mpCampaigns = 0;

	for( i = 0; i < level.campaignCount; i++ ) {
		if( g_campaigns[i].typeBits & (1 << GT_WOLF) )
			mpCampaigns++;
	}

	if( mpCampaigns ) {
		G_Printf( "%i campaigns found:\n", mpCampaigns );
	} else {
		G_Printf( "No campaigns found.\n" );
		return;
	}

	for( i = 0; i < level.campaignCount; i++ ) {
		if( g_campaigns[i].typeBits & (1 << GT_WOLF) )
			G_Printf( " %s\n", g_campaigns[i].shortname );
	}
}



// ydnar: modified from maddoc sp func
extern void ReviveEntity(gentity_t *ent, gentity_t *traceEnt, qboolean resuscitation);
extern int FindClientByName( char *name );

void Svcmd_RevivePlayer( char *name )
{
	int			clientNum;
	gentity_t	*player;
	
	
	if( !g_cheats.integer ) 
	{
		trap_SendServerCommand( -1, va( "print \"Cheats are not enabled on this server.\n\"" ) );
		return;
	}
	
	clientNum = FindClientByName( name );
	if( clientNum < 0 )
		return;
	player = &g_entities[ clientNum ];
	
	ReviveEntity( player, player, qfalse );

}


// fretn - kicking

/*
==================
Svcmd_Kick_f

Kick a user off of the server
==================
*/

// change into qfalse if you want to use the qagame banning system
// which makes it possible to unban IP addresses
#define USE_ENGINE_BANLIST qtrue

static void Svcmd_Kick_f( void ) {
	gclient_t	*cl;
	int			i;
	int			timeout = -1;
	char		sTimeout[MAX_TOKEN_CHARS];
	char		name[MAX_TOKEN_CHARS];

	// make sure server is running
	if ( !G_Is_SV_Running() ) {
		G_Printf( "Server is not running.\n" );
		return;
	}

	if ( trap_Argc() < 2 || trap_Argc() > 3 ) {
		G_Printf ("Usage: kick <player name> [timeout]\n");
		return;
	}

	if( trap_Argc() == 3 ) {
		trap_Argv( 2, sTimeout, sizeof( sTimeout ) );
		timeout = atoi( sTimeout );
	} else {
		timeout = 300;
	}

	trap_Argv(1, name, sizeof(name));
	cl = G_GetPlayerByName( name );//ClientForString( name );

	if ( !cl ) {
		if ( !Q_stricmp(name, "all") ) {
			for (i = 0, cl = level.clients; i < level.numConnectedClients; i++, cl++) {
		
				// dont kick localclients ...
				if ( cl->pers.localClient ) {
					continue;
				}

				if ( timeout != -1 ) {
					char *ip;
					char userinfo[MAX_INFO_STRING];
					
					trap_GetUserinfo( cl->ps.clientNum, userinfo, sizeof( userinfo ) );
					ip = Info_ValueForKey (userinfo, "ip");
					
					// use engine banning system, mods may choose to use their own banlist
					if (USE_ENGINE_BANLIST) { 
						
						// kick but dont ban bots, they arent that lame
						if ( (g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
							timeout = 0;
						}

						trap_DropClient(cl->ps.clientNum, "player kicked", timeout);
					} else {
						trap_DropClient(cl->ps.clientNum, "player kicked", 0);
						
						// kick but dont ban bots, they arent that lame
						if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) )
							AddIPBan( ip );
					}

				} else {
					trap_DropClient(cl->ps.clientNum, "player kicked", 0);				
				}
			}
		} 
#ifndef NO_BOT_SUPPORT
		else if ( !Q_stricmp(name, "allbots") ) {
			for (i = 0, cl = level.clients; i < level.numConnectedClients; i++, cl++) {
				if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
					continue;
				}
				// kick but dont ban bots, they arent that lame
				trap_DropClient(cl->ps.clientNum, "player kicked", 0);
			}
		}
#endif
		return;
	} else {
		// dont kick localclients ...
		if ( cl->pers.localClient ) {
			G_Printf("Cannot kick host player\n");
			return;
		}

		if ( timeout != -1 ) {
			char *ip;
			char userinfo[MAX_INFO_STRING];
			
			trap_GetUserinfo( cl->ps.clientNum, userinfo, sizeof( userinfo ) );
			ip = Info_ValueForKey (userinfo, "ip");
			
			// use engine banning system, mods may choose to use their own banlist
			if (USE_ENGINE_BANLIST) { 
				
				// kick but dont ban bots, they arent that lame
				if ( (g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
					timeout = 0;
				}
				trap_DropClient(cl->ps.clientNum, "player kicked", timeout);
			} else {
				trap_DropClient(cl->ps.clientNum, "player kicked", 0);
				
				// kick but dont ban bots, they arent that lame
				if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) )
					AddIPBan( ip );
			}

		} else {
			trap_DropClient(cl->ps.clientNum, "player kicked", 0);				
		}
	}
}

/*
==================
Svcmd_KickNum_f

Kick a user off of the server
==================
*/
static void Svcmd_KickNum_f( void ) {
	gclient_t	*cl;
	int timeout = -1;
	char	*ip;
	char	userinfo[MAX_INFO_STRING];
	char	sTimeout[MAX_TOKEN_CHARS];
	char	name[MAX_TOKEN_CHARS];
	int		clientNum;

	// make sure server is running
	if ( !G_Is_SV_Running() ) {
		G_Printf( "Server is not running.\n" );
		return;
	}

	if ( trap_Argc() < 2 || trap_Argc() > 3 ) {
		G_Printf ("Usage: kick <client number> [timeout]\n");
		return;
	}

	if( trap_Argc() == 3 ) {
		trap_Argv( 2, sTimeout, sizeof( sTimeout ) );
		timeout = atoi( sTimeout );
	} else {
		timeout = 300;
	}

	trap_Argv(1, name, sizeof(name));
	clientNum = atoi(name);

	cl = G_GetPlayerByNum( clientNum );
	if ( !cl ) {
		return;
	}
	if ( cl->pers.localClient ) {
		G_Printf("Cannot kick host player\n");
		return;
	}
		
	trap_GetUserinfo( cl->ps.clientNum, userinfo, sizeof( userinfo ) );
	ip = Info_ValueForKey (userinfo, "ip");
	// use engine banning system, mods may choose to use their own banlist
	if (USE_ENGINE_BANLIST) { 

		// kick but dont ban bots, they arent that lame
		if ( (g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
			timeout = 0;
		}
		trap_DropClient(cl->ps.clientNum, "player kicked", timeout);
	} else {
		trap_DropClient(cl->ps.clientNum, "player kicked", 0);

		// kick but dont ban bots, they arent that lame
		if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) )
			AddIPBan( ip );
	}
}

// -fretn



//char	*ConcatArgs( int start );


qboolean ConsoleChat(qboolean chatClient) {

	int clientNum = -1;
	char line[MAX_SAY_TEXT];
	int i;
	char *s;
	int pids[MAX_CLIENTS];
	char name[MAX_NAME_LENGTH];
	int pcount = 0;

	if(chatClient) {
		if(trap_Argc() < 3) {
			G_Printf("usage: chatclient [slot#|name] message\n");
			return qfalse;
		}
		trap_Argv(1, name, sizeof(name));
		pcount = ClientNumbersFromString(name, pids);
		if(pcount > 1) {
			G_Printf("chatclient: more than one player matches."
				"nothing sent.\n");
			return qfalse;
		}
		if(pcount < 1) {
			G_Printf("chatclient: no player found\n");
			return qfalse;
		}
		clientNum = pids[0];
		s = Q_AddCR(ConcatArgs(2));
	}
	else {
		s = Q_AddCR(ConcatArgs(1));
	}

	i = 0;
	line[0] = '\0';
	while(*s) {
		if(*s == '\n' || i == (MAX_SAY_TEXT - 1)) {
			line[i] = '\0';
			CPx(clientNum, va(
				"chat \"%s\" -1 0", 
				line));
			line[0] = '\0';
			i = 0;
			*s++;
			continue;
		}
		line[i] = *s;
		*s++;
		i++;
	}
	if(line[0]) {
		line[i] = '\0';
		CPx(clientNum, va(
				"chat \"%s\" -1 0", 
				line));
	}
	if(chatClient) {
		G_LogPrintf(va("chat(client): %d: %s\n",
			clientNum,
			ConcatArgs(1)));
	}
	else {
		G_LogPrintf(va("chat(client): %s\n", 
			ConcatArgs(1)));
	}
	return qtrue;
}



void G_ClearXP() {
	int i, j;
	gentity_t *ent = &g_entities[0];
	
	G_xpsave_resetxp();  
	G_xpsave_writeconfig(); 

	for(i = 0; i < MAX_CLIENTS; i++) {
		trap_Cvar_Set(va("sessionstats%i", i), "");
		// tjw: in case we add this back in
		trap_Cvar_Set(va("wstats%i", i), "");

		ent = &g_entities[i];  
		if(ent && ent->inuse && ent->client) {  
			for(j = 0; j < SK_NUM_SKILLS; j++) {  
				ent->client->sess.skillpoints[j] = 0;  
				ent->client->sess.medals[j] = 0;  
			}  
			G_deleteStats(i);  
		}  
	}
}

/*
==================
Cmd_Insanity_Add_f
==================

Adds a value to jp_insanity (for the math deficient)
*/
void Cmd_Insanity_Add_f( void )
{
	char		*value;

	int			curValue;
	int			modValue;
	int			newValue;

	value = ConcatArgs(2);
	
	modValue = atoi(value);
	curValue = jp_insanity.integer;	

	
	
	// removing a value : check to make sure we're removing a value that's already set
	if (modValue < 0)
	{
		if (!(curValue & modValue))
		{
			G_LogPrintf( va("^9insanity : ^7cannot remove a setting that's not active!\n"));
			return;
		}		
	}

	newValue = modValue + curValue;
	G_LogPrintf(va("^9insanity : curValue : %d, modValue : %d, newValue : %d\n", curValue, modValue, newValue));

	trap_Cvar_Set( "jp_insanity", va("%d", newValue));	
}

void Cmd_Venom_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_VENOM ));
}

void Cmd_Venom_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_VENOM) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_VENOM ));
}

void Cmd_Fireball_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_FIREBOLT ));
}

void Cmd_Fireball_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_FIREBOLT) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_FIREBOLT ));
}

void Cmd_Camera_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_MORTARCAM ));
}

void Cmd_Camera_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_MORTARCAM) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_MORTARCAM ));
}

void Cmd_Guided_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_GUIDED ));
}

void Cmd_Guided_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_GUIDED) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_GUIDED ));
}


void Cmd_Homing_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_HOMING ));
}

void Cmd_Homing_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_HOMING) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_HOMING ));
}

void Cmd_Weapons_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_ALLWEAPONS ));
}

void Cmd_Weapons_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_ALLWEAPONS) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_ALLWEAPONS ));
}

void Cmd_Spawn_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_SPAWN ));
}

void Cmd_Spawn_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_SPAWN) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_SPAWN ));
}

void Cmd_Charge_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_CHARGEBAR ));
}

void Cmd_Charge_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_CHARGEBAR) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_CHARGEBAR ));
}
/*
void Cmd_Regenerate_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_REGENERATE ));
}

void Cmd_Regenerate_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_REGENERATE) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_REGENERATE ));
}*/

void Cmd_Ammunition_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_AMMUNITION ));
}

void Cmd_Ammunition_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_AMMUNITION) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_AMMUNITION ));
}
/*
void Cmd_NoHealth_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_NOHEALTH ));
}

void Cmd_NoHealth_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_NOHEALTH) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_NOHEALTH ));
}

void Cmd_Vampire_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_VAMPIRE ));
}

void Cmd_Vampire_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_VAMPIRE) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_VAMPIRE ));
}
*/
#ifdef __JETPILOT__
void Cmd_Jetpack_On_f( void )
{
	trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer | JP_INSANITY_FLYING ));
}

void Cmd_Jetpack_Off_f( void )
{
	if (jp_insanity.integer & JP_INSANITY_FLYING) 
		trap_Cvar_Set( "jp_insanity", va("%d", jp_insanity.integer - JP_INSANITY_FLYING ));
}
#endif


void G_UpdateForceCvars(void)  
	{  
	char cs[MAX_INFO_STRING];  
	int	i;

	cs[0] = '\0';  
 
	for ( i=0; i < level.forceCvarCount; i++ ){
		Info_SetValueForKey(cs, va("FCV%i", i), va("%s %s", level.forceCvars[i].cvarName, level.forceCvars[i].cvarValue) ); 
	}

	Info_SetValueForKey(cs, "NUM", va("%i", level.forceCvarCount) ); 
	trap_SetConfigstring(CS_FORCECVAR, cs);  
}  


void G_UpdateSvCvars(void)  
	{  
	char cs[MAX_INFO_STRING];  
	int	i;

	cs[0] = '\0';  
 
	for ( i=0; i < level.svCvarsCount; i++ ){
		Info_SetValueForKey(cs, va("SVCV%i", i) , 
		va("%i %s %s %s", level.svCvars[i].mode, level.svCvars[i].cvarName, level.svCvars[i].Val1, level.svCvars[i].Val2 ) ); 
	}

	Info_SetValueForKey(cs, "NUM", va("%i", level.svCvarsCount) ); 

	trap_SetConfigstring(CS_SVCVAR, cs);  
}  


/*
=================
ConsoleCommand

=================
*/
qboolean	ConsoleCommand( void ) {
	char	cmd[MAX_TOKEN_CHARS];

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( Q_stricmp (cmd, "entitylist") == 0 ) {
		Svcmd_EntityList_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "forceteam") == 0 ) {
		Svcmd_ForceTeam_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "game_memory") == 0) {
		Svcmd_GameMem_f();
		return qtrue;
	}

	// jaquboss - ET AAS doesnt work, but we can always use a testtarget :evil:
#ifndef NO_BOT_SUPPORT
	if (Q_stricmp (cmd, "addbot") == 0 && CheatsOkGeneral()) {
		Svcmd_AddBot_f();
		return qtrue;
	}
	if (Q_stricmp (cmd, "removebot") == 0 && CheatsOkGeneral()) {
		Svcmd_AddBot_f();
		return qtrue;
	}
#endif
	//

	if (!Q_stricmp(cmd, "listEntities")){
		int i;

		for (i = 0; i < MAX_GENTITIES; i++){
			if ( g_entities[i].inuse )
				G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}

		return qtrue;
	}

	if (Q_stricmp (cmd, "addip") == 0) {
		Svcmd_AddIP_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "removeip") == 0) {
		Svcmd_RemoveIP_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "listip") == 0) {
		trap_SendConsoleCommand( EXEC_INSERT, "g_banIPs\n" );
		return qtrue;
	}

	if (Q_stricmp (cmd, "listmaxlivesip") == 0) {
		PrintMaxLivesGUID();
		return qtrue;
	}

	// NERVE - SMF
	if (Q_stricmp (cmd, "start_match") == 0) {
		Svcmd_StartMatch_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "reset_match") == 0) {
		Svcmd_ResetMatch_f(qtrue, qtrue);
		return qtrue;
	}

	if (Q_stricmp (cmd, "swap_teams") == 0) {
		Svcmd_SwapTeams_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "shuffle_teams_norestart") == 0) {
		Svcmd_ShuffleTeams_f( qfalse );
		return qtrue;
	}

	if (Q_stricmp (cmd, "shuffle_teams") == 0) {
		Svcmd_ShuffleTeams_f( qtrue );
		return qtrue;
	}

	// -NERVE - SMF

	if (Q_stricmp (cmd, "makeReferee") == 0) {
		G_MakeReferee();
		return qtrue;
	}

	if (Q_stricmp (cmd, "removeReferee") == 0) {
		G_RemoveReferee();
		return qtrue;
	}

	/*if (Q_stricmp (cmd, "mute") == 0) {
		G_MuteClient();
		return qtrue;
	}

	if (Q_stricmp (cmd, "unmute") == 0) {
		G_UnMuteClient();
		return qtrue;
	}*/

	if (Q_stricmp (cmd, "ban") == 0) {
		G_PlayerBan();
		return qtrue;
	}

	if( Q_stricmp( cmd, "campaign" ) == 0 ) {
		Svcmd_Campaign_f();
		return qtrue;
	}

	if( Q_stricmp( cmd, "listcampaigns" ) == 0 ) {
		Svcmd_ListCampaigns_f();
		return qtrue;
	}

#ifndef NO_BOT_SUPPORT
	if (Q_stricmp (cmd, "spawnbot") == 0) {
		Svcmd_SpawnBot();
		return qtrue;
	}
#endif

// START - Mad Doc - TDF
	if (Q_stricmp (cmd, "revive") == 0)
	{
		trap_Argv( 1, cmd, sizeof( cmd ) );
		Svcmd_RevivePlayer( cmd );
		return qtrue;
	}
// END - Mad Doc - TDF

		// fretn - moved from engine
	if (!Q_stricmp(cmd, "kick")) {
		Svcmd_Kick_f();
		return qtrue;
	}
	
	if (!Q_stricmp(cmd, "clientkick")) {
		Svcmd_KickNum_f();
		return qtrue;
	}
	// -fretn

#ifdef OMNIBOTS
	if ( Q_stricmp( cmd, "bot") == 0 ) {
		Bot_Interface_ConsoleCommand();
		return qtrue;
	}
#endif

	if( (!Q_stricmp (cmd, "say") && g_dedicated.integer) ||
		!Q_stricmp (cmd, "svsay") ) {
	
		trap_SendServerCommand( -1,
			va("cpm \"server: %s\n\"",
			Q_AddCR(ConcatArgs(1))));
		return qtrue;
	}

	// OSP - console also gets ref commands
	if(!level.fLocalHost && !Q_stricmp(cmd, "ref") ) {
		trap_Argv(1, cmd, sizeof(cmd));
		if(!G_refCommandCheck(NULL, cmd)) {
			G_refHelp_cmd(NULL);
		}
		return(qtrue);
	}

	if(!Q_stricmp(cmd, "chat") ||
		!Q_stricmp(cmd, "qsay")) {

		ConsoleChat(qfalse);
		return qtrue;
	}

	if(!Q_stricmp(cmd, "chatclient")) {
		ConsoleChat(qtrue);
		return qtrue;
	}
	
	if(!Q_stricmp(cmd, "announce")) {

		trap_SendServerCommand( -1, va("announce \"%s\"", Q_AddCR(ConcatArgs(1))));
		G_LogPrintf(va("announce: %s\n", Q_AddCR(ConcatArgs(1))));
		return qtrue;
	}

	if(!Q_stricmp(cmd, "cp")) {

		trap_SendServerCommand( -1, va(
					"cp \"%s\"", 
					Q_AddCR(ConcatArgs(1))));
		G_LogPrintf(va("cp: %s\n",
			Q_AddCR(ConcatArgs(1))));
		return qtrue;
	}

	if(!Q_stricmp(cmd, "cpmsay") ||
		!Q_stricmp(cmd, "cpm")) {

		trap_SendServerCommand( -1, va(
					"cpm \"%s\" 1", 
					ConcatArgs(1)));
		G_LogPrintf(va("cpm: %s\n", ConcatArgs(1)));
		return qtrue;
	}

	if(!Q_stricmp(cmd, "m") ||
		!Q_stricmp(cmd, "pm") || // jaquboss
		!Q_stricmp(cmd, "priv")) {

		G_PrivateMessage(NULL);
		return qtrue;
	}

	if(!Q_stricmp(cmd, "clearxp")) {
		G_ClearXP();
		return qtrue;
	}

	if (!Q_stricmp(cmd, "forcecvarempty"))
	{
		memset( level.forceCvars, 0, sizeof(level.forceCvars));
		level.forceCvarCount = 0;

		G_UpdateForceCvars();
		return qtrue;
	}

	if (!Q_stricmp( cmd, "reloadConfig" )){

		trap_SetConfigstring( CS_CONFIGNAME, "" );
		memset( &level.config, 0, sizeof(config_t ));
		if (G_LoadConfig( "" ))
			G_Printf( "Reloaded config: %s\n", level.config.name );

		return qtrue;
	}

	if (!Q_stricmp( cmd, "loadConfig" )){
		char scriptName[MAX_QPATH];

		if(trap_Argc() != 2) {
			G_Printf("usage: loadConfig <config name>\n");
			return qtrue;
		}

		trap_Argv(1, scriptName, sizeof(scriptName));

		trap_SetConfigstring( CS_CONFIGNAME, "" );
		memset( &level.config, 0, sizeof(config_t ));
		if ( G_LoadConfig( scriptName ) )
		 G_Printf( "Loaded config: %s\n", level.config.name );

		return qtrue;
	}

	if(!Q_stricmp(cmd, "forcecvar")) {
		// tjw: what's the max for a cvar name?
		char cvarName[MAX_CVAR_VALUE_STRING];
		char cvarValue[MAX_CVAR_VALUE_STRING];

		if(trap_Argc() != 3) {
			G_Printf("usage: forcecvar <cvar name> <cvar value>\n");
			return qtrue;
		}
		trap_Argv(1, cvarName, sizeof(cvarName));
		trap_Argv(2, cvarValue, sizeof(cvarValue));

		// tjw: add the cvar to the list that is fed to 
		//      connecting clients
		if(level.forceCvarCount >= MAX_FORCECVARS) {
			G_Printf("forcecvar: MAX_FORCECVARS hit\n");
			return qtrue;
		}

		Q_strncpyz(level.forceCvars[level.forceCvarCount].cvarName,
			cvarName,
			sizeof(level.forceCvars[0].cvarName));

		Q_strncpyz(level.forceCvars[level.forceCvarCount].cvarValue,
			cvarValue,
			sizeof(level.forceCvars[0].cvarValue));

		level.forceCvarCount++;
			
		// tjw: prevent spamming the client with this twice 
		//      if the command is issued this early nobody
		//      should be connected yet.
		/*if((level.time - level.startTime) >  
			(FRAMETIME * GAME_INIT_FRAMES))  {

			trap_SendServerCommand(-1, 
				va("forcecvar \"%s\" \"%s\"",
				cvarName, cvarValue));
		}*/
		G_UpdateForceCvars();
		return qtrue;
	}

	if (!Q_stricmp(cmd, "sv_cvarempty"))
	{
		memset( level.svCvars, 0, sizeof(level.svCvars));
		level.svCvarsCount = 0;

		G_UpdateSvCvars();
		return qtrue;
	}

	if(!Q_stricmp(cmd, "sv_cvar")) {
		char cvarName[MAX_CVAR_VALUE_STRING];
		char mode[16];
		char cvarValue1[MAX_CVAR_VALUE_STRING];
		char cvarValue2[MAX_CVAR_VALUE_STRING];

		if(trap_Argc() <= 3) {
			G_Printf("usage: sv_cvar <cvar name> <mode> <value1> <value2>\n");
			return qtrue;
		}
		trap_Argv(1, cvarName, sizeof(cvarName));
		trap_Argv(2, mode, sizeof(mode));
		trap_Argv(3, cvarValue1, sizeof(cvarValue1));
			
		if(trap_Argc() == 5) 
			trap_Argv(4, cvarValue2, sizeof(cvarValue2));

		if(level.svCvarsCount >= MAX_SVCVARS) {
			G_Printf("sv_cvar: MAX_SVCVARS hit\n");
			return qtrue;
		}

		if ( !Q_stricmp( mode, "EQ") || !Q_stricmp( mode, "EQUAL") )
			level.svCvars[level.svCvarsCount].mode = SVC_EQUAL;
		else if ( !Q_stricmp( mode, "G") || !Q_stricmp( mode, "GREATER"))
			level.svCvars[level.svCvarsCount].mode = SVC_GREATER;
		else if ( !Q_stricmp( mode, "GE") || !Q_stricmp( mode, "GREATEREQUAL"))
			level.svCvars[level.svCvarsCount].mode = SVC_GREATEREQUAL;
		else if ( !Q_stricmp( mode, "L") || !Q_stricmp( mode, "LOWER"))
			level.svCvars[level.svCvarsCount].mode = SVC_LOWER;	
		else if ( !Q_stricmp( mode, "LE") || !Q_stricmp( mode, "LOWEREQUAL"))
			level.svCvars[level.svCvarsCount].mode = SVC_LOWEREQUAL;
		else if (  !Q_stricmp( mode, "IN") || !Q_stricmp( mode, "INSIDE"))
			level.svCvars[level.svCvarsCount].mode = SVC_INSIDE;	
		else if (  !Q_stricmp( mode, "OUT") || !Q_stricmp( mode, "OUTSIDE"))
			level.svCvars[level.svCvarsCount].mode = SVC_OUTSIDE;	
		else if ( !Q_stricmp( mode, "INCLUDE")  )
			level.svCvars[level.svCvarsCount].mode = SVC_INCLUDE;
		else if ( !Q_stricmp( mode, "EXCLUDE")  )
			level.svCvars[level.svCvarsCount].mode = SVC_EXCLUDE;
		else { 
			G_Printf("sv_cvar: invalid mode\n"); 
			return qtrue;
		}

		if ( trap_Argc() == 5 ){
			Q_strncpyz(level.svCvars[level.svCvarsCount].Val2, cvarValue2, sizeof(level.svCvars[0].Val2));
		} else {
			Q_strncpyz( level.svCvars[level.svCvarsCount].Val2, "", sizeof(level.svCvars[0].Val2));
		}

		Q_strncpyz(level.svCvars[level.svCvarsCount].cvarName,
			cvarName,
			sizeof(level.svCvars[0].cvarName));

		Q_strncpyz(level.svCvars[level.svCvarsCount].Val1,
			cvarValue1,
			sizeof(level.svCvars[0].Val1));

		level.svCvarsCount++;
			
		G_UpdateSvCvars();

		return qtrue;
	}

	if(!Q_stricmp(cmd, "playsound") || !Q_stricmp(cmd, "playsound_env")) {
		G_PlaySound_Cmd();
		return qtrue;
	}

	if(!Q_stricmp(cmd, "readxp")) {
		if(g_tyranny.integer) {
			G_xpsave_readconfig();
		}
		else {
			G_Printf("readxp: g_tyranny must be enabled\n");
		}
		return qtrue;
	}

	if(!Q_stricmp(cmd, "writexp")) {
		int i = 0;

		for( i = 0; i < level.numConnectedClients; i++ ) {
			G_xpsave_add(&g_entities[level.sortedClients[i]]);
		}
		G_xpsave_writeconfig();
		return qtrue;
	}

	if(!Q_stricmp(cmd, "csinfo")) {
		int i = 0;
		G_Printf("CS     Length\n-------------\n");
		for(i=0; i<MAX_CONFIGSTRINGS; i++) {
			if(!level.csLen[i])
				continue;
			G_Printf("%i %i\n", i, level.csLen[i]);
		}
		G_Printf("Total GAMESTATE Length: %i\n", level.csLenTotal);
		return qtrue;
	}

	if(!Q_stricmp(cmd, "csdump")) {
		char csnum_str[4];
		int csnum = 0;
		char cs[BIG_INFO_STRING];
	
		if(trap_Argc() != 2) {
			G_Printf("usage: csdump [csnum]\n");
			return qtrue;
		}
		trap_Argv(1, csnum_str, sizeof(csnum_str));
		csnum = atoi(csnum_str);
		if(csnum >= MAX_CONFIGSTRINGS || csnum < 0) {
			G_Printf("csdump: illegal csnum\n");
			return qtrue;
		}
		trap_GetConfigstring(csnum, cs, sizeof(cs));
		G_Printf("%s\n", cs);
		return qtrue;
	}// jet Pilot - Insanity helper functions

	if (!Q_stricmp(cmd, "insanity_add")) {
		Cmd_Insanity_Add_f();	
		return qtrue;
	}

	if (!Q_stricmp(cmd, "weapons_on")) {
		Cmd_Weapons_On_f();	
		return qtrue;
	}

	if (!Q_stricmp(cmd, "weapons_off")) {
		Cmd_Weapons_Off_f();
		return qtrue;
	}

	if (!Q_stricmp(cmd, "venom_on")) {
		Cmd_Venom_On_f();	
		return qtrue;
	}

	if (!Q_stricmp(cmd, "venom_off")) {
		Cmd_Venom_Off_f();
		return qtrue;
	}

	if (!Q_stricmp(cmd, "fireball_on")) {
		Cmd_Fireball_On_f();	
		return qtrue;
	}

	if (!Q_stricmp(cmd, "fireball_off")) {
		Cmd_Fireball_Off_f();
		return qtrue;
	}

	if (!Q_stricmp(cmd, "camera_on")) {
		Cmd_Camera_On_f();	
		return qtrue;
	}

	if (!Q_stricmp(cmd, "camera_off")) {
		Cmd_Camera_Off_f();
		return qtrue;
	}

	if (!Q_stricmp(cmd, "guided_on")) {
		Cmd_Guided_On_f();	
		return qtrue;
	}
	
	if (!Q_stricmp(cmd, "guided_off")) {
		Cmd_Guided_Off_f();
		return qtrue;
	}

	if (!Q_stricmp(cmd, "homing_on")) {
		Cmd_Homing_On_f();
		return qtrue;
	}	
	
	if (!Q_stricmp(cmd, "homing_off")) {
		Cmd_Homing_Off_f();
		return qtrue;
	}
	
	if (!Q_stricmp(cmd, "spawn_on")) {
		Cmd_Spawn_On_f();
		return qtrue;
	}
	
	if (!Q_stricmp(cmd, "spawn_off")) {
		Cmd_Spawn_Off_f();	
		return qtrue;
	}
	
	if (!Q_stricmp(cmd, "charge_on")) {
		Cmd_Charge_On_f();	
		return qtrue;
	}
	
	if (!Q_stricmp(cmd, "charge_off")) {
		Cmd_Charge_Off_f();
		return qtrue;
	}
	
	// see if this is a a shrubbot command
	// jaquboss, allow only dedicated?
//	if ( g_dedicated.integer ) {
		if(G_shrubbot_cmd_check(NULL)) 
			return qtrue;
//	}

	// everything else will also be printed as a say command
//		trap_SendServerCommand( -1, va("cpm \"server: %s\n\"", ConcatArgs(0) ) );

	// prints to the console instead now

	return qfalse;
}
