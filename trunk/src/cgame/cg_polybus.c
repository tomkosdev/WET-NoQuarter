/*
 * name:	cg_polybus.c
 *
 * desc:
 *
 * NQQS:	healthy
 *
 */

#include "cg_local.h"

#define MAX_PB_BUFFERS	128

polyBuffer_t	cg_polyBuffers[MAX_PB_BUFFERS];
qboolean		cg_polyBuffersInuse[MAX_PB_BUFFERS];

polyBuffer_t* CG_PB_FindFreePolyBuffer(qhandle_t shader, int numVerts, int numIndicies) {
	int i;
	int firstFree = -1;

	// Gordon: first find one with the same shader if possible
	for( i = 0; i < MAX_PB_BUFFERS; ++i ) {
		if( !cg_polyBuffersInuse[i] ) {
			if (firstFree == -1) firstFree = i;
			continue;
		}

		if( cg_polyBuffers[i].shader != shader ) {
			continue;
		}

		if( cg_polyBuffers[i].numIndicies + numIndicies >= MAX_PB_INDICIES ) {
			continue;
		}

		if( cg_polyBuffers[i].numVerts + numVerts >= MAX_PB_VERTS ) {
			continue;
		}

		cg_polyBuffersInuse[i] = qtrue;
		cg_polyBuffers[i].shader = shader;

		return &cg_polyBuffers[i];
	}

	// Gordon: or just find a free one
	// core: we have already found it..
	if (firstFree != -1) {
		cg_polyBuffersInuse[firstFree] = qtrue;
		cg_polyBuffers[firstFree].shader = shader;
		cg_polyBuffers[firstFree].numIndicies = 0;
		cg_polyBuffers[firstFree].numVerts = 0;
		return &cg_polyBuffers[firstFree];
	}

	// or return NULL if no free buffer was found..
	return NULL;
}

void CG_PB_ClearPolyBuffers( void ) {
	// Gordon: changed numIndicies and numVerts to be reset in CG_PB_FindFreePolyBuffer, not here (should save the cache misses we were prolly getting)
	memset( cg_polyBuffersInuse, 0, sizeof(cg_polyBuffersInuse) );
}

void CG_PB_RenderPolyBuffers( void ) {
	int i;

	for( i = 0; i < MAX_PB_BUFFERS; ++i ) {
		if(cg_polyBuffersInuse[i]) {
			trap_R_AddPolyBufferToScene( &cg_polyBuffers[i] );
		}
	}
}
