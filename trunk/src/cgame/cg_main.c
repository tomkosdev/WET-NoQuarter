/*
 * name:	cg_main.c
 *
 * desc:	initialization and primary entry point for cgame
 *
 *
 * NQQS:	inspection required
 */

#include "cg_local.h"

#ifdef __linux__
  extern char* __progname;
#elif defined  __MACOS__
  extern char* __progname; // TODO: check this
#elif defined  WIN32
  #define Rectangle LCC_Rectangle
  #include <windows.h> // HANDLE
  #undef Rectangle

  #include <psapi.h>
#endif

// flag for engine detection - see checkExecutable()
int systemFlag = 0;

displayContextDef_t cgDC;

void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum, qboolean demoPlayback );
void CG_Shutdown( void );
qboolean CG_CheckExecKey( int key );
extern itemDef_t* g_bindItem;
extern qboolean g_waitingForKey;
void CG_ReceiveTeamInfo( unsigned char *buffer, int bufferlen, int commandTime );

#ifdef AUTO_GUID
extern void CG_InitNQGUID();
#endif

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
#if defined(__MACOS__)
#ifndef __GNUC__
#pragma export on
#endif
#endif


#if defined(__x86_64__)
intptr_t vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11  ) {
#else
int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {
#endif

#if defined(__MACOS__)
#ifndef __GNUC__
#pragma export off
#endif
#endif
	switch ( command ) {
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame( arg0, arg1, arg2 );
		return 0;
	case CG_MOUSE_EVENT:
		cgDC.cursorx = cgs.cursorX;
		cgDC.cursory = cgs.cursorY;
		// core: when the limbopanel is open, we want the cursor to be able to move all the way to the right edge of the screen..
		// ..same for the debriefing screen.
		if ( cg.showGameView || cgs.dbShowing ) {
			if (!Ccg_Is43Screen()) cgDC.cursorx *= cgs.adr43;
		}
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0, qtrue);
		return 0;
	case CG_KEY_EVENT:
		CG_KeyEvent(arg0, arg1);
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_GET_TAG:
		return CG_GetTag( arg0, (char *)arg1, (orientation_t *)arg2 );
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_CHECKEXECKEY:
		return CG_CheckExecKey( arg0 );
	case CG_WANTSBINDKEYS:
		return (g_waitingForKey && g_bindItem) ? qtrue : qfalse;
	case CG_INIT:
		EnableStackTrace();
		CG_Init( arg0, arg1, arg2, arg3 );
		cgs.initing = qfalse;
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		DisableStackTrace();
		return 0;
	case CG_MESSAGERECEIVED:
		if ( *(unsigned char*)arg0 == PACKET_S_TEAMPLAYINFOMESSAGE ) {
			// core: arguments are (buffer, buflen, commandTime)
			CG_ReceiveTeamInfo( (unsigned char*)arg0, arg1, arg2 );
			return 0;
		}
/*
		else if (*(unsigned char*)arg0 == PACKET_NQKEYMESSAGE_RECEIVED) {
			// let the client know the server has got the key ...
			return 0;
		}
*/
		return -1;
	default:
		CG_Error( "vmMain: unknown command %i", command );
		break;
	}
	return -1;
}

cg_t			cg;
cgs_t			cgs;
centity_t		cg_entities[MAX_GENTITIES];
weaponInfo_t	cg_weapons[MAX_WEAPONS];
itemInfo_t		cg_items[MAX_ITEMS];

vmCvar_t	cg_runpitch;
vmCvar_t	cg_runroll;
vmCvar_t	cg_bobup;
vmCvar_t	cg_bobpitch;
vmCvar_t	cg_bobroll;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
vmCvar_t	cg_gibs;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_drawSnapshot;
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_drawCrosshairNames;
vmCvar_t	cg_drawCrosshairPickups;
vmCvar_t	cg_cycleAllWeaps;
vmCvar_t	cg_useWeapsForZoom;
vmCvar_t	cg_crosshairSize;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_crosshairHealth;
vmCvar_t	cg_teamChatsOnly;
vmCvar_t	cg_noVoiceChats;		// NERVE - SMF
vmCvar_t	cg_noVoiceText;			// NERVE - SMF
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_drawSpreadScale;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_nopredict;
vmCvar_t	cg_noPlayerAnims;
#ifdef _DEBUG
vmCvar_t	cg_showmiss; // prediction info
vmCvar_t	cg_footsteps; // own footstep sound (delete? this is crap and cheat var)
#endif
vmCvar_t	cg_markTime;
vmCvar_t	cg_brassTime;
// vmCvar_t	cg_letterbox;//----(SA)	added IRATA: removed
vmCvar_t	cg_drawGun;
vmCvar_t	cg_cursorHints;	//----(SA)	added

#ifdef _DEBUG
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
#endif
vmCvar_t	cg_gun_foreshorten;
vmCvar_t	cg_countryflags; //mcwf GeoIP
vmCvar_t	cg_tracerChance;
vmCvar_t	cg_tracerWidth;
vmCvar_t	cg_tracerLength;
vmCvar_t	cg_tracerSpeed;
vmCvar_t	cg_autoswitch;
vmCvar_t	cg_fov;
vmCvar_t	cg_fovy;
vmCvar_t	cg_zoomStepSniper;
vmCvar_t	cg_zoomDefaultSniper;
vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_stereoSeparation; // 3d
vmCvar_t	cg_lagometer;
#ifdef ALLOW_GSYNC
vmCvar_t	cg_synchronousClients;
#endif // ALLOW_GSYNC
vmCvar_t 	cg_teamChatTime;
vmCvar_t 	cg_teamChatHeight;
#ifdef _DEBUG
vmCvar_t 	cg_stats; // displayes client frame number
#endif
vmCvar_t 	cg_buildScript;
vmCvar_t	cg_coronafardist;
vmCvar_t	cg_coronas;
vmCvar_t	cg_paused;
vmCvar_t	cg_predictItems;
vmCvar_t	cg_autoactivate;
vmCvar_t	pmove_fixed;
vmCvar_t	pmove_msec;
vmCvar_t	cg_wolfparticles;
vmCvar_t	cg_smokeparticles;
vmCvar_t	cg_impactparticles;
vmCvar_t	cg_trailparticles;
vmCvar_t	cg_gameType;
vmCvar_t	cg_bloodTime;
vmCvar_t	cg_skybox;
vmCvar_t	cg_message;
vmCvar_t	cg_messageType;
vmCvar_t	cg_messageTarget;
vmCvar_t	cg_timescale;
vmCvar_t	cg_voiceSpriteTime;	// DHM - Nerve
vmCvar_t	cg_drawCompass;
vmCvar_t	cg_drawNotifyText;
vmCvar_t	cg_quickMessageAlt;
vmCvar_t	cg_descriptiveText;
vmCvar_t	cg_redlimbotime;
vmCvar_t	cg_bluelimbotime;
vmCvar_t	cg_antilag;
vmCvar_t	developer;
vmCvar_t	authLevel;
vmCvar_t	cf_wstats;					// Font scale for +wstats window
vmCvar_t	cf_wtopshots;				// Font scale for +wtopshots window
vmCvar_t	cg_skillOptions;
vmCvar_t	cg_announcer;
vmCvar_t	cg_autoAction;
vmCvar_t	cg_autoReload;
vmCvar_t	cg_weapAltReloads;
vmCvar_t	cg_bloodDamageBlend;
vmCvar_t	cg_bloodFlash;
vmCvar_t	cg_complaintPopUp;
vmCvar_t	cg_crosshairAlpha;
vmCvar_t	cg_crosshairAlphaAlt;
vmCvar_t	cg_crosshairColor;
vmCvar_t	cg_crosshairColorAlt;
vmCvar_t	cg_crosshairPulse;
vmCvar_t	cg_drawReinforcementTime;
vmCvar_t	cg_drawWeaponIconFlash;
vmCvar_t	cg_noAmmoAutoSwitch;
vmCvar_t	cg_printObjectiveInfo;
#ifdef MV_SUPPORT
vmCvar_t	cg_specHelp;
#endif
vmCvar_t	cg_uinfo;
vmCvar_t	cg_useScreenshotJPEG;
vmCvar_t	demo_avifpsF1;
vmCvar_t	demo_avifpsF2;
vmCvar_t	demo_avifpsF3;
vmCvar_t	demo_avifpsF4;
vmCvar_t	demo_avifpsF5;
vmCvar_t	demo_drawTimeScale;
vmCvar_t	demo_infoWindow;
#ifdef MV_SUPPORT
vmCvar_t	mv_sensitivity;
#endif
vmCvar_t	int_cl_maxpackets;
vmCvar_t	int_cl_timenudge;
vmCvar_t	int_m_pitch;
vmCvar_t	int_sensitivity;
vmCvar_t	int_timescale;
vmCvar_t	int_ui_blackout;
vmCvar_t	cg_rconPassword;
vmCvar_t	cg_refereePassword;
vmCvar_t	cg_atmosphericEffects;
vmCvar_t	cg_drawRoundTimer;
vmCvar_t	cg_instanttapout;
vmCvar_t	cg_debugSkills;
vmCvar_t	cg_drawFireteamOverlay;
vmCvar_t	cg_drawSmallPopupIcons;
vmCvar_t	cg_smallFont;
//bani - demo recording cvars
vmCvar_t	cl_demorecording;
vmCvar_t	cl_demofilename;
vmCvar_t	cl_demooffset;
//bani - wav recording cvars
vmCvar_t	cl_waverecording;
vmCvar_t	cl_wavefilename;
vmCvar_t	cl_waveoffset;
vmCvar_t	cg_recording_statusline;
vmCvar_t    cg_HUDBackgroundColor;
vmCvar_t	cg_HUDBorderColor;
vmCvar_t	cg_HUDAlpha;
vmCvar_t	cg_goatSound;
vmCvar_t	cg_hitSounds;
vmCvar_t	cg_drawHUDHead; // general drawing of the HUD head (not required for NQ HUD sytle)
vmCvar_t	cg_drawObjIcons;
vmCvar_t	cg_drawPing;
vmCvar_t	cg_drawKillSpree;
vmCvar_t	cg_drawTime;
vmCvar_t	cg_drawTimeSeconds;
vmCvar_t	cg_graphicObituaries;
vmCvar_t	cg_drawCarryWeapons;
vmCvar_t	hud_powerupSize;		// iconSize in G_DrawActivePowerups()
vmCvar_t	hud_skillBarX;
vmCvar_t	hud_skillBarY;
vmCvar_t	hud_skillBarAlpha;
vmCvar_t	hud_rankX;
vmCvar_t	hud_rankY;
vmCvar_t	hud_rankAlpha;
vmCvar_t	hud_drawPowerups;
vmCvar_t	hud_drawAltHUD; // 0=vanilla, 1 = NQ style
vmCvar_t    cg_drawMuzzleFlash;
vmCvar_t	cg_drawTracers;
vmCvar_t	cg_drawHitbox;
vmCvar_t	cg_markDistance;	// used - but only once !!! TODO:  check deletion
vmCvar_t	cg_insanity;
vmCvar_t	cg_drawspeed; 		// forty - speedometer
vmCvar_t	cg_speedunit; 		// forty - speedometer
vmCvar_t	cg_optimizePrediction;
vmCvar_t	cg_locations;
vmCvar_t	cg_fixedFTeamSize;	// fixed or dynamic fire team rectangle
vmCvar_t	cg_popupMessageFilter;
vmCvar_t	cg_weaponBounceSound; // Play Weapon Bounce Sound
vmCvar_t	cg_altHQVoice;
vmCvar_t	cg_rtcwShake;
vmCvar_t	cg_limbo_secondary; // preselection for the secondary limbo weapon: 0=single, 1=dual, 2=smg(always best secondary)
vmCvar_t	cg_modelHilights;
vmCvar_t	cg_FTAutoSelect;
vmCvar_t	cg_drawAuraIcons;
vmCvar_t	cg_automapZoom;
vmCvar_t	cg_shoveSounds; // toogles shove sound 0/1
vmCvar_t	cg_logFile;
vmCvar_t	cg_pmWaitTime;
vmCvar_t	cg_pmFadeTime;
vmCvar_t	cg_pmPopupTime;
vmCvar_t	cg_pmBigPopupTime;
vmCvar_t    cg_spectator;
vmCvar_t	cg_whizzSounds; // bullets flying by (integer part = distance, fraction part = trace.fraction)
vmCvar_t	cg_favWeaponBank; // 0=previously used weapon.  banks: 1=knife, 2=pistol, 3=SMG, 4=grenade // lastUsedWeaponBank - get next weapon after using cretain weapon (like panzer,airstrike,satchel etc)
vmCvar_t    cg_spawnTimer_set;      // spawntimer from etpub
vmCvar_t    cg_spawnTimer_period;   // spawntimer from etpub


typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
	int			modificationCount;
} cvarTable_t;

#ifdef DEBUG
#define CVAR_JET 0
#else
#define CVAR_JET CVAR_CHEAT
#endif


cvarTable_t		cvarTable[] =
{
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{ &cg_buildScript,			"com_buildScript",				"0",		0				},	// force loading of all possible data amd error on failures
	{ &cg_paused,				"cl_paused",					"0",		CVAR_ROM		},

	{ &cg_gameType,				"g_gametype",					"0",		0 				}, // communicated by systeminfo
	{ &cg_antilag,				"g_antilag",					"1",		0				},

	{ &pmove_fixed,				"pmove_fixed",					"0",		CVAR_USERINFO	},
	{ &pmove_msec,				"pmove_msec",					"8",		CVAR_USERINFO	},
#ifdef HW_BAN
	{ NULL,						"cg_hwguid",					"",			CVAR_USERINFO | CVAR_ROM },
#endif // HW_BAN
	{ &cg_autoswitch,			"cg_autoswitch",				"2", 		CVAR_ARCHIVE 	},
	{ &cg_drawGun,				"cg_drawGun",					"1", 		CVAR_ARCHIVE 	},
	{ &cg_cursorHints,			"cg_cursorHints",				"1", 		CVAR_ARCHIVE 	},
	{ &cg_zoomDefaultSniper,	"cg_zoomDefaultSniper",			"20", 		CVAR_ARCHIVE 	}, // JPW NERVE changed per atvi req
	{ &cg_zoomStepSniper,		"cg_zoomStepSniper",			"2", 		CVAR_ARCHIVE 	},
	{ &cg_fov,					"cg_fov",						"90.0",		CVAR_ARCHIVE 	},
	{ &cg_fovy,					"cg_fovy",						"73.74",	CVAR_ARCHIVE 	},
// 	{ &cg_letterbox,			"cg_letterbox",					"0",		CVAR_TEMP		},	//----(SA)	added
	{ &cg_stereoSeparation,		"cg_stereoSeparation",			"0.4",		CVAR_ARCHIVE	},
	{ &cg_shadows,				"cg_shadows",					"1", 		CVAR_ARCHIVE	},
	{ &cg_gibs,					"cg_gibs",						"1", 		CVAR_ARCHIVE	},

	{ &cg_draw2D,				"cg_draw2D",					"1", 		CVAR_ARCHIVE	},
	{ &cg_drawSpreadScale,		"cg_drawSpreadScale",			"1", 		CVAR_ARCHIVE	},
	{ &cg_drawStatus,			"cg_drawStatus",				"1", 		CVAR_ARCHIVE  	},
	{ &cg_drawFPS,				"cg_drawFPS",					"0", 		CVAR_ARCHIVE  	},
	{ &cg_drawSnapshot,			"cg_drawSnapshot",				"0", 		CVAR_ARCHIVE  	},
	{ &cg_drawCrosshair,		"cg_drawCrosshair",				"1", 		CVAR_ARCHIVE 	},
	{ &cg_drawCrosshairNames,	"cg_drawCrosshairNames",		"1", 		CVAR_ARCHIVE 	},
	{ &cg_drawCrosshairPickups, "cg_drawCrosshairPickups",		"1", 		CVAR_ARCHIVE 	},
	{ &cg_useWeapsForZoom,		"cg_useWeapsForZoom",			"1", 		CVAR_ARCHIVE 	},
	{ &cg_cycleAllWeaps,		"cg_cycleAllWeaps",				"1",		CVAR_ARCHIVE 	},
	{ &cg_crosshairSize,		"cg_crosshairSize",				"48",		CVAR_ARCHIVE 	},
	{ &cg_crosshairHealth,		"cg_crosshairHealth",			"0", 		CVAR_ARCHIVE 	},
	{ &cg_crosshairX,			"cg_crosshairX",				"0", 		CVAR_ARCHIVE 	},
	{ &cg_crosshairY,			"cg_crosshairY",				"0",		CVAR_ARCHIVE 	},
	{ &cg_brassTime,			"cg_brassTime",					"2500",		CVAR_ARCHIVE 	}, // JPW NERVE
	{ &cg_markTime,				"cg_marktime",					"20000",	CVAR_ARCHIVE 	},
	{ &cg_lagometer,			"cg_lagometer",					"0",		CVAR_ARCHIVE 	},

	{ &cg_gun_foreshorten, 		"cg_gun_foreshorten", 			"0", 		CVAR_TEMP 		},
	{ &cg_swingSpeed,			"cg_swingSpeed",				"0.1",		CVAR_CHEAT		},	// was 0.3 for Q3
	{ &cg_skybox,				"cg_skybox",					"1",		CVAR_CHEAT		},
	{ &cg_animSpeed,			"cg_animspeed",					"1",		CVAR_CHEAT 		},
	{ &cg_debugEvents,			"cg_debugevents",				"0",		CVAR_CHEAT 		},
	{ &cg_nopredict,			"cg_nopredict",					"0",		CVAR_CHEAT 		},
	{ &cg_noPlayerAnims,		"cg_noplayeranims",				"0",		CVAR_CHEAT 		},
#ifdef _DEBUG
	{ &cg_footsteps,			"cg_footsteps",					"1",		CVAR_CHEAT 		},
#endif
	{ &cg_tracerChance,			"cg_tracerchance",				"0.4",		CVAR_CHEAT 		},
	{ &cg_tracerWidth,			"cg_tracerwidth",				"0.8",		CVAR_CHEAT 		},
	{ &cg_tracerSpeed,			"cg_tracerSpeed",				"4500",		CVAR_CHEAT 		},
	{ &cg_tracerLength,			"cg_tracerlength",				"160",		CVAR_CHEAT 		},
	{ &cg_drawHitbox,			"cg_drawHitbox",				"0",		CVAR_CHEAT		},
	{ &cg_debugAnim,			"cg_debuganim",					"0",		CVAR_JET 		},
	{ &cg_debugPosition,		"cg_debugposition",				"0",		CVAR_JET 		},
	{ &cg_thirdPersonRange,		"cg_thirdPersonRange",			"80",		CVAR_JET 		}, // JPW NERVE per atvi req
	{ &cg_thirdPersonAngle,		"cg_thirdPersonAngle",			"0",		CVAR_JET 		},
	{ &cg_thirdPerson,			"cg_thirdPerson",				"0",		CVAR_JET		},
	{ &developer,				"developer",					"0",		CVAR_JET		},
#ifdef ALLOW_GSYNC
	{ &cg_synchronousClients,	"g_synchronousClients",			"0",		CVAR_SYSTEMINFO | CVAR_CHEAT },	// communicated by systeminfo
#endif // ALLOW_GSYNC
#ifdef _DEBUG
	{ &cg_gun_x, 				"cg_gunX", 						"0", 		CVAR_CHEAT 		},
	{ &cg_gun_y, 				"cg_gunY", 						"0", 		CVAR_CHEAT 		},
	{ &cg_gun_z, 				"cg_gunZ", 						"0", 		CVAR_CHEAT 		},
#endif

	{ &cg_timescale,			"timescale",					"1",		0				},

	{ &cg_countryflags,			"cg_countryflags",				"1",		CVAR_ARCHIVE 	}, //mcwf GeoIP

	{ &cg_runpitch, 			"cg_runpitch",					"0.002", 	CVAR_ARCHIVE	},
	{ &cg_runroll,				"cg_runroll",					"0.005", 	CVAR_ARCHIVE 	},
	{ &cg_bobup ,				"cg_bobup",						"0.005", 	CVAR_ARCHIVE 	},
	{ &cg_bobpitch, 			"cg_bobpitch",					"0.002", 	CVAR_ARCHIVE 	},
	{ &cg_bobroll,				"cg_bobroll",					"0.002", 	CVAR_ARCHIVE 	},

	// JOSEPH 10-27-99
	{ &cg_autoactivate,			"cg_autoactivate",				"1",		CVAR_ARCHIVE	},
	// END JOSEPH

	{ &cg_bloodTime,			"cg_bloodTime",					"120",		CVAR_ARCHIVE	},

	// ydnar: say, team say, etc.
	{ &cg_message,				"cg_message",					"1",		CVAR_TEMP 		},
	{ &cg_messageType,			"cg_messageType",				"1",		CVAR_TEMP 		},
	{ &cg_messageTarget,		"cg_messageTarget",				"",			CVAR_TEMP 		},

	{ &cg_errorDecay,			"cg_errordecay",				"100",		0				},
#ifdef _DEBUG
	{ &cg_showmiss,				"cg_showmiss",					"0",		0				},
#endif
	{ &cg_teamChatTime,			"cg_teamChatTime",				"8000",		CVAR_ARCHIVE  	},
	{ &cg_teamChatHeight,		"cg_teamChatHeight",			"8",		CVAR_ARCHIVE  	},
	{ &cg_coronafardist,		"cg_coronafardist",				"1536",		CVAR_ARCHIVE 	},
	{ &cg_coronas,				"cg_coronas",					"1",		CVAR_ARCHIVE 	},
	{ &cg_predictItems,			"cg_predictItems",				"1",		CVAR_ARCHIVE 	},
#ifdef _DEBUG
	{ &cg_stats,				"cg_stats",						"0",		0				},
#endif
	{ &cg_voiceSpriteTime,		"cg_voiceSpriteTime",			"6000",		CVAR_ARCHIVE	},		// DHM - Nerve
	{ &cg_teamChatsOnly,		"cg_teamChatsOnly",				"0",		CVAR_ARCHIVE	},
	{ &cg_noVoiceChats,			"cg_noVoiceChats",				"0",		CVAR_ARCHIVE	},		// NERVE - SMF
	{ &cg_noVoiceText,			"cg_noVoiceText",				"0",		CVAR_ARCHIVE	},		// NERVE - SMF

	//{ &cg_blood,				"cg_showblood",					"1",		CVAR_ARCHIVE	},

	// Rafael - particle switch
	{ &cg_wolfparticles,		"cg_wolfparticles",				"1",		CVAR_ARCHIVE	},
	{ &cg_smokeparticles,		"cg_smokeparticles",			"1",		CVAR_ARCHIVE	},
	{ &cg_trailparticles,		"cg_trailparticles",			"1",		CVAR_ARCHIVE	},
	{ &cg_impactparticles,		"cg_impactparticles",			"1",		CVAR_ARCHIVE	},
	{ &cg_bluelimbotime,		"", 							"30000",	0 				}, // communicated by systeminfo
	{ &cg_redlimbotime,			"", 							"30000",	0 				}, // communicated by systeminfo
	{ &cg_drawCompass,			"cg_drawCompass",				"1",		CVAR_ARCHIVE 	},
	{ &cg_drawNotifyText,		"cg_drawNotifyText",			"1",		CVAR_ARCHIVE 	},
	{ &cg_quickMessageAlt,		"cg_quickMessageAlt",			"0",		CVAR_ARCHIVE 	},
	{ &cg_descriptiveText,		"cg_descriptiveText",			"0",		CVAR_ARCHIVE 	},
	{ &cf_wstats,				"cf_wstats",					"1.2",		CVAR_ARCHIVE	},
	{ &cf_wtopshots,			"cf_wtopshots",					"1.0",		CVAR_ARCHIVE	},
	{ &cg_announcer,			"cg_announcer",					"1",		CVAR_ARCHIVE	},
	{ &cg_autoAction,			"cg_autoAction", 				"0",		CVAR_ARCHIVE	},
	{ &cg_autoReload,			"cg_autoReload", 				"1",		CVAR_ARCHIVE	},
	{ &cg_weapAltReloads,		"cg_weapAltReloads",			"1",		CVAR_ARCHIVE	},
	{ &cg_bloodDamageBlend,		"cg_bloodDamageBlend",			"1.0",		CVAR_ARCHIVE	},
	{ &cg_bloodFlash,			"cg_bloodFlash",				"1.0",		CVAR_ARCHIVE	},
	{ &cg_complaintPopUp,		"cg_complaintPopUp",			"1",		CVAR_ARCHIVE 	},
	{ &cg_crosshairAlpha,		"cg_crosshairAlpha",			"1.0", 		CVAR_ARCHIVE 	},
	{ &cg_crosshairAlphaAlt,	"cg_crosshairAlphaAlt",			"1.0", 		CVAR_ARCHIVE 	},
	{ &cg_crosshairColor,		"cg_crosshairColor",			"White",	CVAR_ARCHIVE 	},
	{ &cg_crosshairColorAlt,	"cg_crosshairColorAlt",			"White",	CVAR_ARCHIVE 	},
	{ &cg_crosshairPulse,		"cg_crosshairPulse",			"1", 		CVAR_ARCHIVE 	},
	{ &cg_drawReinforcementTime,"cg_drawReinforcementTime",		"1", 		CVAR_ARCHIVE 	},
	{ &cg_drawWeaponIconFlash,	"cg_drawWeaponIconFlash",		"0", 		CVAR_ARCHIVE 	},
	{ &cg_noAmmoAutoSwitch,		"cg_noAmmoAutoSwitch",			"1", 		CVAR_ARCHIVE 	},
	{ &cg_printObjectiveInfo,	"cg_printObjectiveInfo",		"1",		CVAR_ARCHIVE 	},
#ifdef MV_SUPPORT
	{ &cg_specHelp,				"cg_specHelp",					"1",		CVAR_ARCHIVE 	},
#endif
	{ &cg_uinfo,				"cg_uinfo",						"0",		CVAR_ROM | CVAR_USERINFO },
	{ &cg_useScreenshotJPEG,	"cg_useScreenshotJPEG",			"1",		CVAR_ARCHIVE	},
	{ &demo_avifpsF1, 			"demo_avifpsF1", 				"0",		CVAR_ARCHIVE 	},
	{ &demo_avifpsF2, 			"demo_avifpsF2", 				"10",		CVAR_ARCHIVE 	},
	{ &demo_avifpsF3, 			"demo_avifpsF3", 				"15",		CVAR_ARCHIVE 	},
	{ &demo_avifpsF4, 			"demo_avifpsF4", 				"20",		CVAR_ARCHIVE 	},
	{ &demo_avifpsF5, 			"demo_avifpsF5", 				"24",		CVAR_ARCHIVE 	},
	{ &demo_drawTimeScale,		"demo_drawTimeScale",			"1",		CVAR_ARCHIVE 	},
	{ &demo_infoWindow,			"demo_infoWindow",				"1",		CVAR_ARCHIVE 	},

#ifdef MV_SUPPORT
	{ &mv_sensitivity,			"mv_sensitivity",				"20",		CVAR_ARCHIVE	},
#endif
	// Engine mappings
	{ &int_cl_maxpackets,		"cl_maxpackets",				"30",		CVAR_ARCHIVE 	},
	{ &int_cl_timenudge,		"cl_timenudge",					"0",		CVAR_ARCHIVE 	},
	{ &int_m_pitch,				"m_pitch",						"0.022",	CVAR_ARCHIVE 	},
	{ &int_sensitivity,			"sensitivity",					"5",		CVAR_ARCHIVE 	},
	{ &int_ui_blackout,			"ui_blackout",					"0",		CVAR_ROM		},
	// -OSP
	{ &cg_atmosphericEffects,	"cg_atmosphericEffects",		"1",		CVAR_ARCHIVE	},
	{ &authLevel,				"authLevel",					"0",		CVAR_TEMP | CVAR_ROM},
	{ &cg_rconPassword,			"auth_rconPassword",			"",			CVAR_TEMP		},
	{ &cg_refereePassword,		"auth_refereePassword",			"",			CVAR_TEMP		},
	{ &cg_drawRoundTimer,		"cg_drawRoundTimer",			"1",		CVAR_ARCHIVE	},
	// Gordon: optimization cvars: 18/12/02 enabled by default now
	{ &cg_instanttapout,		"cg_instanttapout",				"0",		CVAR_ARCHIVE	},
	{ &cg_debugSkills,			"cg_debugSkills",				"0",		0				},
	{ &cg_drawFireteamOverlay,	"cg_drawFireteamOverlay", 		"1",		CVAR_ARCHIVE 	},
	{ &cg_drawSmallPopupIcons,	"cg_drawSmallPopupIcons", 		"1",		CVAR_ARCHIVE 	},
	{ &cg_smallFont,			"cg_smallFont",					"8",		CVAR_ARCHIVE 	},
	//bani - demo recording cvars
	{ &cl_demorecording,		"cl_demorecording", 			"0",		CVAR_ROM 		},
	{ &cl_demofilename,			"cl_demofilename",				"",			CVAR_ROM 		},
	{ &cl_demooffset,			"cl_demooffset",				"0",		CVAR_ROM 		},
	//bani - wav recording cvars
	{ &cl_waverecording,		"cl_waverecording",				"0",		CVAR_ROM 		},
	{ &cl_wavefilename,			"cl_wavefilename",				"",			CVAR_ROM 		},
	{ &cl_waveoffset,			"cl_waveoffset",				"0",		CVAR_ROM 		},
	{ &cg_recording_statusline, "cg_recording_statusline",		"9",		CVAR_ARCHIVE	},
	{ &cg_drawKillSpree,		"cg_drawKillSpree",				"2",		CVAR_ARCHIVE	},
	{ &cg_drawPing,				"cg_drawPing",					"0",		CVAR_ARCHIVE	},
	{ &cg_drawTime,				"cg_drawTime",					"1",		CVAR_ARCHIVE	},
	{ &cg_drawTimeSeconds,		"cg_drawTimeSeconds",			"0",		CVAR_ARCHIVE	},
	{ &cg_graphicObituaries,	"cg_graphicObituaries",			"0",		CVAR_ARCHIVE	},
	{ &cg_drawCarryWeapons,		"cg_drawCarryWeapons",			"0",		CVAR_ARCHIVE	},
	{ &cg_insanity,				"",			                    "0",		0				},
	{ &cg_HUDBackgroundColor,	"cg_HUDBackgroundColor",		".16 .2 .17",	CVAR_ARCHIVE },
	{ &cg_HUDBorderColor,		"cg_HUDBorderColor",			".5  .5 .5",	CVAR_ARCHIVE },
	{ &cg_HUDAlpha,				"cg_HUDAlpha",					"0.8",		CVAR_ARCHIVE	},
	{ &cg_hitSounds,			"cg_hitSounds",					"1",		CVAR_ARCHIVE	},
	{ &cg_goatSound,			"cg_goatSound",					"3",		CVAR_ARCHIVE	},
	{ &cg_drawHUDHead,			"cg_drawHUDHead",				"1",		CVAR_ARCHIVE	},
	{ &cg_drawObjIcons,			"cg_drawObjIcons",				"0",		CVAR_ARCHIVE	},
	{ &cg_markDistance,			"cg_markDistance",				"384",		CVAR_ARCHIVE	},
	{ &hud_powerupSize,			"hud_powerupSize",				"18",		CVAR_ARCHIVE	},
	{ &hud_skillBarX,			"hud_skillBarX",				"44",		CVAR_ARCHIVE	},
	{ &hud_skillBarY,			"hud_skillBarY",				"388",		CVAR_ARCHIVE	},
	{ &hud_skillBarAlpha,		"hud_skillBarAlpha",			"1.0",		CVAR_ARCHIVE	},
	{ &hud_rankX,				"hud_rankX",					"112",		CVAR_ARCHIVE	},
	{ &hud_rankY,				"hud_rankY",					"408",		CVAR_ARCHIVE	},
	{ &hud_rankAlpha,			"hud_rankAlpha",				"1.0",		CVAR_ARCHIVE	},
	{ &hud_drawPowerups,		"hud_drawPowerups",				"1",		CVAR_ARCHIVE	},
	{ &hud_drawAltHUD,			"hud_drawAltHUD",				"1",		CVAR_ARCHIVE	}, // jaquboss
	{ &cg_drawMuzzleFlash,		"cg_drawMuzzleFlash",			"1",		CVAR_ARCHIVE	},
	{ &cg_drawTracers,			"cg_drawTracers",				"1",		CVAR_ARCHIVE	},
	{ &cg_drawspeed,			"cg_drawspeed",					"0",		CVAR_ARCHIVE	},
	{ &cg_speedunit,			"cg_speedunit",					"0",		CVAR_ARCHIVE	},
	{ &cg_optimizePrediction,	"cg_optimizePrediction",		"1",		CVAR_ARCHIVE	},
	{ &cg_popupMessageFilter,	"cg_popupMessageFilter",		"0",		CVAR_ARCHIVE	},
	{ &cg_locations,			"cg_locations",					"3",		CVAR_ARCHIVE	},
	{ &cg_fixedFTeamSize,		"cg_fixedFTeamSize",			"1",		CVAR_ARCHIVE	},
	{ &cg_weaponBounceSound,	"cg_weaponBounceSound",			"1",		CVAR_ARCHIVE	},
	{ &cg_altHQVoice,			"cg_altHQVoice",				"0",		CVAR_ARCHIVE	},
	{ &cg_rtcwShake,			"cg_rtcwShake",					"0",		CVAR_ARCHIVE	},
	{ &cg_limbo_secondary,		"cg_limbo_secondary",			"2",		CVAR_ARCHIVE	},
	{ &cg_modelHilights,		"cg_modelHilights",				"1",		CVAR_ARCHIVE	},	// sucky thing, adds not really good option to turn it off, but serves better for forcecvar/sv_cvar changes
	{ &cg_drawAuraIcons,		"cg_drawAuraIcons",				"1",		CVAR_ARCHIVE	},
	{ &cg_FTAutoSelect,			"cg_FTAutoSelect",				"1",		CVAR_ARCHIVE	},
	{ &cg_automapZoom,			"cg_automapZoom",				"5.159",	CVAR_ARCHIVE	},
	{ &cg_shoveSounds,			"cg_shoveSounds",				"1",		CVAR_ARCHIVE	},
	{ &cg_logFile,				"cg_logFile",					"", 		CVAR_ARCHIVE	}, // client.log IRATA: we don't log the chats per default - disabled now
	{ &cg_pmWaitTime,			"cg_pmWaitTime",				"2",		CVAR_ARCHIVE	},
	{ &cg_pmFadeTime,			"cg_pmFadeTime",				"2.5",		CVAR_ARCHIVE	},
	{ &cg_pmPopupTime,			"cg_pmPopupTime",				"1",		CVAR_ARCHIVE	},
	{ &cg_pmBigPopupTime,		"cg_pmBigPopupTime",			"3.5",		CVAR_ARCHIVE	},
	{ &cg_skillOptions,			"cg_skillViewOptions",			"31",		CVAR_ARCHIVE	},
	{ &cg_spectator,			"cg_spectator",					"3",		CVAR_ARCHIVE	},
	{ &cg_whizzSounds,			"cg_whizzSounds",				"1",		CVAR_ARCHIVE	},
	{ &cg_favWeaponBank,		"cg_favWeaponBank",				"0",		CVAR_ARCHIVE	},
	{ &cg_spawnTimer_set,       "cg_spawnTimer_set",            "-1",       CVAR_TEMP       },  // from etpub
	{ &cg_spawnTimer_period,    "cg_spawnTimer_period",         "0",        CVAR_TEMP       },  // from etpub
};

int			cvarTableSize = sizeof( cvarTable ) / sizeof( cvarTable[0] );
qboolean	cvarsLoaded = qfalse;
void CG_setClientFlags(void);

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	char		var[MAX_TOKEN_CHARS];

	CG_Printf("%d client cvars in use.\n", cvarTableSize);

	// trap_Cvar_Set( "cg_letterbox", "0" );	// force this for people who might have it in their

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; ++i, ++cv ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
		if(cv->vmCvar != NULL) {
			// rain - force the update to range check this cvar on first run
			if (cv->vmCvar == &cg_errorDecay) {
				cv->modificationCount = !cv->vmCvar->modificationCount;
			}
			else {
				cv->modificationCount = cv->vmCvar->modificationCount;
			}
		}
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	// Gordon: um, here, why?
	CG_setClientFlags();
	BG_setCrosshair(cg_crosshairColor.string, cg.xhairColor, cg_crosshairAlpha.value, "cg_crosshairColor");
	BG_setCrosshair(cg_crosshairColorAlt.string, cg.xhairColorAlt, cg_crosshairAlphaAlt.value, "cg_crosshairColorAlt");

	cvarsLoaded = qtrue;
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	qboolean	fSetFlags = qfalse;
	cvarTable_t	*cv;

	if(!cvarsLoaded) return;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; ++i, ++cv ) {
		if(cv->vmCvar) {
			trap_Cvar_Update( cv->vmCvar );
			if(cv->modificationCount != cv->vmCvar->modificationCount) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				// Check if we need to update any client flags to be sent to the server
				if(cv->vmCvar == &cg_autoAction || cv->vmCvar == &cg_autoReload ||
				   cv->vmCvar == &int_cl_timenudge || cv->vmCvar == &int_cl_maxpackets ||
				   cv->vmCvar == &cg_autoactivate || cv->vmCvar == &cg_predictItems ||
				   cv->vmCvar == &cg_weapAltReloads   )
				{
					fSetFlags = qtrue;
				}
				else if(cv->vmCvar == &cg_crosshairColor || cv->vmCvar == &cg_crosshairAlpha) {
					BG_setCrosshair(cg_crosshairColor.string, cg.xhairColor, cg_crosshairAlpha.value, "cg_crosshairColor");
				}
				else if(cv->vmCvar == &cg_crosshairColorAlt || cv->vmCvar == &cg_crosshairAlphaAlt) {
					BG_setCrosshair(cg_crosshairColorAlt.string, cg.xhairColorAlt, cg_crosshairAlphaAlt.value, "cg_crosshairColorAlt");
				}
				else if(cv->vmCvar == &cg_rconPassword && *cg_rconPassword.string) {
					trap_SendConsoleCommand( va( "rconAuth %s", cg_rconPassword.string ) );
				}
				else if(cv->vmCvar == &cg_refereePassword && *cg_refereePassword.string) {
					trap_SendConsoleCommand( va( "ref %s", cg_refereePassword.string ) );
				}
				else if( cv->vmCvar == &cg_HUDBorderColor || cv->vmCvar == &cg_HUDBackgroundColor || cv->vmCvar == &cg_HUDAlpha ) {
					jP_SetHUDColors();
				}
				else if(cv->vmCvar == &demo_infoWindow) {
					if(demo_infoWindow.integer == 0 && cg.demohelpWindow == SHOW_ON) {
						CG_ShowHelp_On(&cg.demohelpWindow);
					} else if(demo_infoWindow.integer > 0 && cg.demohelpWindow != SHOW_ON) {
						CG_ShowHelp_On(&cg.demohelpWindow);
					}
				}
				else if (cv->vmCvar == &cg_errorDecay) {
					// rain - cap errordecay because
					// prediction is EXTREMELY broken
					// right now.
					if (cg_errorDecay.value < 0.0) {
						trap_Cvar_Set("cg_errorDecay", "0");
					}
					else if (cg_errorDecay.value > 500.0) {
						trap_Cvar_Set("cg_errorDecay", "500");
					}
				}
			}
		}
	}

	// Send any relevent updates
	if(fSetFlags) {
		CG_setClientFlags();
	}
}

void CG_RestoreProfile(void) {
	int i;

	for( i=0; i<cg.cvarBackupsCount; ++i ) {
		trap_Cvar_Set(cg.cvarBackups[i].cvarName, cg.cvarBackups[i].cvarValue);
	}

}

void CG_setClientFlags(void) {
	if(cg.demoPlayback) return;

	cg.pmext.bAutoReload = (cg_autoReload.integer > 0);
	cg.pmext.weapAltReload = ( cg_weapAltReloads.integer > 0 );
	trap_Cvar_Set("cg_uinfo", va("%d %d %d",
											 // Client Flags
											(
												((cg_autoReload.integer > 0) ? CGF_AUTORELOAD : 0) |
												((cg_autoAction.integer & AA_STATSDUMP) ? CGF_STATSDUMP : 0) |
												((cg_autoactivate.integer > 0) ? CGF_AUTOACTIVATE : 0) |
												((cg_predictItems.integer > 0) ? CGF_PREDICTITEMS : 0) |
												((cg_weapAltReloads.integer > 0 ) ? CGF_WEAPALTRELOAD : 0 )
												// Add more in here, as needed
											),

											// Timenudge
											int_cl_timenudge.integer,
											// MaxPackets
											int_cl_maxpackets.integer
									   ));
}

int CG_CrosshairPlayer( void ) {
	if ( cg.time > ( cg.crosshairClientTime + SECONDS_1 ) ) {
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
#ifdef MV_SUPPORT
	// OSP - used for messaging clients in the currect active window
	if(cg.mvTotalClients > 0) return(cg.mvCurrentActive->mvInfo & MV_PID);
	// OSP
#endif
	return((!cg.attackerTime) ? -1 : cg.snap->ps.persistant[PERS_ATTACKER]);
}

void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);
	if ( !Q_strncmp( text, "[cgnotify]", 10 ) ) {
		char buf[1024];

		if ( !cg_drawNotifyText.integer ) {
			Q_strncpyz( buf, &text[10], 1013 );
			trap_Print( buf );
			return;
		}

		CG_AddToNotify( &text[10] );
		Q_strncpyz( buf, &text[10], 1013 );
		Q_strncpyz( text, "[skipnotify]", 13 );
		Q_strcat( text, 1011, buf );
	}

	trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Error( text );
}

#ifndef CGAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link (FIXME)

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	CG_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	CG_Printf("%s", text);
}

#endif

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );
	return buffer;
}

// Cleans a string for filesystem compatibility
void CG_nameCleanFilename(const char *pszIn, char *pszOut, unsigned int dwOutSize) {
	unsigned int dwCurrLength = 0;

	while(*pszIn && dwCurrLength < dwOutSize) {
		if(*pszIn == 27 || *pszIn == '^') {
			pszIn++;
			dwCurrLength++;

			if(*pszIn) {
				pszIn++;		// skip color code
				dwCurrLength++;
				continue;
			}
		}

		// Illegal Windows characters
		if(*pszIn == '\\' || *pszIn == '/' || *pszIn == ':' || *pszIn == '"' ||
		   *pszIn == '*'  || *pszIn == '?' || *pszIn == '<' || *pszIn == '>' ||
		   *pszIn == '|'  || *pszIn == '.') {
			pszIn++;
			dwCurrLength++;
			continue;
		}

		if(*pszIn <= 32) {
			pszIn++;
			dwCurrLength++;
			continue;
		}

		*pszOut++ = *pszIn++;
		dwCurrLength++;
	}

	*pszOut = 0;
}

// Standard naming for screenshots/demos
char *CG_generateFilename(void) {
	qtime_t ct;
	const char *pszServerInfo = CG_ConfigString(CS_SERVERINFO);

	trap_RealTime(&ct);

	return(va("%d-%02d-%02d-%02d%02d%02d-%s%s",
								1900+ct.tm_year, ct.tm_mon+1,ct.tm_mday,
								ct.tm_hour, ct.tm_min, ct.tm_sec,
								Info_ValueForKey(pszServerInfo, "mapname"),
#ifdef MV_SUPPORT
								(cg.mvTotalClients < 1) ?
#endif
								""
#ifdef MV_SUPPORT
								: "-MVD"
#endif
								));
}

int CG_findClientNum(char *s) {
	int			id;
	char		s2[64], n2[64];
	qboolean	fIsNumber = qtrue;
	int			clientNum;

	// See if its a number or string
	for(id=0; id<strlen(s) && s[id] != 0; ++id) {
		if(s[id] < '0' || s[id] > '9') {
			fIsNumber = qfalse;
			break;
		}
	}

	// numeric values are just slot numbers
	if(fIsNumber) {
		id = atoi(s);
		if(id >= 0 && id < cgs.maxclients && cgs.clientinfo[id].infoValid) return(id);
	}

	// check for a name match
	BG_cleanName(s, s2, sizeof(s2), qfalse);
	for( id = 0; id < cgs.numValidClients; ++id ) {
		clientNum = cgs.validClients[id];
		BG_cleanName(cgs.clientinfo[clientNum].name, n2, sizeof(n2), qfalse);
		if(!Q_stricmp(n2, s2)) return(clientNum);
	}

	CG_Printf("[cgnotify]%s ^3%s^7 %s.\n", CG_TranslateString("User"), s, CG_TranslateString("is not on the server"));
	return(-1);
}

void CG_printConsoleString(char *str) {
	CG_Printf("[skipnotify]%s", str);
}

void CG_LoadObjectiveData( void ) {
	pc_token_t token, token2;
	int handle;

	if( cg_gameType.integer == GT_WOLF_LMS ) {
		handle = trap_PC_LoadSource( va( "maps/%s_lms.objdata", Q_strlwr(cgs.rawmapname) ) );
	}
	else {
		handle = trap_PC_LoadSource( va( "maps/%s.objdata", Q_strlwr(cgs.rawmapname) ) );
	}

	if( !handle ) {
		return;
	}

	while( 1 ) {
		if( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}

		if( !Q_stricmp( token.string, "wm_mapdescription" ) ) {
			if( !trap_PC_ReadToken( handle, &token ) ) {
				CG_Printf( "^1ERROR: bad objdata line : team parameter required\n" );
				break;
			}

			if( !trap_PC_ReadToken( handle, &token2 ) ) {
				CG_Printf( "^1ERROR: bad objdata line : description parameter required\n" );
				break;
			}

			if( !Q_stricmp( token.string, "axis" ) ) {
				Q_strncpyz( cg.objMapDescription_Axis, token2.string, sizeof( cg.objMapDescription_Axis ) );
			}
			else if( !Q_stricmp( token.string, "allied" ) ) {
				Q_strncpyz( cg.objMapDescription_Allied, token2.string, sizeof( cg.objMapDescription_Allied ) );
			}
			else if( !Q_stricmp( token.string, "neutral" ) ) {
				Q_strncpyz( cg.objMapDescription_Neutral, token2.string, sizeof( cg.objMapDescription_Neutral ) );
			}
		}
		else if( !Q_stricmp( token.string, "wm_objective_axis_desc" ) ) {
			int i;

			if( !PC_Int_Parse( handle, &i ) ) {
				CG_Printf( "^1ERROR: bad objdata line : number parameter required\n" );
				break;
			}

			if( !trap_PC_ReadToken( handle, &token ) ) {
				CG_Printf( "^1ERROR: bad objdata line :  description parameter required\n" );
				break;
			}

			i--;

			if( i < 0 || i >= MAX_OBJECTIVES ) {
				CG_Printf( "^1ERROR: bad objdata line : invalid objective number\n" );
				break;
			}

			Q_strncpyz( cg.objDescription_Axis[i], token.string, sizeof( cg.objDescription_Axis[i] ) );
		}
		else if( !Q_stricmp( token.string, "wm_objective_allied_desc" ) ) {
			int i;

			if( !PC_Int_Parse( handle, &i ) ) {
				CG_Printf( "^1ERROR: bad objdata line : number parameter required\n" );
				break;
			}

			if( !trap_PC_ReadToken( handle, &token ) ) {
				CG_Printf( "^1ERROR: bad objdata line :  description parameter required\n" );
				break;
			}

			i--;

			if( i < 0 || i >= MAX_OBJECTIVES ) {
				CG_Printf( "^1ERROR: bad objdata line : invalid objective number\n" );
				break;
			}

			Q_strncpyz( cg.objDescription_Allied[i], token.string, sizeof( cg.objDescription_Allied[i] ) );
		}
	}

	trap_PC_FreeSource( handle );
}

//========================================================================
void CG_SetupDlightstyles(void) {
	int			i, j;
	char		*str;
	char		*token;
	int			entnum;
	centity_t	*cent;

	cg.lightstylesInited = qtrue;

	for (i=1; i<MAX_DLIGHT_CONFIGSTRINGS; ++i) {
		str = (char *) CG_ConfigString (CS_DLIGHTS + i);
		if(!strlen(str))
			break;

		token = COM_Parse (&str);	// ent num
		entnum = atoi(token);
		cent = &cg_entities[entnum];

		token = COM_Parse (&str);	// stylestring
		Q_strncpyz(cent->dl_stylestring, token, strlen(token));

		token = COM_Parse (&str);	// offset
		cent->dl_frame		= atoi(token);
		cent->dl_oldframe	= cent->dl_frame - 1;
		if(cent->dl_oldframe < 0)
			cent->dl_oldframe = strlen(cent->dl_stylestring);

		token = COM_Parse (&str);	// sound id
		cent->dl_sound = atoi(token);

		token = COM_Parse (&str);	// attenuation
		cent->dl_atten = atoi(token);

		for(j=0;j<strlen(cent->dl_stylestring);j++) {
			cent->dl_stylestring[j] += cent->dl_atten;	// adjust character for attenuation/amplification
			// clamp result
			if(cent->dl_stylestring[j] < 'a')	cent->dl_stylestring[j] = 'a';
			if(cent->dl_stylestring[j] > 'z')	cent->dl_stylestring[j] = 'z';
		}

		cent->dl_backlerp	= 0.0f;
		cent->dl_time		= cg.time;
	}

}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level

IRATA modified: 'precache' added - we don't use as many sounds as items so I didn't put this into bgitemlist
We can use our quick and dirty hash value to get the required sound
Computer says "no" ... this is used to register item pick-up sounds.
=================
*/
static void CG_RegisterItemSounds() {

	if( developer.integer ) {
		CG_Printf("...register item pickup sounds\n");
	}

	cgs.media.itemPickupAmmo 		= trap_S_RegisterSound("sound/misc/am_pkup.wav", qfalse );
	cgs.media.itemPickupHealth 		= trap_S_RegisterSound("sound/misc/health_pickup.wav", qfalse );
	cgs.media.itemPickupHealthSmall = trap_S_RegisterSound("sound/items/n_health.wav", qfalse );
	cgs.media.itemPickupHot 		= trap_S_RegisterSound("sound/items/hot_pickup.wav", qfalse );
	cgs.media.itemPickupCold 		= trap_S_RegisterSound("sound/items/cold_pickup.wav", qfalse );
	cgs.media.itemPickupWeapon 		= trap_S_RegisterSound("sound/misc/w_pkup.wav" , qfalse );
	cgs.media.itemPickupHelmet 		= trap_S_RegisterSound("sound/pickup/powerups/helmet.wav", qfalse );
	cgs.media.itemPickupFlakjacket 	= trap_S_RegisterSound("sound/pickup/powerups/flakjacket.wav", qfalse );
	cgs.media.itemPickupUniform 	= trap_S_RegisterSound("sound/pickup/powerups/uniform.wav", qfalse );
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int		i;
	char	name[MAX_QPATH];
	bg_speaker_t *speaker;

	CG_LoadingString( ":voice chats:" );
	CG_LoadVoiceChats();
	CG_SoundInit();
	CG_LoadingString( ":script speakers:" );
	BG_ClearScriptSpeakerPool();
	BG_LoadSpeakerScript( va( "sound/maps/%s.sps", cgs.rawmapname ) );

	for( i = 0; i < BG_NumScriptSpeakers(); ++i ) {
		speaker = BG_GetScriptSpeaker( i );
		speaker->noise = trap_S_RegisterSound( speaker->filename, qfalse );
	}

	CG_LoadingString( ":game sounds:" );

	cgs.media.noAmmoSound       = trap_S_RegisterSound( "sound/weapons/misc/fire_dry.wav",              qfalse );
	cgs.media.noFireUnderwater  = trap_S_RegisterSound( "sound/weapons/misc/fire_water.wav",            qfalse );
	cgs.media.selectSound       = trap_S_RegisterSound( "sound/weapons/misc/change.wav",                qfalse );
	cgs.media.landHurt          = trap_S_RegisterSound( "sound/player/land_hurt.wav",                   qfalse );
	cgs.media.gibSound          = trap_S_RegisterSound( "sound/player/gib.wav",                         qfalse );
	cgs.media.dynamitebounce1   = trap_S_RegisterSound( "sound/weapons/dynamite/dynamite_bounce.wav",   qfalse );
	cgs.media.satchelbounce1    = trap_S_RegisterSound( "sound/weapons/satchel/satchel_bounce.wav",     qfalse );
	cgs.media.landminebounce1   = trap_S_RegisterSound( "sound/weapons/landmine/mine_bounce.wav",       qfalse );

	cgs.media.watrInSound       = trap_S_RegisterSound( "sound/player/water_in.wav",                    qfalse );
	cgs.media.watrOutSound      = trap_S_RegisterSound( "sound/player/water_out.wav",                   qfalse );
	cgs.media.watrUnSound       = trap_S_RegisterSound( "sound/player/water_un.wav",                    qfalse );
	cgs.media.watrGaspSound     = trap_S_RegisterSound( "sound/player/gasp.wav",                        qfalse );
	cgs.media.underWaterSound   = trap_S_RegisterSound( "sound/player/underwater.wav",                  qfalse );

	cgs.media.fkickwall         = trap_S_RegisterSound( "sound/weapons/melee/fstatck.wav",              qfalse );
	cgs.media.fkickflesh        = trap_S_RegisterSound( "sound/weapons/melee/fstatck.wav",              qfalse );
	cgs.media.fkickmiss         = trap_S_RegisterSound( "sound/weapons/melee/fstmiss.wav",              qfalse );


	for( i = 0; i < 2; ++i ) {
		cgs.media.grenadebounce[FOOTSTEP_NORMAL][i] = \
		cgs.media.grenadebounce[FOOTSTEP_GRAVEL][i] = \
		cgs.media.grenadebounce[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound( va( "sound/weapons/grenade/bounce_hard%i.wav", i+1 ), qfalse );

		cgs.media.grenadebounce[FOOTSTEP_METAL][i]  = \
		cgs.media.grenadebounce[FOOTSTEP_ROOF][i]   = trap_S_RegisterSound( va( "sound/weapons/grenade/bounce_metal%i.wav", i+1 ), qfalse );

		cgs.media.grenadebounce[FOOTSTEP_WOOD][i]   = trap_S_RegisterSound( va( "sound/weapons/grenade/bounce_wood%i.wav", i+1 ), qfalse );

		cgs.media.grenadebounce[FOOTSTEP_GRASS][i]  = \
		cgs.media.grenadebounce[FOOTSTEP_SNOW][i]   = \
		cgs.media.grenadebounce[FOOTSTEP_CARPET][i] = trap_S_RegisterSound( va( "sound/weapons/grenade/bounce_soft%i.wav", i+1 ), qfalse );

	}

	cgs.media.landSound[FOOTSTEP_NORMAL]  =	trap_S_RegisterSound( "sound/player/footsteps/stone_jump.wav",  qfalse );
	cgs.media.landSound[FOOTSTEP_SPLASH]  =	trap_S_RegisterSound( "sound/player/footsteps/water_jump.wav",  qfalse );
	cgs.media.landSound[FOOTSTEP_METAL]   =	trap_S_RegisterSound( "sound/player/footsteps/metal_jump.wav",  qfalse );
	cgs.media.landSound[FOOTSTEP_WOOD]    =	trap_S_RegisterSound( "sound/player/footsteps/wood_jump.wav",   qfalse );
	cgs.media.landSound[FOOTSTEP_GRASS]   =	trap_S_RegisterSound( "sound/player/footsteps/grass_jump.wav",  qfalse );
	cgs.media.landSound[FOOTSTEP_GRAVEL]  =	trap_S_RegisterSound( "sound/player/footsteps/gravel_jump.wav", qfalse );
	cgs.media.landSound[FOOTSTEP_ROOF]    =	trap_S_RegisterSound( "sound/player/footsteps/roof_jump.wav",   qfalse );
	cgs.media.landSound[FOOTSTEP_SNOW]    =	trap_S_RegisterSound( "sound/player/footsteps/snow_jump.wav",   qfalse );
	cgs.media.landSound[FOOTSTEP_CARPET]  =	trap_S_RegisterSound( "sound/player/footsteps/carpet_jump.wav", qfalse );

	for (i = 0; i < 4; ++i) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/stone%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound( name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/water%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound( name, qfalse );

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/metal%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound( name, qfalse );

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/wood%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_WOOD][i] = trap_S_RegisterSound( name, qfalse );

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/grass%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_GRASS][i] = trap_S_RegisterSound( name, qfalse );

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/gravel%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_GRAVEL][i] = trap_S_RegisterSound( name, qfalse );

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/roof%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_ROOF][i] = trap_S_RegisterSound(  name, qfalse );

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/snow%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SNOW][i] = trap_S_RegisterSound( name, qfalse );

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/carpet%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_CARPET][i] = trap_S_RegisterSound( name, qfalse );
	}

	CG_RegisterItemSounds();


	// core: if the old/original configstring method is active,
	// load the values from the ConfigString
	if ( cgs.csMethod == 0 ) {
		const char *soundName;
		for ( i = 1 ; i < MAX_SOUNDS ; ++i ) {
			soundName = CG_ConfigString( CS_SOUNDS+i );
			if ( !soundName[0] ) break;
			if ( soundName[0] == '*' ) continue;       // custom sound
			// Ridah, register sound scripts seperately
			if ( !strstr(soundName, ".wav") ) {
				CG_SoundScriptPrecache( soundName );
			}
			else {
				cgs.gameSounds[i] = trap_S_RegisterSound( soundName, qfalse );
			}
		}
	}

	cgs.media.countFight            = trap_S_RegisterSound( "sound/osp/fight.wav",                              qfalse );
	cgs.media.countPrepare          = trap_S_RegisterSound( "sound/osp/prepare.wav",                            qfalse );

	cgs.media.knifeKill				= trap_S_RegisterSound( "sound/osp/goat.wav",							    qfalse );
	cgs.media.headShot				= trap_S_RegisterSound( "sound/jclient/hithead.wav",					    qfalse );
	cgs.media.bodyShot				= trap_S_RegisterSound( "sound/jclient/hit.wav",						    qfalse );
	cgs.media.teamShot				= trap_S_RegisterSound( "sound/jclient/hitteam.wav",					    qfalse );
	cgs.media.goombaSound			= trap_S_RegisterSound( "sound/jetpilot/goomba.wav",					    qfalse );

	cgs.media.flameSound			= trap_S_RegisterSound( "sound/weapons/flamethrower/flame_burn.wav",	    qfalse );
	cgs.media.flameBlowSound		= trap_S_RegisterSound( "sound/weapons/flamethrower/flame_pilot.wav",	    qfalse );
	cgs.media.flameStartSound		= trap_S_RegisterSound( "sound/weapons/flamethrower/flame_up.wav",		    qfalse );
	cgs.media.flameStreamSound		= trap_S_RegisterSound( "sound/weapons/flamethrower/flame_fire.wav",	    qfalse );

    cgs.media.grenadePulseSound4 	= trap_S_RegisterSound( "sound/weapons/grenade/gren_timer4.wav", 		    qfalse );
	cgs.media.grenadePulseSound3 	= trap_S_RegisterSound( "sound/weapons/grenade/gren_timer3.wav", 		    qfalse );
	cgs.media.grenadePulseSound2 	= trap_S_RegisterSound( "sound/weapons/grenade/gren_timer2.wav", 		    qfalse );
	cgs.media.grenadePulseSound1 	= trap_S_RegisterSound( "sound/weapons/grenade/gren_timer1.wav", 		    qfalse );

	cgs.media.boneBounceSound       = trap_S_RegisterSound( "sound/world/boardbreak.wav",                       qfalse );

	cgs.media.sfx_rockexp           = trap_S_RegisterSound( "sound/weapons/rocket/rocket_expl.wav",             qfalse );
	cgs.media.sfx_rockexpDist       = trap_S_RegisterSound( "sound/weapons/rocket/rocket_expl_far.wav",         qfalse );

	cgs.media.sfx_artilleryExp[0]   = trap_S_RegisterSound( "sound/weapons/artillery/artillery_expl_1.wav",     qfalse );
	cgs.media.sfx_artilleryExp[1]   = trap_S_RegisterSound( "sound/weapons/artillery/artillery_expl_2.wav",     qfalse );
	cgs.media.sfx_artilleryExp[2]   = trap_S_RegisterSound( "sound/weapons/artillery/artillery_expl_3.wav",     qfalse );
	cgs.media.sfx_artilleryDist     = trap_S_RegisterSound( "sound/weapons/artillery/artillery_expl_far.wav",   qfalse );

	// arty detection
	cgs.media.sfx_artilleryDist_1[0] = trap_S_RegisterSound( "sound/weapons/artillery/artillery_expl_far1.wav", qfalse );
	cgs.media.sfx_artilleryDist_1[1] = trap_S_RegisterSound( "sound/weapons/artillery/artillery_expl_far2.wav", qfalse );
	cgs.media.sfx_artilleryDist_1[2] = trap_S_RegisterSound( "sound/weapons/artillery/artillery_expl_far3.wav", qfalse );

	cgs.media.sfx_airstrikeExp[0]   = trap_S_RegisterSound( "sound/weapons/airstrike/airstrike_expl_1.wav",     qfalse );
	cgs.media.sfx_airstrikeExp[1]   = trap_S_RegisterSound( "sound/weapons/airstrike/airstrike_expl_2.wav",     qfalse );
	cgs.media.sfx_airstrikeExp[2]   = trap_S_RegisterSound( "sound/weapons/airstrike/airstrike_expl_3.wav",     qfalse );
	cgs.media.sfx_airstrikeDist     = trap_S_RegisterSound( "sound/weapons/airstrike/airstrike_expl_far.wav",   qfalse );

	cgs.media.sfx_dynamiteexp       = trap_S_RegisterSound( "sound/weapons/dynamite/dynamite_expl.wav",         qfalse );
	cgs.media.sfx_dynamiteexpDist   = trap_S_RegisterSound( "sound/weapons/dynamite/dynamite_expl_far.wav",     qfalse );

	cgs.media.sfx_satchelexp        = trap_S_RegisterSound( "sound/weapons/satchel/satchel_expl.wav",           qfalse );
	cgs.media.sfx_satchelexpDist    = trap_S_RegisterSound( "sound/weapons/satchel/satchel_expl_far.wav",       qfalse );
	cgs.media.sfx_landmineexp       = trap_S_RegisterSound( "sound/weapons/landmine/mine_expl.wav",             qfalse );
	cgs.media.sfx_landmineexpDist   = trap_S_RegisterSound( "sound/weapons/landmine/mine_expl_far.wav",         qfalse );
	cgs.media.sfx_mortarexp[0]      = trap_S_RegisterSound( "sound/weapons/mortar/mortar_expl1.wav",            qfalse );
	cgs.media.sfx_mortarexp[1]      = trap_S_RegisterSound( "sound/weapons/mortar/mortar_expl2.wav",            qfalse );
	cgs.media.sfx_mortarexp[2]      = trap_S_RegisterSound( "sound/weapons/mortar/mortar_expl3.wav",            qfalse );
	cgs.media.sfx_mortarexp[3]      = trap_S_RegisterSound( "sound/weapons/mortar/mortar_expl.wav",             qfalse );
	cgs.media.sfx_mortarexpDist     = trap_S_RegisterSound( "sound/weapons/mortar/mortar_expl_far.wav",         qfalse );
	cgs.media.sfx_grenexp           = trap_S_RegisterSound( "sound/weapons/grenade/gren_expl.wav",              qfalse );
	cgs.media.sfx_grenexpDist       = trap_S_RegisterSound( "sound/weapons/grenade/gren_expl_far.wav",          qfalse );
	cgs.media.sfx_rockexpWater      = trap_S_RegisterSound( "sound/weapons/grenade/gren_expl_water.wav",        qfalse );


	for(i = 0; i < 3; i++) {
		cgs.media.sfx_brassSound[BRASSSOUND_METAL][i][0]  =	trap_S_RegisterSound (va("sound/weapons/misc/shell_metal%i.wav",	i + 1), qfalse );
		cgs.media.sfx_brassSound[BRASSSOUND_METAL][i][1]  =	trap_S_RegisterSound (va("sound/weapons/misc/sg_shell_metal%i.wav",	i + 1), qfalse );
		cgs.media.sfx_brassSound[BRASSSOUND_SOFT][i][0]   =	trap_S_RegisterSound (va("sound/weapons/misc/shell_soft%i.wav",		i + 1), qfalse );
		cgs.media.sfx_brassSound[BRASSSOUND_SOFT][i][1]   =	trap_S_RegisterSound (va("sound/weapons/misc/sg_shell_soft%i.wav",	i + 1), qfalse );
		cgs.media.sfx_brassSound[BRASSSOUND_STONE][i][0]  =	trap_S_RegisterSound (va("sound/weapons/misc/shell_stone%i.wav",	i + 1), qfalse );
		cgs.media.sfx_brassSound[BRASSSOUND_STONE][i][1]  =	trap_S_RegisterSound (va("sound/weapons/misc/sg_shell_stone%i.wav",	i + 1), qfalse );
		cgs.media.sfx_brassSound[BRASSSOUND_WOOD][i][0]   =	trap_S_RegisterSound (va("sound/weapons/misc/shell_wood%i.wav",		i + 1), qfalse );
		cgs.media.sfx_brassSound[BRASSSOUND_WOOD][i][1]   =	trap_S_RegisterSound (va("sound/weapons/misc/sg_shell_wood%i.wav",	i + 1), qfalse );
		cgs.media.sfx_rubbleBounce[i]                     =	trap_S_RegisterSound (va("sound/world/debris%i.wav",				i + 1), qfalse );
	}

	cgs.media.sfx_knifehit[0]     =	trap_S_RegisterSound ("sound/weapons/knife/knife_hit1.wav",             qfalse );
	cgs.media.sfx_knifehit[1]     =	trap_S_RegisterSound ("sound/weapons/knife/knife_hit2.wav",             qfalse );
	cgs.media.sfx_knifehit[2]     =	trap_S_RegisterSound ("sound/weapons/knife/knife_hit3.wav",             qfalse );
	cgs.media.sfx_knifehit[3]     =	trap_S_RegisterSound ("sound/weapons/knife/knife_hit4.wav",             qfalse );
	cgs.media.sfx_knifehit[4]     =	trap_S_RegisterSound ("sound/weapons/knife/knife_hitwall1.wav",         qfalse );

	cgs.media.sfx_poisonhit[0]    = trap_S_RegisterSound ("sound/weapons/poison/poison_hit.wav",            qfalse );
	cgs.media.sfx_poisonhit[1]    =	trap_S_RegisterSound ("sound/weapons/poison/poison_hitwall.wav",        qfalse );

	for(i = 0; i < 5; i++) {
		cgs.media.sfx_bullet_fleshhit[i]      =	trap_S_RegisterSound (va("sound/weapons/impact/flesh%i.wav",	i+1),	qfalse );
		cgs.media.sfx_bullet_metalhit[i]      =	trap_S_RegisterSound (va("sound/weapons/impact/metal%i.wav",	i+1),	qfalse );
		cgs.media.sfx_bullet_woodhit[i]       =	trap_S_RegisterSound (va("sound/weapons/impact/wood%i.wav",		i+1),	qfalse );
		cgs.media.sfx_bullet_glasshit[i]      =	trap_S_RegisterSound (va("sound/weapons/impact/glass%i.wav",	i+1),	qfalse );
		cgs.media.sfx_bullet_stonehit[i]      =	trap_S_RegisterSound (va("sound/weapons/impact/stone%i.wav",	i+1),	qfalse );
		cgs.media.sfx_bullet_waterhit[i]      =	trap_S_RegisterSound (va("sound/weapons/impact/water%i.wav",	i+1),	qfalse );
		cgs.media.sfx_bullet_whizz[i]         = trap_S_RegisterSound (va("sound/weapons/impact/whizz%i.wav",	i+1),	qfalse );
	}

	cgs.media.uniformPickup       =	trap_S_RegisterSound( "sound/misc/body_pickup.wav",     qfalse );
	cgs.media.buildDecayedSound   =	trap_S_RegisterSound( "sound/world/build_abort.wav",    qfalse );

	cgs.media.sndLimboSelect      =	trap_S_RegisterSound( "sound/menu/select.wav",          qfalse );
	cgs.media.sndLimboFilter      =	trap_S_RegisterSound( "sound/menu/filter.wav",          qfalse );
	// cgs.media.sndLimboCancel      =	trap_S_RegisterSound( "sound/menu/cancel.wav",          qfalse );

	cgs.media.sndRankUp           =	trap_S_RegisterSound ("sound/misc/rank_up.wav",         qfalse );
	cgs.media.sndSkillUp          =	trap_S_RegisterSound ("sound/misc/skill_up.wav",        qfalse );

	cgs.media.sndMedicCall[0]     =	trap_S_RegisterSound ("sound/chat/axis/medic.wav",      qfalse );
	cgs.media.sndMedicCall[1]     =	trap_S_RegisterSound ("sound/chat/allies/medic.wav",    qfalse );

	cgs.media.shoveSound	      = trap_S_RegisterSound ("sound/weapons/impact/flesh1.wav",qfalse );
	cgs.media.knifeThrow          = trap_S_RegisterSound ("sound/weapons/knife/throw.wav",  qfalse );
	cgs.media.PrivateMessageBeep  = trap_S_RegisterSound ("sound/misc/pm.wav",              qfalse);

	CG_LoadingString( ":caching sounds:" );

    cgs.cachedSounds[GAMESOUND_PLAYER_GURP1]        = trap_S_RegisterSound( "sound/player/gurp1.wav",                       qfalse );
    cgs.cachedSounds[GAMESOUND_PLAYER_GURP2]        = trap_S_RegisterSound( "sound/player/gurp2.wav",                       qfalse );
    cgs.cachedSounds[GAMESOUND_WPN_AIRSTRIKE_PLANE] = trap_S_RegisterSound( "sound/weapons/airstrike/airstrike_plane.wav",  qfalse );
	cgs.cachedSounds[GAMESOUND_WPN_ARTILLERY_FLY_1] = trap_S_RegisterSound( "sound/weapons/artillery/artillery_fly_1.wav",  qfalse );
	cgs.cachedSounds[GAMESOUND_WPN_ARTILLERY_FLY_2] = trap_S_RegisterSound( "sound/weapons/artillery/artillery_fly_2.wav",  qfalse );
    cgs.cachedSounds[GAMESOUND_WPN_ARTILLERY_FLY_3] = trap_S_RegisterSound( "sound/weapons/artillery/artillery_fly_3.wav",  qfalse );
    cgs.cachedSounds[GAMESOUND_WORLD_JUMPPAD]       = trap_S_RegisterSound( "sound/world/jumppad.wav",                      qfalse );
	cgs.cachedSounds[GAMESOUND_WORLD_BUILD]         = trap_S_RegisterSound( "sound/world/build.wav",                        qfalse );
	cgs.cachedSounds[GAMESOUND_WORLD_CHAIRCREAK]    = trap_S_RegisterSound( "sound/world/chaircreak.wav",                   qfalse );
    cgs.cachedSounds[GAMESOUND_MOOVERS_LOCK]        = trap_S_RegisterSound( "sound/movers/doors/default_door_locked.wav",   qfalse );
    cgs.cachedSounds[GAMESOUND_MISC_RESUSCITATE]    = trap_S_RegisterSound( "sound/misc/vo_resuscitate.wav",                qfalse );
	cgs.cachedSounds[GAMESOUND_MISC_REVIVE]         = trap_S_RegisterSound( "sound/misc/vo_revive.wav",                     qfalse );
    cgs.cachedSounds[GAMESOUND_MISC_WHACK]          = trap_S_RegisterSound( "sound/jetpilot/whack.wav",                     qfalse );
    cgs.cachedSounds[GAMESOUND_MISC_WINDFLY]        = trap_S_RegisterSound( "sound/misc/windfly.wav",                       qfalse );
    cgs.cachedSounds[GAMESOUND_MISC_REFEREE]        = trap_S_RegisterSound( "sound/misc/referee.wav",                       qfalse );
    cgs.cachedSounds[GAMESOUND_MISC_VOTE]           = trap_S_RegisterSound( "sound/misc/vote.wav",                          qfalse );
    cgs.cachedSounds[GAMESOUND_MISC_BANNED]         = trap_S_RegisterSound( "sound/osp/banned.wav",                         qfalse );
    cgs.cachedSounds[GAMESOUND_MISC_KICKED]         = trap_S_RegisterSound( "sound/osp/kicked.wav",                         qfalse );
    cgs.cachedSounds[GAMESOUND_KILL_MULTI]          = trap_S_RegisterSound( "sound/spree/multikill.wav",                    qfalse );
    cgs.cachedSounds[GAMESOUND_KILL_ULTRA]          = trap_S_RegisterSound( "sound/spree/ultrakill.wav" ,                   qfalse );
    cgs.cachedSounds[GAMESOUND_KILL_MONSTER]        = trap_S_RegisterSound( "sound/spree/monsterkill.wav",                  qfalse );
    cgs.cachedSounds[GAMESOUND_KILL_MEGA]           = trap_S_RegisterSound( "sound/spree/megakill.wav",                     qfalse );
    cgs.cachedSounds[GAMESOUND_KILL_LUDICROUS]      = trap_S_RegisterSound( "sound/spree/ludicrouskill.wav",                qfalse );
    cgs.cachedSounds[GAMESOUND_SPREE_KILLINGSPREE]  = trap_S_RegisterSound( "sound/spree/killingspree.wav",                 qfalse );
    cgs.cachedSounds[GAMESOUND_SPREE_RAMPAGE]       = trap_S_RegisterSound( "sound/spree/rampage.wav",                      qfalse );
    cgs.cachedSounds[GAMESOUND_SPREE_DOMINATING]    = trap_S_RegisterSound( "sound/spree/dominating.wav",                   qfalse );
    cgs.cachedSounds[GAMESOUND_SPREE_UNSTOPPABLE]   = trap_S_RegisterSound( "sound/spree/unstoppable.wav",                  qfalse );
    cgs.cachedSounds[GAMESOUND_SPREE_GODLIKE]       = trap_S_RegisterSound( "sound/spree/godlike.wav",                      qfalse );
    cgs.cachedSounds[GAMESOUND_SPREE_WICKEDSICK]    = trap_S_RegisterSound( "sound/spree/wickedsick.wav",                   qfalse );
    cgs.cachedSounds[GAMESOUND_SPREE_POTTER]        = trap_S_RegisterSound( "sound/spree/potter.wav",                       qfalse );
    cgs.cachedSounds[GAMESOUND_SPREE_HOLYCOW]       = trap_S_RegisterSound( "sound/spree/holyshit.wav",                     qfalse );
    cgs.cachedSounds[GAMESOUND_SPREE_HUMILIATION]   = trap_S_RegisterSound( "sound/spree/humiliation.wav",                  qfalse );
    cgs.cachedSounds[GAMESOUND_ANN_FIRSTBLOOD]      = trap_S_RegisterSound( "sound/spree/firstblood.wav",                   qfalse );
    cgs.cachedSounds[GAMESOUND_ANN_FIRTSHEADSHOT]   = trap_S_RegisterSound( "sound/spree/firstheadshot.wav",                qfalse );

    // IRATA: we always do the precaching
	//if( cg_buildScript.integer ) {
		CG_PrecacheFXSounds();
	//}
}

//===================================================================================

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	char		name[MAX_QPATH];
	int			i;
	static char		*sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	CG_LoadingString( cgs.mapname );

	trap_R_LoadWorldMap( cgs.mapname );

	CG_LoadingString( ":entities:" );

	numSplinePaths = 0;
	numPathCorners = 0;

	cg.numOIDtriggers2 = 0;

	BG_ClearAnimationPool();

	BG_ClearCharacterPool();

	BG_InitWeaponStrings();

	CG_ParseEntitiesFromString();

	CG_LoadObjectiveData();

	// precache status bar pics
	CG_LoadingString( ":game media:" );

	CG_LoadingString( ":textures:" );

	for ( i=0 ; i<11 ; ++i) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader( sb_nums[i] );
	}

	cgs.media.fleshSmokePuffShader = trap_R_RegisterShader("fleshimpactsmokepuff"); // JPW NERVE
	cgs.media.smokePuffShader = trap_R_RegisterShader( "smokePuff" );

	// RF, blood cloud
	cgs.media.bloodCloudShader = trap_R_RegisterShader("bloodCloud");

	// Rafael - cannon
	cgs.media.smokePuffShaderdirty = trap_R_RegisterShader( "smokePuffdirty" );
	cgs.media.smokePuffShaderb1 = trap_R_RegisterShader( "smokePuffblack1" );
	cgs.media.smokePuffShaderb2 = trap_R_RegisterShader( "smokePuffblack2" );
	cgs.media.smokePuffShaderb3 = trap_R_RegisterShader( "smokePuffblack3" );
	cgs.media.smokePuffShaderb4 = trap_R_RegisterShader( "smokePuffblack4" );
	cgs.media.smokePuffShaderb5 = trap_R_RegisterShader( "smokePuffblack5" );
	// done

	// Rafael - bleedanim
	for( i = 0; i < 5; ++i ) {
		cgs.media.viewBloodAni[i] = trap_R_RegisterShader (va("viewBloodBlend%i", i+1));
	}

	// cgs.media.viewFlashBlood = trap_R_RegisterShader( "viewFlashBlood" ); // IRATA: unused
	for( i = 0; i < 16; ++i ) {
		cgs.media.viewFlashFire[i] = trap_R_RegisterShader( va("viewFlashFire%i", i+1) );
	}

	// jaquboss load for rage pro only, but i doubt that such ancient hardware is still used
	if ( cgs.glconfig.hardwareType == GLHW_RAGEPRO ) {
		cgs.media.smokePuffRageProShader = trap_R_RegisterShader( "smokePuffRagePro" );
	}

	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader( "shotgunSmokePuff" );
	cgs.media.bloodTrailShader		= trap_R_RegisterShader( "bloodTrail" );
	cgs.media.reticleShaderSimple	= trap_R_RegisterShader( "gfx/misc/reticlesimple" );
	cgs.media.binocShaderSimple		= trap_R_RegisterShader( "gfx/misc/binocsimple"	);
	cgs.media.snowShader			= trap_R_RegisterShader( "snow_tri" );
	cgs.media.oilParticle			= trap_R_RegisterShader( "oilParticle" );
	cgs.media.oilSlick				= trap_R_RegisterShader( "oilSlick" );
	cgs.media.waterBubbleShader		= trap_R_RegisterShader( "waterBubble" );
	cgs.media.tracerShader			= trap_R_RegisterShader( "gfx/misc/tracer" );
	cgs.media.usableHintShader		= trap_R_RegisterShader( "gfx/2d/usableHint" );
	cgs.media.notUsableHintShader	= trap_R_RegisterShader( "gfx/2d/notUsableHint" );
	cgs.media.doorHintShader		= trap_R_RegisterShader( "gfx/2d/doorHint" );
	cgs.media.doorRotateHintShader	= trap_R_RegisterShader( "gfx/2d/doorRotateHint" );

	// Arnout: these were never used in default wolf ..
	cgs.media.doorLockHintShader	= trap_R_RegisterShader( "gfx/2d/lockedhint" );
	cgs.media.doorRotateLockHintShader	= trap_R_RegisterShader( "gfx/2d/lockedhint" ); // IRATA: same as doorLockHintShader // TomekKromek: nah, not the same
	cgs.media.mg42HintShader		= trap_R_RegisterShader( "gfx/2d/mg42Hint" );
	cgs.media.breakableHintShader	= trap_R_RegisterShader( "gfx/2d/breakableHint" );
	// cgs.media.chairHintShader		= trap_R_RegisterShader( "gfx/2d/chairHint" );  // IRATA: unused
	cgs.media.alarmHintShader		= trap_R_RegisterShader( "gfx/2d/alarmHint" );
	cgs.media.healthHintShader		= trap_R_RegisterShader( "gfx/2d/healthHint" );
	cgs.media.knifeHintShader		= trap_R_RegisterShader( "gfx/2d/knifeHint" );
	cgs.media.ladderHintShader		= trap_R_RegisterShader( "gfx/2d/ladderHint" );
	cgs.media.buttonHintShader		= trap_R_RegisterShader( "gfx/2d/buttonHint" );
	cgs.media.waterHintShader		= trap_R_RegisterShader( "gfx/2d/waterHint" );
	cgs.media.weaponHintShader		= trap_R_RegisterShader( "gfx/2d/weaponHint" );
	cgs.media.ammoHintShader		= trap_R_RegisterShader( "gfx/2d/ammoboxHint" );
	cgs.media.powerupHintShader		= trap_R_RegisterShader( "gfx/2d/powerupHint" );
	cgs.media.inventoryHintShader	= trap_R_RegisterShader( "gfx/2d/inventoryHint" );

	cgs.media.friendShader			= trap_R_RegisterShaderNoMip( "gfx/2d/friendlycross.tga" );

	cgs.media.buildHintShader		= trap_R_RegisterShader( "gfx/2d/buildHint" );		// DHM - Nerve
	cgs.media.disarmHintShader		= trap_R_RegisterShader( "gfx/2d/disarmHint" );		// DHM - Nerve
	cgs.media.reviveHintShader		= trap_R_RegisterShader( "gfx/2d/reviveHint" );		// DHM - Nerve
	cgs.media.dynamiteHintShader	= trap_R_RegisterShader( "gfx/2d/dynamiteHint" );	// DHM - Nerve
	cgs.media.reviveActivateHintShader	= trap_R_RegisterShader( "gfx/2d/reviveActivateHint" );

	cgs.media.tankHintShader		= trap_R_RegisterShaderNoMip( "gfx/2d/tankHint" );
	cgs.media.satchelchargeHintShader = trap_R_RegisterShaderNoMip( "gfx/2d/satchelchargeHint" ),
	cgs.media.landmineHintShader	= trap_R_RegisterShaderNoMip( "gfx/2d/landmineHint" );
	cgs.media.uniformHintShader		= trap_R_RegisterShaderNoMip( "gfx/2d/uniformHint" );

	if( cgs.ccLayers ) {
		for( i = 0; i < cgs.ccLayers; ++i ) {
			cgs.media.commandCentreMapShader[i]		= trap_R_RegisterShaderNoMip( va( "levelshots/%s_%i_cc.tga", cgs.rawmapname, i ) );
			cgs.media.commandCentreMapShaderTrans[i]= trap_R_RegisterShaderNoMip( va( "levelshots/%s_%i_cc_trans", cgs.rawmapname, i ) );
			cgs.media.commandCentreAutomapShader[i]	= trap_R_RegisterShaderNoMip( va( "levelshots/%s_%i_cc_automap", cgs.rawmapname, i ) );
		}
	}
	else {
		cgs.media.commandCentreMapShader[0]			= trap_R_RegisterShaderNoMip( va( "levelshots/%s_cc.tga", cgs.rawmapname ) );
		cgs.media.commandCentreMapShaderTrans[0]	= trap_R_RegisterShaderNoMip( va( "levelshots/%s_cc_trans", cgs.rawmapname ) );
		cgs.media.commandCentreAutomapShader[0]		= trap_R_RegisterShaderNoMip( va( "levelshots/%s_cc_automap", cgs.rawmapname ) );
	}

	cgs.media.blackmask = trap_R_RegisterShaderNoMip( "images/blackmask" ); // etpro icons support


	cgs.media.commandCentreAutomapMaskShader = trap_R_RegisterShaderNoMip( "levelshots/automap_mask" );
	cgs.media.commandCentreAutomapBorderShader = trap_R_RegisterShaderNoMip( "ui/assets2/maptrim_long" );
	cgs.media.commandCentreAutomapBorder2Shader = trap_R_RegisterShaderNoMip( "ui/assets2/maptrim_long2" );
	cgs.media.commandCentreAutomapCornerShader = trap_R_RegisterShaderNoMip( "ui/assets2/maptrim_edge.tga" );
	cgs.media.commandCentreAxisMineShader	= trap_R_RegisterShaderNoMip( "sprites/landmine_axis" );
	cgs.media.commandCentreAlliedMineShader	= trap_R_RegisterShaderNoMip( "sprites/landmine_allied" );
	cgs.media.commandCentreSpawnShader[0] = trap_R_RegisterShaderNoMip( "gfx/limbo/cm_flagaxis" );
	cgs.media.commandCentreSpawnShader[1] = trap_R_RegisterShaderNoMip( "gfx/limbo/cm_flagallied" );
	cgs.media.compassConstructShader =		trap_R_RegisterShaderNoMip( "sprites/construct.tga" );
    cgs.media.countryFlags = trap_R_RegisterShaderNoMip( "gfx/flags/world_flags" ); //mcwf GeoIP

	cgs.media.compass2Shader =				trap_R_RegisterShaderNoMip( "gfx/2d/compass2.tga" );
	cgs.media.compassShader =				trap_R_RegisterShaderNoMip( "gfx/2d/compass.tga" );
	cgs.media.buddyShader =					trap_R_RegisterShaderNoMip( "sprites/buddy.tga" );

	for ( i = 0 ; i < NUM_CROSSHAIRS ; ++i ) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshair%c", 'a'+i) );
		cg.crosshairShaderAlt[i] = trap_R_RegisterShader( va("gfx/2d/crosshair%c_alt", 'a'+i) );
	}

	for ( i = 0 ; i < SK_NUM_SKILLS ; ++i ) {
		cgs.media.medals[i] = trap_R_RegisterShaderNoMip( va( "gfx/limbo/medals0%i", i ) );
	}

	cgs.media.backTileShader =	trap_R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.teamStatusBar =	trap_R_RegisterShader( "gfx/2d/colorbar.tga" );

	cgs.media.hudSprintBar =	trap_R_RegisterShader("sprintbar");
	cgs.media.hudAlliedHelmet = trap_R_RegisterShader("AlliedHelmet");
	cgs.media.hudAxisHelmet =	trap_R_RegisterShader("AxisHelmet");

	cgs.media.projectionshadow = trap_R_RegisterShader("projectionshadow");

	CG_LoadingString( ":models:" );

	cgs.cachedModels[GAMEMODEL_WORLD_FLAGPOLE]			= trap_R_RegisterModel("models/multiplayer/flagpole/flagpole.md3");
	cgs.cachedModels[GAMEMODEL_WORLD_DOGTAGS]			= trap_R_RegisterModel("models/multiplayer/dogtags/dogtags.md3");
	// cgs.cachedModels[GAMEMODEL_WORLD_SEARCHLIGHT]		= trap_R_RegisterModel("models/mapobjects/light/searchlight_pivot.md3"); // ** was used in "SP_misc_spotlight"
	// cgs.cachedModels[GAMEMODEL_WORLD_ALARMBOX]			= trap_R_RegisterModel("models/mapobjects/electronics/alarmbox.md3");
	// cgs.cachedModels[GAMEMODEL_WORLD_BOX_32]			= trap_R_RegisterModel("models/mapobjects/boxes/box32.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_BOX_48]			= trap_R_RegisterModel("models/mapobjects/boxes/box48.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_BOX_64]			= trap_R_RegisterModel("models/mapobjects/boxes/box64.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_BENCH]				= trap_R_RegisterModel("models/furniture/bench/bench_sm.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_RADIO]				= trap_R_RegisterModel("models/mapobjects/electronics/radio1.md3" ); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_RADIOS]			= trap_R_RegisterModel("models/mapobjects/electronics/radios.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_CASTLEBED]			= trap_R_RegisterModel("models/furniture/bed/castlebed.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_TABLE_56X112]		= trap_R_RegisterModel("models/furniture/table/56x112tablew.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_CRATE_32X64]		= trap_R_RegisterModel("models/furniture/crate/crate32x64.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_WOODFLIP]			= trap_R_RegisterModel("models/furniture/table/woodflip.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_LOCKER]			= trap_R_RegisterModel("models/furniture/storage/lockertall.md3"); // **
	cgs.cachedModels[GAMEMODEL_WORLD_CHAIR_OFFICE]		= trap_R_RegisterModel("models/furniture/chair/chair_office3.md3");
	cgs.cachedModels[GAMEMODEL_WORLD_CHAIR_HIBACK]		= trap_R_RegisterModel("models/furniture/chair/hiback5.md3");
	cgs.cachedModels[GAMEMODEL_WORLD_CHAIR_SIDECHAIR]	= trap_R_RegisterModel("models/furniture/chair/sidechair3.md3");
	// cgs.cachedModels[GAMEMODEL_WORLD_CHAIR_CHAT]		= trap_R_RegisterModel("models/furniture/chair/chair_chat.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_CHAIR_CHATARM]		= trap_R_RegisterModel("models/furniture/chair/chair_chatarm.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_DESKLAMP]			= trap_R_RegisterModel("models/furniture/lights/desklamp.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_BARREL_C]			= trap_R_RegisterModel("models/furniture/barrel/barrel_c.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_BARREL_D]			= trap_R_RegisterModel("models/furniture/barrel/barrel_d.md3");// **
	// cgs.cachedModels[GAMEMODEL_WORLD_BARREL_B]			= trap_R_RegisterModel("models/furniture/barrel/barrel_b.md3");  // **
	// cgs.cachedModels[GAMEMODEL_WORLD_CRATE_64]			= trap_R_RegisterModel("models/furniture/crate/crate64.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_CRATE_32]			= trap_R_RegisterModel("models/furniture/crate/crate32.md3"); // **
	// cgs.cachedModels[GAMEMODEL_WORLD_FOOTLOCKER]		= trap_R_RegisterModel("models/mapobjects/furniture/footlocker.md3"); // **
	cgs.cachedModels[GAMEMODEL_MISC_ROCKET]				= trap_R_RegisterModel("models/ammo/rocket/rocket.md3");
	cgs.cachedModels[GAMEMODEL_WPN_MG42]				= trap_R_RegisterModel("models/multiplayer/mg42/mg42.md3");
	cgs.cachedModels[GAMEMODEL_WPN_MG42_B]				= trap_R_RegisterModel("models/mapobjects/weapons/mg42b.md3");

	cgs.media.machinegunBrassModel	= trap_R_RegisterModel( "models/weapons2/shells/m_shell.md3" );
	cgs.media.panzerfaustBrassModel = trap_R_RegisterModel( "models/weapons2/shells/pf_shell.md3" );

	// Rafael
	cgs.media.smallgunBrassModel = trap_R_RegisterModel ( "models/weapons2/shells/sm_shell.md3" );

	//----(SA) wolf debris

	cgs.media.debBlock[0] 			= trap_R_RegisterModel( "models/mapobjects/debris/brick1.md3" );
	cgs.media.debBlock[1] 			= trap_R_RegisterModel( "models/mapobjects/debris/brick2.md3" );
	cgs.media.debBlock[2] 			= trap_R_RegisterModel( "models/mapobjects/debris/brick3.md3" );
	cgs.media.debBlock[3] 			= trap_R_RegisterModel( "models/mapobjects/debris/brick4.md3" );
	cgs.media.debBlock[4] 			= trap_R_RegisterModel( "models/mapobjects/debris/brick5.md3" );
	cgs.media.debBlock[5] 			= trap_R_RegisterModel( "models/mapobjects/debris/brick6.md3" );

	cgs.media.debRock[0]			= trap_R_RegisterModel( "models/mapobjects/debris/rubble1.md3" );
	cgs.media.debRock[1]			= trap_R_RegisterModel( "models/mapobjects/debris/rubble2.md3" );
	cgs.media.debRock[2]			= trap_R_RegisterModel( "models/mapobjects/debris/rubble3.md3" );

	cgs.media.debWood[0] 			= trap_R_RegisterModel( "models/gibs/wood/wood1.md3" );
	cgs.media.debWood[1] 			= trap_R_RegisterModel( "models/gibs/wood/wood2.md3" );
	cgs.media.debWood[2] 			= trap_R_RegisterModel( "models/gibs/wood/wood3.md3" );
	cgs.media.debWood[3] 			= trap_R_RegisterModel( "models/gibs/wood/wood4.md3" );
	cgs.media.debWood[4] 			= trap_R_RegisterModel( "models/gibs/wood/wood5.md3" );
	cgs.media.debWood[5] 			= trap_R_RegisterModel( "models/gibs/wood/wood6.md3" );

	cgs.media.debFabric[0] 			= trap_R_RegisterModel( "models/shards/fabric1.md3" );
	cgs.media.debFabric[1] 			= trap_R_RegisterModel( "models/shards/fabric2.md3" );
	cgs.media.debFabric[2] 			= trap_R_RegisterModel( "models/shards/fabric3.md3" );

	//----(SA) end

	cgs.media.spawnInvincibleShader = trap_R_RegisterShader( "sprites/shield" );
	cgs.media.scoreEliminatedShader = trap_R_RegisterShader( "sprites/skull" );
	cgs.media.medicReviveShader		= trap_R_RegisterShader( "sprites/medic_revive" );

	cgs.media.poisonOverlay			= trap_R_RegisterShader( "gfx/misc/poisonoverlay" );

	cgs.media.resuscitateShader		= trap_R_RegisterShader( "sprites/resuscitate" );
	cgs.media.poisonedIcon			= trap_R_RegisterShader( "sprites/poisoned" );

	cgs.media.buyIcon				= trap_R_RegisterShader( "sprites/buyIcon" );

	cgs.media.voiceChatShader		= trap_R_RegisterShader( "sprites/voiceChat" );
	cgs.media.balloonShader			= trap_R_RegisterShader( "sprites/nq_talk" );		// core: was "sprites/balloon3"..
	cgs.media.objectiveShader		= trap_R_RegisterShader( "sprites/objective" );
	cgs.media.readyIcon				= trap_R_RegisterShader( "sprites/ready" );

	//----(SA)	water splash
	cgs.media.waterSplashModel		= trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.waterSplashShader		= trap_R_RegisterShader( "waterSplash" );
	//----(SA)	end

	// Ridah, spark particles
	cgs.media.sparkParticleShader	= trap_R_RegisterShader( "sparkParticle" );
	cgs.media.smokeTrailShader		= trap_R_RegisterShader( "smokeTrail" );
	cgs.media.flamethrowerFireStream = trap_R_RegisterShader( "flamethrowerFireStream" );
	cgs.media.onFireShader2			= trap_R_RegisterShader( "entityOnFire1" );
	cgs.media.onFireShader			= trap_R_RegisterShader( "entityOnFire2" );
	cgs.media.sparkFlareShader		= trap_R_RegisterShader( "sparkFlareParticle" );
	cgs.media.spotLightShader		= trap_R_RegisterShader( "spotLight" );
	cgs.media.spotLightBeamShader	= trap_R_RegisterShader( "lightBeam" );
	// cgs.media.bulletParticleTrailShader = trap_R_RegisterShader( "bulletParticleTrail" ); // IRATA - unused
	cgs.media.smokeParticleShader	= trap_R_RegisterShader( "smokeParticle" );

	// DHM - Nerve :: bullet hitting dirt
	cgs.media.dirtParticle1Shader 	= trap_R_RegisterShader( "dirt_splash" );
	cgs.media.dirtParticle2Shader 	= trap_R_RegisterShader( "water_splash" );

	cgs.media.genericConstructionShader =		trap_R_RegisterShader( "textures/sfx/construction" );
	cgs.media.alliedUniformShader	= trap_R_RegisterShader( "sprites/uniform_allied" );
	cgs.media.axisUniformShader		= trap_R_RegisterShader( "sprites/uniform_axis" );

	// used in:
	// command map
	cgs.media.ccFilterPics[0] = trap_R_RegisterShaderNoMip( "gfx/limbo/filter_axis" );
	cgs.media.ccFilterPics[1] = trap_R_RegisterShaderNoMip( "gfx/limbo/filter_allied" );
	cgs.media.ccFilterPics[2] = trap_R_RegisterShaderNoMip( "gfx/limbo/filter_spawn" );

	cgs.media.ccFilterPics[3] = trap_R_RegisterShaderNoMip( "gfx/limbo/filter_bo" );
	cgs.media.ccFilterPics[4] = trap_R_RegisterShaderNoMip( "gfx/limbo/filter_healthammo" );
	cgs.media.ccFilterPics[5] = trap_R_RegisterShaderNoMip( "gfx/limbo/filter_construction" );
	cgs.media.ccFilterPics[6] = trap_R_RegisterShaderNoMip( "gfx/limbo/filter_destruction" );
	cgs.media.ccFilterPics[7] = trap_R_RegisterShaderNoMip( "gfx/limbo/filter_objective" );

	cgs.media.ccFilterBackOn =	trap_R_RegisterShaderNoMip( "gfx/limbo/filter_back_on" );
	cgs.media.ccFilterBackOff = trap_R_RegisterShaderNoMip( "gfx/limbo/filter_back_off" );

	// used in:
	//  statsranksmedals
	//	command map
	//	limbo menu
	cgs.media.ccStamps[0] =				trap_R_RegisterShaderNoMip( "ui/assets2/stamp_complete" );
	cgs.media.ccStamps[1] =				trap_R_RegisterShaderNoMip( "ui/assets2/stamp_failed" );

	cgs.media.ccPlayerHighlight =		trap_R_RegisterShaderNoMip( "ui/assets/mp_player_highlight.tga" );
	cgs.media.ccConstructIcon[0] =		trap_R_RegisterShaderNoMip( "gfx/limbo/cm_constaxis" );
	cgs.media.ccConstructIcon[1] =		trap_R_RegisterShaderNoMip( "gfx/limbo/cm_constallied" );
	cgs.media.ccDestructIcon[0][0] =	trap_R_RegisterShaderNoMip( "gfx/limbo/cm_axisgren" );
	cgs.media.ccDestructIcon[0][1] =	trap_R_RegisterShaderNoMip( "gfx/limbo/cm_alliedgren" );
	cgs.media.ccDestructIcon[1][0] =	trap_R_RegisterShaderNoMip( "gfx/limbo/cm_satchel" );
	cgs.media.ccDestructIcon[1][1] =	trap_R_RegisterShaderNoMip( "gfx/limbo/cm_satchel" );
	cgs.media.ccDestructIcon[2][0] =	trap_R_RegisterShaderNoMip( "gfx/limbo/cm_dynamite" );
	cgs.media.ccDestructIcon[2][1] =	trap_R_RegisterShaderNoMip( "gfx/limbo/cm_dynamite" );
	cgs.media.ccTankIcon =				trap_R_RegisterShaderNoMip( "gfx/limbo/cm_churchill" );

	cgs.media.ccCmdPost[0] =			trap_R_RegisterShaderNoMip( "gfx/limbo/cm_bo_axis" );
	cgs.media.ccCmdPost[1] =			trap_R_RegisterShaderNoMip( "gfx/limbo/cm_bo_allied" );

	cgs.media.ccMortarHit =				trap_R_RegisterShaderNoMip( "gfx/limbo/mort_hit" );
	cgs.media.ccMortarTarget =			trap_R_RegisterShaderNoMip( "gfx/limbo/mort_target" );
	cgs.media.ccMortarTargetArrow =		trap_R_RegisterShaderNoMip( "gfx/limbo/mort_targetarrow" );

	cgs.media.skillPics[SK_BATTLE_SENSE]								= trap_R_RegisterShaderNoMip( "gfx/limbo/ic_battlesense" );
	cgs.media.skillPics[SK_HEAVY_WEAPONS]								= trap_R_RegisterShaderNoMip( "gfx/limbo/ic_soldier" );
	cgs.media.skillPics[SK_FIRST_AID]									= trap_R_RegisterShaderNoMip( "gfx/limbo/ic_medic" );
	cgs.media.skillPics[SK_EXPLOSIVES_AND_CONSTRUCTION]					= trap_R_RegisterShaderNoMip( "gfx/limbo/ic_engineer" );
	cgs.media.skillPics[SK_SIGNALS]										= trap_R_RegisterShaderNoMip( "gfx/limbo/ic_fieldops" );
	cgs.media.skillPics[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS]	= trap_R_RegisterShaderNoMip( "gfx/limbo/ic_covertops" );
	cgs.media.skillPics[SK_LIGHT_WEAPONS]								= trap_R_RegisterShaderNoMip( "gfx/limbo/ic_lightweap" );

	cgs.media.specGlowShader[0] =  trap_R_RegisterShader( "models/players/common/specGlow");
	cgs.media.specGlowShader[1] =  trap_R_RegisterShader( "models/players/common/specGlowWeapon");

	// jaquboss special obituaries
	cgs.media.fallObituary			= trap_R_RegisterShader( "gfx/hud/fallObituary");
	cgs.media.crushObituary			= trap_R_RegisterShader( "gfx/hud/crushObituary");
	cgs.media.drownObituary			= trap_R_RegisterShader( "gfx/hud/drownObituary");
	cgs.media.artyObituary			= trap_R_RegisterShader( "gfx/hud/artyObituary");
	cgs.media.airstrikeObituary		= trap_R_RegisterShader( "gfx/hud/airstrikeObituary");
	cgs.media.poisonObituary		= trap_R_RegisterShader( "gfx/hud/poisonObituary");
	cgs.media.goombaObituary		= trap_R_RegisterShader( "gfx/hud/goombaObituary");
	cgs.media.shoveObituary			= trap_R_RegisterShader( "gfx/hud/shoveObituary");

	cgs.media.muteIcon				= trap_R_RegisterShaderNoMip( "gfx/hud/ic_mute" );

	CG_LoadRankIcons();

	// Gordon: limbo menu setup
	CG_LimboPanel_Init();

	CG_ChatPanel_Setup();

	CG_Fireteams_Setup();

	cgs.media.railCoreShader =	trap_R_RegisterShaderNoMip( "railCore" );	// (SA) for debugging server traces
	cgs.media.ropeShader =		trap_R_RegisterShader( "textures/props/cable_m01" );

	cgs.media.thirdPersonBinocModel = trap_R_RegisterModel( "models/multiplayer/binocs/binocs.md3" );				// NERVE - SMF
	cgs.media.flamebarrel = trap_R_RegisterModel ( "models/furniture/barrel/barrel_a.md3" );
	cgs.media.mg42muzzleflash = trap_R_RegisterModel ("models/weapons2/machinegun/mg42_flash.md3" );

	// Rafael shards
	cgs.media.shardGlass1 = trap_R_RegisterModel( "models/shards/glass1.md3" );
	cgs.media.shardGlass2 = trap_R_RegisterModel( "models/shards/glass2.md3" );
	cgs.media.shardWood1 = trap_R_RegisterModel( "models/shards/wood1.md3" );
	cgs.media.shardWood2 = trap_R_RegisterModel( "models/shards/wood2.md3" );
	cgs.media.shardMetal1 = trap_R_RegisterModel( "models/shards/metal1.md3" );
	cgs.media.shardMetal2 = trap_R_RegisterModel( "models/shards/metal2.md3" );
	// done

	cgs.media.shardRubble1 = trap_R_RegisterModel ("models/mapobjects/debris/brick000.md3"); // **
	cgs.media.shardRubble2 = trap_R_RegisterModel ("models/mapobjects/debris/brick001.md3"); // **
	cgs.media.shardRubble3 = trap_R_RegisterModel ("models/mapobjects/debris/brick002.md3"); // **

	// end of jP additions

	for (i=0; i<MAX_LOCKER_DEBRIS; ++i) {
		Com_sprintf (name, sizeof(name), "models/mapobjects/debris/personal%i.md3", i + 1);
		cgs.media.shardJunk[i] = trap_R_RegisterModel (name);
	}

	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

// TODO: FIXME:  REMOVE REGISTRATION OF EACH MODEL FOR EVERY LEVEL LOAD

	//----(SA)	okay, new stuff to intialize rather than doing it at level load time (or "give all" time)
	//			(I'm certainly not against being efficient here, but I'm tired of the rocket launcher effect only registering
	//			sometimes and want it to work for sure for this demo)

	CG_LoadingString( ":weapons:" );

	// make sure we have latest configstring

	Q_strncpyz(cgs.weaponScriptsDir, Info_ValueForKey(CG_ConfigString( CS_NOQUARTERINFO ), "W" ), MAX_CVAR_VALUE_STRING );

	for( i = WP_NONE; i < WP_NUM_WEAPONS; ++i ) {
		BG_RegisterWeapon( i, qfalse ); // jaquboss
	}

	CG_LoadingString( ":items:" );
	for ( i = 1 ; i < bg_numItems ; ++i ) {
		CG_RegisterItemVisuals( i );
	}

	cgs.media.rocketExplosionShader	= trap_R_RegisterShader( "rocketExplosion" );

	cgs.media.hWeaponSnd =			trap_S_RegisterSound( "sound/weapons/mg42/mg42_fire.wav", qfalse  );
	cgs.media.hWeaponEchoSnd =		trap_S_RegisterSound( "sound/weapons/mg42/mg42_far.wav", qfalse );
	cgs.media.hWeaponHeatSnd =		trap_S_RegisterSound( "sound/weapons/mg42/mg42_heat.wav", qfalse  );

	cgs.media.hWeaponSnd_2 =		trap_S_RegisterSound( "sound/weapons/browning/browning_fire.wav", qfalse  );
	cgs.media.hWeaponEchoSnd_2 =	trap_S_RegisterSound( "sound/weapons/browning/browning_far.wav", qfalse );
	cgs.media.hWeaponHeatSnd_2 =	trap_S_RegisterSound( "sound/weapons/browning/browning_heat.wav", qfalse  );

	cgs.media.minePrimedSound =		trap_S_RegisterSound( "sound/weapons/landmine/mine_on.wav", qfalse );

    cgs.media.radioSound[0] =       trap_S_RegisterSound( "sound/weapons/radio/callArty_allies.wav", qfalse );
    cgs.media.radioSound[1] =       trap_S_RegisterSound( "sound/weapons/radio/callArty_axis.wav", qfalse );

	// wall marks
	cgs.media.bulletMarkShader =	trap_R_RegisterShaderNoMip( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader =		trap_R_RegisterShaderNoMip( "gfx/damage/burn_med_mrk" );
	cgs.media.shadowFootShader =	trap_R_RegisterShaderNoMip( "markShadowFoot" );
	cgs.media.shadowTorsoShader =	trap_R_RegisterShaderNoMip( "markShadowTorso" );
	cgs.media.wakeMarkShader =		trap_R_RegisterShaderNoMip( "wake" );
	cgs.media.wakeMarkShaderAnim =	trap_R_RegisterShaderNoMip( "wakeAnim" ); // (SA)

	//----(SA)	added
	cgs.media.bulletMarkShaderMetal =	trap_R_RegisterShaderNoMip( "gfx/damage/metal_mrk" );
	cgs.media.bulletMarkShaderWood =	trap_R_RegisterShaderNoMip( "gfx/damage/wood_mrk" );
	cgs.media.bulletMarkShaderGlass =	trap_R_RegisterShaderNoMip( "gfx/damage/glass_mrk" );

	for ( i = 0 ; i < 5 ; ++i ) {
		char	name[32];
		Com_sprintf( name, sizeof(name), "blood_dot%i", i+1 );
		cgs.media.bloodDotShaders[i] = trap_R_RegisterShader( name );
	}

	CG_LoadingString( ":inline models:" );

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	// TAT 12/23/2002 - as a safety check, let's not let the number of models exceed MAX_MODELS
	if (cgs.numInlineModels > MAX_MODELS) {
		CG_Error("CG_RegisterGraphics: Too many inline models: %i\n", cgs.numInlineModels );
	}

	for ( i = 1 ; i < cgs.numInlineModels ; ++i ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; ++j ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	// core: if the old/original configstring method is active,
	// load the values from the ConfigString
	if ( cgs.csMethod == 0 ) {
		CG_LoadingString( ":server models:" );

		// register all the server specified models
		for ( i=1; i<MAX_MODELS; ++i ) {
			const char *modelName = CG_ConfigString( CS_MODELS+i );

			if ( !modelName[0] ) break;
			cgs.gameModels[i] = trap_R_RegisterModel( modelName );
		}

		for ( i=1; i<MAX_CS_SKINS; ++i) {
			const char *skinName = CG_ConfigString( CS_SKINS+i );

			if ( !skinName[0] ) break;
			cgs.gameModelSkins[i] = trap_R_RegisterSkin( skinName );
		}

		for ( i=1; i<MAX_CS_SHADERS; ++i ) {
			const char *shaderName = CG_ConfigString( CS_SHADERS+i );

			if ( !shaderName[0] ) break;
			cgs.gameShaders[i] = shaderName[0] == '*' ? trap_R_RegisterShader( shaderName+ 1 ) : trap_R_RegisterShaderNoMip( shaderName );
			Q_strncpyz( cgs.gameShaderNames[i], shaderName[0] == '*' ? shaderName + 1 : shaderName, MAX_QPATH );
		}

		for ( i=1; i<MAX_CHARACTERS; ++i ) {
			const char *characterName = CG_ConfigString( CS_CHARACTERS+i );
			if ( !characterName[0] ) break;
			if( !BG_FindCharacter( characterName ) ) {
				cgs.gameCharacters[ i ] = BG_FindFreeCharacter( characterName );
				Q_strncpyz( cgs.gameCharacters[ i ]->characterFile, characterName, sizeof(cgs.gameCharacters[ i ]->characterFile) );
				if( !CG_RegisterCharacter( characterName, cgs.gameCharacters[ i ] ) ) {
					CG_Error( "ERROR: CG_RegisterGraphics: failed to load character file '%s'\n", characterName );
				}
			}
		}
	}

	CG_LoadingString( ":particles:" );
	CG_ClearParticles ();
	InitSmokeSprites();

	CG_LoadingString( ":classes:" );
	CG_RegisterPlayerClasses();

	CG_LoadingString( ":sprites:" );
	CG_InitPMGraphics();

	CG_LoadingString( ":media:" );

	// mounted gun on tank models
	cgs.media.hMountedMG42Base =	trap_R_RegisterModel( "models/mapobjects/tanks_sd/mg42nestbase.md3" );
	cgs.media.hMountedMG42Nest =	trap_R_RegisterModel( "models/mapobjects/tanks_sd/mg42nest.md3" );
	cgs.media.hMountedMG42 =		trap_R_RegisterModel( "models/mapobjects/tanks_sd/mg42.md3" );
	cgs.media.hMountedBrowning =	trap_R_RegisterModel( "models/multiplayer/browning/thirdperson.md3" );

	// FIXME: temp models
	cgs.media.hMountedFPMG42 =		trap_R_RegisterModel( "models/multiplayer/mg42/v_mg42.md3" );
	cgs.media.hMountedFPBrowning =	trap_R_RegisterModel( "models/multiplayer/browning/tankmounted.md3" );

	// medic icon for commandmap
	cgs.media.medicIcon_cm			= trap_R_RegisterShaderNoMip("sprites/voiceMedic_cm");
	cgs.media.medicIcon				= trap_R_RegisterShaderNoMip("sprites/voiceMedic");
	cgs.media.disguiseShader		= trap_R_RegisterShaderNoMip("sprites/disguised" );


	trap_R_RegisterFont( "ariblk", 27, &cgs.media.limboFont1 );
	trap_R_RegisterFont( "ariblk", 16, &cgs.media.limboFont1_lo );
	trap_R_RegisterFont( "courbd", 30, &cgs.media.limboFont2 );

	cgs.media.medal_back =				trap_R_RegisterShaderNoMip( "gfx/limbo/medal_back" );

	cgs.media.limboNumber_roll =		trap_R_RegisterShaderNoMip( "gfx/limbo/number_roll" );
	cgs.media.limboNumber_back =		trap_R_RegisterShaderNoMip( "gfx/limbo/number_back" );
	cgs.media.limboStar_roll =			trap_R_RegisterShaderNoMip( "gfx/limbo/skill_roll" );
	cgs.media.limboStar_back =			trap_R_RegisterShaderNoMip( "gfx/limbo/skill_back" );
	cgs.media.limboLight_on =			trap_R_RegisterShaderNoMip( "gfx/limbo/redlight_on" );
	cgs.media.limboLight_on2 =			trap_R_RegisterShaderNoMip( "gfx/limbo/redlight_on02" );
	cgs.media.limboLight_off =			trap_R_RegisterShaderNoMip( "gfx/limbo/redlight_off" );

	cgs.media.limboWeaponNumber_off =	trap_R_RegisterShaderNoMip( "gfx/limbo/but_weap_off" );
	cgs.media.limboWeaponNumber_on =	trap_R_RegisterShaderNoMip( "gfx/limbo/but_weap_on" );
	cgs.media.limboWeaponCard =			trap_R_RegisterShaderNoMip( "gfx/limbo/weap_card" );

	cgs.media.limboWeaponCardSurroundH = trap_R_RegisterShaderNoMip( "gfx/limbo/butsur_hor" );
	cgs.media.limboWeaponCardSurroundV = trap_R_RegisterShaderNoMip( "gfx/limbo/butsur_vert" );
	cgs.media.limboWeaponCardSurroundC = trap_R_RegisterShaderNoMip( "gfx/limbo/butsur_corn" );

	cgs.media.limboWeaponCardOOS =		trap_R_RegisterShaderNoMip( "gfx/limbo/outofstock" );

	cgs.media.limboWeaponCardNadesOOS =		trap_R_RegisterShaderNoMip( "gfx/limbo/nadesoutofstock" );

	cgs.media.limboClassButtons[PC_ENGINEER] =	trap_R_RegisterShaderNoMip( "gfx/limbo/ic_engineer"		);
	cgs.media.limboClassButtons[PC_SOLDIER] =	trap_R_RegisterShaderNoMip( "gfx/limbo/ic_soldier"		);
	cgs.media.limboClassButtons[PC_COVERTOPS] = trap_R_RegisterShaderNoMip( "gfx/limbo/ic_covertops"	);
	cgs.media.limboClassButtons[PC_FIELDOPS] =	trap_R_RegisterShaderNoMip( "gfx/limbo/ic_fieldops"		);
	cgs.media.limboClassButtons[PC_MEDIC] =		trap_R_RegisterShaderNoMip( "gfx/limbo/ic_medic"		);
	cgs.media.limboSkillsBS =					trap_R_RegisterShaderNoMip( "gfx/limbo/ic_battlesense"	);
	cgs.media.limboSkillsLW =					trap_R_RegisterShaderNoMip( "gfx/limbo/ic_lightweap"	);

	cgs.media.limboClassButton2Back_on =		trap_R_RegisterShaderNoMip( "gfx/limbo/skill_back_on"		);
	cgs.media.limboClassButton2Back_off =		trap_R_RegisterShaderNoMip( "gfx/limbo/skill_back_off"		);

	for (i=0; i< NUM_SKILL_LEVELS-1; ++i ) {
		cgs.media.limboClassButton2Wedge_off[i] =		trap_R_RegisterShaderNoMip( va("gfx/limbo/skill_%ipiece_off", i+1 )	);
		cgs.media.limboClassButton2Wedge_on[i] =		trap_R_RegisterShaderNoMip( va("gfx/limbo/skill_%ipiece",	i+1 )	);
	}

	cgs.media.limboClassButtons2[PC_ENGINEER] =	trap_R_RegisterShaderNoMip( "gfx/limbo/skill_engineer"		);
	cgs.media.limboClassButtons2[PC_SOLDIER] =	trap_R_RegisterShaderNoMip( "gfx/limbo/skill_soldier"		);
	cgs.media.limboClassButtons2[PC_COVERTOPS]= trap_R_RegisterShaderNoMip( "gfx/limbo/skill_covops"		);
	cgs.media.limboClassButtons2[PC_FIELDOPS] =	trap_R_RegisterShaderNoMip( "gfx/limbo/skill_fieldops"		);
	cgs.media.limboClassButtons2[PC_MEDIC] =	trap_R_RegisterShaderNoMip( "gfx/limbo/skill_medic"			);

	// jP - new HUD shaders
	cgs.media.hudFlakJacket[0] 			= trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/flakjacket_axis"	);
	cgs.media.hudFlakJacket[1]			= trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/flakjacket_allies"	);
	cgs.media.hudHelmet[0]				= trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/helmet_axis"		);
	cgs.media.hudHelmet[1]				= trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/helmet_allies"		);
	cgs.media.hudAdrenaline				= trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/adrenaline"		);
	cgs.media.hudBinoculars				= trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/binoculars"		);
	cgs.media.hudMorale[0]				= trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/morale_axis"		);
	cgs.media.hudMorale[1]				= trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/morale_allies"		);
	cgs.media.hudCaduceus				= trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/caduceus"			);
	cgs.media.hudEngineer				= trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/fastCool"			);

	cgs.media.pistolHintShader[0]		= trap_R_RegisterShaderNoMip( "gfx/2d/pistolShotColt"		);
	cgs.media.pistolHintShader[1]		= trap_R_RegisterShaderNoMip( "gfx/2d/pistolShotLuger"		);

// End of jP additions

	cgs.media.auraMorale[0]				= trap_R_RegisterShaderNoMip( "sprites/morale_axis_aura"		);
	cgs.media.auraMorale[1]				= trap_R_RegisterShaderNoMip( "sprites/morale_allies_aura"	);
	cgs.media.auraCaduceus				= trap_R_RegisterShaderNoMip( "sprites/caduceus_aura"			);
	cgs.media.auraEngineer				= trap_R_RegisterShaderNoMip( "sprites/fastCool_aura"			);


	cgs.media.limboTeamButtonBack_on	= trap_R_RegisterShaderNoMip( "gfx/limbo/but_team_on"		);
	cgs.media.limboTeamButtonBack_off	= trap_R_RegisterShaderNoMip( "gfx/limbo/but_team_off"		);
	cgs.media.limboTeamButtonAllies		= trap_R_RegisterShaderNoMip( "gfx/limbo/but_team_allied"	);
	cgs.media.limboTeamButtonAxis		= trap_R_RegisterShaderNoMip( "gfx/limbo/but_team_axis"		);
	cgs.media.limboTeamButtonSpec		= trap_R_RegisterShaderNoMip( "gfx/limbo/but_team_spec"		);


	cgs.media.limboBlendThingy			= trap_R_RegisterShaderNoMip( "gfx/limbo/cc_blend"			);
	cgs.media.limboWeaponBlendThingy	= trap_R_RegisterShaderNoMip( "gfx/limbo/weap_blend"		);

	cgs.media.limboCounterBorder =				trap_R_RegisterShaderNoMip( "gfx/limbo/number_border"	);

	cgs.media.hudPowerIcon =					trap_R_RegisterShaderNoMip( "gfx/hud/ic_power"			);
	cgs.media.hudSprintIcon =					trap_R_RegisterShaderNoMip( "gfx/hud/ic_stamina"		);
	cgs.media.hudHealthIcon =					trap_R_RegisterShaderNoMip( "gfx/hud/ic_health"			);

	cgs.media.limboWeaponCard1 =				trap_R_RegisterShaderNoMip( "gfx/limbo/weaponcard01"	);
	cgs.media.limboWeaponCard2 =				trap_R_RegisterShaderNoMip( "gfx/limbo/weaponcard02"	);
	cgs.media.limboWeaponCard3 =				trap_R_RegisterShaderNoMip( "gfx/limbo/weaponcard03"	);
	cgs.media.limboWeaponCardArrow =			trap_R_RegisterShaderNoMip( "gfx/limbo/weap_dnarrow.tga");


	cgs.media.limboObjectiveBack[0]	=			trap_R_RegisterShaderNoMip( "gfx/limbo/objective_back_axis" );
	cgs.media.limboObjectiveBack[1]	=			trap_R_RegisterShaderNoMip( "gfx/limbo/objective_back_allied" );
	cgs.media.limboObjectiveBack[2]	=			trap_R_RegisterShaderNoMip( "gfx/limbo/objective_back" );

	cgs.media.limboClassBar =					trap_R_RegisterShaderNoMip( "gfx/limbo/lightup_bar" );

	cgs.media.limboBriefingButtonOn =			trap_R_RegisterShaderNoMip( "gfx/limbo/but_play_on" );
	cgs.media.limboBriefingButtonOff =			trap_R_RegisterShaderNoMip( "gfx/limbo/but_play_off" );
	cgs.media.limboBriefingButtonStopOn =		trap_R_RegisterShaderNoMip( "gfx/limbo/but_stop_on" );
	cgs.media.limboBriefingButtonStopOff =		trap_R_RegisterShaderNoMip( "gfx/limbo/but_stop_off" );

	cgs.media.limboSpectator =					trap_R_RegisterShaderNoMip( "gfx/limbo/spectator" );
	cgs.media.limboRadioBroadcast =				trap_R_RegisterShaderNoMip( "ui/assets/radio_tower" );

	cgs.media.cursorIcon =						trap_R_RegisterShaderNoMip( "ui/assets/3_cursor3" );

	cgs.media.hudDamagedStates[0] =				trap_R_RegisterSkin( "models/players/hud/damagedskins/blood01.skin" );
	cgs.media.hudDamagedStates[1] =				trap_R_RegisterSkin( "models/players/hud/damagedskins/blood02.skin" );
	cgs.media.hudDamagedStates[2] =				trap_R_RegisterSkin( "models/players/hud/damagedskins/blood03.skin" );
	cgs.media.hudDamagedStates[3] =				trap_R_RegisterSkin( "models/players/hud/damagedskins/blood04.skin" );

	cgs.media.adrenalineEyesSkin	=			trap_R_RegisterSkin( "models/players/hud/adreneyes.skin" );

	cgs.media.browningIcon =					trap_R_RegisterShaderNoMip( "icons/iconw_browning_1_select" );

	cgs.media.axisFlag =						trap_R_RegisterShaderNoMip( "gfx/limbo/flag_axis" );
	cgs.media.alliedFlag =						trap_R_RegisterShaderNoMip( "gfx/limbo/flag_allied" );
	cgs.media.disconnectIcon =					trap_R_RegisterShaderNoMip( "gfx/2d/net" );


	cgs.media.nadeDanger[0]	=					trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/pineapple" );
	cgs.media.nadeDanger[1]	=					trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/masher" );
	cgs.media.artyDanger	=					trap_R_RegisterShaderNoMip( "gfx/hud/noquarter/saber" );

	cgs.media.tv_grain	=						trap_R_RegisterShaderNoMip( "gfx/misc/tv_grain" );

	for( i = 0; i < 6; ++i ) {
		cgs.media.fireteamicons[i] =			trap_R_RegisterShaderNoMip( va( "gfx/hud/fireteam/fireteam%i", i+1 ) );
	}

	// IRATA: objective icons
	cgs.media.gerusaFlag 	= 					trap_R_RegisterShaderNoMip( "ui/assets/gerusa_flag" );
	cgs.media.gerFlag 		= 					trap_R_RegisterShaderNoMip( "ui/assets/ger_flag" );
	cgs.media.usaFlag		= 					trap_R_RegisterShaderNoMip( "ui/assets/usa_flag" );

	// IRATA: intermission flags
	cgs.media.flagshaderAxis =					trap_R_RegisterShaderNoMip( "ui/assets/portraits/axis_win_flag.jpg" );
	cgs.media.nameshaderAxis = 					trap_R_RegisterShaderNoMip( "ui/assets/portraits/text_axis.tga" );
	cgs.media.flagshaderAllies = 				trap_R_RegisterShaderNoMip( "ui/assets/portraits/allies_win_flag.jpg" );
	cgs.media.nameshaderAllies = 				trap_R_RegisterShaderNoMip( "ui/assets/portraits/text_allies.tga" );
	cgs.media.win = 							trap_R_RegisterShaderNoMip( "ui/assets/portraits/text_win.tga" );
}

/*
===================
CG_RegisterClients

===================
*/
static void CG_RegisterClients( void ) {
	int		i;

	for (i=0 ; i<cgs.maxclients ; ++i) {
		const char		*clientInfo = CG_ConfigString( CS_PLAYERS+i );

		if ( !clientInfo[0] ) {
			continue;
		}
		CG_NewClientInfo( i );
	}
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

int CG_ConfigStringCopy( int index, char* buff, int buffsize ) {
	Q_strncpyz( buff, CG_ConfigString( index ), buffsize );
	return strlen( buff );
}


/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void ) {
	// start the background music
	char	*s = (char *)CG_ConfigString( CS_MUSIC );
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	if(strlen(parm1)) {
		trap_S_StartBackgroundTrack( parm1, parm2, 0 );
	}
}

/*
==============
CG_QueueMusic
==============
*/
void CG_QueueMusic( void ) {
	// prepare the next background track
	char	*s = (char *)CG_ConfigString( CS_MUSIC_QUEUE );
	char	parm[MAX_QPATH];

	Q_strncpyz( parm, COM_Parse( &s ), sizeof( parm ) );

	// even if no strlen(parm).  we want to be able to clear the queue

	// TODO: \/		the values stored in here will be made accessable so
	//				it doesn't have to go through startbackgroundtrack() (which is stupid)
	trap_S_StartBackgroundTrack( parm, "", -2);	// '-2' for 'queue looping track' (QUEUED_PLAY_LOOPED)
}

//===========================================================================

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
/* IRATA: unused
qboolean CG_Asset_Parse(int handle) {
	pc_token_t token;
	const char *tempStr;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}

	while ( 1 ) {
		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize, fontIndex;
			if( !PC_Int_Parse( handle, &fontIndex ) || !PC_String_Parse (handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) ) {
				return qfalse;
			}
			if( fontIndex < 0 || fontIndex >= 6 ) {
				return qfalse;
			}
			cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.fonts[fontIndex] );
			continue;
		}

		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qtrue );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qtrue  );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qtrue );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qtrue );
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) {
			if (!PC_String_Parse(handle, &cgDC.Assets.cursorStr)) {
				return qfalse;
			}
			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &cgDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &cgDC.Assets.shadowColor)) {
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			continue;
		}
	}
	//return qfalse;
}
*/

static qboolean CG_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
	return qfalse;
}

static int CG_FeederCount(float feederID) {
	int count = 0;

	if (feederID == FEEDER_REDTEAM_LIST) {
		int i;

		for (i = 0; i < cg.numScores; ++i) {
			if (cg.scores[i].team == TEAM_AXIS) {
				count++;
			}
		}
	}
	else if (feederID == FEEDER_BLUETEAM_LIST) {
		int i;

		for (i = 0; i < cg.numScores; ++i) {
			if (cg.scores[i].team == TEAM_ALLIES) {
				count++;
			}
		}
	}
	else if (feederID == FEEDER_SCOREBOARD) {
		return cg.numScores;
	}
	return count;
}


///////////////////////////
///////////////////////////

static clientInfo_t * CG_InfoFromScoreIndex(int index, int team, int *scoreIndex) {
	int i, count = 0;

	for (i = 0; i < cg.numScores; ++i) {
		if (cg.scores[i].team == team) {
			if (count == index) {
				*scoreIndex = i;
				return &cgs.clientinfo[cg.scores[i].client];
			}
			count++;
		}
	}

	*scoreIndex = index;
	return &cgs.clientinfo[ cg.scores[index].client ];
}

static const char *CG_FeederItemText(float feederID, int index, int column, qhandle_t *handle, int *numhandles ) {
	int scoreIndex = 0;
	clientInfo_t *info = NULL;
	int team = -1;
	score_t *sp = NULL;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_AXIS;
	}
	else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_ALLIES;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	if (info && info->infoValid) {
		switch (column) {
			case 0:
			break;
			case 3:
				return info->name;
			break;
			case 4:
				return va("%i", info->score);
			break;
			case 5:
				return va("%4i", sp->time);
			break;
			case 6:
				if ( sp->ping == -1 ) {
					return "connecting";
				}
				return va("%4i", sp->ping);
			break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage(float feederID, int index) {
	return 0;
}

static void CG_FeederSelection(float feederID, int index) {
	int i, count = 0;
	int team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_AXIS : TEAM_ALLIES;

	for (i = 0; i < cg.numScores; ++i) {
		if (cg.scores[i].team == team) {
			if (index == count) {
				cg.selectedScore = i;
			}
			count++;
		}
	}
}

float CG_Cvar_Get(const char *cvar) {
	char buff[128];

	memset(buff, 0, sizeof(buff));
	trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return atof(buff);
}

void CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style) {
	CG_Text_Paint(x, y, scale, color, text, 0, limit, style);
}

static int CG_OwnerDrawWidth(int ownerDraw, float scale) {
	//switch (ownerDraw) {
	//	default:
	//		break;
	//}
	return 0;
}

/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu() {
	cgDC.registerShaderNoMip	= &trap_R_RegisterShaderNoMip;
	cgDC.setColor				= &trap_R_SetColor;
	cgDC.drawHandlePic			= &CG_DrawPic;
	cgDC.drawStretchPic 		= &trap_R_DrawStretchPic;
	cgDC.drawText				= &CG_Text_Paint;
	cgDC.drawTextExt			= &CG_Text_Paint_Ext;
	cgDC.textWidth				= &CG_Text_Width;
	cgDC.textWidthExt			= &CG_Text_Width_Ext;
	cgDC.textHeight				= &CG_Text_Height;
	cgDC.textHeightExt			= &CG_Text_Height_Ext;
	cgDC.textFont				= &CG_Text_SetActiveFont;
	cgDC.registerModel			= &trap_R_RegisterModel;
	cgDC.modelBounds			= &trap_R_ModelBounds;
	cgDC.fillRect 				= &CG_FillRect;
	cgDC.drawRect 				= &CG_DrawRect;
	cgDC.drawSides				= &CG_DrawSides;
	cgDC.drawTopBottom			= &CG_DrawTopBottom;
	cgDC.clearScene				= &trap_R_ClearScene;
	cgDC.addRefEntityToScene	= &trap_R_AddRefEntityToScene;
	cgDC.renderScene			= &trap_R_RenderScene;
	cgDC.registerFont			= &trap_R_RegisterFont;
	cgDC.ownerDrawItem			= &CG_OwnerDraw;
	cgDC.getValue				= &CG_GetValue;
	cgDC.ownerDrawVisible		= &CG_OwnerDrawVisible;
	cgDC.runScript				= &CG_RunMenuScript;
	cgDC.getTeamColor			= &CG_GetTeamColor;
	cgDC.setCVar				= trap_Cvar_Set;
	cgDC.getCVarString			= trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue			= CG_Cvar_Get;
	cgDC.drawTextWithCursor		= &CG_Text_PaintWithCursor;
	cgDC.setOverstrikeMode 		= &trap_Key_SetOverstrikeMode;
	cgDC.getOverstrikeMode 		= &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound		= &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey		= &CG_OwnerDrawHandleKey;
	cgDC.feederCount			= &CG_FeederCount;
	cgDC.feederItemImage		= &CG_FeederItemImage;
	cgDC.feederItemText			= &CG_FeederItemText;
	cgDC.feederSelection		= &CG_FeederSelection;
	cgDC.setBinding				= &trap_Key_SetBinding;				// NERVE - SMF
	cgDC.getBindingBuf			= &trap_Key_GetBindingBuf;			// NERVE - SMF
	cgDC.getKeysForBinding		= &trap_Key_KeysForBinding;
	cgDC.keynumToStringBuf		= &trap_Key_KeynumToStringBuf;		// NERVE - SMF
	cgDC.translateString		= &CG_TranslateString;				// NERVE - SMF
	cgDC.Error 					= &Com_Error;
	cgDC.Print 					= &Com_Printf;
	cgDC.ownerDrawWidth			= &CG_OwnerDrawWidth;
	cgDC.registerSound			= &trap_S_RegisterSound;
	cgDC.startBackgroundTrack	= &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack	= &trap_S_StopBackgroundTrack;
	cgDC.descriptionForCampaign = &CG_DescriptionForCampaign;
	cgDC.nameForCampaign		= &CG_NameForCampaign;
	cgDC.add2dPolys				= &trap_R_Add2dPolys;
	cgDC.updateScreen			= &trap_UpdateScreen;
	cgDC.getHunkData			= &trap_GetHunkData;
	cgDC.getConfigString		= &CG_ConfigStringCopy;

	cgDC.xscale 				= cgs.screenXScale;
	cgDC.yscale 				= cgs.screenYScale;

	Init_Display(&cgDC);

	Menu_Reset();

	CG_Text_SetActiveFont( 0 );
}

void CG_AssetCache() {
	cgDC.Assets.gradientBar 		= trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	cgDC.Assets.fxBasePic			= trap_R_RegisterShaderNoMip( ART_FX_BASE );
	cgDC.Assets.fxPic[0] 			= trap_R_RegisterShaderNoMip( ART_FX_RED );
	cgDC.Assets.fxPic[1] 			= trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	cgDC.Assets.fxPic[2] 			= trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	cgDC.Assets.fxPic[3] 			= trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	cgDC.Assets.fxPic[4] 			= trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	cgDC.Assets.fxPic[5] 			= trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	cgDC.Assets.fxPic[6] 			= trap_R_RegisterShaderNoMip( ART_FX_WHITE );
	cgDC.Assets.scrollBar			= trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	cgDC.Assets.scrollBarArrowDown	= trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	cgDC.Assets.scrollBarArrowUp	= trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	cgDC.Assets.scrollBarArrowLeft	= trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	cgDC.Assets.scrollBarThumb		= trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	cgDC.Assets.sliderBar			= trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	cgDC.Assets.sliderThumb			= trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
}


// kw: tries to exec a cfg file if it is found
qboolean CG_execFile( char *filename ) {
	int handle = trap_PC_LoadSource( va( "%s.cfg", filename) );

	trap_PC_FreeSource(handle);
	if( !handle ) {
		// file not found
		return qfalse;
	}
	trap_SendConsoleCommand( va("exec %s.cfg\n", filename) );
	return qtrue;
}

#ifdef HW_BAN

// TODO:
//
// *GetParsedIP does already exist in g_client.c
// - use one function and make it available to both game and cgame

// redeye - copied from ETPub, courtesy of Dens
const char *GetParsedIP(const char *ipadd) {
	// code by Dan Pop, http://bytes.com/forum/thread212174.html
	unsigned b1, b2, b3, b4, port = 0;
	unsigned char c;
	int rc;
	static char ipge[20];

	if(!Q_strncmp(ipadd, "localhost", 9)) {
		return "localhost";
	}

	rc = sscanf(ipadd, "%3u.%3u.%3u.%3u:%u%c", &b1, &b2, &b3, &b4, &port, &c);
	if (rc < 4 || rc > 5) {
		return NULL;
	}
	if ( (b1 | b2 | b3 | b4) > 255 || port > 65535) {
		return NULL;
	}
	if (strspn(ipadd, "0123456789.:") < strlen(ipadd)) {
		return NULL;
	}
	sprintf(ipge, "%u.%u.%u.%u", b1, b2, b3, b4);
	return ipge;
}

unsigned int GetParsedPort(const char *ipadd) {
	// code by Dan Pop, http://bytes.com/forum/thread212174.html
	unsigned b1, b2, b3, b4, port = 0;
	unsigned char c;
	int rc;

	if(!Q_strncmp(ipadd, "localhost", 9)) {
		return 27960;
	}

	rc = sscanf(ipadd, "%3u.%3u.%3u.%3u:%u%c", &b1, &b2, &b3, &b4, &port, &c);
	if (rc < 4 || rc > 5) {
		return 0;
	}

	if ( (b1 | b2 | b3 | b4) > 255 || port > 65535) {
		return 0;
	}

	return port;
}
#endif // HW_BAN

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
#ifdef _DEBUG
#define DEBUG_INITPROFILE_INIT int elapsed, dbgTime = trap_Milliseconds();
#define DEBUG_INITPROFILE_EXEC(f) if( developer.integer ) { CG_Printf("^5%s passed in %i msec\n", f, elapsed = trap_Milliseconds()-dbgTime );  dbgTime += elapsed; }
#endif // _DEBUG

char* BindingFromName(const char *cvar);

qboolean isET260( void );

#ifdef SYSTEM_CHECK
/*
 * Helper function to get size of file
 *
 */
int fsize(FILE *fp) {
    int prev=ftell(fp);
    int sz;
    fseek(fp, 0L, SEEK_END);
    sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}

/*
 * WIP
 * Get the name of the executable
 * TODO: check file size of executable (for ET it's fixed 2.60. 260b)
 * TODO: check build date
 * TODO: send this info to server via messages ... if there is no message received we also know it's 2.55
 *		 find a way to resend info if UDP packet is lost
 *
 *  System flags:
 *  1 - vanilla client 2.60b
 *  2 - vanilla client 2.60
 *  4 - etlegacy
 *  8 - openwolf
 *  16 - ET/GPL executable
 *  32 - free
 *  64 - other
 *  ------------------------------------------------------------------------
 *  Since PB is gone (see pb /players cmd) let admins know the client system
 *  If non of these flags are set we have case 'other'
 *  128 	- windows 32 bit
 *  256 	- windows 64 bit
 *  512 	- linux 32 bit
 *  1024 	- linux 64 bit
 *  2048 	- mac 32 bit
 *  4096    - mac 64 bit
 *
 *  ...
 *  32768   - untrusted, always but not in case 260/b and correct file size
 */
void checkExecutable() {
	char basepath[MAX_OSPATH];
	qboolean untrusted = qtrue;
	int fileSize = 0;

// FIXME/TODO move this down to enable other engines
#ifndef _DEBUG
	if ( !isET260() ) trap_Error("");
#endif

	trap_Cvar_VariableStringBuffer("fs_basepath", basepath, sizeof(basepath));

/* alternative way for linux - not used atm
#ifdef __linux__
	// get the status file from /proc and read first line which contains info
	// about the process name (which is of interest to get used engine
	// note: as long as execuable is not renamed this should work properly
	char *fname = va("/proc/%d/status", getpid());
	FILE *file = fopen(fname,"r");

	if (file == NULL) {
		CG_Error("Bad system");
		//CG_Printf("File NOT %s found\n", fname);
	}
	else {
		char s[80]; // max file line length in bytes

		// CG_Printf("File %s found\n", fname);
		if (fgets(s, 80, file) == NULL) CG_Error("Bad game system.");
		//CG_Printf("Line: %s", s);
		fclose(file);

		// first line of status file is
		// Name:<tab>processname<newline>
		engine = strtok( s, "\t" ); // remove Name:
		if (engine == NULL || !engine[0]) CG_Error("Bad game system.");

		engine = strtok( NULL, "\t" );
		if (engine == NULL || !engine[0]) CG_Error("Bad game system.");

		engine = strtok( engine, "\n" ); // remove newline and rest
		if (engine == NULL || !engine[0]) CG_Error("Bad game system.");

		CG_Printf("Used executable: %s\n", engine);
	}
#endif
*/

#ifdef __linux__
	// debug
	CG_Printf("Executable name: %s\n", __progname);

	if (strcmp(__progname, "et.x86") == 0) {
		char *fname = va("%s/%s", basepath, __progname);
		FILE *file = NULL;

		if ( !isET260() ) trap_Error("");

		// Note: if we this fails client will crash
		file = fopen(fname,"r");
		fileSize = fsize( file  );
		fclose(file);

		// 2.60b
		CG_Printf("File size: %i\n", fileSize);
		if (fileSize == 1604328 ) { // 2.60b -> 1604328
			untrusted = qfalse;
			systemFlag |= 1;
		}
		else if (fileSize == 10) { // 2.60 TODO
			untrusted = qfalse;
			systemFlag |= 2;
		}
		else {
			systemFlag |= 16;
		}

	}
	else if (strcmp(__progname, "etl") == 0) {
		systemFlag |= 4;
	}
	else if (strcmp(__progname, "wolfet") == 0) {
		systemFlag |= 8;
	}
	else { // other
		systemFlag |= 64;
	}

#endif

#ifdef WIN32
	{
		HANDLE 						process=GetCurrentProcess();

		if (process != NULL ) {
			char* progname = "ET.exe"; // FIXME
			TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

			GetModuleBaseName( process, NULL, szProcessName, sizeof(szProcessName)/sizeof(TCHAR) );

			// Print the process name and identifier.
			CG_Printf("Processname = %s\n", szProcessName);
			// ... end FIXME


			if (strcmp(progname, "ET.exe") == 0) {
				char *fname = va("%s/%s", basepath, progname);

				if ( !isET260() ) trap_Error("");

				if (fileSize == 1286144 ) { // 2.60b -> TODO
					untrusted = qfalse;
					systemFlag |= 1;
				}
				else if (fileSize == 10) { // 2.60 TODO
					untrusted = qfalse;
					systemFlag |= 2;
				}
				else {
					systemFlag |= 16;
				}

			}
			else if (strcmp(progname, "etl.exe")) { // TODO
				systemFlag |= 4;
			}
			else if (strcmp(progname, "wolfet.exe") == 0) { // TODO
				systemFlag |= 8;
			}
			else { // other
				systemFlag |= 64;
			}
		}
	}
#endif

#ifdef __MACOS__
	{
		char *fname = va("%s/%s", basepath, __progname);
		FILE *file = NULL;
		static char versionStr[256];

		trap_Cvar_VariableStringBuffer( "version", versionStr, 256 );

		//if ( !isET260() ) trap_Error("");

		// Note: if we this fails client will crash
		file = fopen(fname,"r");
		fileSize = fsize( file  );
		fclose(file);

		// 2.60d
		CG_Printf("Version: %s\n", versionStr);
		CG_Printf("File size: %s %s\n", __progname, fileSize);
	}
#endif

	if (untrusted == qtrue) {

		systemFlag |= 32768; // incorrect file size for vanilla client (GPL self compiled) or other
	}

// OS check - if these flags are not set: OS is 'other'
#ifdef __linux__
	#ifdef __x86_64__
	systemFlag |= 1024;
	#else
	systemFlag |= 512;
	#endif
#endif

#ifdef WIN32 // TODO: detect 64 bit
	systemFlag |= 128;
#endif

#ifdef __MACOS__ // TODO: detect 64 bit (if there is a way to do AND if it does exist)
	systemFlag |= 2048;
#endif

	{
		unsigned char	string[9]; // PACKET_OFFSET + 4 (int)

		*(unsigned char *)&string[0]	= PACKET_C_SENDS_CLIENT_INFO;	// packet type..
		*(int *)&string[PACKET_OFFSET] = systemFlag;

		// debug
		//CG_Printf("checkExecutable() sending system info : %i\n", systemFlag);
		//CG_Printf("SENDING INFO: %s\n", string);
		trap_SendMessage( (char*)&string, 9 );
	}
}

/*
 * This function determines other valid clients
 *
 * TODO/FIXME: check OS
 * 		 check mod_version?
 * 		check cl_punkbuster
 * 		arch, net/mac,

char* isETClone( void ) {
	static char versionStr[256];
	static char versionbuild[256];

	trap_Cvar_VariableStringBuffer( "version", versionStr, 256 );

	trap_Cvar_VariableStringBuffer( "dsd", versionbuild, 256 );


	//TODO: identify unique engine cvars and store them

	if (strstr(versionStr, "OpenWolf")) {
		return "OpenWolf";
	}

	// detect etlegacy

	// xreal ?

	// all d versions ???
	//if (strstr(versionStr, "ET 2.60d ")) {
	//	return qtrue;
	//}

#if defined(__MACOS__)
	//if (!Q_stricmp(versionStr, "ET 2.60d OSX-universal JAN 20 2007"))
		return "Mac";
#endif

	return "";
}
*/
#endif // SYSTEM_CHECK

void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum, qboolean demoPlayback ) {
	const char	*s;
	int			i;
#ifdef HW_BAN
	const char*         serverInfo;

	const char* 		remoteServerName;
	char				remoteServerWithPort[MAX_INFO_VALUE];
	const char*			remoteServerIP;
	unsigned int        remoteServerPort = 0;
#endif // HW_BAN

#ifdef _DEBUG
	DEBUG_INITPROFILE_INIT
#endif // _DEBUG

#ifdef SYSTEM_CHECK
	checkExecutable();
#else
#ifndef _DEBUG
	if ( !isET260() ) trap_Error("");
#endif
#endif

	// tjw: clean up the config backup if one exists
	CG_RestoreProfile();

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof(cg_entities) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );
	memset( cg_items, 0, sizeof(cg_items) );

	cgs.initing = qtrue;

	// core: initialize data for faster direct indexed access..
	Init_FindClipForWeapon();
	Init_FindAmmoForWeapon();

	// core: reset weapon restriction data..
	C_ResetWeaponRestrictions();

	for( i = 0; i < cgs.maxclients; ++i ) {
		cg.artilleryRequestTime[i] = -99999;
	}

	CG_InitStatsDebug();

	cgs.ccZoomFactor = 1.f;

	// OSP - sync to main refdef
	cg.refdef_current = &cg.refdef;

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0f;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0f;
	// core: widescreen support..
	cgs.aspectratio = (float)(cgs.glconfig.vidWidth) / cgs.glconfig.vidHeight;
	cgs.aspectratio1 = 1.0f / cgs.aspectratio;
	cgs.adr43 = cgs.aspectratio * RPRATIO43;	// aspectratio / (4/3)
	cgs.r43da = RATIO43 * cgs.aspectratio1;		// (4/3) / aspectratio
	cgs.wideXoffset = (cgs.aspectratio != RATIO43)? (640.0f * cgs.adr43 - 640.0f) * 0.5f : 0.0f;

	// RF, init the anim scripting
	cgs.animScriptData.soundIndex = CG_SoundScriptPrecache;
	cgs.animScriptData.playSound = CG_SoundPlayIndexedScript;

	cg.clientNum = clientNum;		// NERVE - SMF - TA merge

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	cgs.ccRequestedObjective = -1;
	cgs.ccCurrentCamObjective = -2;

	// CHRUKER: b079 - Background images on the loading screen were not visible on the first call
	trap_R_SetColor( NULL );

    	// load a few needed things before we do any screen updates
	cgs.media.charsetShader		= trap_R_RegisterShader( "gfx/2d/hudchars" ); //trap_R_RegisterShader( "gfx/2d/bigchars" );
	// JOSEPH 4-17-00
	cgs.media.menucharsetShader = trap_R_RegisterShader( "gfx/2d/hudchars" );
	// END JOSEPH
	cgs.media.whiteShader		= trap_R_RegisterShader( "white" );
	cgs.media.charsetProp		= trap_R_RegisterShaderNoMip( "menu/art/font1_prop.tga" );
	cgs.media.charsetPropGlow	= trap_R_RegisterShaderNoMip( "menu/art/font1_prop_glo.tga" );
	cgs.media.charsetPropB		= trap_R_RegisterShaderNoMip( "menu/art/font2_prop.tga" );

	CG_RegisterCvars();

	if ( cg_logFile.string[0] ) {
		trap_FS_FOpenFile( cg_logFile.string, &cg.logFile, FS_APPEND );

		if (!cg.logFile) {
			CG_Printf( "^3WARNING: Couldn't open client log: %s\n", cg_logFile.string);
		}
	}
	else {
		CG_Printf( "Not logging client output to disk.\n" );
	}

	CG_InitConsoleCommands();

	// Gordon: moved this up so it's initialized for the loading screen
	CG_LoadHudMenu();      // load new hud stuff
	CG_AssetCache();

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	cg.warmupCount = -1;

	CG_ParseServerinfo();
	CG_ParseWolfinfo();		// NERVE - SMF

	// core: new configstring method..
	C_ParseCSMethodInfo();

	// kw: try execing map autoexec scripts
	if( !CG_execFile( va("autoexec_%s", cgs.rawmapname) ) ) {
		CG_execFile( "autoexec_default" );
	}

	cgs.campaignInfoLoaded = qfalse;
	if( cgs.gametype == GT_WOLF_CAMPAIGN ) {
		CG_LocateCampaign();
	}
	else if( cgs.gametype == GT_WOLF_STOPWATCH ||
			cgs.gametype == GT_WOLF_LMS ||
			cgs.gametype == GT_WOLF ||
			cgs.gametype == GT_WOLF_MAPVOTE ) {
		CG_LocateArena();
	}

	CG_ClearTrails();
	// CG_ClearParticles(); IRATA: was done twice - see CG_RegisterGraphics() (about 20 lines later of this function)
	// InitSmokeSprites(); IRATA: was done twice - see CG_RegisterGraphics() (about 20 lines later of this function)

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if ( strcmp( s, GAME_VERSION ) ) {
		CG_Error( "Client/Server game mismatch: '%s/%s'", GAME_VERSION, s );
	}

	// IRATA no use for this
	// trap_Cvar_Set( "cg_etVersion", GAME_VERSION_DATED );	// So server can check

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	s = CG_ConfigString( CS_INTERMISSION_START_TIME );
	cgs.intermissionStartTime = atoi( s );


	// OSP
	CG_ParseReinforcementTimes(CG_ConfigString(CS_REINFSEEDS));

	CG_initStrings();
	CG_windowInit();

#ifdef _DEBUG
	DEBUG_INITPROFILE_EXEC ( "initialization" )
#endif // DEBUG

	// load the new map
	CG_LoadingString( ":collision map:" );

	trap_CM_LoadMap( cgs.mapname );

#ifdef _DEBUG
	DEBUG_INITPROFILE_EXEC ( "loadmap" )
#endif // DEBUG

	String_Init();

	cg.loading = qtrue;		// force players to load instead of defer

	CG_LoadingString( ":sounds:" );
	CG_RegisterSounds();

#ifdef _DEBUG
	DEBUG_INITPROFILE_EXEC ( "sounds" )
#endif // DEBUG

	CG_LoadingString( ":graphics:" );
	CG_RegisterGraphics();

	CG_LoadingString( ":flamechunks:" );
	CG_InitFlameChunks();		// RF, register and clear all flamethrower resources

#ifdef _DEBUG
	DEBUG_INITPROFILE_EXEC ( "graphics" )
#endif // DEBUG

	// core: initialize data for faster direct indexed access..
	Init_Obituaries();

	CG_LoadingString( ":clients:" );
	CG_RegisterClients();		// if low on memory, some clients will be deferred

#ifdef _DEBUG
	DEBUG_INITPROFILE_EXEC ( "clients" )
#endif // DEBUG

	cg.loading = qfalse;	// future players will be deferred

	CG_InitLocalEntities();
	BG_BuildSplinePaths();
	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	cg.lightstylesInited = qfalse;

	CG_LoadingString( "" );
	if ( cgs.csMethod == 0 ) CG_ShaderStateChanged();
	CG_ChargeTimesChanged();

	trap_S_ClearLoopingSounds();
	trap_S_ClearSounds( qfalse );

	cg.teamWonRounds[1] = atoi( CG_ConfigString( CS_ROUNDSCORES1 ) );
	cg.teamWonRounds[0] = atoi( CG_ConfigString( CS_ROUNDSCORES2 ) );

	cg.filtercams = atoi( CG_ConfigString( CS_FILTERCAMS ) ) ? qtrue : qfalse;

	CG_ParseFireteams();

	CG_InitPM();

	CG_InitMM();

	CG_ParseSpawns();

	CG_ParseTagConnects();

#ifdef _DEBUG
	DEBUG_INITPROFILE_EXEC ( "misc" )
#endif // DEBUG

	CG_ParseSkyBox();
	CG_SetupCabinets();

	trap_S_FadeAllSound(1.0f, 0, qfalse);	// fade sound up

	// OSP
	cgs.dumpStatsFile = 0;
	cgs.dumpStatsTime = 0;

	cg.crosshairMine = -1;
	cg.crosshairDynamite = -1;

	// jaquboss clientSide completly now
	CG_LoadLocations();

    memset( cgs.clientLocation, 0, sizeof( cgs.clientLocation ) );

#ifdef __SSYSTEM__
	for ( i = 0 ; i < NUM_PLAYER_CLASSES ; ++i ){
		CG_LoadPcInfo(i,TEAM_AXIS);
		CG_LoadPcInfo(i,TEAM_ALLIES);
	}
#endif

	// jaquboss grab the NQ info so we are sure there is latest
	CG_ParseNQinfo();
	CG_UpdateSkills();
	CG_UpdateForceCvars();
	CG_UpdateSvCvars();

	// IRATA: init the HUD colors from the config (HUDx vars are not cg_HUDx vars! - we have to to this explicitly)
	jP_SetHUDColors();

	// rebind weapalt to +attack2
	if ( Q_stricmp(BindingFromName( "weapalt" ), "[weapalt]" )  ) {
		trap_SendConsoleCommand( va("bind %s +attack2\n", BindingFromName( "weapalt" ) ));
	}

	CG_ParseOIDInfos();

#ifdef HW_BAN
	//Set the HWGUID based on the server Name, server Port and client id
	trap_Cvar_VariableStringBuffer("cl_currentServerIP", remoteServerWithPort, sizeof(remoteServerWithPort));

	serverInfo = CG_ConfigString( CS_SERVERINFO );
	remoteServerName = Info_ValueForKey( serverInfo, "sv_hostname" );

	remoteServerIP   = GetParsedIP(remoteServerWithPort);
	remoteServerPort = GetParsedPort(remoteServerWithPort);

	trap_Cvar_Set("cg_hwguid", getEncryptedHWGuid(remoteServerName, remoteServerPort, clientNum));
	//trap_Cvar_Set("cg_hwguid", va("Seeds: %s %u %u %u %u\n", remoteServerName,
	//	calculateCRC32(remoteServerName, strlen(remoteServerName)),
	//	remoteServerPort * remoteServerPort,
	//	calculateCRC32(remoteServerName, strlen(remoteServerName)) * clientNum,
	//	calculateCRC32(remoteServerName, strlen(remoteServerName)) * remoteServerPort));
#endif // HW_BAN

#ifdef AUTO_GUID
	// move to ui (so it's already set if cgame is not loaded yet)
	// or just move it up ?
    CG_InitNQGUID();
#endif

	// core: new configstring handling..
	// Set the flag to send a request to the server,
	// for sending the new configstrings..
	cgs.csMethod_refresh = 1;
	cgs.csMethod_request = 0;
	cgs.csMethod_requesting = 0;
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
	CG_EventHandling( CGAME_EVENT_NONE, qtrue );
	if(cg.demoPlayback) {
		trap_Cvar_Set("timescale", "1");
	}

	if ( cg.logFile ){
		trap_FS_FCloseFile( cg.logFile );
		cg.logFile = 0;
	}

	CG_RestoreProfile();
}

qboolean CG_CheckExecKey( int key ) {
	if( !cg.showFireteamMenu ) {
		return qfalse;
	}

	return CG_FireteamCheckExecKey( key, qfalse );
}

void jP_SetHUDColors(void) {
	vec3_t	newBack;
	vec3_t	newBorder;
	int		count;//, i;
	char	*nextVal;
	char	background[MAX_CVAR_VALUE_STRING];
	char	border[MAX_CVAR_VALUE_STRING];

	Q_strncpyz( border,		cg_HUDBorderColor.string, 	  sizeof(cg_HUDBorderColor.string)		);
	Q_strncpyz( background, cg_HUDBackgroundColor.string, sizeof(cg_HUDBackgroundColor.string)	);

	if (!Q_stricmp( border, "default")||!Q_stricmp( border, "def")) {
		trap_Cvar_Set( "cg_HUDBorderColor", ".5 .5 .5");
	}
	else if (!Q_stricmp( border, "red")) {
		trap_Cvar_Set( "cg_HUDBorderColor", ".75 .0 .0");
	}
	else if (!Q_stricmp( border, "green")) {
		trap_Cvar_Set( "cg_HUDBorderColor", ".0 .75 .0");
	}
	else if (!Q_stricmp( border, "blue")) {
		trap_Cvar_Set( "cg_HUDBorderColor", ".0 .0 .75");
	}
	else if (!Q_stricmp( border, "black")) {
		trap_Cvar_Set( "cg_HUDBorderColor", ".0 .0 .0");
	}
	else if (!Q_stricmp( border, "white")) {
		trap_Cvar_Set( "cg_HUDBorderColor", "1.0 1.0 1.0");
	}
	else if (!Q_stricmp( border, "dkgrey") || !Q_stricmp( border, "darkgrey") ||
		       !Q_stricmp( border, "dkgray") || !Q_stricmp( border, "darkgray")) {
		trap_Cvar_Set( "cg_HUDBorderColor", ".25 .25 .25");
	}
	else if (!Q_stricmp( border, "grey")   || !Q_stricmp( border, "gray")) {
		trap_Cvar_Set( "cg_HUDBorderColor", ".5 .5 .5");
	}
	else if (!Q_stricmp( border, "cthulhu")   || !Q_stricmp( border, "cthulhu_green")) {
		trap_Cvar_Set( "cg_HUDBorderColor", "0 .25 .25");
	}

	if (!Q_stricmp( background, "default")||!Q_stricmp( background, "def")) {
		trap_Cvar_Set( "cg_HUDBackgroundColor", ".5 .5 .5");
	}
	else if (!Q_stricmp( background, "red")) {
		trap_Cvar_Set( "cg_HUDBackgroundColor", ".75 .0 .0");
	}
	else if (!Q_stricmp( background, "green")) {
		trap_Cvar_Set( "cg_HUDBackgroundColor", ".0 .75 .0");
	}
	else if (!Q_stricmp( background, "blue")) {
		trap_Cvar_Set( "cg_HUDBackgroundColor", ".0 .0 .75");
	}
	else if (!Q_stricmp( background, "black")) {
		trap_Cvar_Set( "cg_HUDBackgroundColor", ".0 .0 .0");
	}
	else if (!Q_stricmp( background, "white")) {
		trap_Cvar_Set( "cg_HUDBackgroundColor", "1.0 1.0 1.0");
	}
	else if (!Q_stricmp( background, "dkgrey") || !Q_stricmp( background, "darkgrey") ||
		       !Q_stricmp( background, "dkgray") || !Q_stricmp( background, "darkgray")) {
		trap_Cvar_Set( "cg_HUDBackgroundColor", ".25 .25 .25");
	}
	else if (!Q_stricmp( background, "grey")   || !Q_stricmp( background, "gray")) {
		trap_Cvar_Set( "cg_HUDBackgroundColor", ".5 .5 .5");
	}
	else if (!Q_stricmp( border, "cthulhu")   || !Q_stricmp( border, "cthulhu_green")) {
		trap_Cvar_Set( "cg_HUDBorderColor", "0 .25 .25");
	}

	nextVal = strtok(border, " ");

	for (count = 0; count < 3 && nextVal; ++count) {
		newBorder[count] = atof(nextVal);
		nextVal			 = strtok(NULL, " ,");
	}

	nextVal = strtok(background, " ");

	for (count = 0; count < 3 && nextVal; ++count) {
		newBack[count] = atof(nextVal);
		nextVal		   = strtok(NULL, " ,");
	}

	HUD_Alpha = cg_HUDAlpha.value;
	Vector4Set(HUD_Border,	   newBorder[0], newBorder[1], newBorder[2], HUD_Alpha);
	Vector4Set(HUD_Background, newBack[0],   newBack[1],   newBack[2],   HUD_Alpha);
}

qhandle_t CG_GetGameModel ( int index ) {
    // Chached game file
    if (index < GAMEMODEL_MAX) {
        return cgs.cachedModels[index];
    }

	return (cgs.gameModels[index-GAMEMODEL_MAX] ? cgs.gameModels[index-GAMEMODEL_MAX] : 0);
}

sfxHandle_t CG_GetGameSound ( int index ) {
    // Cached game file
    if (index < GAMESOUND_MAX) {
        return cgs.cachedSounds[index];
    }
    return cgs.gameSounds[index-GAMESOUND_MAX] ? cgs.gameSounds[index-GAMESOUND_MAX] : 0;
}

// dvl - real time stamp
char *CG_GetRealTime(void) {
	qtime_t tm;

	trap_RealTime(&tm);
	return va("%2i:%s%i:%s%i",
				tm.tm_hour,
				(tm.tm_min > 9 ? "" : "0"),// minute padding
				tm.tm_min,
				(tm.tm_sec > 9 ? "" : "0"),// second padding
				tm.tm_sec );
}

void QDECL CG_WriteToLog( const char *fmt, ... ) {
	if ( !cg.logFile ) {
		return;
	}
	else {
		va_list			argptr;
		char			string[1024];
		int				l;

		Com_sprintf( string, sizeof(string), "%s ", CG_GetRealTime() );

		l = strlen( string );

		va_start( argptr, fmt );
		Q_vsnprintf( string + l, sizeof( string ) - l, fmt, argptr );
		va_end( argptr );

		trap_FS_Write( string, strlen( string ), cg.logFile );
	}
}

void QDECL CG_WriteToLog( const char *fmt, ... ) _attribute((format(printf,1,2)));

void CG_ReceiveTeamInfo( unsigned char *buffer, int bufferlen, int commandTime ) {
	int		i;
	int		client;
	int		numSortedTeamPlayers = *(int *)(buffer+1);
	int		ofs = PACKET_OFFSET;

	// core: The old check was limited to max. 32 players per team..
	if( numSortedTeamPlayers < 0 || numSortedTeamPlayers >= MAX_CLIENTS ) {
		CG_Printf( "CG_ReceiveTeamInfo: numSortedTeamPlayers out of range (%d)", numSortedTeamPlayers );
		return;
	}

	// safety check..
	if ( numSortedTeamPlayers*19 + ofs > bufferlen ) {
		CG_Printf( "CG_ReceiveTeamInfo: buffer size mismatch (%d vs %d)", numSortedTeamPlayers*19 + ofs, bufferlen );
		return;
	}

	for ( i = 0 ; i < numSortedTeamPlayers ; i++ ) {
		client = (int)(*(unsigned char *)(buffer+ofs));

		if( client < 0 || client >= MAX_CLIENTS ) {
		  CG_Printf( "CG_ReceiveTeamInfo: bad client number: %d", client );
		  return;
		}

		cgs.clientinfo[	client ].location[0]	= *(int *)(buffer+ofs+1);
		cgs.clientinfo[	client ].location[1]	= *(int *)(buffer+ofs+5);
		cgs.clientinfo[	client ].location[2]	= *(int *)(buffer+ofs+9);
		cgs.clientinfo[	client ].health			= (int)(*(short *)(buffer+ofs+13));
		cgs.clientinfo[	client ].powerups		= *(int *)(buffer+ofs+15);

		ofs += 19;
	}
}
