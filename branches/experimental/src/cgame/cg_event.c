/*
 * name:		cg_event.c
 *
 * desc:		handle entity events at snapshot or playerstate transitions
 *
 * NQQS:
 */
#include "cg_local.h"

extern void CG_StartShakeCamera( float param );
extern void CG_Tracer( vec3_t source, vec3_t dest, int sparks );
extern int bg_numGameSound;
//==========================================================================

// short killingspree messages for announcements..
const char *SpreeMsg[] = {
	"Killing Spree!",
	"Rampage",
	"Dominating",
	"Bloodbath",
	"Slaughterhouse",
	"Havoc",
	"God of War",
	"Prophet of Doom",
	"Shitstorm",
	"Shitstorm"
};

// see g_main.c multiKillName
const char *MultikillMsg[] = {
	"",
	"Multi kill",
	"Ultra kill",
	"Monster kill",
	"Mega kill",
	"Ludicrous kill",
};


void CG_AddPMItem2( popupMessageType_t type, const char* msg1, qhandle_t actionShader, const char* msg2, int actionShaderWScale, vec3_t color);

/*
=============
CG_CheckFilterObituary

	redeye - this handles only obituary messages
=============
*/
static qboolean CG_CheckFilterObituary(int attacker, clientInfo_t *ca, int target)
{
	qboolean filter = qfalse;

	if (attacker != target) {
		if ((cg_popupMessageFilter.integer & POPUP_FILTER_ENEMYKILLS)
				&& ca
				&& (ca->team != cg.snap->ps.persistant[PERS_TEAM] || ca->team == TEAM_SPECTATOR)) {
			filter = qtrue;
		}
		else if ((cg_popupMessageFilter.integer & POPUP_FILTER_TEAMKILLS) &&
				ca && ca->team == cg.snap->ps.persistant[PERS_TEAM]) {
			filter = qtrue;
		}
	}
	if ((attacker == target || attacker == ENTITYNUM_WORLD) &&
			(cg_popupMessageFilter.integer & POPUP_FILTER_SELFKILLS)) {
		filter = qtrue;
	}
	if ((attacker == cg.clientNum || target == cg.clientNum)) {
		filter = (cg_popupMessageFilter.integer & POPUP_FILTER_OWNKILLS)? qtrue : qfalse;
	}
	return filter;
}

vec3_t OB_YELLOW =	{1.f,1.f,0.f};
vec3_t OB_RED	 =	{1.f,0.f,0.f};

/*
=============
CG_Obituary
=============
*/
extern void C_ResetPlayerPath( int clientNum );

static void CG_Obituary( entityState_t *ent ) {
	int				mod					= ent->eventParm;
	int				target				= ent->otherEntityNum;
	int 			attacker			= ent->otherEntityNum2;
	char			*message			= NULL;
	char			*message2			= NULL;
	char			*tkString			= "";
	char			targetName[32];
	char			attackerName[32];
	clientInfo_t	*ci;
	clientInfo_t	*ca; // JPW NERVE ca = attacker
	qhandle_t		deathShader			= cgs.media.pmImages[PM_DEATH];
	qhandle_t		icon				= cgs.media.pmImages[PM_DEATH];
	weapon_t		weapon				= ent->weapon;
	int				flags				= ent->dmgFlags;
	vec3_t			color				= {1.f,1.f,1.f};
	qboolean		overrideIcon		= qfalse;
	qboolean		filter				= qfalse;
	qboolean		console				= ((cg_popupMessageFilter.integer & POPUP_FILTER_CONSOLEKILLS) == 0)? qtrue : qfalse;



	if ( target < 0 || target >= MAX_CLIENTS ) {
		CG_Error( "CG_Obituary: target out of range" );
	}

	//--------------------------
	// core: smoother player-icons movement on the commandmap..
	// used in cg_commandmap.c
	C_ResetPlayerPath( target );
	//--------------------------

	ci = &cgs.clientinfo[target];

	if ( attacker < 0 || attacker >= MAX_CLIENTS ) {
		attacker = ENTITYNUM_WORLD;
		ca		 = NULL;
	}
	else {
		ca		 = &cgs.clientinfo[attacker];
	}

	Q_strncpyz( targetName, ci->name, sizeof(targetName) - 2);
	strcat( targetName, S_COLOR_WHITE );

	filter = CG_CheckFilterObituary(attacker, ca, target);

	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		weapon = WP_NONE;
	}

	// check for single client messages
	if ( !ca ) {
		// core: don't even bother to makeup a message if filter is active..
		if ( filter ) return;

		switch( mod ) {
			case MOD_CENSORED:
				overrideIcon = qtrue;
				message = "has angered the censorship gods.";
				break;
			case MOD_SUICIDE:
				overrideIcon = qtrue;
				message = "committed suicide.";
				break;
			case MOD_FALLING:
				overrideIcon = qtrue;
				icon = cgs.media.fallObituary;
				message = "rediscovered gravity.";
				break;
			case MOD_CRUSH:
				overrideIcon = qtrue;
				icon = cgs.media.crushObituary;
				message = "was crushed.";
				break;
			case MOD_WATER:
				icon = cgs.media.drownObituary;
				message = "drowned.";
				break;
			case MOD_SLIME:
				overrideIcon = qtrue;
				icon = cgs.media.drownObituary;
				message = "died by toxic materials.";
				break;
			case MOD_TRIGGER_HURT:
				overrideIcon = qtrue;
				message = "was mortally wounded.";
				break;
			case MOD_TELEFRAG:
				overrideIcon = qtrue;
				message = "was telefragged.";
				break;
			case MOD_TARGET_LASER:
				overrideIcon = qtrue;
				message = "was target of too strong beam.";
				break;
			case MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER:
				overrideIcon = qtrue;
				icon = cg_weapons[WP_PLIERS].weaponIcon[1];
				message = "stood in the wrong place at the wrong time.";
				break;
			case MOD_LAVA:
				overrideIcon = qtrue;
				message = "tried to swim in lava.";
				break;
			case MOD_MAPMORTAR:
			case MOD_MAPMORTAR_SPLASH:
				message = "was killed by mortar support.";
				break;
			default:
				message = NULL;
				break;
		}
	}
	// check for suicides (non /kill)
	else if( attacker == target ) {
		// core: don't even bother to makeup a message if filter is active..
		if ( filter ) return;

		switch (mod) {
			case MOD_DYNAMITE:
				message = "dynamited himself to pieces.";
				break;
			case MOD_GRENADE_LAUNCHER:
				message = "dove on his own grenade.";
				break;
			case MOD_GRENADE_PINEAPPLE: // rain - added PINEAPPLE
				message = "tried to put the pin back in.";
				break;
			case MOD_PANZERFAUST:
				message = "panzernoobed himself.";
				break;
			case MOD_BAZOOKA:
				message = "bazooka'd himself.";
				break;
			case MOD_FLAMETHROWER: // rain
				message = "tried to tame fire.";
				break;
			case MOD_AIRSTRIKE:
				overrideIcon = qtrue;
				icon =	cgs.media.airstrikeObituary;
				message = "obliterated himself.";
				break;
			case MOD_ARTY:
				overrideIcon = qtrue;
				icon =	cgs.media.artyObituary;
				message = "fired-for-effect on himself.";
				break;
			case MOD_EXPLOSIVE:
				overrideIcon = qtrue;
				message = "died in his own explosion.";
				break;
			// rain - everything from this point on is sorted by MOD, didn't
			// resort existing messages to avoid differences between pre
			// and post-patch code (for source patching)
			case MOD_GPG40:
			case MOD_M7: // rain
				//bani - more amusing, less wordy
				message = "ate his own rifle grenade.";
				break;
			case MOD_LANDMINE: // rain
				//bani - slightly more amusing
				message = "forgot where he planted his landmine.";
				break;
			case MOD_SATCHEL: // rain
				message = "embraced his own satchel explosion.";
				break;
			case MOD_CRUSH_CONSTRUCTION: // rain
				overrideIcon = qtrue;
				icon =	cg_weapons[WP_PLIERS].weaponIcon[1];
				message = "engineered himself into oblivion.";
				break;
			case MOD_CRUSH_CONSTRUCTIONDEATH: // rain
				overrideIcon = qtrue;
				icon =	cg_weapons[WP_PLIERS].weaponIcon[1];
				message = "buried himself alive.";
				break;
			case MOD_MORTAR: // rain
				message = "never saw his own mortar round coming.";
				break;
			case MOD_SMOKEGRENADE: // rain
				// bani - more amusing
				message = "danced on his airstrike marker.";
				break;
			case MOD_POISON: // Meyer - for when !poison'd
				overrideIcon = qtrue;
				icon =	cgs.media.poisonObituary;
				message = "tasted his own poison.";
				break;

			// no obituary message if changing teams
			case MOD_SWITCHTEAM:
				return;
// jaquboss,
// added hopefuly all bullet weapons ( except fixed ones, which doesnt share code anyway ),
// may happen in 2.55 when crouching and shooting at ground, or in all ET versions when just shooting metal surface
			case MOD_MG42:
			case MOD_STEN_MKII:
			case MOD_STG44:
			case MOD_MP34:
			case MOD_BAR:
			case MOD_SHOTGUN:
			case MOD_BROWNING:
			case MOD_THOMPSON:
			case MOD_MP40:
			case MOD_LUGER:
			case MOD_COLT:
			case MOD_STEN:
			case MOD_SILENCER:
			case MOD_SILENCED_COLT:
			case MOD_AKIMBO_COLT:
			case MOD_AKIMBO_LUGER:
			case MOD_AKIMBO_SILENCEDCOLT:
			case MOD_AKIMBO_SILENCEDLUGER:
			case MOD_KAR98:
			case MOD_CARBINE:
			case MOD_GARAND:
			case MOD_GARAND_SCOPE:
			case MOD_K43:
			case MOD_K43_SCOPE:
			case MOD_FG42:
			case MOD_FG42_SCOPE:
			case MOD_VENOM:
				message = "found a way to shoot himself!";
				break;
			case MOD_THROWKNIFE:
				message = "played with knives!";
				break;
			default:
				overrideIcon = qtrue;
				message = "killed himself.";
				break;
		}
	}

	// overwrite dirty from weapfile if exists and is not WP_NONE
	if ( message && weapon != WP_NONE && BG_Weapons[weapon].selfKillMessage[0]) {
		message = (char *)&BG_Weapons[weapon].selfKillMessage;
	}

	// if we've got a message assigned, deal with it and we're done
	if ( message ) {
		if (cg_graphicObituaries.integer) {
			int scale=1;

			if ( weapon != WP_NONE && !overrideIcon ){
				if ( cg_drawSmallPopupIcons.integer && cg_weapons[ weapon ].weaponIcon[0] ){
					icon = cg_weapons[ weapon ].weaponIcon[0];
					scale = weaponIconScale(weapon);
				}
				else if ( cg_weapons[ weapon ].weaponIcon[1] )  {
					icon = cg_weapons[ weapon ].weaponIcon[1];
					scale = weaponIconScale(weapon);
				}
			}
			CG_AddPMItem2(PM_DEATH, targetName, icon, " ", scale, OB_YELLOW );
			if (console) {
				trap_Print( va( "%s %s\n", targetName, CG_TranslateString(message) ) );
			}
			return;
		}
		else {
			message = CG_TranslateString( message );
			CG_AddPMItem( PM_DEATH, va( "%s^7 %s", targetName, message ), deathShader, OB_YELLOW );
			if (console) {
				trap_Print( va("%s %s\n", targetName, CG_TranslateString(message)) );
			}
			return;
		}
	}


	// check for kill messages from the current clientNum
	if( attacker == cg.snap->ps.clientNum ) {
		char	*s;
		qboolean		ownkill_centerprint	= qtrue;

		if ( ci->team == ca->team ) {
			if (mod == MOD_SWAP_PLACES) {
				if ( cg.snap->ps.clientNum == cg.clientNum ) {
					s = va("%s %s", CG_TranslateString("You swapped places with"), targetName );
				} else {
					s = va("%s ^7%s %s", cgs.clientinfo[attacker].name, CG_TranslateString("swapped places with"), targetName );
				}
			}
			else {
				if ( cg.snap->ps.clientNum == cg.clientNum ) {
					s = va("%s %s", CG_TranslateString("You killed ^1TEAMMATE^7"), targetName );
				} else {
					s = va("%s ^7%s %s", cgs.clientinfo[attacker].name, CG_TranslateString("killed ^1TEAMMATE^7"), targetName );
				}
			}
		}
		else {
			if ( cg.snap->ps.clientNum == cg.clientNum ) {
				s = va("%s %s", CG_TranslateString("You killed"), targetName );
			} else {
				s = va("%s ^7%s %s", cgs.clientinfo[attacker].name, CG_TranslateString("killed"), targetName );
			}

			// core: own kills name announces..
			if ( cg_killAnnouncer.integer & (KILLANNOUNCE_OWN | KILLANNOUNCE_OWN_NO_CP) ) {
				clientInfo_t ci = cgs.clientinfo[attacker];
				if ( ci.infoValid ) {
					CG_AddAnnouncer( va("^7%s",ci.name), "kill", 0, 0.5, 4000, 0.75,0.5,0, ANNOUNCER_TOP );
					if ( cg_killAnnouncer.integer & KILLANNOUNCE_OWN_NO_CP ) {
						ownkill_centerprint = qfalse;
					}
				}
			}
		}
		// print the text message
		if ( ownkill_centerprint ) {
			CG_PriorityCenterPrint( s, SCREEN_HEIGHT * 0.75, BIGCHAR_WIDTH * 0.6, 1 );
		}
	}

	// check for double client messages
	if ( !ca ) {
		strcpy( attackerName, "noname" );
	}
	else {
		Q_strncpyz( attackerName, ca->name, sizeof(attackerName) - 2);
		strcat( attackerName, S_COLOR_WHITE );

		// check for kill messages about the current clientNum
		if ( target == cg.snap->ps.clientNum ) {
			Q_strncpyz( cg.killerName, attackerName, sizeof( cg.killerName ) );
		}
	}

	if( ca ) {

		//----------------------------------------------------------------------------
		// core: Killer Cam (like the mortarcam in a sperate overlay window)..
		if (  target == cg.snap->ps.clientNum && attacker != cg.snap->ps.clientNum ) {
			// set the killer..
			cg.latestKiller = &cg_entities[attacker];
			cg.killerCamTime = cg.time + 5000;
		}
		//----------------------------------------------------------------------------

		//core: make sounds, even when a (text)filter is active..
		if ( attacker == cg.snap->ps.clientNum || target == cg.snap->ps.clientNum ) {
			switch( mod ) {
				case MOD_KNIFE:
					// OSP - goat luvin
					if( cg_goatSound.integer & SOUND_KNIFE_FRONT ) {
						trap_S_StartSound( cg.snap->ps.origin, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.knifeKill );
					}
					break;

				case MOD_BACKSTAB:
					if( cg_goatSound.integer & SOUND_KNIFE_BACK ) {
						trap_S_StartSound( cg.snap->ps.origin, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.knifeKill );
					}
					break;

				case MOD_THROWKNIFE:
					if( cg_goatSound.integer & SOUND_KNIFE_THROW ) {
						trap_S_StartSound( cg.snap->ps.origin, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.knifeKill );
					}
					break;

				case MOD_GOOMBA:
					// josh - Goomba sound
					trap_S_StartSound( cg.snap->ps.origin, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.goombaSound ); //knifeKill );
					break;
			}
		}
		if ( filter ) return;

		switch( mod ) {
		case MOD_THROWKNIFE:
			message = "was impaled by";
			message2 = "'s throwing knife.";
			break;

		case MOD_KNIFE:
			message = "was stabbed by";
			message2 = "'s knife.";
			break;

		case MOD_BACKSTAB:
			overrideIcon = qtrue;
			icon = cg_weapons[WP_KNIFE].weaponIcon[1];
			message = "was backstabbed by";
			message2 = "'s knife.";
			break;

		case MOD_GOOMBA:					// josh
			overrideIcon = qtrue;
			icon =	cgs.media.goombaObituary;
			message  = "experienced death from above by";
			message2 = ".";
			break;

		case MOD_AKIMBO_COLT:
			message  = "was gunned down by";
			message2 = "'s Akimbo .45ACP 1911s.";
			break;

		case MOD_AKIMBO_SILENCEDCOLT:
			message  = "was silently gunned down by";
			message2 = "'s Akimbo .45ACP 1911s.";
			break;

		case MOD_AKIMBO_LUGER:
			message  = "was gunned down by";
			message2 = "'s Akimbo Luger 9mms.";
			break;

		case MOD_AKIMBO_SILENCEDLUGER:
			message  = "was silently gunned down by";
			message2 = "'s Akimbo Luger 9mms.";
			break;

		case MOD_SILENCER:
			message  = "was silently killed by";
			message2 = "'s Luger.";
			break;

		case MOD_LUGER:
			message  = "was killed by";
			message2 = "'s Luger 9mm.";
			break;

		case MOD_SILENCED_COLT:
			message  = "was silently killed by";
			message2 = "'s Colt.";
			break;

		case MOD_COLT:
			message  = "was killed by";
			message2 = "'s .45ACP 1911.";
			break;

		case MOD_MP40:
			message  = "was killed by";
			message2 = "'s MP40.";
			break;

		case MOD_THOMPSON:
			message  = "was killed by";
			message2 = "'s Thompson.";
			break;

		case MOD_STEN:
			message  = "was killed by";
			message2 = "'s Sten.";
			break;

		case MOD_STEN_MKII:
			message  = "was cut up by";
			message2 = "'s Sten Mk II.";
			break;

		case MOD_VENOM:
			message = "was ventilated";
			message2 = "'s Venom.";
			break;

		case MOD_STG44:
			message  = "was shredded by";
			message2 = "'s StG44.";
			break;

		case MOD_BAR:
			message  = "was gunned down by";
			message2 = "'s BAR.";
			break;

		case MOD_DYNAMITE:
			message  = "was detonated by";
			message2 = "'s dynamite.";
			break;

		case MOD_PANZERFAUST:
			message  = "was blasted by";
			message2 = "'s Panzerfaust.";
			break;

		case MOD_GRENADE_LAUNCHER:			// German 'stick' grenade
			message  = "was exploded by";
			message2 = "'s grenade.";
			break;

		case MOD_GRENADE_PINEAPPLE:			// Allied fragmentation grenade
			message  = "was exploded by";
			message2 = "'s grenade.";
			break;

		case MOD_FLAMETHROWER:
			message  = "was incinerated by";
			message2 = "'s flamethrower.";
			break;

		case MOD_MORTAR:
			message  = "never saw";
			message2 = "'s mortar round coming.";
			break;

		case MOD_MACHINEGUN:
			overrideIcon = qtrue;
			icon =	cg_weapons[WP_MOBILE_MG42].weaponIcon[1];
			message  = "was perforated by";
			message2 = "'s crew-served MG.";
			break;

		case MOD_BROWNING:
			overrideIcon = qtrue;
			icon =	cg_weapons[WP_MOBILE_BROWNING].weaponIcon[1];
			message  = "was perforated by";
			message2 = "'s tank-mounted Browning .30 Cal.";
			break;

		case MOD_MG42:
			overrideIcon = qtrue;
			icon =	cg_weapons[WP_MOBILE_MG42].weaponIcon[1];
			message  = "was perforated by";
			message2 = "'s tank-mounted MG42.";
			break;

		case MOD_AIRSTRIKE:
			overrideIcon = qtrue;
			icon =	cgs.media.airstrikeObituary;
			message  = "was blasted by";
			message2 = "'s support fire.";
			break;

		case MOD_ARTY:
			overrideIcon = qtrue;
			icon =	cgs.media.artyObituary;
			message  = "was shelled by";
			message2 = "'s artillery support.";
			break;

		case MOD_SWAP_PLACES:
			overrideIcon = qtrue;
			message  = "^2swapped places with^7";
			message2 = "";
			break;

		case MOD_KAR98:						// Engineer's Karabiner Kar98k
		case MOD_K43:						// Covert Op's Mauser K43
			message  = "was killed by";
			message2 = "'s K43.";
			break;

		case MOD_CARBINE:					// Engineer's Carbine
		case MOD_GARAND:					// Covert's Scoped Garand
			message  = "was killed by";
			message2 = "'s M1 Garand.";
			break;

		case MOD_GPG40:						// allied grenade launcher
		case MOD_M7:						// axis grenade launcher
			message  = "was blasted by";
			message2 = "'s rifle grenade.";
			break;

		case MOD_LANDMINE:
			message  = "failed to spot";
			message2 = "'s Landmine.";
			break;

		case MOD_CRUSH_CONSTRUCTION:
			overrideIcon = qtrue;
			icon =	cg_weapons[WP_PLIERS].weaponIcon[1];
			message  = "got caught in";
			message2 = "'s construction madness.";
			break;

		case MOD_CRUSH_CONSTRUCTIONDEATH:
			overrideIcon = qtrue;
			icon =	cg_weapons[WP_PLIERS].weaponIcon[1];
			message  = "got buried under";
			message2 = "'s rubble.";
			break;

		case MOD_MOBILE_MG42:				// Axis mobile Machine Gun
			message  = "was mown down by";
			message2 = "'s Mobile MG42.";
			break;

		case MOD_GARAND_SCOPE:				// Allied Covert Op's Garand, through scope
			message  = "was silenced by";
			message2 = "'s M1 Garand.";
			break;

		case MOD_K43_SCOPE:					// Allied Covert Op's K43, through scope
			message  = "was silenced by";
			message2 = "'s K43.";
			break;

		case MOD_FG42:						// Axis FG42 Paratrooper Rifle
			message  = "was killed by";
			message2 = "'s FG42.";
			break;

		case MOD_FG42_SCOPE:					// Axis FG42 Paratrooper Rifle, through scope
			message  = "was sniped by";
			message2 = "'s FG42.";
			break;

		case MOD_SATCHEL:
			message  = "was blasted by";
			message2 = "'s Satchel Charge.";
			break;

		case MOD_SMOKEGRENADE:
			message  = "stood on";
			message2 = "'s airstrike marker.";
			break;

		case MOD_POISON:					// josh
			overrideIcon = qtrue;
			icon =	cgs.media.poisonObituary;
			message	 = "was lethally injected by";
			message2 = "'s poison needle.";
			break;

		case MOD_FEAR:
			overrideIcon = qtrue;
			message  = "wimped out of a fight with";
			message2 = ".";
			break;

		case MOD_SHOTGUN:
			message  = "was blown away by";
			message2 = "'s Winchester M97.";
			break;

		case MOD_MP34:
			message  = "died in a hail of lead from";
			message2 = "'s MP34.";
			break;

		case MOD_BAZOOKA:
			message  = "was blown to smithereens by";
			message2 = "'s Bazooka.";
			break;

		case MOD_MOBILE_BROWNING:			// Allied mobile Machine Gun
			message	 = "was cut to ribbons by";
			message2 = "'s mobile Browning .30 Cal.";
			break;

		case MOD_KICKED:
			overrideIcon = qtrue;
			message  = "got his ass kicked by";
			message2 = ".";
			break;

		case MOD_SHOVE:
			overrideIcon = qtrue;
			icon =	cgs.media.shoveObituary;
			message  = "was thrown to his doom by";
			message2 = ".";
			break;

		default:
			overrideIcon = qtrue;
			message  = "was killed by";
			message2 = ".";
			break;
		}


		// overwrite dirty from weapfile if exists
		if (weapon != WP_NONE && mod != MOD_THROWKNIFE ) {
			if ( message && BG_Weapons[weapon].KillMessage[0] && BG_Weapons[weapon].KillMessage2[0] ) {
				// change the two message parts at the same time,
				// we do not want half a message taken from the .weap file, and the other half from code defaults..
				message = (char *)&BG_Weapons[weapon].KillMessage;
				message2 = (char *)&BG_Weapons[weapon].KillMessage2;
			}
		}

		// special cases again
		if ( flags & OBIT_EXECUTED ) {
			message   =  "was executed by";
			message2  =  ".";
		}
		// special cases end

		if( ci->team == ca->team && mod != MOD_SWAP_PLACES) { // jaquboss - not for swapping places
			tkString = "^1TEAM KILL:^7 ";
			VectorCopy( OB_RED, color);
		}

		if ( message ) {
			if (cg_graphicObituaries.integer) {
				int scale=1;

				if ( weapon != WP_NONE && !overrideIcon ){
					if ( cg_drawSmallPopupIcons.integer && cg_weapons[ weapon ].weaponIcon[0] ){
						icon = cg_weapons[ weapon ].weaponIcon[0];
						scale = weaponIconScale(weapon);
					}
					else if ( cg_weapons[ weapon ].weaponIcon[1] )  {
						icon = cg_weapons[ weapon ].weaponIcon[1];
						scale = weaponIconScale(weapon);
					}
				}
				if ( cg_graphicObituaries.integer == 1 ) {
					CG_AddPMItem2(PM_DEATH, attackerName, icon, targetName, scale, color);
				}
				else {
					CG_AddPMItem2(PM_DEATH, targetName, icon, attackerName, scale, color);
				}
				if (console) {
					// jaquboss - output it in console
					trap_Print( va( "%s %s %s%s\n", targetName, CG_TranslateString(message), attackerName, CG_TranslateString(message2) ) );
				}
				return; // jaquboss don't say killed himselfs
			}
			else {
				message = CG_TranslateString( message );

				if ( message2 ) {
					message2 = CG_TranslateString( message2 );
					CG_AddPMItem( PM_DEATH, va( "%s%s^7 %s %s^7%s", tkString, targetName, message, attackerName, message2 ), deathShader, color );
					if (console) {
						trap_Print( va("%s%s %s %s%s\n", tkString, targetName, message, attackerName, message2) );
					}
				}
				return;
			}
		}

	}

	// we don't know what it was
	if ( !filter ) {
		CG_AddPMItem( PM_DEATH, va( "%s^7 died.", targetName ), deathShader, OB_YELLOW );
		if (console) {
			trap_Print( va("%s died.\n", targetName) );
		}
	}
}
//==========================================================================

// from cg_weapons.c
extern int CG_WeaponIndex( int weapnum, int *bank, int *cycle);


/*
================
CG_ItemPickup

A new item was picked up this frame
================
*/
static void CG_ItemPickup( int itemNum ) {
	int itemid = bg_itemlist[itemNum].giTag;
	qhandle_t shader;
	popupMessageType_t msgType;

	// Jaybird
	// crapshoot: added IT_WEAPON case and set msgType here instead of shader
	switch (bg_itemlist[itemNum].giType) {
	case IT_AMMO:
		msgType = PM_AMMOPICKUP;
		break;
	case IT_WEAPON:
		if ( itemid == WP_AMMO )
			msgType = PM_AMMOPICKUP;
		else
			msgType = PM_MESSAGE;
		break;
	case IT_HEALTH:
		msgType = PM_HEALTHPICKUP;
		break;
	case IT_TEAM: // crapshoot bug #109
		msgType = PM_OBJECTIVE;
		break;
	default:
		msgType = PM_MESSAGE;
		break;
	}

	shader = cgs.media.pmImages[msgType];
	CG_AddPMItem( msgType, va( "Picked up %s", CG_PickupItemText( itemNum ) ), shader, NULL );

	// see if it should be the grabbed weapon
	if ( bg_itemlist[itemNum].giType == IT_WEAPON ) {

 		if ( cg_autoswitch.integer && cg.predictedPlayerState.weaponstate != WEAPON_RELOADING ) {

		//	0 - "Off"
		//	1 - "Always Switch"
		//	2 - "If New"
		//	3 - "If Better"
		//	4 - "New or Better"

			// don't ever autoswitch to secondary fire weapons
			// Gordon: Leave autoswitch to secondary kar/carbine as they use alt ammo and arent zoomed: Note, not that it would do this anyway as it isnt in a bank....
			if( itemid != WP_FG42SCOPE && itemid != WP_GARAND_SCOPE && itemid != WP_K43_SCOPE && itemid != WP_AMMO) {
				//----(SA)	modified

			// no weap currently selected, always just select the new one
				if(!cg.weaponSelect) {
					cg.weaponSelectTime	= cg.time;
					cg.weaponSelect		= itemid;
				}

		// 1 - always switch to new weap (Q3A default)
				else if(cg_autoswitch.integer == 1) {
					cg.weaponSelectTime	= cg.time;
					cg.weaponSelect		= itemid;
				}
				else {

		// 2 - switch to weap if it's not already in the player's inventory (Wolf default)
		// 4 - both 2 and 3

					// FIXME:	this works fine for predicted pickups (when you walk over the weapon), but not for
					//			manual pickups (activate item)
					if (cg_autoswitch.integer == 2 || cg_autoswitch.integer == 4) {
						if(!COM_BitCheck( cg.snap->ps.weapons, itemid )) {
							cg.weaponSelectTime	= cg.time;
							cg.weaponSelect		= itemid;
						}
					}	// end 2

		// 3 - switch to weap if it's in a bank greater than the current weap
		// 4 - both 2 and 3
					if (cg_autoswitch.integer == 3 || cg_autoswitch.integer == 4) {
						int wpbank_cur, wpbank_pickup;

						// switch away only if a primary weapon is selected (read: don't switch away if current weap is a secondary mode)
						if( CG_WeaponIndex(cg.weaponSelect, &wpbank_cur, NULL) ) {
							if(CG_WeaponIndex(itemid, &wpbank_pickup, NULL)) {
								if( wpbank_pickup > wpbank_cur ) {
									cg.weaponSelectTime	= cg.time;
									cg.weaponSelect		= itemid;
								}
							}
						}
					}	// end 3

				}	// end cg_autoswitch.integer != 1

			}

		}	// end cg_autoswitch.integer
	}	// end bg_itemlist[itemNum].giType == IT_WEAPON
}

/*
================
CG_PainEvent

Also called by playerstate transition
================
*/

/*never used
typedef struct {
	char *tag;
	int refEntOfs;
	int anim;
} painAnimForTag_t;
*/

void CG_PainEvent( centity_t *cent, int health, qboolean crouching ) {

	// don't do more than two pain sounds a second
	if ( cg.time - cent->pe.painTime < 500 ) {
		return;
	}

	// save pain time for programitic twitch animation
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}

typedef struct fxSound_s {
	int max;
	qhandle_t sound[3];
	const char *soundfile[3];
} fxSound_t;

static fxSound_t fxSounds[FXTYPE_MAX] = {
	// wood
	{ 1, { -1, -1, -1 }, { "sound/world/boardbreak.wav", NULL, NULL } },
	// glass
	{ 3, { -1, -1, -1 }, { "sound/world/glassbreak1.wav", "sound/world/glassbreak2.wav", "sound/world/glassbreak3.wav" } },
	// metal
	{ 1, { -1, -1, -1 }, { "sound/world/metalbreak.wav", NULL, NULL } },
	// gibs
	{ 1, { -1, -1, -1 }, { "sound/player/gib.wav", NULL, NULL } },
	// brick
	{ 1, { -1, -1, -1 }, { "sound/world/debris1.wav", NULL, NULL } },
	// stone
	{ 1, { -1, -1, -1 }, { "sound/world/stonefall.wav", NULL, NULL } },
	// fabric
	{ 1, { -1, -1, -1 }, { "sound/world/fabricbreak.wav", NULL, NULL } }
};

void CG_PrecacheFXSounds( void ) {
	int i, j;

	for( i = FXTYPE_WOOD; i < FXTYPE_MAX; ++i ) {
		for( j = 0; j < fxSounds[i].max; ++j ) {
			fxSounds[i].sound[j] = trap_S_RegisterSound( fxSounds[i].soundfile[j], qfalse );
		}
	}
}

void CG_Explodef(vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader);
void CG_RubbleFx(vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader, float speedscale, float sizescale);

/*
==============
CG_Explode
	the old cent-based explode calls will still work with this pass-through
==============
*/
void CG_Explode(centity_t *cent, vec3_t origin, vec3_t dir, qhandle_t shader) {

	// inherit shader
	// (SA) FIXME: do this at spawn time rather than explode time so any new necessary shaders are created earlier
	if(cent->currentState.eFlags & EF_INHERITSHADER) {
		if(!shader) {
			qhandle_t inheritmodel = cgs.inlineDrawModel[cent->currentState.modelindex];	// okay, this should be better.
			if(inheritmodel)
				shader = trap_R_GetShaderFromModel(inheritmodel, 0, 0);
		}
	}

	if( !cent->currentState.dl_intensity ) {
		sfxHandle_t sound;
		int			index = cent->currentState.frame;

		if ( index < FXTYPE_WOOD || index >= FXTYPE_MAX ) {
			index = FXTYPE_WOOD;
		}

		sound = random()*fxSounds[index].max;

		if( fxSounds[index].sound[sound] == -1 ) {
			fxSounds[index].sound[sound] = trap_S_RegisterSound( fxSounds[index].soundfile[sound], qfalse );
		}

		sound = fxSounds[index].sound[sound];

		CG_Explodef(	origin,
						dir,
						cent->currentState.density,			// mass
						index,								// type
						sound,								// sound
						cent->currentState.weapon,			// forceLowGrav
						shader
						);
	}
	else {
		sfxHandle_t sound;

		if( cent->currentState.dl_intensity == -1 ) {
			sound = 0;
		}
		else {
			sound = CG_GetGameSound(cent->currentState.dl_intensity);
		}

		CG_Explodef(	origin,
						dir,
						cent->currentState.density,			// mass
						cent->currentState.frame,			// type
						sound,								// sound
						cent->currentState.weapon,			// forceLowGrav
						shader
						);
	}

}

/*
==============
CG_Explode
	the old cent-based explode calls will still work with this pass-through
==============
*/
void CG_Rubble(centity_t *cent, vec3_t origin, vec3_t dir, qhandle_t shader) {

	// inherit shader
	// (SA) FIXME: do this at spawn time rather than explode time so any new necessary shaders are created earlier
	if(cent->currentState.eFlags & EF_INHERITSHADER) {
		if(!shader) {
			qhandle_t inheritmodel = cgs.inlineDrawModel[cent->currentState.modelindex];	// okay, this should be better.
			if(inheritmodel)
				shader = trap_R_GetShaderFromModel(inheritmodel, 0, 0);
		}
	}

	if( !cent->currentState.dl_intensity ) {
		sfxHandle_t sound;
		int			index= cent->currentState.frame;

		if ( index < FXTYPE_WOOD || index >= FXTYPE_MAX ) {
			index = FXTYPE_WOOD;
		}

		sound = random()*fxSounds[index].max;

		if( fxSounds[index].sound[sound] == -1 ) {
			fxSounds[index].sound[sound] = trap_S_RegisterSound( fxSounds[index].soundfile[sound], qfalse );
		}

		sound = fxSounds[index].sound[sound];

		CG_RubbleFx(	origin,
						dir,
						cent->currentState.density,			// mass
						index,								// type
						sound,								// sound
						cent->currentState.weapon,			// forceLowGrav
						shader,
						cent->currentState.angles2[0],
						cent->currentState.angles2[1]
						);
	}
	else {
		sfxHandle_t sound;

		if( cent->currentState.dl_intensity == -1 ) {
			sound = 0;
		}
		else {
			sound = CG_GetGameSound(cent->currentState.dl_intensity);
		}

		CG_RubbleFx(	origin,
						dir,
						cent->currentState.density,			// mass
						cent->currentState.frame,			// type
						sound,								// sound
						cent->currentState.weapon,			// forceLowGrav
						shader,
						cent->currentState.angles2[0],
						cent->currentState.angles2[1]
						);
	}
}

/*
==============
CG_RubbleFx
==============
*/
void CG_RubbleFx(vec3_t origin, vec3_t dir, int mass, int type, sfxHandle_t sound, int forceLowGrav, qhandle_t shader, float speedscale, float sizescale) {
	int i;
	localEntity_t	*le;
	refEntity_t		*re;
	int				howmany, total, totalsounds = 0;
	int				pieces[6];	// how many of each piece
	qhandle_t		modelshader = 0;
	float			materialmul = 1;	// multiplier for different types
	leBounceSoundType_t snd;
	int				hmodel;
	float			scale;
	int				endtime;

	memset(&pieces, 0, sizeof(pieces));

	pieces[5]	= (int)(mass / 250.0f);
	pieces[4]	= (int)(mass / 76.0f);
	pieces[3]	= (int)(mass / 37.0f);	// so 2 per 75
	pieces[2]	= (int)(mass / 15.0f);
	pieces[1]	= (int)(mass / 10.0f);
	pieces[0]	= (int)(mass / 5.0f);

	if(pieces[0] > 20)	pieces[0] = 20;	// cap some of the smaller bits so they don't get out of control
	if(pieces[1] > 15)	pieces[1] = 15;
	if(pieces[2] > 10)	pieces[2] = 10;

	if(type == FXTYPE_WOOD ) {	// cap wood even more since it's often grouped, and the small splinters can add up
		if(pieces[0] > 10)	pieces[0] = 10;
		if(pieces[1] > 10)	pieces[1] = 10;
		if(pieces[2] > 10)	pieces[2] = 10;
	}

	total = pieces[5] + pieces[4] + pieces[3] + pieces[2] + pieces[1] + pieces[0];

	if(sound) {
		trap_S_StartSound( origin, -1, CHAN_AUTO, sound);
	}

	if(shader) {	// shader passed in to use
		modelshader = shader;
	}

	for(i=0;i<6;++i) {
		snd = LEBS_NONE;
		hmodel = 0;

		for(howmany = 0; howmany < pieces[i]; ++howmany) {

			scale = 1.0f;
			endtime = 0;	// set endtime offset for faster/slower fadeouts

			switch(type) {
			case FXTYPE_WOOD:	// "wood"
				snd = LEBS_WOOD;
				hmodel = cgs.media.debWood[i];

				if(i==0)		scale = 0.5f;
				else if(i==1)	scale = 0.6f;
				else if(i==2)	scale = 0.7f;
				else if(i==3)	scale = 0.5f;
				//					else goto pass;

				if(i<3)
					endtime = -3000;	// small bits live 3 sec shorter than normal
				break;

			case FXTYPE_GLASS:	// "glass"
				snd = LEBS_NONE;
				if(i==5)			hmodel = cgs.media.shardGlass1;
				else if(i==4)		hmodel = cgs.media.shardGlass2;
				else if(i==2)		hmodel = cgs.media.shardGlass2;
				else if(i==1) {
					hmodel = cgs.media.shardGlass2;
					scale = 0.5f;
				}
				else	goto pass;
				break;

			case FXTYPE_METAL:	// "metal"
				snd = LEBS_METAL;
				if(i==5)			hmodel = cgs.media.shardMetal1;
				else if(i==4)		hmodel = cgs.media.shardMetal2;
				else if(i==2)		hmodel = cgs.media.shardMetal2;
				else if(i==1) {
					hmodel = cgs.media.shardMetal2;
					scale = 0.5f;
				}
				else	goto pass;
				break;

				// jaquboss - TODO load data for this one, but not really needed I guess
			case FXTYPE_GIBS:	// "gibs"
			/*	snd = LEBS_BLOOD;
				if(i==5)			hmodel = cgs.media.gibIntestine;
				else if(i==4)		hmodel = cgs.media.gibLeg;
				else if(i==2)		hmodel = cgs.media.gibChest;

				else*/	goto pass;
				break;

			case FXTYPE_BRICK:	// "brick"
				snd = LEBS_ROCK;
				hmodel = cgs.media.debBlock[i];
				break;

			case FXTYPE_STONE:	// "rock"
				snd = LEBS_ROCK;
				if(i==5)			hmodel = cgs.media.debRock[2];	// temporarily use the next smallest rock piece
				else if(i==4)		hmodel = cgs.media.debRock[2];
				else if(i==3)		hmodel = cgs.media.debRock[1];
				else if(i==2)		hmodel = cgs.media.debRock[0];
				else if(i==1)		hmodel = cgs.media.debBlock[1];	// temporarily use the small block pieces
				else				hmodel = cgs.media.debBlock[0];	// temporarily use the small block pieces

				if(i<=2)
					endtime = -2000;	// small bits live 2 sec shorter than normal
				break;

			case FXTYPE_FABRIC:	// "fabric"
				if(i==5)			hmodel = cgs.media.debFabric[0];
				else if(i==4)		hmodel = cgs.media.debFabric[1];
				else if(i==2)		hmodel = cgs.media.debFabric[2];

				else if(i==1) {
					hmodel = cgs.media.debFabric[2];
					scale = 0.5;
				}

				else	goto pass;	// (only do 5, 4, 2 and 1)
				break;
			}

			le = CG_AllocLocalEntity();
			re = &le->refEntity;

			le->leType				= LE_FRAGMENT;
			le->startTime			= cg.time;

			le->endTime				= (le->startTime + 5000 + random() * 5000) + endtime;

			// as it turns out, i'm not sure if setting the re->axis here will actually do anything
			//			AxisClear(re->axis);
			//			re->axis[0][0] =
			//			re->axis[1][1] =
			//			re->axis[2][2] = scale;
			//
			//			if(scale != 1.0)
			//				re->nonNormalizedAxes = qtrue;

			le->sizeScale = scale * sizescale;

			if(type == FXTYPE_GLASS) {	// glass
				// Rafael added this because glass looks funky when it fades out
				// TBD: need to look into this so that they fade out correctly
				re->fadeStartTime		= le->endTime;
				re->fadeEndTime			= le->endTime;
			}
			else {
				re->fadeStartTime		= le->endTime - 4000;
				re->fadeEndTime			= le->endTime;
			}

			if( total > 5 ) {
				if( totalsounds > 5 || (howmany % 8) != 0 )
					snd = LEBS_NONE;
				else
					totalsounds++;
			}

			le->lifeRate	= 1.0/(le->endTime - le->startTime);
			le->leFlags		= LEF_TUMBLE;
			le->leMarkType	= 0;

			VectorCopy( origin, re->origin );
			AxisCopy( axisDefault, re->axis );

			le->leBounceSoundType = snd;
			re->hModel = hmodel;

			// inherit shader
			if(modelshader) {
				re->customShader = modelshader;
			}

			re->radius = 1000;

			// trying to make this a little more interesting
			if(type == FXTYPE_FABRIC) {	// "fabric"
				le->pos.trType = TR_GRAVITY_FLOAT;	// the fabric stuff will change to use something that looks better
			}
			else {
				if(! forceLowGrav && rand()&1)		// if low gravity is not forced and die roll goes our way use regular grav
					le->pos.trType = TR_GRAVITY;
				else
					le->pos.trType = TR_GRAVITY_LOW;
			}

			switch(type) {
			case FXTYPE_FABRIC:	// fabric
				le->bounceFactor	= 0.0;
				materialmul			= 0.3;	// rotation speed
				break;
			default:
				le->bounceFactor	= 0.4;
				break;
			}


			// rotation
			le->angles.trType = TR_LINEAR;
			le->angles.trTime = cg.time;
			le->angles.trBase[0] = rand()&31;
			le->angles.trBase[1] = rand()&31;
			le->angles.trBase[2] = rand()&31;
			le->angles.trDelta[0] = ((100 + (rand()&500)) - 300) * materialmul;
			le->angles.trDelta[1] = ((100 + (rand()&500)) - 300) * materialmul;
			le->angles.trDelta[2] = ((100 + (rand()&500)) - 300) * materialmul;


			//			if(type == 6)	// fabric
			//				materialmul = 1;		// translation speed


			VectorCopy( origin, le->pos.trBase );
			VectorNormalize(dir);
			le->pos.trTime = cg.time;

			// (SA) hoping that was just intended to represent randomness
			//			if (cent->currentState.angles2[0] || cent->currentState.angles2[1] || cent->currentState.angles2[2])
			if (le->angles.trBase[0] == 1 || le->angles.trBase[1] == 1 || le->angles.trBase[2] == 1 ) {
				le->pos.trType = TR_GRAVITY;
				VectorScale(dir, 10 * 8, le->pos.trDelta);
				le->pos.trDelta[0] += ((random() * 400) - 200) * speedscale;
				le->pos.trDelta[1] += ((random() * 400) - 200) * speedscale;
				le->pos.trDelta[2] = ((random() * 400) + 400) * speedscale;

			} else {
				// location
				VectorScale(dir, 200 + mass, le->pos.trDelta);
				le->pos.trDelta[0] += ((random() * 200) - 100);
				le->pos.trDelta[1] += ((random() * 200) - 100);

				if(dir[2])
					le->pos.trDelta[2] = random() * 200 * materialmul;	// randomize sort of a lot so they don't all land together
				else
					le->pos.trDelta[2] = random() * 20;
			}
		}
pass:
		continue;
	}
}

/*
==============
CG_Explodef
	made this more generic for spawning hits and breaks without needing a *cent
==============
*/
void CG_Explodef(vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader) {
	int i;
	localEntity_t	*le;
	refEntity_t		*re;
	int				howmany, total, totalsounds = 0;
	int				pieces[6];	// how many of each piece
	qhandle_t		modelshader = 0;
	float			materialmul = 1;	// multiplier for different types
	leBounceSoundType_t snd;
	int				hmodel;
	float			scale;
	int				endtime;

	memset(&pieces, 0, sizeof(pieces));

	pieces[5]	= (int)(mass / 250.0f);
	pieces[4]	= (int)(mass / 76.0f);
	pieces[3]	= (int)(mass / 37.0f);	// so 2 per 75
	pieces[2]	= (int)(mass / 15.0f);
	pieces[1]	= (int)(mass / 10.0f);
	pieces[0]	= (int)(mass / 5.0f);

	if(pieces[0] > 20)	pieces[0] = 20;	// cap some of the smaller bits so they don't get out of control
	if(pieces[1] > 15)	pieces[1] = 15;
	if(pieces[2] > 10)	pieces[2] = 10;

	if(type == FXTYPE_WOOD ) {	// cap wood even more since it's often grouped, and the small splinters can add up
		if(pieces[0] > 10)	pieces[0] = 10;
		if(pieces[1] > 10)	pieces[1] = 10;
		if(pieces[2] > 10)	pieces[2] = 10;
	}

	total = pieces[5] + pieces[4] + pieces[3] + pieces[2] + pieces[1] + pieces[0];

	if(sound) {
		trap_S_StartSound( origin, -1, CHAN_AUTO, sound);
	}

	if(shader) {	// shader passed in to use
		modelshader = shader;
	}

	for(i=0;i<6;++i) {
		snd = LEBS_NONE;
		hmodel = 0;

		for(howmany = 0; howmany < pieces[i]; ++howmany) {

			scale = 1.0f;
			endtime = 0;	// set endtime offset for faster/slower fadeouts

			switch(type) {
			case FXTYPE_WOOD:	// "wood"
				snd = LEBS_WOOD;
				hmodel = cgs.media.debWood[i];

				if(i==0)		scale = 0.5f;
				else if(i==1)	scale = 0.6f;
				else if(i==2)	scale = 0.7f;
				else if(i==3)	scale = 0.5f;
				//					else goto pass;

				if(i<3) {
					endtime = -3000;	// small bits live 3 sec shorter than normal
				}
				break;

			case FXTYPE_GLASS:	// "glass"
				snd = LEBS_NONE;
				if(i==5)			hmodel = cgs.media.shardGlass1;
				else if(i==4)		hmodel = cgs.media.shardGlass2;
				else if(i==2)		hmodel = cgs.media.shardGlass2;
				else if(i==1) {
					hmodel = cgs.media.shardGlass2;
					scale = 0.5f;
				}
				else {
					goto pass;
				}
				break;

			case FXTYPE_METAL:	// "metal"
				snd = LEBS_BRASS;
				if(i==5)			hmodel = cgs.media.shardMetal1;
				else if(i==4)		hmodel = cgs.media.shardMetal2;
				else if(i==2)		hmodel = cgs.media.shardMetal2;
				else if(i==1) {
					hmodel = cgs.media.shardMetal2;
					scale = 0.5f;
				}
				else {
					goto pass;
				}
				break;

			case FXTYPE_GIBS:	// "gibs"
				snd = LEBS_BLOOD;
				/*if(i==5)			hmodel = cgs.media.gibIntestine;
				else if(i==4)		hmodel = cgs.media.gibLeg;
				else if(i==2)		hmodel = cgs.media.gibChest;
				else*/goto pass;
				break;

			case FXTYPE_BRICK:	// "brick"
				snd = LEBS_ROCK;
				hmodel = cgs.media.debBlock[i];
				break;

			case FXTYPE_STONE:	// "rock"
				snd = LEBS_ROCK;
				if(i==5)			hmodel = cgs.media.debRock[2];	// temporarily use the next smallest rock piece
				else if(i==4)		hmodel = cgs.media.debRock[2];
				else if(i==3)		hmodel = cgs.media.debRock[1];
				else if(i==2)		hmodel = cgs.media.debRock[0];
				else if(i==1)		hmodel = cgs.media.debBlock[1];	// temporarily use the small block pieces
				else				hmodel = cgs.media.debBlock[0];	// temporarily use the small block pieces

				if(i<=2) {
					endtime = -2000;	// small bits live 2 sec shorter than normal
				}
				break;

			case FXTYPE_FABRIC:	// "fabric"
				if(i==5)			hmodel = cgs.media.debFabric[0];
				else if(i==4)		hmodel = cgs.media.debFabric[1];
				else if(i==2)		hmodel = cgs.media.debFabric[2];

				else if(i==1) {
					hmodel = cgs.media.debFabric[2];
					scale = 0.5;
				}
				else {
					goto pass;	// (only do 5, 4, 2 and 1)
				}
				break;
			}

			le = CG_AllocLocalEntity();
			re = &le->refEntity;

			le->leType				= LE_FRAGMENT;
			le->startTime			= cg.time;

			le->endTime				= (le->startTime + 5000 + random() * 5000) + endtime;

			// as it turns out, i'm not sure if setting the re->axis here will actually do anything
			//			AxisClear(re->axis);
			//			re->axis[0][0] =
			//			re->axis[1][1] =
			//			re->axis[2][2] = scale;
			//
			//			if(scale != 1.0)
			//				re->nonNormalizedAxes = qtrue;

			le->sizeScale = scale;

			if(type == FXTYPE_GLASS) {	// glass
				// Rafael added this because glass looks funky when it fades out
				// TBD: need to look into this so that they fade out correctly
				re->fadeStartTime		= le->endTime;
				re->fadeEndTime			= le->endTime;
			}
			else {
				re->fadeStartTime		= le->endTime - 4000;
				re->fadeEndTime			= le->endTime;
			}

			if( total > 5 ) {
				if( totalsounds > 5 || (howmany % 8) != 0 ) {
					snd = LEBS_NONE;
				}
				else {
					totalsounds++;
				}
			}

			le->lifeRate	= 1.0/(le->endTime - le->startTime);
			le->leFlags		= LEF_TUMBLE;
			le->leMarkType	= 0;

			VectorCopy( origin, re->origin );
			AxisCopy( axisDefault, re->axis );

			le->leBounceSoundType = snd;
			re->hModel = hmodel;

			// inherit shader
			if(modelshader) {
				re->customShader = modelshader;
			}

			re->radius = 1000;

			// trying to make this a little more interesting
			if(type == FXTYPE_FABRIC) {	// "fabric"
				le->pos.trType = TR_GRAVITY_FLOAT;	// the fabric stuff will change to use something that looks better
			}
			else {
				if(! forceLowGrav && rand()&1) {	// if low gravity is not forced and die roll goes our way use regular grav
					le->pos.trType = TR_GRAVITY;
				}
				else {
					le->pos.trType = TR_GRAVITY_LOW;
				}
			}

			switch(type) {
			case FXTYPE_FABRIC:	// fabric
				le->bounceFactor	= 0.0;
				materialmul			= 0.3;	// rotation speed
				break;
			default:
				le->bounceFactor	= 0.4;
				break;
			}


			// rotation
			le->angles.trType = TR_LINEAR;
			le->angles.trTime = cg.time;
			le->angles.trBase[0] = rand()&31;
			le->angles.trBase[1] = rand()&31;
			le->angles.trBase[2] = rand()&31;
			le->angles.trDelta[0] = ((100 + (rand()&500)) - 300) * materialmul;
			le->angles.trDelta[1] = ((100 + (rand()&500)) - 300) * materialmul;
			le->angles.trDelta[2] = ((100 + (rand()&500)) - 300) * materialmul;


			//			if(type == 6)	// fabric
			//				materialmul = 1;		// translation speed


			VectorCopy( origin, le->pos.trBase );
			VectorNormalize(dir);
			le->pos.trTime = cg.time;

			// (SA) hoping that was just intended to represent randomness
			//			if (cent->currentState.angles2[0] || cent->currentState.angles2[1] || cent->currentState.angles2[2])
			if (le->angles.trBase[0] == 1 || le->angles.trBase[1] == 1 || le->angles.trBase[2] == 1 ) {
				le->pos.trType = TR_GRAVITY;
				VectorScale(dir, 10 * 8, le->pos.trDelta);
				le->pos.trDelta[0] += ((random() * 100) - 50);
				le->pos.trDelta[1] += ((random() * 100) - 50);
				le->pos.trDelta[2] = (random() * 200) + 200;

			}
			else {
				// location
				VectorScale(dir, 200 + mass, le->pos.trDelta);
				le->pos.trDelta[0] += ((random() * 100) - 50);
				le->pos.trDelta[1] += ((random() * 100) - 50);

				if(dir[2]) {
					le->pos.trDelta[2] = random() * 200 * materialmul;	// randomize sort of a lot so they don't all land together
				}
				else {
					le->pos.trDelta[2] = random() * 20;
				}
			}
		}
pass:
		continue;
	}
}

/*
==============
CG_Effect
	Quake ed -> target_effect (0 .5 .8) (-6 -6 -6) (6 6 6) fire explode smoke debris gore lowgrav
==============
*/
void CG_Effect(centity_t *cent, vec3_t origin, vec3_t dir)
{

	VectorSet(dir, 0, 0, 1);	// straight up.

	if(cent->currentState.eventParm & 1) {	// fire
		CG_MissileHitWall( WP_DYNAMITE, 0, origin, dir, 0 );
		return;
	}

	// (SA) right now force smoke on any explosions
//	if(cent->currentState.eventParm & 4)	// smoke
	if(cent->currentState.eventParm & 7) {
		int i, j;
		vec3_t sprVel, sprOrg;
		// explosion sprite animation
		VectorScale( dir, 16, sprVel );
		for (i=0; i<5; ++i) {
			for (j=0;j<3;++j) {
				sprOrg[j] = origin[j] + 64*dir[j] + 24*crandom();
			}
			sprVel[2] += rand()%50;
			CG_ParticleExplosion( "blacksmokeanim", sprOrg, sprVel, 3500+rand()%250, 10, 250+rand()%60, qfalse ); // JPW NERVE was smokeanimb
		}
	}


	if(cent->currentState.eventParm & 2) {	// explode

		vec4_t		projection, color;
		vec3_t 		sprVel, sprOrg;

		trap_S_StartSound( origin, -1, CHAN_AUTO, cgs.media.sfx_rockexp );

		// new explode	(from rl)
		VectorMA( origin, 16.0f, dir, sprOrg );
		VectorScale( dir, 100, sprVel );
		if (cg_wolfparticles.integer) {
			CG_ParticleExplosion( "explode1", sprOrg, sprVel, 500, 20, 160, qtrue );
		}

		VectorSet( projection, 0, 0, -1 );
		projection[ 3 ] = 64.0f;
		Vector4Set( color, 1.0f, 1.0f, 1.0f, 1.0f );
		trap_R_ProjectDecal( cgs.media.burnMarkShader, 1, (vec3_t*) origin, projection, color, cg_markTime.integer, (cg_markTime.integer >> 4) );
	}


	if(cent->currentState.eventParm & 8) { // rubble
		// share the cg_explode code with func_explosives
		const char *s;
		qhandle_t	sh = 0;	// shader handle

		vec3_t newdir = {0, 0, 0};

		if (cent->currentState.angles2[0] || cent->currentState.angles2[1] || cent->currentState.angles2[2]) {
			VectorCopy (cent->currentState.angles2, newdir);
		}

		s = CG_ConfigString( CS_TARGETEFFECT );	// see if ent has a shader specified
		if(s && strlen(s) > 0)
			sh = trap_R_RegisterShader(va("textures/%s", s));	// FIXME: don't do this here.  only for testing

		cent->currentState.eFlags &= ~EF_INHERITSHADER;	// don't try to inherit shader
		cent->currentState.dl_intensity = 0;		// no sound
		CG_Explode(cent, origin, newdir, sh);
	}

	if(cent->currentState.eventParm & 64) {// debris trails (the black strip that Ryan did)
		CG_AddDebris( origin, dir,
						280,	// speed
						1400,	// duration
						7 + rand()%2 );	// count
	}
}

/*
CG_Shard

	We should keep this separate since there will be considerable differences
	in the physical properties of shard vrs debris. not to mention the fact
	there is no way we can quantify what type of effects the designers will
	potentially desire. If it is still possible to merge the functionality of
	cg_shard into cg_explode at a latter time I would have no problem with that
	but for now I want to keep it separate
*/
void CG_Shard(centity_t *cent, vec3_t origin, vec3_t dir)
{
	localEntity_t	*le;
	refEntity_t		*re;
	int				type = cent->currentState.density;
	int				howmany = cent->currentState.frame;
	int				i;
	int				rval;
	qboolean		isflyingdebris = qfalse;

	for(i = 0;i < howmany; ++i) {
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leType				= LE_FRAGMENT;
		le->startTime			= cg.time;
		le->endTime				= le->startTime + 5000 + random() * 5000;

//----(SA)	fading out
			re->fadeStartTime		= le->endTime - 1000;
			re->fadeEndTime			= le->endTime;
//----(SA)	end

		if (type == 999) {
			le->startTime			= cg.time;
			le->endTime				= le->startTime + 100;
			re->fadeStartTime		= le->endTime - 100;
			re->fadeEndTime			= le->endTime;
			type = 1;

			isflyingdebris = qtrue;
		}


		le->lifeRate			= 1.0/(le->endTime - le->startTime);
		le->leFlags				= LEF_TUMBLE;
		le->bounceFactor		= 0.4;
		// le->leBounceSoundType	= LEBS_WOOD;
		le->leMarkType			= 0;

		VectorCopy( origin, re->origin );
		AxisCopy( axisDefault, re->axis );

		if (type == FXTYPE_GLASS) {// glass

			rval = rand()%2;

			if (rval)
				re->hModel = cgs.media.shardGlass1;
			else
				re->hModel = cgs.media.shardGlass2;
		}
		else if (type == FXTYPE_WOOD) {// wood

			rval = rand()%2;

			if (rval)
				re->hModel = cgs.media.shardWood1;
			else
				re->hModel = cgs.media.shardWood2;
		}
		else if (type == FXTYPE_METAL) { // metal

			rval = rand()%2;

			if (rval)
				re->hModel = cgs.media.shardMetal1;
			else
				re->hModel = cgs.media.shardMetal2;
		}
		// TODO ?
		/*else if (type == 3) // ceramic
		{
			rval = rand()%2;

			if (rval)
				re->hModel = cgs.media.shardCeramic1;
			else
				re->hModel = cgs.media.shardCeramic2;
		}*/
		else if (type == FXTYPE_BRICK || type == FXTYPE_STONE) { // rubble

			rval = rand()%3;

			if (rval == 1)
				re->hModel = cgs.media.shardRubble1;
			else if (rval == 2)
				re->hModel = cgs.media.shardRubble2;
			else
				re->hModel = cgs.media.shardRubble3;

		}
		else {
			CG_Printf( "CG_Debris has an unknown type\n" );
		}

		// location
		if (isflyingdebris)
			le->pos.trType = TR_GRAVITY_LOW;
		else
			le->pos.trType = TR_GRAVITY;

		VectorCopy( origin, le->pos.trBase );
		VectorNormalize(dir);
		VectorScale(dir, 10 * howmany, le->pos.trDelta);
		le->pos.trTime = cg.time;
		le->pos.trDelta[0] += ((random() * 100) - 50);
		le->pos.trDelta[1] += ((random() * 100) - 50);
		if (type)
			le->pos.trDelta[2] = (random() * 200) + 100;	// randomize sort of a lot so they don't all land together
		else // glass
			le->pos.trDelta[2] = (random() * 100) + 50;	// randomize sort of a lot so they don't all land together

		// rotation
		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand()&31;
		le->angles.trBase[1] = rand()&31;
		le->angles.trBase[2] = rand()&31;
		le->angles.trDelta[0] = (100 + (rand()&500)) - 300;
		le->angles.trDelta[1] = (100 + (rand()&500)) - 300;
		le->angles.trDelta[2] = (100 + (rand()&500)) - 300;

	}
}

void CG_ShardJunk (centity_t *cent, vec3_t origin, vec3_t dir)
{
	localEntity_t	*le 	= CG_AllocLocalEntity();
	refEntity_t		*re 	= &le->refEntity;

	le->leType				= LE_FRAGMENT;
	le->startTime			= cg.time;
	le->endTime				= le->startTime + 5000 + random() * 5000;

	re->fadeStartTime		= le->endTime - 1000;
	re->fadeEndTime			= le->endTime;

	le->lifeRate			= 1.0/(le->endTime - le->startTime);
	le->leFlags				= LEF_TUMBLE;
	le->bounceFactor		= 0.4;
	le->leMarkType			= 0;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );

	re->hModel = cgs.media.shardJunk[rand()%MAX_LOCKER_DEBRIS];

	le->pos.trType = TR_GRAVITY;

	VectorCopy( origin, le->pos.trBase );
	VectorNormalize(dir);
	VectorScale(dir, 10 * 8, le->pos.trDelta);
	le->pos.trTime = cg.time;
	le->pos.trDelta[0] += ((random() * 100) - 50);
	le->pos.trDelta[1] += ((random() * 100) - 50);

	le->pos.trDelta[2] = (random() * 100) + 50;	// randomize sort of a lot so they don't all land together

	// rotation
	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	//le->angles.trBase[0] = rand()&31;
	//le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;

	//le->angles.trDelta[0] = (100 + (rand()&500)) - 300;
	//le->angles.trDelta[1] = (100 + (rand()&500)) - 300;
	le->angles.trDelta[2] = (100 + (rand()&500)) - 300;
}

// Gordon: debris test
void CG_Debris (centity_t *cent, vec3_t origin, vec3_t dir) {
	localEntity_t	*le 	= CG_AllocLocalEntity();
	refEntity_t		*re 	= &le->refEntity;

	le->leType				= LE_FRAGMENT;
	le->startTime			= cg.time;
	le->endTime				= le->startTime + 5000 + random() * 5000;

	re->fadeStartTime		= le->endTime - 1000;
	re->fadeEndTime			= le->endTime;

	le->lifeRate			= 1.0/(le->endTime - le->startTime);
	le->leFlags				= LEF_TUMBLE | LEF_TUMBLE_SLOW;
	le->bounceFactor		= 0.4;
	le->leMarkType			= 0;
	le->breakCount			= 1;
	le->sizeScale			= 0.5;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );

	re->hModel = cgs.inlineDrawModel[cent->currentState.modelindex];

	le->pos.trType = TR_GRAVITY;

	VectorCopy( origin, le->pos.trBase );
	VectorCopy( dir, le->pos.trDelta );
	le->pos.trTime = cg.time;

	// rotation
	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[2] = rand()&31;

	// TODO: check this
	//le->angles.trDelta[2] = (100 + (rand()&500)) - 300;
	//le->angles.trDelta[2] = (50 + (rand()&400)) - 100;
	le->angles.trDelta[2] = (50 + (rand()&400)) - 100;
}
// ===================

void CG_MortarImpact( centity_t *cent, vec3_t origin, int sfx, qboolean dist )
{
	if( sfx >= 0 ) {
		trap_S_StartSound( origin, -1, CHAN_AUTO, cgs.media.sfx_mortarexp[sfx] );
	}

	if( dist ) {
		vec3_t	gorg, norm;
		float	gdist;

		VectorSubtract( origin, cg.refdef_current->vieworg, norm );
		gdist = VectorNormalize( norm );
		if(gdist > 1200 && gdist < 8000) {	// 1200 is max cam shakey dist (2*600) use gorg as the new sound origin
			VectorMA( cg.refdef_current->vieworg, 800, norm, gorg ); // non-distance falloff makes more sense; sfx2range was gdist*0.2
																// sfx2range is variable to give us minimum volume control different explosion sizes (see mortar, panzerfaust, and grenade)
			trap_S_StartSoundEx( gorg, -1, CHAN_WEAPON, cgs.media.sfx_mortarexpDist, SND_NOCUT);
		}

		if( cent->currentState.clientNum == cg.snap->ps.clientNum && cg.mortarImpactTime != -2 ) {
			VectorCopy( origin, cg.mortarImpactPos );
			cg.mortarImpactTime = cg.time;
			cg.mortarImpactOutOfMap = qfalse;
		}
	}
}

void CG_MortarMiss( centity_t *cent, vec3_t origin )
{
	if( cent->currentState.clientNum == cg.snap->ps.clientNum && cg.mortarImpactTime != -2 ) {
		VectorCopy( origin, cg.mortarImpactPos );
		cg.mortarImpactTime = cg.time;
		if( cent->currentState.density ) {
			cg.mortarImpactOutOfMap = qtrue;
		}
		else {
			cg.mortarImpactOutOfMap = qfalse;
		}
	}
}

// a convenience function for all footstep/faldamage sound playing
// core: check if sounds are not too far away to be able to hear them..
qboolean SoundWithinDistance( entityState_t *es )
{
	vec3_t playerOrg;
	vec3_t viewerOrg;
	float dist;

	VectorCopy(es->pos.trBase, playerOrg);
	VectorCopy(cg.refdef_current->vieworg, viewerOrg);
	dist = Distance( playerOrg, viewerOrg );
	return ( dist > 1536 )? qfalse : qtrue;
}

// a convenience function for all footstep sound playing
static void CG_StartFootStepSound( bg_playerclass_t* classInfo, entityState_t *es, sfxHandle_t sfx )
{
	if( cg_footsteps.integer ) {
		//core: don't start a noise if too far away..
		if ( SoundWithinDistance( es ) ) {
			trap_S_StartSound( NULL, es->number, CHAN_BODY, sfx );
		}
	}
}

// a convenience function for all falldamage sound playing
void CG_StartFallDamageSound( entityState_t *es, bg_character_t *character, qboolean hurtsound, centity_t *cent ) {
	if ( SoundWithinDistance( es ) ) {
		sfxHandle_t sfx = ( es->eventParm )? cgs.media.landSound[es->eventParm] : cgs.media.landSound[character->animModelInfo->footsteps];

		if( es->eventParm != FOOTSTEP_TOTAL ) {
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, sfx );
		}

		// make a hurtsound too?
		if ( hurtsound ) {
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landHurt );
			cent->pe.painTime = cg.time;	// don't play a pain sound right after this
		}
	}
}

/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
extern void CG_AddBulletParticles( vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale );
// JPW NERVE
// jpw
#define	DEBUGNAME(x) if(cg_debugEvents.integer){CG_Printf(x"\n");}

void CG_WeaponFireRecoil( int weapon );

void CG_EntityEvent( centity_t *cent, vec3_t position )
{
	entityState_t		*es = &cent->currentState;
	int					event = es->event & ~EV_EVENT_BITS;
	const char			*s;
	int					clientNum;

	bg_playerclass_t	*classInfo;
	bg_character_t		*character;

	static int		footstepcnt = 0;
	static int		splashfootstepcnt = 0;


	if ( cg_debugEvents.integer && event != EV_RAILTRAIL ) {
		CG_Printf( "time:%i ent:%3i  event:%3i ", cg.time, es->number, event );
	}

	if ( !event ) {
		DEBUGNAME("ZEROEVENT");
		return;
	}

	clientNum = es->clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}

	classInfo = CG_PlayerClassForClientinfo( &cgs.clientinfo[ clientNum ], cent );
	character = CG_CharacterForClientinfo( &cgs.clientinfo[ clientNum ], cent );

	switch ( event ) {
	//
	// movement generated events
	//

	case EV_FOOTSTEP:
		DEBUGNAME("EV_FOOTSTEP");
		if( es->eventParm != FOOTSTEP_TOTAL ) {
			if( es->eventParm ) {
				CG_StartFootStepSound( classInfo, es, cgs.media.footsteps[ es->eventParm ][footstepcnt] );
			}
			else {
				CG_StartFootStepSound( classInfo, es, cgs.media.footsteps[ character->animModelInfo->footsteps ][footstepcnt] );
			}
		}
		break;
	case EV_FOOTSPLASH:
		DEBUGNAME("EV_FOOTSPLASH");
		CG_StartFootStepSound( classInfo, es, cgs.media.footsteps[ FOOTSTEP_SPLASH ][splashfootstepcnt] );
		break;
	case EV_FOOTWADE:
		DEBUGNAME("EV_FOOTWADE");
		CG_StartFootStepSound( classInfo, es, cgs.media.footsteps[ FOOTSTEP_SPLASH ][splashfootstepcnt] );
		break;
	case EV_SWIM:
		DEBUGNAME("EV_SWIM");
		CG_StartFootStepSound( classInfo, es, cgs.media.footsteps[ FOOTSTEP_SPLASH ][footstepcnt] );
		break;

	//
	// weapon events
	//
	case EV_FIRE_WEAPON:
	case EV_FIRE_WEAPONB:
		DEBUGNAME("EV_FIRE_WEAPON");
        if( es->eventParm > 0 && cent->currentState.clientNum == cg.snap->ps.clientNum ) {
            if (es->eventParm != WP_BINOCULARS && cg.snap->ps.eFlags & EF_ZOOMING)
			    break;
        }

		if ((es->weapon == WP_KNIFE || es->weapon == WP_KNIFE_KABAR) && es->clientNum == cg.snap->ps.clientNum ){
			cg.knifeBloodVal -= 10;
			if ( cg.knifeBloodVal < 0 )
				cg.knifeBloodVal = 0;
		}

        CG_FireWeapon( cent, event, es->eventParm == WP_BINOCULARS );
		if( event == EV_FIRE_WEAPONB )	// akimbo firing
			cent->akimboFire = qtrue;
		else
			cent->akimboFire = qfalse;
		break;
	case EV_FIRE_WEAPON_LASTSHOT:
		DEBUGNAME("EV_FIRE_WEAPON_LASTSHOT");
		CG_FireWeapon( cent, event, qfalse );
		break;

	case EV_MG42BULLET_HIT_WALL:
		DEBUGNAME("EV_MG42BULLET_HIT_WALL");
		{
			vec3_t				dir;

			ByteToDir( es->eventParm, dir );
			CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD, es->otherEntityNum2, es->origin2[0], es->effect1Time, qfalse, NULL );
		}
		break;
	case EV_MG42BULLET_HIT_FLESH:
		DEBUGNAME("EV_MG42BULLET_HIT_FLESH");
		{
			vec3_t				dir;

			ByteToDir( es->eventParm, dir );
			CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qtrue, es->effect2Time, es->otherEntityNum2, 0, es->effect1Time, qtrue, es->origin2);
		}
		break;
    // Single event for all player hitscan weapons attack result
	case EV_BULLET:
		DEBUGNAME("EV_BULLET");
        {
			int			seed = es->eventParm;
			vec3_t		start, end, forward;
			float		tracedist = (IS_SCOPED_WEAPON(es->weapon))? 2*MAX_TRACE : MAX_TRACE;

            // need spread again
            if (es->eType == ET_PLAYER) {
            	float		r, u, spread;
            	vec3_t		right, up;

                CG_CalcMuzzlePoints( cent, es->weapon, start );
                AngleVectors (cent->lerpAngles, forward, right, up);
                spread = BG_Weapons[es->weapon].spread;

                switch( es->weapon ) {
		            case WP_MOBILE_BROWNING_SET:
		            case WP_MOBILE_MG42_SET:
		            case WP_BAR_SET:
			            VectorMA(start, 48.0f, forward, start);
			            break;
		            default:
			            break;
				}

				// no spread reduction if airborne
				if( es->groundEntityNum == ENTITYNUM_NONE ) {
					spread = BG_Weapons[es->weapon].spread * 2.0;
				}
				else {
					switch( es->weapon ) {
						// case WP_SHOTGUN: IRATA: removed Bullet_Fire is not used by the shotgun
						// light weapons
						case WP_LUGER:
						case WP_COLT:
						case WP_MP40:
						case WP_THOMPSON:
						case WP_STEN:
						case WP_SILENCER:
						case WP_SILENCED_COLT:
						// CHRUKER: b045 - Akimbo weapons also need spread reduction
						case WP_AKIMBO_LUGER:
						case WP_AKIMBO_COLT:
						case WP_AKIMBO_SILENCEDLUGER:
						case WP_AKIMBO_SILENCEDCOLT:
						// jet Pilot - New Weapons that recieve a spread reduction
						case WP_BAR:
						case WP_STG44:
						case WP_STEN_MKII:
							if(cgs.clientinfo[es->clientNum].skillBits[SK_LIGHT_WEAPONS] & (1<<3) ) {
								spread *= .65f;
							}
							if ( BG_Weapons[es->weapon].CrouchSpreadRatio && es->eFlags & EF_CROUCHING ) {
								spread *= BG_Weapons[es->weapon].CrouchSpreadRatio;
							}
							else if ( BG_Weapons[es->weapon].ProneSpreadRatio && es->eFlags & EF_PRONE ) {
								spread *= BG_Weapons[es->weapon].ProneSpreadRatio;
							}
							break;
						default:
							break;
					}

				}

				VectorMA(start, tracedist, forward, end);
			    r = Q_crandom(&seed)*spread;
			    u = Q_crandom(&seed)*spread;
			    VectorMA(end, r, right, end);
			    VectorMA(end, u, up, end);
            }
            else {
                VectorCopy(es->origin, start);
                AngleVectors (es->angles, forward, NULL, NULL);
			    VectorMA(start, tracedist, forward, end);
            }
            CG_BulletFire(end, es->number);
        }
		break;

	case EV_BULLET_HIT_WALL:
		DEBUGNAME("EV_BULLET_HIT_WALL");
		{
			vec3_t				dir;

			ByteToDir( es->eventParm, dir );
			CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD, es->otherEntityNum2, es->modelindex2, 0, es->dmgFlags ? qtrue : qfalse, es->origin2 );
		}
		break;

    case EV_BULLET_HIT_FLESH:
		DEBUGNAME("EV_BULLET_ACCURATE");
		{
			vec3_t				dir;

			CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm, es->otherEntityNum2, 0, 0, es->dmgFlags ? qtrue : qfalse, es->origin2);
		}
		break;

	case EV_FIRE_WEAPON_MOUNTEDMG42:
	case EV_FIRE_WEAPON_MG42:
		DEBUGNAME("EV_FIRE_WEAPON_MG42");
		CG_FireWeapon( cent, event, qfalse );
		break;

	case EV_FLAMETHROWER_EFFECT:
		DEBUGNAME("EV_FLAMETHROWER_EFFECT");
		CG_FireFlameChunks( cent, cent->currentState.origin, cent->currentState.apos.trBase, 0.6, qtrue, qtrue );
		break;

	//
	// falling events
	//
	case EV_FALL_SHORT:
		DEBUGNAME("EV_FALL_SHORT");
		CG_StartFallDamageSound( es, character, qfalse, cent );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -8;
			cg.landTime = cg.time;
		}
		break;
	case EV_FALL_DMG_10:
		DEBUGNAME("EV_FALL_DMG_10");
		CG_StartFallDamageSound( es, character, qtrue, cent );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
		}
		break;
	case EV_FALL_DMG_15:
		DEBUGNAME("EV_FALL_DMG_15");
		CG_StartFallDamageSound( es, character, qtrue, cent );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
		}
		break;
	case EV_FALL_DMG_25:
		DEBUGNAME("EV_FALL_DMG_25");
		CG_StartFallDamageSound( es, character, qtrue, cent );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}
		break;
	case EV_FALL_DMG_50:
		DEBUGNAME("EV_FALL_DMG_50");
		CG_StartFallDamageSound( es, character, qtrue, cent );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}
		break;
	case EV_FALL_NDIE:
		DEBUGNAME("EV_FALL_NDIE");
		CG_StartFallDamageSound( es, character, qtrue, cent );
		// splat
		break;


	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:		// smooth out step up transitions
		DEBUGNAME("EV_STEPx");
	{
		float	oldStep;
		int		delta;
		int		step;

		if ( clientNum != cg.predictedPlayerState.clientNum ) {
			break;
		}

		// if we are interpolating, we don't need to smooth steps
		if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) ||
			cg_nopredict.integer
#ifdef ALLOW_GSYNC
			|| cg_synchronousClients.integer
#endif // ALLOW_GSYNC
			)
		{
			break;
		}
		// check for stepping up before a previous step is completed
		delta = cg.time - cg.stepTime;

		if (delta < STEP_TIME) {
			oldStep = cg.stepChange * (STEP_TIME - delta) / STEP_TIME;
		}
		else {
			oldStep = 0;
		}

		// add this amount
		step = 4 * (event - EV_STEP_4 + 1 );
		cg.stepChange = oldStep + step;
		if ( cg.stepChange > MAX_STEP_CHANGE ) {
			cg.stepChange = MAX_STEP_CHANGE;
		}
		cg.stepTime = cg.time;
		break;
	}

	case EV_WATER_TOUCH:
		DEBUGNAME("EV_WATER_TOUCH");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrInSound );
		break;
	case EV_WATER_LEAVE:
		DEBUGNAME("EV_WATER_LEAVE");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound );
		break;
	case EV_WATER_UNDER:
		DEBUGNAME("EV_WATER_UNDER");

		if( es->eventParm )
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound );

		if( cg.clientNum == es->number ) {
			if ( cgs.clientinfo[cg.snap->ps.clientNum].skillBits[SK_BATTLE_SENSE] & (1<<6)  )
				cg.waterundertime = cg.time + HOLDBREATHTIME_LONG;
			else
				cg.waterundertime = cg.time + HOLDBREATHTIME;
		}

//----(SA)	this fog stuff for underwater is really just a test for feasibility of creating the under-water effect that way.
//----(SA)	the related issues of load/savegames, death underwater, etc. are not handled at all.
//----(SA)	the actual problem, of course, is doing underwater stuff when the water is very turbulant and you can't simply
//----(SA)	do things based on the players head being above/below the water brushes top surface. (since the waves can potentially be /way/ above/below that)

		// DHM - Nerve :: causes problems in multiplayer...
		break;
	case EV_WATER_CLEAR:
		DEBUGNAME("EV_WATER_CLEAR");;
		// jet Pilot - TODOLvl 7 covert ops make no sound when exiting the water slowly
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound );
		if( es->eventParm )
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrGaspSound );
		break;

	case EV_WEAP_OVERHEAT:
		DEBUGNAME("EV_WEAP_OVERHEAT");

		// start weapon idle animation
		if(es->number == cg.snap->ps.clientNum) {
			cg.predictedPlayerState.weapAnim = ( ( cg.predictedPlayerState.weapAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | PM_IdleAnimForWeapon(cg.snap->ps.weapon);
			cent->overheatTime = cg.time;	// used to make the barrels smoke when overheated
		}

		if( es->eFlags & EF_MG42_ACTIVE ) {
			trap_S_StartSoundVControl( NULL, es->number, CHAN_AUTO, cgs.media.hWeaponHeatSnd, 255 );
		}
		else if( es->eFlags & EF_MOUNTEDTANK ) {
			if( cg_entities[cg_entities[cg_entities[ es->number ].tagParent].tankparent].currentState.density & 8 ) {
				trap_S_StartSoundVControl( NULL, es->number, CHAN_AUTO, cgs.media.hWeaponHeatSnd_2, 255 );
			}
			else {
				trap_S_StartSoundVControl( NULL, es->number, CHAN_AUTO, cgs.media.hWeaponHeatSnd, 255 );
			}
		}
		else if( cg_weapons[es->weapon].overheatSound ) {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cg_weapons[es->weapon].overheatSound );
		}
		break;

	case EV_GRENADE_BOUNCE:
		DEBUGNAME("EV_GRENADE_BOUNCE");

		// DYNAMITE // Gordon: or LANDMINE FIXME: change this? (mebe a metallic sound)
		if( es->weapon == WP_SATCHEL ) {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.satchelbounce1 );
		}
		else if( es->weapon == WP_DYNAMITE ) {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.dynamitebounce1 );
		}
		else if( es->weapon == WP_LANDMINE ) {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.landminebounce1 );
		}
		else {
		// GRENADES
			if( es->eventParm != FOOTSTEP_TOTAL ) {
				if ( rand() & 1 ) {
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.grenadebounce[ es->eventParm ][0] );
				}
				else {
					trap_S_StartSound( NULL, es->number, CHAN_AUTO,  cgs.media.grenadebounce[ es->eventParm ][1] );
				}
			}
		}
		break;

	case EV_FILL_CLIP:
		DEBUGNAME("EV_FILL_CLIP");
		if( cgs.clientinfo[es->number].skillBits[SK_LIGHT_WEAPONS] & (1<<2) &&
			BG_isLightWeaponSupportingFastReload( es->weapon ) && cg_weapons[es->weapon].reloadFastSound )
			trap_S_StartSound (NULL, es->number, CHAN_WEAPON, cg_weapons[es->weapon].reloadFastSound );
		else if(cg_weapons[es->weapon].reloadSound)
			trap_S_StartSound (NULL, es->number, CHAN_WEAPON, cg_weapons[es->weapon].reloadSound ); // JPW NERVE following sherman's SP fix, should allow killing reload sound when player dies
		break;

	case EV_SHOTGUN_PUMP:
		DEBUGNAME("EV_SHOTGUN_PUMP");
		trap_S_StartSound (NULL, es->number, CHAN_WEAPON, cg_weapons[es->weapon].reloadFastSound ); // hijacking this sound for a shotgun
		break;
	case EV_SHOTGUN_FIRED:
		DEBUGNAME("EV_SHOTGUN_FIRED");
		{
			float		r, u, spread;
			int			i, randSeed;
			int			seed = es->eventParm;
			vec3_t		start, end, right, up, forward;

            CG_CalcMuzzlePoints( cent, es->weapon, start );
            AngleVectors (cent->lerpAngles, forward, right, up);

            spread = BG_Weapons[WP_SHOTGUN].spread;

            if( cgs.clientinfo[es->clientNum].skillBits[SK_LIGHT_WEAPONS] & (1<<3) ) {
		        spread *= .75f;
	        }

			for ( i = 0; i < MAX_SHOTGUN_PELLETS; ++i ) {
				randSeed = seed;
				r = Q_crandom(&randSeed)*spread;
				u = Q_crandom(&randSeed)*spread;
				VectorMA (start, MAX_TRACE, forward, end);
				VectorMA (end, r, right, end);
				VectorMA (end, u, up, end);
                CG_BulletFire(end, es->number);
				seed = seed * 1.33;
			}
		}
		break;

	case EV_NOAMMO:
	case EV_WEAPONSWITCHED:
		DEBUGNAME("EV_NOAMMO");

		switch(es->weapon) {
			case WP_GRENADE_LAUNCHER:
			case WP_GRENADE_PINEAPPLE:
			case WP_DYNAMITE:
			case WP_LANDMINE:
			case WP_SATCHEL:
			case WP_SATCHEL_DET:
			case WP_SMOKE_BOMB:
			case WP_AMMO:
			case WP_MEDKIT:
				break;
			default:
				trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
				break;
		}

		if ( es->number == cg.snap->ps.clientNum ) {
			qboolean noammo = qfalse;
			// core: if we get a request to switch weapon, then do it.. (ammo available or not)
			if (event == EV_WEAPONSWITCHED) {
				noammo = qtrue;
			}
			else if ( cg_noAmmoAutoSwitch.integer == 1 && !CG_WeaponSelectable(cg.weaponSelect) ) {
				noammo = qtrue;
			}
			else {
				switch(es->weapon) {
					case WP_PANZERFAUST:
					case WP_BAZOOKA:
						// Alice: Added to prevent autoswitching when client has Auto Switch NO
						if ( cg_noAmmoAutoSwitch.integer != 2 ) {
							noammo = qtrue;
						}
						break;
					case WP_MORTAR_SET:
					case WP_MORTAR2_SET:
					case WP_GRENADE_LAUNCHER:
					case WP_GRENADE_PINEAPPLE:
					case WP_DYNAMITE:
					case WP_SMOKE_MARKER:
					case WP_ARTY:
					case WP_LANDMINE:
					case WP_SATCHEL:
					case WP_SATCHEL_DET:
					case WP_SMOKE_BOMB:
					case WP_AMMO:
					case WP_MEDKIT:
						noammo = qtrue;
						break;
					default:
						break;
				}
			}
			if ( noammo ) {
				if ( es->weapon == WP_SATCHEL_DET ) {
					cg.weaponSelect = WP_SATCHEL_DET;
				}
				else if ( es->weapon == WP_SMOKE_MARKER ) {
					cg.weaponSelect = WP_SMOKE_MARKER;
				}
				CG_OutOfAmmoChange( event == EV_WEAPONSWITCHED ? qfalse : qtrue);
			}
		}
		break;
	case EV_CHANGE_WEAPON:
		DEBUGNAME("EV_CHANGE_WEAPON");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.selectSound );
		break;
	case EV_CHANGE_WEAPON_2:
		DEBUGNAME("EV_CHANGE_WEAPON");

		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.selectSound );

		if( es->number == cg.snap->ps.clientNum ) {
			int newweap = 0;

			// client will get this message if reloading while using an alternate weapon
			// client should voluntarily switch back to primary at that point
			switch(es->weapon) {
				case WP_FG42SCOPE:
					newweap = WP_FG42;
					break;
				case WP_GARAND_SCOPE:
					newweap = WP_GARAND;
					break;
				case WP_K43_SCOPE:
					newweap = WP_K43;
					break;
				default:
					break;
			}

			if( newweap ) {
				CG_FinishWeaponChange( es->weapon, newweap );
			}
		}
		break;

	case EV_ITEM_PICKUP:
	case EV_ITEM_PICKUP_QUIET:
		DEBUGNAME("EV_ITEM_PICKUP");
		{
			gitem_t	*item;
			int		index = es->eventParm;		// player predicted

			if ( index < 1 || index >= bg_numItems ) {
				break;
			}
			item = &bg_itemlist[ index ];

			if(event == EV_ITEM_PICKUP)	{// not quiet

				// powerups and team items will have a separate global sound, this one
				// will be played at prediction time
				if( item->giType == IT_TEAM) {
					trap_S_StartSound (NULL, es->number, CHAN_AUTO,	trap_S_RegisterSound( "sound/misc/w_pkup.wav", qfalse ) );
				}
				else {
					trap_S_StartSound (NULL, es->number, CHAN_AUTO,	trap_S_RegisterSound( item->pickup_sound, qfalse ) );
				}
			}

			// show icon and name on status bar
			if ( es->number == cg.snap->ps.clientNum ) {
				CG_ItemPickup( index );
			}

		}
		break;
	case EV_GLOBAL_ITEM_PICKUP:
		DEBUGNAME("EV_GLOBAL_ITEM_PICKUP");
		{
			gitem_t	*item;
			int		index = es->eventParm;		// player predicted

			if ( index < 1 || index >= bg_numItems ) {
				break;
			}
			item = &bg_itemlist[ index ];
			if( *item->pickup_sound ) {
				// powerup pickups are global
				trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, trap_S_RegisterSound( item->pickup_sound, qfalse ) );	// FIXME: precache
			}

			// show icon and name on status bar
			if ( es->number == cg.snap->ps.clientNum ) {
				CG_ItemPickup( index );
			}
		}
		break;

	case EV_KICK:
		DEBUGNAME("EV_KICK");
		if ( es->clientNum == cg.snap->ps.clientNum ) {
			CG_WeaponFireRecoil( WP_FOOTKICK );
		}
		break;
	case EV_WOLFKICK_HIT_WALL:
		DEBUGNAME( "EV_WOLFKICK_HIT_WALL" );
		{
			vec3_t	dir;
			ByteToDir( es->eventParm, dir );
			CG_MissileHitWall( WP_FOOTKICK, 0, position, dir, 0 );	// (SA) modified to send missilehitwall surface parameters
		}
		break;
	case EV_WOLFKICK_HIT_FLESH:
		DEBUGNAME( "EV_WOLFKICK_HIT_FLESH" );
		{
			vec3_t	dir;
			ByteToDir( es->eventParm, dir );
			CG_MissileHitPlayer( cent, WP_FOOTKICK, position, dir, es->otherEntityNum2 );
		}
		break;
	case EV_WOLFKICK_MISS:
		DEBUGNAME( "EV_WOLFKICK_MISS" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.fkickmiss );
		break;
	case EV_THROWKNIFE:
		DEBUGNAME("EV_THROWKNIFE");
		CG_FireWeapon( cent, event, qfalse );
		break;

	case EV_NOFIRE_UNDERWATER:
		DEBUGNAME("EV_NOFIRE_UNDERWATER");
		if(cgs.media.noFireUnderwater)
			trap_S_StartSound (NULL, es->number, CHAN_WEAPON, cgs.media.noFireUnderwater);
		break;

// JPW NERVE
	case EV_SPINUP:
		DEBUGNAME("EV_SPINUP");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cg_weapons[es->weapon].spinupSound );
		break;

	//
	// missile impacts
	//
	case EV_MISSILE_HIT:
		DEBUGNAME("EV_MISSILE_HIT");
		{
			vec3_t	dir;

			ByteToDir( es->eventParm, dir );
			CG_MissileHitPlayer( cent, es->weapon, position, dir, es->otherEntityNum );
			if ((es->weapon == WP_KNIFE || es->weapon == WP_KNIFE_KABAR) && es->clientNum == cg.snap->ps.clientNum ){
				cg.knifeBloodVal += 45;
				if ( cg.knifeBloodVal > 255 )
					cg.knifeBloodVal = 255;
			}
			if( es->weapon == WP_MORTAR_SET || es->weapon == WP_MORTAR2_SET ) {
				if( !es->legsAnim ) {
					CG_MortarImpact( cent, position, 3, qtrue );
				}
				else {
					CG_MortarImpact( cent, position, -1, qtrue );
				}
			}
		}
		break;
	case EV_MISSILE_MISS_SMALL:
		DEBUGNAME("EV_MISSILE_MISS");
		{
			vec3_t	dir;

			ByteToDir( es->eventParm, dir );
			CG_MissileHitWallSmall( es->weapon, 0, position, dir );
		}
		break;
	case EV_MISSILE_MISS:
		DEBUGNAME("EV_MISSILE_MISS");
		{
			vec3_t	dir;

			ByteToDir( es->eventParm, dir );
			CG_MissileHitWall( es->weapon, 0, position, dir, 0 );	// (SA) modified to send missilehitwall surface parameters
			if ((es->weapon == WP_KNIFE || es->weapon == WP_KNIFE_KABAR) && es->clientNum == cg.snap->ps.clientNum ){
				cg.knifeBloodVal -= 10;
				if ( cg.knifeBloodVal < 0 )
					cg.knifeBloodVal = 0;
			}
			if( es->weapon == WP_MORTAR_SET || es->weapon == WP_MORTAR2_SET) {
				if( !es->legsAnim ) {
					CG_MortarImpact( cent, position, 3, qtrue );
				}
				else {
					CG_MortarImpact( cent, position, -1, qtrue );
				}
			}
		}
		break;
	case EV_MISSILE_MISS_LARGE:
		DEBUGNAME("EV_MISSILE_MISS_LARGE");
		{
			vec3_t	dir;

			ByteToDir( es->eventParm, dir );
			if( es->weapon == WP_ARTY || es->weapon == WP_SMOKE_MARKER ) {
				CG_MissileHitWall( es->weapon, 0, position, dir, 0);	// (SA) modified to send missilehitwall surface parameters
			}
			else {
				CG_MissileHitWall( VERYBIGEXPLOSION, 0, position, dir, 0);	// (SA) modified to send missilehitwall surface parameters
			}
		}
		break;

	case EV_MORTAR_IMPACT:
		DEBUGNAME("EV_MORTAR_IMPACT");
		CG_MortarImpact( cent, position, rand()%3, qfalse );
		break;
	case EV_MORTAR_MISS:
		DEBUGNAME("EV_MORTAR_MISS");
		CG_MortarMiss( cent, position );
		break;


	case EV_SHOVE_SOUND:
		DEBUGNAME("EV_SHOVE_SOUND");
		if ( cg_shoveSounds.integer ){
			trap_S_StartSoundVControl( NULL, es->number, CHAN_AUTO, cgs.media.shoveSound, 255 );
		}
		break;
	case EV_GENERAL_SOUND:
		DEBUGNAME("EV_GENERAL_SOUND");
		{
			sfxHandle_t	sound = CG_GetGameSound( es->eventParm );

			if ( sound ) {
				trap_S_StartSoundVControl( NULL, es->number, CHAN_VOICE, sound, 255 );
			}
			else {
				if ( es->eventParm >= GAMESOUND_MAX ) {
					if ( cgs.csMethod == 0 ) {
						// s = CG_ConfigString( CS_SOUNDS + (es->eventParm) );
						s = CG_ConfigString( CS_SOUNDS + (es->eventParm-GAMESOUND_MAX) );
					}
					else {
						s = (char *)&cgs.gameSoundNames[ es->eventParm-GAMESOUND_MAX ];
					}
					if( CG_SoundPlaySoundScript( s, NULL, es->number, qfalse ) ) {
						break;
					}
					sound = CG_CustomSound( es->number, s );
					if ( sound ) {
						trap_S_StartSoundVControl( NULL, es->number, CHAN_VOICE, sound, 255 );
					}
				}
			}
		}
		break;
	case EV_GENERAL_SOUND_VOLUME:
		DEBUGNAME("EV_GENERAL_SOUND_VOLUME");
		{
			int			volume = es->onFireStart;
			sfxHandle_t	sound = CG_GetGameSound( es->eventParm );

			if ( sound ) {
				trap_S_StartSoundVControl( NULL, es->number, CHAN_VOICE, sound, volume );
			}
			else {
				if ( es->eventParm >= GAMESOUND_MAX ) {
					if ( cgs.csMethod == 0 ) {
						s = CG_ConfigString( CS_SOUNDS + (es->eventParm-GAMESOUND_MAX) );
					}
					else {
						s = (char *)&cgs.gameSoundNames[ es->eventParm-GAMESOUND_MAX ];
					}
					if( CG_SoundPlaySoundScript( s, NULL, es->number, qfalse ) ) {
						break;
					}
					sound = CG_CustomSound( es->number, s );
					if ( sound ) {
						trap_S_StartSoundVControl( NULL, es->number, CHAN_VOICE, sound, volume );
					}
				}
			}
		}
		break;
	case EV_GLOBAL_TEAM_SOUND:
		DEBUGNAME("EV_GLOBAL_TEAM_SOUND");
		if( cgs.clientinfo[ cg.snap->ps.clientNum ].team != es->teamNum ) {
			break;
		}
	case EV_GLOBAL_SOUND:	// play from the player's head so it never diminishes
		DEBUGNAME("EV_GLOBAL_SOUND");
		{
			sfxHandle_t	sound = CG_GetGameSound( es->eventParm );

			// core: killingspree/multikill name announces..
			if ( cg_killAnnouncer.integer & KILLANNOUNCE_SPREES ) {
				// is this a sound for a spree-/multi-kill?
				if ( es->effect3Time == -1966 ) {
					if ( es->effect1Time >=0 && es->effect1Time < MAX_CLIENTS ) {
						clientInfo_t ci = cgs.clientinfo[es->effect1Time];
						if ( ci.infoValid ) {
							char		*str = va("^7%s", ci.name);
							qboolean	spree = (es->effect2Time & 0x80)? qtrue : qfalse;
							int			level = es->effect2Time & 0x7F;

							if ( spree ) {
								CG_AddAnnouncer( str, (char *)SpreeMsg[level], 0, 0.5, 4000, 1,0,0, ANNOUNCER_TOP );
							}
							else {
								CG_AddAnnouncer( str, (char *)MultikillMsg[level], 0, 0.5, 4000, 1,0,0, ANNOUNCER_TOP );
							}
						}
					}
				}
			}

			if ( sound ) {
				trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, sound );
			}
			else {
				if ( es->eventParm >= GAMESOUND_MAX ) {
					if ( cgs.csMethod == 0 ) {
						s = CG_ConfigString( CS_SOUNDS + (es->eventParm-GAMESOUND_MAX) );
					}
					else {
						s = (char *)&cgs.gameSoundNames[ es->eventParm-GAMESOUND_MAX ];
					}
					if( CG_SoundPlaySoundScript( s, NULL, -1, qtrue ) ) {
						break;
					}
					sound = CG_CustomSound( es->number, s );
					if ( sound ) {
						trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, sound );
					}
				}
			}
		}
		break;
	// DHM - Nerve
	case EV_GLOBAL_CLIENT_SOUND:
		DEBUGNAME("EV_GLOBAL_CLIENT_SOUND");
		{
			if ( cg.snap->ps.clientNum == es->teamNum ) {
				sfxHandle_t sound = CG_GetGameSound( es->eventParm );
				if ( sound ) {
					trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, sound );
				}
				else {
					if ( es->eventParm >= GAMESOUND_MAX ) {
						if ( cgs.csMethod == 0 ) {
							s = CG_ConfigString( CS_SOUNDS + (es->eventParm-GAMESOUND_MAX) );
						}
						else {
							s = (char *)&cgs.gameSoundNames[ es->eventParm-GAMESOUND_MAX ];
						}
						if( CG_SoundPlaySoundScript( s, NULL, -1, (es->effect1Time ? qfalse : qtrue) ) ) {
							break;
						}
						sound = CG_CustomSound( es->number, s );
						if ( sound ) {
							trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, sound );
						}
					}
				}
			}
		}
		break;
	// dhm - end

	case EV_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
		DEBUGNAME("EV_PAIN");
		if ( cent->currentState.number != cg.snap->ps.clientNum ) {
			CG_PainEvent( cent, es->eventParm, qfalse );
		}
		break;
	case EV_CROUCH_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
		DEBUGNAME("EV_CROUCH_PAIN");
		if ( cent->currentState.number != cg.snap->ps.clientNum ) {
			CG_PainEvent( cent, es->eventParm, qtrue );
		}
		break;
	case EV_BODY_DP:
		DEBUGNAME("EV_BODY_DP");
		if ( &cg_entities[es->otherEntityNum2] && &cg_entities[es->otherEntityNum2].pe )
			memset(&cg_entities[es->otherEntityNum2].pe,0,sizeof(playerEntity_t));
		break;
	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
	case EV_DEATH4:
		DEBUGNAME("EV_DEATHx");
		{
			int				c;
			weaponInfo_t	*weapon = &cg_weapons[es->weapon];

			cg_entities[es->otherEntityNum2].pe.deathTime = cg.time;

			if ( !weapon->deathBySound[0] ){
				break;
			}

			c = (event - EV_DEATH1) % weapon->deathBySoundNum;

			if ( weapon->deathBySound[c] ) {
				trap_S_StartSound( NULL, es->number, CHAN_WEAPON, weapon->deathBySound[c] );

				if ( !weapon->deathBySoundFar[0] )
					break;

				c = (event - EV_DEATH1) % weapon->deathBySoundFarNum;

				if( weapon->deathBySoundFar[c])	{// check for echo
					float				gdist;
					// JPW NERVE copied here for mg42 SFX event
					vec3_t				porg, gorg, norm;	// player/gun origin

					VectorCopy(es->pos.trBase, gorg);
					VectorCopy(cg.refdef_current->vieworg, porg);
					VectorSubtract(gorg, porg, norm);
					gdist = VectorNormalize(norm);

					if(gdist > 512 && gdist < 4096) {
						VectorMA(cg.refdef_current->vieworg, 64, norm, gorg);	// sound-on-a-stick
						trap_S_StartSoundEx( gorg, es->number, CHAN_WEAPON, weapon->deathBySoundFar[c], SND_NOCUT);
					}
				}
			}


		}
		break;

	case EV_BOUNCE_SOUND:
		DEBUGNAME("EV_BOUNCE_SOUND");
		if (cg_weaponBounceSound.integer){
			weaponInfo_t	*weapon = &cg_weapons[es->weapon];

			if ( (!es->eventParm && weapon->bounceSound) || (es->eventParm  && weapon->clipBounceSound ))
			trap_S_StartSound( NULL, es->number, CHAN_WEAPON, es->eventParm ? weapon->clipBounceSound : weapon->bounceSound );
		}
		break;

	case EV_SPAWN:
		DEBUGNAME("EV_SPAWN");
		cent->pe.spawnTime = cg.time;
		break;

	case EV_OBITUARY:
		DEBUGNAME("EV_OBITUARY");
		CG_Obituary( es );
		break;

	case EV_LOSE_HAT:
		DEBUGNAME("EV_LOSE_HAT");
		{
			vec3_t				dir;
			ByteToDir( es->eventParm, dir );
			if ( cent->currentState.powerups & (1<<PW_HELMETSHIELD) ) {
				CG_LoseACC( cent, tv(0, 0, 1), ACC_SHIELD, "tag_mouth", qtrue );
				CG_LoseACC( cent, dir, ACC_HAT, "tag_mouth", qtrue );
			}
			else {
				CG_LoseACC( cent, dir, cgs.clientinfo[clientNum].rank >= NUM_EXPERIENCE_LEVELS-1 ? ACC_MOUTH2 : ACC_HAT, "tag_mouth", qtrue );
			}
		}
		break;

	case EV_GIB_PLAYER:
		DEBUGNAME("EV_GIB_PLAYER");
		{
			vec3_t	dir;
			trap_S_StartSound( es->pos.trBase, -1, CHAN_AUTO, cgs.media.gibSound );
			ByteToDir( es->eventParm, dir );
			CG_GibPlayer( cent, cent->lerpOrigin, dir );
		}
		break;


	// JPW NERVE -- swiped from SP/Sherman
	case EV_STOPSTREAMINGSOUND:
		DEBUGNAME("EV_STOPSTREAMINGSOUND");
		trap_S_StartSoundEx( NULL, es->number, CHAN_WEAPON, 0, SND_CUTOFF_ALL );	// kill weapon sound (could be reloading)
		break;
	case EV_STOPLOOPINGSOUND:
		DEBUGNAME("EV_STOPLOOPINGSOUND");
		es->loopSound = 0;
		break;


	case EV_RAILTRAIL:
		DEBUGNAME("EV_RAILTRAIL");
		{
			vec3_t color =  {es->angles[0]/255.f, es->angles[1]/255.f, es->angles[2]/255.f };

			CG_RailTrail( color, es->origin2, es->pos.trBase, es->dmgFlags, es->effect1Time );
		}
		break;

	// Rafael particles
	case EV_SMOKE:
		DEBUGNAME("EV_SMOKE");
		if (cent->currentState.density == 3) {
			CG_ParticleSmoke (cgs.media.smokePuffShaderdirty, cent);
		}
		else if (!(cent->currentState.density)) {
			CG_ParticleSmoke (cgs.media.smokePuffShader, cent);
		}
		else {
			CG_ParticleSmoke (cgs.media.smokePuffShader, cent);
		}
		break;

	case EV_RUMBLE_EFX:
		DEBUGNAME("EV_RUMBLE_EFX");
		{
			float pitch = cent->currentState.angles[0];
			float yaw = cent->currentState.angles[1];
			CG_RumbleEfx ( pitch, yaw );
		}
		break;

	case EV_EMITTER:
		DEBUGNAME("EV_EMITTER");
		{
			localEntity_t	*le = CG_AllocLocalEntity();

			le->leType = LE_EMITTER;
			le->startTime = cg.time;
			le->endTime = le->startTime + 20000;
			le->pos.trType = TR_STATIONARY;
			VectorCopy( cent->currentState.origin, le->pos.trBase );
			VectorCopy( cent->currentState.origin2, le->angles.trBase );
			le->ownerNum = 0;
		}
		break;

	case EV_MG42EFX:
		DEBUGNAME("EV_MG42EFX");
		CG_MG42EFX (cent);
		break;

	// for func_exploding
	case EV_EXPLODE:
		DEBUGNAME("EV_EXPLODE");
		{
			vec3_t	dir;
			ByteToDir( es->eventParm, dir );
			CG_Explode(cent, position, dir, 0);
		}
		break;

	case EV_RUBBLE:
		DEBUGNAME("EV_RUBBLE");
		{
			vec3_t	dir;
			ByteToDir( es->eventParm, dir );
			CG_Rubble(cent, position, dir, 0);
		}
		break;

	// Gordon: debris test
	case EV_DEBRIS:
		DEBUGNAME("EV_DEBRIS");
		CG_Debris( cent, position, cent->currentState.origin2 );
		break;
	// ===================

	// for target_effect
	case EV_EFFECT:
		DEBUGNAME("EV_EFFECT");
		{
			vec3_t	dir;
			ByteToDir( es->eventParm, dir );
			CG_Effect(cent, position, dir);
		}
		break;

	case EV_MORTAREFX:	// mortar firing
		DEBUGNAME("EV_MORTAREFX");
		CG_MortarEFX (cent);
		break;

	case EV_DISGUISE_SOUND:
		DEBUGNAME("EV_DISGUISE_SOUND");
		trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.uniformPickup );
		break;
	case EV_BUILDDECAYED_SOUND:
		DEBUGNAME("EV_BUILDDECAYED_SOUND");
		trap_S_StartSound( cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.media.buildDecayedSound );
		break;

	case EV_SHAKE:
		DEBUGNAME("EV_SHAKE");
		{
			vec3_t v;
			float len;

			VectorSubtract( cg.snap->ps.origin, cent->lerpOrigin, v );
			len = VectorLength (v);

			if(len > cent->currentState.onFireStart) {
				break;
			}

			len = 1.0f - (len / (float)cent->currentState.onFireStart);
			len = min(1.f, len);

			CG_StartShakeCamera( len );
		}

		break;

	case EV_ALERT_SPEAKER:
		DEBUGNAME("EV_ALERT_SPEAKER");
		switch( cent->currentState.otherEntityNum2 )
		{
			case 1:		CG_UnsetActiveOnScriptSpeaker( cent->currentState.otherEntityNum );	break;
			case 2:		CG_SetActiveOnScriptSpeaker( cent->currentState.otherEntityNum );	break;
			case 0:
			default:	CG_ToggleActiveOnScriptSpeaker( cent->currentState.otherEntityNum );break;
		}
		break;

	case EV_POPUPMESSAGE:
		DEBUGNAME("EV_POPUPMESSAGE");
		{
			const char* str = CG_GetPMItemText( cent );

			if( str ) {
				qhandle_t shader = CG_GetPMItemIcon( cent );
				CG_AddPMItem( cent->currentState.effect1Time, str, shader, NULL );
			}
			CG_PlayPMItemSound( cent );
		}
		break;
	case EV_MISSIONMESSAGE:
		DEBUGNAME("EV_MISSIONMESSAGE");
		{
			const char* str = CG_GetMMItemText( cent );

			if( str ) {
				qhandle_t shader = CG_GetMMItemIcon( cent );
				CG_AddMMItem( cent->currentState.effect1Time, str, shader, NULL, cent->currentState.weapon );
			}
			if ( cent->currentState.loopSound ) {
				CG_PlayMMItemSound( cent );
			}
		}
		break;
	case EV_AIRSTRIKEMESSAGE:
		DEBUGNAME("EV_AIRSTRIKEMESSAGE");
		{
			const char* wav = NULL;

			switch( cent->currentState.density ) {
				case 0: // too many called
					if( cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_AXIS ) {
						wav = "axis_hq_airstrike_denied";
					}
					else {
						wav = "allies_hq_airstrike_denied";
					}
					break;
				case 1: // aborting can't see target
					if( cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_AXIS ) {
						wav = "axis_hq_airstrike_abort";
					}
					else {
						wav = "allies_hq_airstrike_abort";
					}
					break;
				case 2: // firing for effect
					if( cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_AXIS ) {
						wav = "axis_hq_airstrike";
					}
					else {
						wav = "allies_hq_airstrike";
					}
					break;
			}
			if( wav ) {
				CG_SoundPlaySoundScript( wav, NULL, -1, (es->effect1Time ? qfalse : qtrue) );
			}
		}
		break;
	case EV_ARTYMESSAGE:
		DEBUGNAME("EV_ARTYMESSAGE");
		{
			const char* wav = NULL;
			switch( cent->currentState.density ) {
				case 0: // too many called
					if( cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_AXIS ) {
						wav = "axis_hq_ffe_denied";
					}
					else {
						wav = "allies_hq_ffe_denied";
					}
					break;
				case 1: // aborting can't see target
					if( cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_AXIS ) {
						wav = "axis_hq_ffe_abort";
					}
					else {
						wav = "allies_hq_ffe_abort";
					}
					break;
				case 2: // firing for effect
					if( cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_AXIS ) {
						wav = "axis_hq_ffe";
					}
					else {
						wav = "allies_hq_ffe";
					}
					break;
			}
			if( wav ) {
				CG_SoundPlaySoundScript( wav, NULL, -1, (es->effect1Time ? qfalse : qtrue) );
			}
		}
		break;

	case EV_MEDIC_CALL:
		DEBUGNAME("EV_MEDIC_CALL");
		switch( cgs.clientinfo[ cent->currentState.number ].team ) {
			case TEAM_AXIS:
				trap_S_StartSound( NULL, cent->currentState.number, CHAN_AUTO, cgs.media.sndMedicCall[0] );
				break;
			case TEAM_ALLIES:
				trap_S_StartSound( NULL, cent->currentState.number, CHAN_AUTO, cgs.media.sndMedicCall[1] );
				break;
			default: // shouldn't happen
				break;
		}
		break;

	case EV_FX_SOUND:
		DEBUGNAME("EV_FX_SOUND");
		{
			sfxHandle_t sound;
			int			index = es->eventParm;

			if ( index < FXTYPE_WOOD || index >= FXTYPE_MAX ) {
				index = 0;
			}

			sound = (random()*fxSounds[ index ].max);
			if( fxSounds[ index ].sound[ sound ] == -1 ) {
				fxSounds[ index ].sound[ sound ] = trap_S_RegisterSound( fxSounds[ index ].soundfile[ sound ], qfalse );
			}

			sound = fxSounds[ index ].sound[ sound ];
			trap_S_StartSoundVControl( NULL, es->number, CHAN_VOICE, sound, 255 );
		}
		break;

	case EV_WEAPALT:
		DEBUGNAME("EV_WEAPALT");
		if ( es->number == cg.snap->ps.clientNum )
			CG_FinishWeaponChange( cg.weaponSelect, es->eventParm );
		break;

	case EV_PRIVATE_MESSAGE:
		DEBUGNAME("EV_PRIVATE_MESSAGE");
		if ( es->number == cg.clientNum )
			trap_S_StartSound( NULL, es->number, CHAN_LOCAL, cgs.media.PrivateMessageBeep );
		break;

	case EV_ARTY_DETECTED:
		DEBUGNAME("EV_ARTY_DETECTED");
		// IlDuca - add cg_skillviewoptions check before play the sound
		if ( es->number == cg.clientNum && cg_skillOptions.integer & 16 ) {
				int sfx = rand()%3;
				trap_S_StartSound( NULL, es->number, CHAN_LOCAL, cgs.media.sfx_artilleryDist_1[sfx] );
		}
		break;

	// Rafael snow pvs check
	case EV_SNOW_ON:
		DEBUGNAME("EV_SNOW_ON");
		CG_SnowLink (cent, qtrue);
		break;
	case EV_SNOW_OFF:
		DEBUGNAME("EV_SNOW_OFF");
		CG_SnowLink (cent, qfalse);
		break;
	case EV_SNOWFLURRY:
		DEBUGNAME("EV_SNOWFLURRY");
		CG_ParticleSnowFlurry (cgs.media.snowShader, cent);
		break;
	case EV_ITEM_POP:
		DEBUGNAME("EV_ITEM_POP");
		break;
	case EV_ITEM_RESPAWN:
		DEBUGNAME("EV_ITEM_RESPAWN");
		break;
	case EV_OILPARTICLES:
		DEBUGNAME("EV_OILPARTICLES");
		CG_Particle_OilParticle (cgs.media.oilParticle, cent->currentState.origin, cent->currentState.origin2, cent->currentState.time, cent->currentState.density);
		break;
	case EV_OILSLICK:
		DEBUGNAME("EV_OILSLICK");
		CG_Particle_OilSlick (cgs.media.oilSlick, cent);
		break;
	case EV_OILSLICKREMOVE:
		DEBUGNAME("EV_OILSLICKREMOVE");
		CG_OilSlickRemove (cent);
		break;

	case EV_DUST:
		DEBUGNAME("EV_DUST");
		CG_ParticleDust (cent, cent->currentState.origin, cent->currentState.angles);
		break;

	case EV_SPARKS_ELECTRIC:
	case EV_SPARKS:
		DEBUGNAME("EV_SPARKS");
		{
			int numsparks;
			int	i;
			int	duration;
			float	x,y;
			float	speed;
			vec3_t	source, dest;

			if (!(cent->currentState.density)) {
				cent->currentState.density = 1;
			}

			numsparks = rand()%cent->currentState.density;
			duration = cent->currentState.frame;
			x = cent->currentState.angles2[0];
			y = cent->currentState.angles2[1];
			speed = cent->currentState.angles2[2];

			if (!numsparks) {
				numsparks = 1;
			}

			for (i=0; i<numsparks; ++i) {
				if (event == EV_SPARKS_ELECTRIC) {
					VectorCopy (cent->currentState.origin, source);

					VectorCopy (source, dest);
					dest[0] += ((rand()&31)-16);
					dest[1] += ((rand()&31)-16);
					dest[2] += ((rand()&31)-16);

					CG_Tracer (source, dest, 1);
				}
				else {
					CG_ParticleSparks (cent->currentState.origin, cent->currentState.angles, duration, x, y, speed);
				}
			}

		}
		break;

	case EV_GUNSPARKS:
		DEBUGNAME("EV_GUNSPARKS");
		{
			int numsparks = cent->currentState.density;
			int speed = cent->currentState.angles2[2];

			CG_AddBulletParticles( cent->currentState.origin, cent->currentState.angles, speed, 800, numsparks, 1.0f );
		}
		break;

	case EV_SHARD:
		DEBUGNAME("EV_SHARD");
		{
			vec3_t	dir;
			ByteToDir( es->eventParm, dir );
			CG_Shard(cent, position, dir);
		}
		break;

	case EV_JUNK:
		DEBUGNAME("EV_JUNK");
		{
			vec3_t	dir;
			ByteToDir (es->eventParm, dir);
			{
				int i;
				int	rval = rand()%3 + 3;

				for (i=0; i<rval; ++i) {
					CG_ShardJunk (cent, position, dir);
				}
			}
		}
		break;

	case EV_DEBUG_LINE:
		DEBUGNAME("EV_DEBUG_LINE");
		CG_Beam( cent );
		break;

	default:
		DEBUGNAME("UNKNOWN");
		CG_Error( "Unknown event: %i", event );
		break;
	}


	{
		int	rval = rand()&3;

		if (splashfootstepcnt != rval)
			splashfootstepcnt = rval;
		else
			splashfootstepcnt++;

		if (splashfootstepcnt > 3)
			splashfootstepcnt = 0;


		if (footstepcnt != rval)
			footstepcnt = rval;
		else
			footstepcnt++;

		if (footstepcnt > 3)
			footstepcnt = 0;
	}
}

/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents( centity_t *cent ) {
	int			i;
    qboolean	eventSkipped = qfalse;

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin, qfalse, cent->currentState.effect2Time );
	CG_SetEntitySoundPosition( cent );

	// check for event-only entities
	if ( cent->currentState.eType > ET_EVENTS ) {
		if ( cent->previousEvent ) {
			return;	// already fired
		}
		// if this is a player event set the entity number of the client entity number
		cent->previousEvent = 1;
		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	}
	else {
		// DHM - Nerve :: Entities that make it here are Not TempEntities.
		//		As far as we could tell, for all non-TempEntities, the
		//		circular 'events' list contains the valid events.  So we
		//		skip processing the single 'event' field and go straight
		//		to the circular list.
		eventSkipped = qtrue;
	}

    if(!eventSkipped) {
	    CG_EntityEvent( cent, cent->lerpOrigin );
	    // DHM - Nerve :: Temp ents return after processing
	    return;
    }

	// check the sequencial list
	// if we've added more events than can fit into the list, make sure we only add them once
	if (cent->currentState.eventSequence < cent->previousEventSequence) {
		cent->previousEventSequence -= (1 << 8);	// eventSequence is sent as an 8-bit through network stream
	}
	if (cent->currentState.eventSequence - cent->previousEventSequence > MAX_EVENTS) {
		cent->previousEventSequence = cent->currentState.eventSequence - MAX_EVENTS;
	}

	for ( i = cent->previousEventSequence ; i != cent->currentState.eventSequence; ++i ) {
		cent->currentState.event = cent->currentState.events[ i & (MAX_EVENTS-1) ];
		cent->currentState.eventParm = cent->currentState.eventParms[ i & (MAX_EVENTS-1) ];
		CG_EntityEvent( cent, cent->lerpOrigin );
	}
	cent->previousEventSequence = cent->currentState.eventSequence;

	// set the event back so we don't think it's changed next frame (unless it really has)
	cent->currentState.event = cent->previousEvent;
}
