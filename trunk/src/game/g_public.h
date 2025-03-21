 /*
 * name:		g_public.h - Copyright (C) 1999-2000 Id Software, Inc.
 *
 * desc:		game module information visible to server
 *
 * NQQS:
 *
 */

#ifndef __G_PUBLIC_H__
#define __G_PUBLIC_H__

#define	GAME_API_VERSION	8

// entity->svFlags
// the server does not know how to interpret most of the values
// in entityStates (level eType), so the game must explicitly flag
// special server behaviors
#define	SVF_NOCLIENT				0x00000001	// don't send entity to clients, even if it has effects
#define SVF_VISDUMMY				0x00000004	// this ent is a "visibility dummy" and needs it's master to be sent to
												// clients that can see it even if they can't see the master ent
#define SVF_BOT						0x00000008	//
// #define SVF_POW					0x00000010	// Gordon: stole SVF_CASTAI as it's no longer used IRATA: unused

#define	SVF_BROADCAST				0x00000020	// send to all connected clients
#define	SVF_PORTAL					0x00000040	// merge a second pvs at origin2 into snapshots
#define	SVF_BLANK					0x00000080	// Gordon: removed SVF_USE_CURRENT_ORIGIN as it plain doesnt do anything
#define	SVF_NOFOOTSTEPS				0x00000100

// MrE:
#define SVF_CAPSULE					0x00000200	// use capsule for collision detection
#define SVF_VISDUMMY_MULTIPLE		0x00000400	// so that one vis dummy can add to snapshot multiple speakers

// recent id changes
#define SVF_SINGLECLIENT			0x00000800	// only send to a single client (entityShared_t->singleClient)
#define SVF_NOSERVERINFO			0x00001000	// don't send CS_SERVERINFO updates to this client so that it can be
												// updated for ping tools without lagging clients

#define SVF_NOTSINGLECLIENT			0x00002000	// send entity to everyone but one client (entityShared_t->singleClient)

// Gordon:
#define SVF_IGNOREBMODELEXTENTS		0x00004000	// just use origin for in pvs check for snapshots, ignore the bmodel extents
#define SVF_SELF_PORTAL				0x00008000	// use self->origin2 as portal // NOTE: There is a bug in the engine - this is broken! see http://bani.anime.net/banimod/forums/viewtopic.php?t=6184
#define SVF_SELF_PORTAL_EXCLUSIVE	0x00010000	// use self->origin2 as portal and DONT add self->origin PVS ents

//===============================================================


typedef qboolean (*addToSnapshotCallback)( int entityNum, int clientNum );

// IRATA: don't change this struct!
typedef struct {

	qboolean	linked;				// qfalse if not in any good cluster
	int			linkcount;

	int			svFlags;			// SVF_NOCLIENT, SVF_BROADCAST, etc
	int			singleClient;		// only send to this client when SVF_SINGLECLIENT is set

	qboolean	bmodel;				// if false, assume an explicit mins / maxs bounding box
									// only set by trap_SetBrushModel
	vec3_t		mins, maxs;
	int			contents;			// CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc
									// a non-solid entity should set to 0

	vec3_t		absmin, absmax;		// derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t		currentOrigin;
	vec3_t		currentAngles;

	// when a trace call is made and passEntityNum != ENTITYNUM_NONE,
	// an ent will be excluded from testing if:
	// ent->s.number == passEntityNum	(don't interact with self)
	// ent->s.ownerNum = passEntityNum	(don't interact with your own missiles)
	// entity[ent->s.ownerNum].ownerNum = passEntityNum	(don't interact with other missiles from owner)
	int			ownerNum;
	int			eventTime;

	int			worldflags;		// DHM - Nerve

	qboolean	snapshotCallback;
} entityShared_t;

// the server looks at a sharedEntity, which is the start of the game's gentity_t structure
typedef struct {
	entityState_t	s;				// communicated by server to clients
	entityShared_t	r;				// shared by both the server system and game
} sharedEntity_t;

//===============================================================

//
// system traps provided by the main engine
//
typedef enum {
	//============== general Quake services ==================

	G_PRINT = 0,		// ( const char *string );
	// print message on the local console

	G_ERROR,		// ( const char *string );
	// abort the game

	G_MILLISECONDS,	// ( void );
	// get current time for profiling reasons
	// this should NOT be used for any game related tasks,
	// because it is not journaled

	// console variable interaction
	G_CVAR_REGISTER,	// ( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
	G_CVAR_UPDATE,	// ( vmCvar_t *vmCvar );
	G_CVAR_SET,		// ( const char *var_name, const char *value );
	G_CVAR_VARIABLE_INTEGER_VALUE,	// ( const char *var_name );

	G_CVAR_VARIABLE_STRING_BUFFER,	// ( const char *var_name, char *buffer, int bufsize );

	G_CVAR_LATCHEDVARIABLESTRINGBUFFER,

	G_ARGC,			// ( void );
	// ClientCommand and ServerCommand parameter access

	G_ARGV,			// ( int n, char *buffer, int bufferLength );

	G_FS_FOPEN_FILE,	// ( const char *qpath, fileHandle_t *file, fsMode_t mode );
	G_FS_READ,		// ( void *buffer, int len, fileHandle_t f );
	G_FS_WRITE,		// ( const void *buffer, int len, fileHandle_t f );
	G_FS_RENAME,
	G_FS_FCLOSE_FILE,		// ( fileHandle_t f );

	G_SEND_CONSOLE_COMMAND,	// ( const char *text );
	// add commands to the console as if they were typed in
	// for map changing, etc


	//=========== server specific functionality =============

	G_LOCATE_GAME_DATA,		// ( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
	//							playerState_t *clients, int sizeofGameClient );
	// the game needs to let the server system know where and how big the gentities
	// are, so it can look at them directly without going through an interface

	G_DROP_CLIENT,		// ( int clientNum, const char *reason );
	// kick a client off the server with a message

	G_SEND_SERVER_COMMAND,	// ( int clientNum, const char *fmt, ... );
	// reliably sends a command string to be interpreted by the given
	// client.  If clientNum is -1, it will be sent to all clients

	G_SET_CONFIGSTRING,	// ( int num, const char *string );
	// config strings hold all the index strings, and various other information
	// that is reliably communicated to all clients
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	// All confgstrings are cleared at each level start.

	G_GET_CONFIGSTRING,	// ( int num, char *buffer, int bufferSize );

	G_GET_USERINFO,		// ( int num, char *buffer, int bufferSize );
	// userinfo strings are maintained by the server system, so they
	// are persistant across level loads, while all other game visible
	// data is completely reset

	G_SET_USERINFO,		// ( int num, const char *buffer );

	G_GET_SERVERINFO,	// ( char *buffer, int bufferSize );
	// the serverinfo info string has all the cvars visible to server browsers

	G_SET_BRUSH_MODEL,	// ( gentity_t *ent, const char *name );
	// sets mins and maxs based on the brushmodel name

	G_TRACE,	// ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection against all linked entities

	G_POINT_CONTENTS,	// ( const vec3_t point, int passEntityNum );
	// point contents against all linked entities

	G_IN_PVS,			// ( const vec3_t p1, const vec3_t p2 );

	G_IN_PVS_IGNORE_PORTALS,	// ( const vec3_t p1, const vec3_t p2 );

	G_ADJUST_AREA_PORTAL_STATE,	// ( gentity_t *ent, qboolean open );

	G_AREAS_CONNECTED,	// ( int area1, int area2 );

	G_LINKENTITY,		// ( gentity_t *ent );
	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.

	G_UNLINKENTITY,		// ( gentity_t *ent );
	// call before removing an interactive entity

	G_ENTITIES_IN_BOX,	// ( const vec3_t mins, const vec3_t maxs, gentity_t **list, int maxcount );
	// EntitiesInBox will return brush models based on their bounding box,
	// so exact determination must still be done with EntityContact

	G_ENTITY_CONTACT,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape

#ifdef OMNIBOTS
	// IRATA: used by OmniBot
	G_BOT_ALLOCATE_CLIENT = 36,	// ( int clientNum );
#endif

	G_GET_USERCMD = 38,	// ( int clientNum, usercmd_t *cmd )

	G_GET_ENTITY_TOKEN,	// qboolean ( char *buffer, int bufferSize )
	// Retrieves the next string token from the entity spawn text, returning
	// false when all tokens have been parsed.
	// This should only be done at GAME_INIT time.

	G_FS_GETFILELIST,
	G_DEBUG_POLYGON_CREATE,
	G_DEBUG_POLYGON_DELETE,
	G_REAL_TIME,
	G_SNAPVECTOR,
// MrE:

	G_TRACECAPSULE,	// ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection using capsule against all linked entities

	G_ENTITY_CONTACTCAPSULE,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape
// done.

	G_GETTAG,

	G_REGISTERTAG,
	// Gordon: load a serverside tag

	G_REGISTERSOUND,	// xkan, 10/28/2002 - register the sound
	G_GET_SOUND_LENGTH,	// xkan, 10/28/2002 - get the length of the sound

#ifdef OMNIBOTS
	// 200
	BOTLIB_GET_CONSOLE_MESSAGE = 210, // ( int client, char *message, int size );
	BOTLIB_USER_COMMAND = 211,			 // ( int client, usercmd_t *ucmd );

	// 400
	BOTLIB_EA_COMMAND = 407,
#endif

	// 500
	// files
	BOTLIB_PC_LOAD_SOURCE = 579,
	BOTLIB_PC_FREE_SOURCE,
	BOTLIB_PC_READ_TOKEN,
	BOTLIB_PC_SOURCE_FILE_AND_LINE,
	BOTLIB_PC_UNREAD_TOKEN,
#ifdef PUNKBUSTER
	// PB_STAT_REPORT = 584,
#endif
	// zinx
	G_SENDMESSAGE = 585,
	G_MESSAGESTATUS,
	// -zinx
} gameImport_t;


//
// functions exported by the game subsystem
//
typedef enum {
	GAME_INIT = 0,	// ( int levelTime, int randomSeed, int restart );
	// init and shutdown will be called every single level
	// The game should call G_GET_ENTITY_TOKEN to parse through all the
	// entity configuration text and spawn gentities.

	GAME_SHUTDOWN,	// (void);

	GAME_CLIENT_CONNECT,	// ( int clientNum, qboolean firstTime, qboolean isBot );
	// return NULL if the client is allowed to connect, otherwise return
	// a text string with the reason for denial

	GAME_CLIENT_BEGIN,				// ( int clientNum );

	GAME_CLIENT_USERINFO_CHANGED,	// ( int clientNum );

	GAME_CLIENT_DISCONNECT,			// ( int clientNum );

	GAME_CLIENT_COMMAND,			// ( int clientNum );

	GAME_CLIENT_THINK,				// ( int clientNum );

	GAME_RUN_FRAME,					// ( int levelTime );

	GAME_CONSOLE_COMMAND,			// ( void );
	// ConsoleCommand will be called when a command has been issued
	// that is not recognized as a builtin function.
	// The game can issue trap_argc() / trap_argv() commands to get the command
	// and parameters.  Return qfalse if the game doesn't recognize it as a command.

	GAME_SNAPSHOT_CALLBACK = 10,			// ( int entityNum, int clientNum ); // return qfalse if you don't want it to be added

	// IRATA: these gameExport_t are not used anymore
	// BOTAI_START_FRAME,              // ( int time ); - NOT USED IN NQ - keep for compatibility

	// Ridah, Cast AI
	//BOT_VISIBLEFROMPOS, // - NOT USED IN NQ - keep for compatibility
	//BOT_CHECKATTACKATPOS,//- NOT USED IN NQ - keep for compatibility
	// done.

	// zinx
	GAME_MESSAGERECEIVED = 14,			// ( int cno, const char *buf, int buflen, int commandTime );
	// -zinx
} gameExport_t;

#endif
