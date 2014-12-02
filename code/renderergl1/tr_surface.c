/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_surf.c
#include "tr_local.h"
#if idppc_altivec && !defined(MACOS_X)
#include <altivec.h>
#endif

/*

  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/


//============================================================================


/*
==============
RB_CheckOverflow
==============
*/
void RB_CheckOverflow( int verts, int indexes ) {
	if (tess.numVertexes + verts < SHADER_MAX_VERTEXES
		&& tess.numIndexes + indexes < SHADER_MAX_INDEXES) {
		return;
	}

	RB_EndSurface();

	if ( verts >= SHADER_MAX_VERTEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES );
	}
	if ( indexes >= SHADER_MAX_INDEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES );
	}

	RB_BeginSurface(tess.shader, tess.fogNum );
}


/*
==============
RB_AddQuadStampExt
==============
*/
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2 ) {
	vec3_t		normal;
	int			ndx;

	RB_CHECKOVERFLOW( 4, 6 );

	ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes ] = ndx;
	tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx + 3;

	tess.indexes[ tess.numIndexes + 3 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 5 ] = ndx + 2;

	tess.xyz[ndx][0] = origin[0] + left[0] + up[0];
	tess.xyz[ndx][1] = origin[1] + left[1] + up[1];
	tess.xyz[ndx][2] = origin[2] + left[2] + up[2];

	tess.xyz[ndx+1][0] = origin[0] - left[0] + up[0];
	tess.xyz[ndx+1][1] = origin[1] - left[1] + up[1];
	tess.xyz[ndx+1][2] = origin[2] - left[2] + up[2];

	tess.xyz[ndx+2][0] = origin[0] - left[0] - up[0];
	tess.xyz[ndx+2][1] = origin[1] - left[1] - up[1];
	tess.xyz[ndx+2][2] = origin[2] - left[2] - up[2];

	tess.xyz[ndx+3][0] = origin[0] + left[0] - up[0];
	tess.xyz[ndx+3][1] = origin[1] + left[1] - up[1];
	tess.xyz[ndx+3][2] = origin[2] + left[2] - up[2];


	// constant normal all the way around
	VectorSubtract( vec3_origin, backEnd.viewParms.or.axis[0], normal );

	tess.normal[ndx][0] = tess.normal[ndx+1][0] = tess.normal[ndx+2][0] = tess.normal[ndx+3][0] = normal[0];
	tess.normal[ndx][1] = tess.normal[ndx+1][1] = tess.normal[ndx+2][1] = tess.normal[ndx+3][1] = normal[1];
	tess.normal[ndx][2] = tess.normal[ndx+1][2] = tess.normal[ndx+2][2] = tess.normal[ndx+3][2] = normal[2];
	
	// standard square texture coordinates
	tess.texCoords[ndx][0][0] = tess.texCoords[ndx][1][0] = s1;
	tess.texCoords[ndx][0][1] = tess.texCoords[ndx][1][1] = t1;

	tess.texCoords[ndx+1][0][0] = tess.texCoords[ndx+1][1][0] = s2;
	tess.texCoords[ndx+1][0][1] = tess.texCoords[ndx+1][1][1] = t1;

	tess.texCoords[ndx+2][0][0] = tess.texCoords[ndx+2][1][0] = s2;
	tess.texCoords[ndx+2][0][1] = tess.texCoords[ndx+2][1][1] = t2;

	tess.texCoords[ndx+3][0][0] = tess.texCoords[ndx+3][1][0] = s1;
	tess.texCoords[ndx+3][0][1] = tess.texCoords[ndx+3][1][1] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?
	* ( unsigned int * ) &tess.vertexColors[ndx] = 
	* ( unsigned int * ) &tess.vertexColors[ndx+1] = 
	* ( unsigned int * ) &tess.vertexColors[ndx+2] = 
	* ( unsigned int * ) &tess.vertexColors[ndx+3] = 
		* ( unsigned int * )color;


	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

/*
==============
RB_AddQuadStamp
==============
*/
void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, byte *color ) {
	RB_AddQuadStampExt( origin, left, up, color, 0, 0, 1, 1 );
}

/*
==============
RB_SurfaceSprite
==============
*/
static void RB_SurfaceSprite( void ) {
	vec3_t		left, up;
	float		radius;
	float		rotation;

	// calculate the xyz locations for the four corners
	#ifdef ELITEFORCE
	radius = backEnd.currentEntity->e.data.sprite.radius;
	rotation = backEnd.currentEntity->e.data.sprite.rotation;
	#else
	radius = backEnd.currentEntity->e.radius;
	rotation = backEnd.currentEntity->e.rotation;
	#endif

	if ( rotation == 0 ) {
		VectorScale( backEnd.viewParms.or.axis[1], radius, left );
		VectorScale( backEnd.viewParms.or.axis[2], radius, up );
	} else {
		float	s, c;
		float	ang;
		
		ang = M_PI * rotation / 180;
		s = sin( ang );
		c = cos( ang );

		VectorScale( backEnd.viewParms.or.axis[1], c * radius, left );
		VectorMA( left, -s * radius, backEnd.viewParms.or.axis[2], left );

		VectorScale( backEnd.viewParms.or.axis[2], c * radius, up );
		VectorMA( up, s * radius, backEnd.viewParms.or.axis[1], up );
	}
	if ( backEnd.viewParms.isMirror ) {
		VectorSubtract( vec3_origin, left, left );
	}

	RB_AddQuadStamp( backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA );
}


/*
=============
RB_SurfacePolychain
=============
*/
static void RB_SurfacePolychain( srfPoly_t *p ) {
	int		i;
	int		numv;

	RB_CHECKOVERFLOW( p->numVerts, 3*(p->numVerts - 2) );

	// fan triangles into the tess array
	numv = tess.numVertexes;
	for ( i = 0; i < p->numVerts; i++ ) {
		VectorCopy( p->verts[i].xyz, tess.xyz[numv] );
		tess.texCoords[numv][0][0] = p->verts[i].st[0];
		tess.texCoords[numv][0][1] = p->verts[i].st[1];
		*(int *)&tess.vertexColors[numv] = *(int *)p->verts[ i ].modulate;

		numv++;
	}

	// generate fan indexes into the tess array
	for ( i = 0; i < p->numVerts-2; i++ ) {
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	tess.numVertexes = numv;
}


/*
=============
RB_SurfaceTriangles
=============
*/
static void RB_SurfaceTriangles( srfTriangles_t *srf ) {
	int			i;
	drawVert_t	*dv;
	float		*xyz, *normal, *texCoords;
	byte		*color;
	int			dlightBits;
	qboolean	needsNormal;

	dlightBits = srf->dlightBits;
	tess.dlightBits |= dlightBits;

	RB_CHECKOVERFLOW( srf->numVerts, srf->numIndexes );

	for ( i = 0 ; i < srf->numIndexes ; i += 3 ) {
		tess.indexes[ tess.numIndexes + i + 0 ] = tess.numVertexes + srf->indexes[ i + 0 ];
		tess.indexes[ tess.numIndexes + i + 1 ] = tess.numVertexes + srf->indexes[ i + 1 ];
		tess.indexes[ tess.numIndexes + i + 2 ] = tess.numVertexes + srf->indexes[ i + 2 ];
	}
	tess.numIndexes += srf->numIndexes;

	dv = srf->verts;
	xyz = tess.xyz[ tess.numVertexes ];
	normal = tess.normal[ tess.numVertexes ];
	texCoords = tess.texCoords[ tess.numVertexes ][0];
	color = tess.vertexColors[ tess.numVertexes ];
	needsNormal = tess.shader->needsNormal;

	for ( i = 0 ; i < srf->numVerts ; i++, dv++, xyz += 4, normal += 4, texCoords += 4, color += 4 ) {
		xyz[0] = dv->xyz[0];
		xyz[1] = dv->xyz[1];
		xyz[2] = dv->xyz[2];

		if ( needsNormal ) {
			normal[0] = dv->normal[0];
			normal[1] = dv->normal[1];
			normal[2] = dv->normal[2];
		}

		texCoords[0] = dv->st[0];
		texCoords[1] = dv->st[1];

		texCoords[2] = dv->lightmap[0];
		texCoords[3] = dv->lightmap[1];

		*(int *)color = *(int *)dv->color;
	}

	for ( i = 0 ; i < srf->numVerts ; i++ ) {
		tess.vertexDlightBits[ tess.numVertexes + i] = dlightBits;
	}

	tess.numVertexes += srf->numVerts;
}



/*
==============
RB_SurfaceBeam
==============
*/
static void RB_SurfaceBeam( void )
{
#define NUM_BEAM_SEGS 6
	refEntity_t *e;
	int	i;
	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	e = &backEnd.currentEntity->e;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );

	VectorScale( perpvec, 4, perpvec );

	for ( i = 0; i < NUM_BEAM_SEGS ; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
//		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	GL_Bind( tr.whiteImage );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	qglColor3f( 1, 0, 0 );

	qglBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i <= NUM_BEAM_SEGS; i++ ) {
		qglVertex3fv( start_points[ i % NUM_BEAM_SEGS] );
		qglVertex3fv( end_points[ i % NUM_BEAM_SEGS] );
	}
	qglEnd();
}

//================================================================================

static void DoRailCore( const vec3_t start, const vec3_t end, const vec3_t up, float len, float spanWidth )
{
	float		spanWidth2;
	int			vbase;
	float		t = len / 256.0f;

	RB_CHECKOVERFLOW( 4, 6 );

	vbase = tess.numVertexes;

	spanWidth2 = -spanWidth;

	// FIXME: use quad stamp?
	VectorMA( start, spanWidth, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] * 0.25;
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] * 0.25;
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] * 0.25;
	tess.numVertexes++;

	VectorMA( start, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.numVertexes++;

	VectorMA( end, spanWidth, up, tess.xyz[tess.numVertexes] );

	tess.texCoords[tess.numVertexes][0][0] = t;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.numVertexes++;

	VectorMA( end, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = t;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = vbase;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 2;

	tess.indexes[tess.numIndexes++] = vbase + 2;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 3;
}

static void DoRailDiscs( int numSegs, const vec3_t start, const vec3_t dir, const vec3_t right, const vec3_t up )
{
	int i;
	vec3_t	pos[4];
	vec3_t	v;
	int		spanWidth = r_railWidth->integer;
	float c, s;
	float		scale;

	if ( numSegs > 1 )
		numSegs--;
	if ( !numSegs )
		return;

	scale = 0.25;

	for ( i = 0; i < 4; i++ )
	{
		c = cos( DEG2RAD( 45 + i * 90 ) );
		s = sin( DEG2RAD( 45 + i * 90 ) );
		v[0] = ( right[0] * c + up[0] * s ) * scale * spanWidth;
		v[1] = ( right[1] * c + up[1] * s ) * scale * spanWidth;
		v[2] = ( right[2] * c + up[2] * s ) * scale * spanWidth;
		VectorAdd( start, v, pos[i] );

		if ( numSegs > 1 )
		{
			// offset by 1 segment if we're doing a long distance shot
			VectorAdd( pos[i], dir, pos[i] );
		}
	}

	for ( i = 0; i < numSegs; i++ )
	{
		int j;

		RB_CHECKOVERFLOW( 4, 6 );

		for ( j = 0; j < 4; j++ )
		{
			VectorCopy( pos[j], tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = ( j < 2 );
			tess.texCoords[tess.numVertexes][0][1] = ( j && j != 3 );
			tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
			tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
			tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
			tess.numVertexes++;

			VectorAdd( pos[j], dir, pos[j] );
		}

		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 0;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 1;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 3;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 3;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 1;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 2;
	}
}

/*
** RB_SurfaceRailRinges
*/
static void RB_SurfaceRailRings( void ) {
	refEntity_t *e;
	int			numSegs;
	int			len;
	vec3_t		vec;
	vec3_t		right, up;
	vec3_t		start, end;

	e = &backEnd.currentEntity->e;

	VectorCopy( e->oldorigin, start );
	VectorCopy( e->origin, end );

	// compute variables
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );
	MakeNormalVectors( vec, right, up );
	numSegs = ( len ) / r_railSegmentLength->value;
	if ( numSegs <= 0 ) {
		numSegs = 1;
	}

	VectorScale( vec, r_railSegmentLength->value, vec );

	DoRailDiscs( numSegs, start, vec, right, up );
}

/*
** RB_SurfaceRailCore
*/
static void RB_SurfaceRailCore( void ) {
	refEntity_t *e;
	int			len;
	vec3_t		right;
	vec3_t		vec;
	vec3_t		start, end;
	vec3_t		v1, v2;

	e = &backEnd.currentEntity->e;

	VectorCopy( e->oldorigin, start );
	VectorCopy( e->origin, end );

	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	// compute side vector
	VectorSubtract( start, backEnd.viewParms.or.origin, v1 );
	VectorNormalize( v1 );
	VectorSubtract( end, backEnd.viewParms.or.origin, v2 );
	VectorNormalize( v2 );
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	DoRailCore( start, end, right, len, r_railCoreWidth->integer );
}

/*
** RB_SurfaceLightningBolt
*/
static void RB_SurfaceLightningBolt( void ) {
	refEntity_t *e;
	int			len;
	vec3_t		right;
	vec3_t		vec;
	vec3_t		start, end;
	vec3_t		v1, v2;
	int			i;

	e = &backEnd.currentEntity->e;

	VectorCopy( e->oldorigin, end );
	VectorCopy( e->origin, start );

	// compute variables
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	// compute side vector
	VectorSubtract( start, backEnd.viewParms.or.origin, v1 );
	VectorNormalize( v1 );
	VectorSubtract( end, backEnd.viewParms.or.origin, v2 );
	VectorNormalize( v2 );
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	for ( i = 0 ; i < 4 ; i++ ) {
		vec3_t	temp;

		DoRailCore( start, end, right, len, 8 );
		RotatePointAroundVector( temp, vec, right, 45 );
		VectorCopy( temp, right );
	}
}

#ifdef ELITEFORCE

/*
==============
RB_SurfaceOrientedSprite
==============
*/
static void RB_SurfaceOrientedSprite( void )
{
	vec3_t		left, up;
	float		radius;
	float		rotation;

	// calculate the xyz locations for the four corners
	radius = backEnd.currentEntity->e.data.sprite.radius;
	rotation = backEnd.currentEntity->e.data.sprite.rotation;

	if (rotation == 0)
	{
		VectorScale( backEnd.currentEntity->e.axis[1], radius, left );
		VectorScale( backEnd.currentEntity->e.axis[2], radius, up );
	}
	else
	{
		float	s, c;
		float	ang;
		
		ang = M_PI * rotation / 180;
		s = sin( ang );
		c = cos( ang );

		VectorScale( backEnd.currentEntity->e.axis[1], c * radius, left );
		VectorMA( left, -s * radius, backEnd.currentEntity->e.axis[2], left );

		VectorScale( backEnd.currentEntity->e.axis[2], c * radius, up );
		VectorMA( up, s * radius, backEnd.currentEntity->e.axis[1], up );
	}
	if ( backEnd.viewParms.isMirror )
		VectorSubtract( vec3_origin, left, left );

	RB_AddQuadStamp( backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA );
}

void RB_Line(vec3_t start, vec3_t end, vec3_t linedirection, vec3_t left,
	     vec3_t *corners, float starttex, float endtex, refEntity_t *e)
{
	int ndx, numind;
	color4ub_t *vertcols;

	RB_CHECKOVERFLOW( 4, 6 );

	// Set up the triangles ..
	ndx = tess.numVertexes;
	numind = tess.numIndexes;

	tess.indexes[ numind ] = ndx;
	tess.indexes[ numind + 1 ] = ndx + 1;
	tess.indexes[ numind + 2 ] = ndx + 3;

	tess.indexes[ numind + 3 ] = ndx + 3;
	tess.indexes[ numind + 4 ] = ndx + 1;
	tess.indexes[ numind + 5 ] = ndx + 2;

	// now create the corner vertices

	if(corners)
	{
		// we have the corner points for the start already given.
		VectorCopy(corners[0], tess.xyz[ndx]);
		VectorCopy(corners[1], tess.xyz[ndx+1]);
	}
	else
	{
		// start left corner
		VectorAdd(start, left, tess.xyz[ndx]);
		// start right corner
		VectorSubtract(start, left, tess.xyz[ndx+1]);
	}
	// end right corner
	VectorSubtract(end, left, tess.xyz[ndx+2]);
	// end left corner
	VectorAdd(end, left, tess.xyz[ndx+3]);
	
	if(corners)
	{
		// save the end corner points here.
		VectorCopy(tess.xyz[ndx+3], corners[0]);
		VectorCopy(tess.xyz[ndx+2], corners[1]);
	}

	// Texture stuff....
	tess.texCoords[ndx][0][0] = 0;
	tess.texCoords[ndx][0][1] = starttex;

	tess.texCoords[ndx+1][0][0] = 1;
	tess.texCoords[ndx+1][0][1] = starttex;
	
	tess.texCoords[ndx+2][0][0] = 1;
	tess.texCoords[ndx+2][0][1] = endtex;

	tess.texCoords[ndx+3][0][0] = 0;
	tess.texCoords[ndx+3][0][1] = endtex;

	vertcols = tess.vertexColors;

	vertcols[ndx][0] = vertcols[ndx+1][0] = vertcols[ndx+2][0] = vertcols[ndx+3][0] = e->shaderRGBA[0];
	vertcols[ndx][1] = vertcols[ndx+1][1] = vertcols[ndx+2][1] = vertcols[ndx+3][1] = e->shaderRGBA[1];
	vertcols[ndx][2] = vertcols[ndx+1][2] = vertcols[ndx+2][2] = vertcols[ndx+3][2] = e->shaderRGBA[2];
	vertcols[ndx][3] = vertcols[ndx+1][3] = vertcols[ndx+2][3] = vertcols[ndx+3][3] = e->shaderRGBA[3];

	tess.numVertexes += 4;
	tess.numIndexes += 6;


}

// Create a normal vector and scale it to the right length
void RB_LineNormal(vec3_t vec1, vec3_t vec2, float scale, vec3_t result)
{
	// Create the offset vector for the width of the line
	CrossProduct(vec1, vec2, result);
	// Normalize the offset vector first.
	VectorNormalize(result);
	// Scale the offset vector to the intended width.
	VectorScale(result, scale / 2, result);
}

// Let's draw a thick line from A to B and dance happily around a christmas tree!
void RB_SurfaceLine( void )
{
	refEntity_t *e;
	vec3_t left;		// I vote the green party...
	vec3_t start, end, linedirection, start2origin;
	shader_t *surfshader;
	
	e = &backEnd.currentEntity->e;

	// Getting up before 1:00 pm to attend HM I lectures finally paid off..

	// Get the start and end point of the line
	VectorCopy(e->origin, start);
	VectorCopy(e->oldorigin, end);

	// Direction vector for the line:
	VectorSubtract(end, start, linedirection);
	// Direction vector for the start to current point of view
	VectorSubtract(backEnd.viewParms.or.origin, start, start2origin);

	RB_LineNormal(start2origin, linedirection, e->data.line.width, left);
	RB_Line(start, end, linedirection, left, NULL, 0, e->data.line.stscale, e); 

	// Hack to make the dreadnought lightning bolt work: set the correct cull type...
	if((surfshader = R_GetShaderByHandle(e->customShader)))
		surfshader->cullType = CT_TWO_SIDED;
}

void RB_SurfaceOrientedLine(void)
{
	refEntity_t *e;
	vec3_t left;
	vec3_t linedirection;
	shader_t *surfshader;

	e = &backEnd.currentEntity->e;

	VectorSubtract(e->oldorigin, e->origin, linedirection);

	VectorCopy(e->axis[1], left);
	VectorNormalize(left);
	VectorScale(left, e->data.line.width / 2, left);
	
	RB_Line(e->origin, e->oldorigin, linedirection, left, NULL, 0, 1, e);

	surfshader = R_GetShaderByHandle(e->customShader);
	surfshader->cullType = CT_TWO_SIDED;

}

// This time it's not a rectangle but a trapezoid I guess ...
void RB_SurfaceLine2( void )
{
	refEntity_t *e;
	vec3_t startleft, endleft;		// I still vote the green party...
	vec3_t start, end, linedirection, start2origin;
	color4ub_t *vertcols;
	int ndx, numind;
	
	RB_CHECKOVERFLOW( 6, 12 );
	
	e = &backEnd.currentEntity->e;

	// Set up the triangle ..
	ndx = tess.numVertexes;
	numind = tess.numIndexes;

	tess.indexes[ numind ] = ndx;
	tess.indexes[ numind + 1 ] = ndx + 1;
	tess.indexes[ numind + 2 ] = ndx + 5;
	tess.indexes[ numind + 3 ] = ndx + 5;
	tess.indexes[ numind + 4 ] = ndx + 4;
	tess.indexes[ numind + 5 ] = ndx + 1;
	tess.indexes[ numind + 6 ] = ndx + 1;
	tess.indexes[ numind + 7 ] = ndx + 4;
	tess.indexes[ numind + 8 ] = ndx + 3;
	tess.indexes[ numind + 9 ] = ndx + 3;
	tess.indexes[ numind + 10 ] = ndx + 2;
	tess.indexes[ numind + 11 ] = ndx + 1;


	// Get the start and end point of the line
	VectorCopy(e->origin, start);
	VectorCopy(e->oldorigin, end);

	// Direction vector for the line:
	VectorSubtract(end, start, linedirection);
	// Direction vector for the start to current point of view
	VectorSubtract(backEnd.viewParms.or.origin, start, start2origin);

	// Create the offset vector for the width of the line
	CrossProduct(start2origin, linedirection, startleft);
	// Normalize the offset vector first.
	VectorNormalize(startleft);
	// The normal of the triangle we just thought up:


	// Scale the offset vector to the intended width.
	VectorScale(startleft, e->data.line.width2 / 2, endleft);
	VectorScale(startleft, e->data.line.width / 2, startleft);

	// now create the corner vertices

	// start left corner
	VectorAdd(start, startleft, tess.xyz[ndx]);
	// start point
	VectorCopy(start, tess.xyz[ndx+1]);
	// start right corner
	VectorSubtract(start, startleft, tess.xyz[ndx+2]);
	// end right corner
	VectorSubtract(end, endleft, tess.xyz[ndx+3]);
	// end point
	VectorCopy(end, tess.xyz[ndx+4]);
	// end left corner	
	VectorAdd(end, endleft, tess.xyz[ndx+5]);

	// Texture stuff....
	tess.texCoords[ndx][0][0] = 0;
	tess.texCoords[ndx][0][1] = 0;

	tess.texCoords[ndx+1][0][0] = 0.5;
	tess.texCoords[ndx+1][0][1] = 0;
	
	tess.texCoords[ndx+2][0][0] = 1;
	tess.texCoords[ndx+2][0][1] = 0;

	tess.texCoords[ndx+3][0][0] = 1;
	tess.texCoords[ndx+3][0][1] = 1;

	tess.texCoords[ndx+4][0][0] = 0.5;
	tess.texCoords[ndx+4][0][1] = 1;

	tess.texCoords[ndx+5][0][0] = 0;
	tess.texCoords[ndx+5][0][1] = 1;

	vertcols = tess.vertexColors;

	vertcols[ndx][0] = vertcols[ndx+1][0] = vertcols[ndx+2][0] =
		vertcols[ndx+3][0] = vertcols[ndx+4][0] = vertcols[ndx+5][0] = e->shaderRGBA[0];
	vertcols[ndx][1] = vertcols[ndx+1][1] = vertcols[ndx+2][1] =
		vertcols[ndx+3][1] = vertcols[ndx+4][1] = vertcols[ndx+5][1] = e->shaderRGBA[1];
	vertcols[ndx][2] = vertcols[ndx+1][2] = vertcols[ndx+2][2] =
		vertcols[ndx+3][2] = vertcols[ndx+4][2] = vertcols[ndx+5][2] = e->shaderRGBA[2];
	vertcols[ndx][3] = vertcols[ndx+1][3] = vertcols[ndx+2][3] =
		vertcols[ndx+3][3] = vertcols[ndx+4][3] = vertcols[ndx+5][3] = e->shaderRGBA[3];

	tess.numVertexes += 6;
	tess.numIndexes += 12;
}

// Draw a cubic bezier curve for the imod weapon.
#define BEZIER_SEGMENTS 32
#define BEZIER_STEPSIZE 1.0 / BEZIER_SEGMENTS

void RB_SurfaceBezier(void)
{
	refEntity_t *e = &backEnd.currentEntity->e;
	double tvar = 0, one_tvar; // use double to not lose as much precision on cubing and squaring.
	float starttc, endtc;
	vec3_t segstartpos, segendpos, prevend[2];
	vec3_t linedirection, start2origin, left;
	int index;

	VectorCopy(e->origin, segstartpos);

	// start of the bezier curve is pointy
	VectorCopy(segstartpos, prevend[0]);
	VectorCopy(segstartpos, prevend[1]);
	
	// iterate through all segments we have to create.
	while(tvar <= 1.0)
	{
		// get the texture position for the rectangular approximating the bezier curve.
		starttc = tvar / 2.0;
		tvar += BEZIER_STEPSIZE;
		endtc = tvar / 2.0;

		one_tvar = 1.0 - tvar;

		// get the endpoint for this segment.

		for(index = 0; index < 3; index++)
		{
			// C(t) = P0 (t-1)^3 + 3 P1 t (t-1)^2 + 3 P2 t^2 (t-1) + P3 t^3
			segendpos[index] =
					e->origin[index] * one_tvar * one_tvar * one_tvar +
					3 * e->data.bezier.control1[index] * tvar * one_tvar * one_tvar +
					3 * e->data.bezier.control2[index] * tvar * tvar * one_tvar +
					e->oldorigin[index] * tvar * tvar * tvar;
		}
		
		// Direction vector for the line:
		VectorSubtract(segendpos, segstartpos, linedirection);
		// Direction vector for the start to current point of view
		VectorSubtract(backEnd.viewParms.or.origin, segstartpos, start2origin);

		RB_LineNormal(start2origin, linedirection, e->data.bezier.width, left);
		RB_Line(segstartpos, segendpos, linedirection, left, prevend, starttc, endtc, e);
		
		// Our next segment starts where the old one ends.
		VectorCopy(segendpos, segstartpos);
	}
}

// this actually isn't a cylinder but a frustum, but for the sake of naming
// conventions we'll keep calling it a cylinder.

// number of rectangles we'll use to approximate a frustum of a cone.
#define CYLINDER_MAXPLANES 64
#define LOD_CONSTANT	4.0f

void RB_SurfaceCylinder(void)
{
	int planes, index = 0;	// calculate the number of planes based on the level of detail.
	vec3_t bottom, top, bottom2top, zerodeg_bottom, zerodeg_top, faxis;
	vec3_t bottomcorner, topcorner, vieworigin;
	float anglestep, tcstep = 0, width, width2, height;
	int ndx, numind;
	color4ub_t *vertcols;
	refEntity_t *e = &backEnd.currentEntity->e;
	// for LOD calculation:
	vec3_t bottom2origin, top2origin, lodcalc, projection;
	float maxradius, startdistance, enddistance, distance;

	height = e->data.cylinder.height;
	width = e->data.cylinder.width;
	width2 = e->data.cylinder.width2;
	maxradius = (width > width2) ? width : width2;

	// get the start and end point of the frustum:
	
	VectorCopy(backEnd.viewParms.or.origin, vieworigin);
	VectorCopy(e->axis[0], faxis);
	VectorCopy(e->origin, bottom);
	VectorScale(faxis, height, bottom2top);
	VectorAdd(bottom, bottom2top, top);

	// Get distance from frustum:
	// first distance from endpoints
	VectorSubtract(vieworigin, bottom, bottom2origin);
	VectorSubtract(vieworigin, top, top2origin);
	
	// get projection of view origin on the middle line:
	distance = DotProduct(bottom2origin, faxis);
	
	startdistance = VectorLength(bottom2origin);
	enddistance = VectorLength(top2origin);

	if(distance < 0 || distance > height)
	{
		// projected point is not between bottom and top.
		distance = (startdistance < enddistance) ? startdistance : enddistance;
	}
	else
	{
		VectorMA(bottom, distance, faxis, projection);
		VectorSubtract(vieworigin, projection, lodcalc);
		distance = VectorLength(lodcalc);
	
		if(startdistance < distance)
			distance = startdistance;
		if(enddistance < distance)
			distance = enddistance;
	}			
	
	planes = LOD_CONSTANT * maxradius/distance * CYLINDER_MAXPLANES;
	
	if(planes < 8)
		planes = 8;
	else if(planes > CYLINDER_MAXPLANES)
		planes = CYLINDER_MAXPLANES;

	RB_CHECKOVERFLOW(4 * planes, 6 * planes);

	// create a normalized perpendicular vector to the frustum height.
	PerpendicularVector(zerodeg_bottom, faxis);

	// This vector gets rotated to create the top ring
	VectorScale(zerodeg_bottom, width2 / 2.0f, zerodeg_top);
	// Likewise the vector for the lower ring:
	VectorScale(zerodeg_bottom, width / 2.0f, zerodeg_bottom);

	anglestep = 360.0f / planes;
	if(e->data.cylinder.wrap)
		tcstep = e->data.cylinder.stscale / planes;
	
	ndx = tess.numVertexes;
	numind = tess.numIndexes;
	vertcols = tess.vertexColors;

	// this loop is creating all surface planes.
	while(index < planes)
	{
		index++;
	
		// Set up the indexes for the new plane:
		tess.indexes[numind++] = ndx;
		tess.indexes[numind++] = ndx + 2;
		tess.indexes[numind++] = ndx + 3;
		
		tess.indexes[numind++] = ndx;
		tess.indexes[numind++] = ndx + 3;
		tess.indexes[numind++] = ndx + 1;
		
		if(index <= 1)
		{
			VectorAdd(bottom, zerodeg_bottom, tess.xyz[ndx]);
			VectorAdd(top, zerodeg_top, tess.xyz[ndx + 1]);
		}
		else
		{
			VectorCopy(bottomcorner, tess.xyz[ndx]);
			VectorCopy(topcorner, tess.xyz[ndx + 1]);
		}

		// Create on the right side two new vertices, first the bottom, then the top one.
		if(index >= planes)
		{
			VectorAdd(bottom, zerodeg_bottom, bottomcorner);
			VectorAdd(top, zerodeg_top, topcorner);
		}
		else
		{
			RotatePointAroundVector(bottomcorner, faxis, zerodeg_bottom, anglestep * index);
			VectorAdd(bottom, bottomcorner, bottomcorner);
			RotatePointAroundVector(topcorner, faxis, zerodeg_top, index * anglestep);
			VectorAdd(top, topcorner, topcorner);
		}
		
		VectorCopy(bottomcorner, tess.xyz[ndx + 2]);
		VectorCopy(topcorner, tess.xyz[ndx + 3]);
		
		// Take care about the texture stuff
		if(e->data.cylinder.wrap)
		{
			tess.texCoords[ndx][0][0] = tcstep * (index - 1);
			tess.texCoords[ndx][0][1] = 0;
			
			tess.texCoords[ndx+1][0][0] = tcstep * (index - 1);
			tess.texCoords[ndx+1][0][1] = 1;

			tess.texCoords[ndx+2][0][0] = tcstep * index;
			tess.texCoords[ndx+2][0][1] = 0;
			
			tess.texCoords[ndx+3][0][0] = tcstep * index;
			tess.texCoords[ndx+3][0][1] = 1;
		}
		else
		{
			tess.texCoords[ndx][0][0] = 0;
			tess.texCoords[ndx][0][1] = 0;
			
			tess.texCoords[ndx+1][0][0] = 0;
			tess.texCoords[ndx+1][0][1] = 1;

			tess.texCoords[ndx+2][0][0] = e->data.cylinder.stscale;
			tess.texCoords[ndx+2][0][1] = 0;
			
			tess.texCoords[ndx+3][0][0] = e->data.cylinder.stscale;
			tess.texCoords[ndx+3][0][1] = 1;
		}

		vertcols[ndx][0] = vertcols[ndx+1][0] = vertcols[ndx+2][0] = vertcols[ndx+3][0] = e->shaderRGBA[0];
		vertcols[ndx][1] = vertcols[ndx+1][1] = vertcols[ndx+2][1] = vertcols[ndx+3][1] = e->shaderRGBA[1];
		vertcols[ndx][2] = vertcols[ndx+1][2] = vertcols[ndx+2][2] = vertcols[ndx+3][2] = e->shaderRGBA[2];
		vertcols[ndx][3] = vertcols[ndx+1][3] = vertcols[ndx+2][3] = vertcols[ndx+3][3] = 0xff;
                               
		ndx += 4;
	}
	
	tess.numVertexes = ndx;
	tess.numIndexes = numind;	
}

// Draws lightning using a multisegment line with jumpy connecting points.
#define ELECTRICITY_SEGMENTS 16
#define ELECTRICITY_STEPSIZE 1.0 / ELECTRICITY_SEGMENTS

void RB_SurfaceElectricity(void)
{
	vec3_t start, end, linedirection, segstartpos, segendpos;
	vec3_t prevend[2], left, start2origin, start2end;
	refEntity_t *e = &backEnd.currentEntity->e;
	float width, deviation, veclen;
	int index;
	
	VectorCopy(e->origin, start);
	VectorCopy(e->oldorigin, end);
	VectorSubtract(end, start, start2end);
	veclen = VectorLength(start2end);
	
	width = e->data.electricity.width;
	deviation = e->data.electricity.deviation;

	VectorCopy(start, segendpos);
	
	for(index = 0; index < ELECTRICITY_SEGMENTS; index++)
	{
		VectorCopy(segendpos, segstartpos);
		
		if(index > ELECTRICITY_SEGMENTS - 2)
		{
			// This is the last segment.
			VectorCopy(end, segendpos);
		}
		else
		{
			if(index > ELECTRICITY_SEGMENTS - 3)
			{
				// The line should have somewhat deviated by now.
				// Make the last two segments go to the endpoint

				// get the middle point between startpos and endpos
				VectorAdd(segstartpos, end, segendpos);
				VectorScale(segendpos, 0.5f, segendpos);
			}

			// Slightly misplace the next point on the line.
			segendpos[PITCH] += (start2end[PITCH] + crandom() * deviation * veclen) * ELECTRICITY_STEPSIZE;
			segendpos[YAW] += (start2end[YAW] + crandom() * deviation * veclen) * ELECTRICITY_STEPSIZE;
			segendpos[ROLL] += (start2end[ROLL] + crandom() * deviation * veclen / 2.0) * ELECTRICITY_STEPSIZE;
		}

		// Direction vector for the line:
		VectorSubtract(segendpos, segstartpos, linedirection);
		// Direction vector for the start to current point of view
		VectorSubtract(backEnd.viewParms.or.origin, segstartpos, start2origin);
		
		RB_LineNormal(start2origin, linedirection, width, left);

		if(!index)
		{
			// this is our first segment, create the starting points
			VectorAdd(segstartpos, left, prevend[0]);
			VectorSubtract(segstartpos, left, prevend[1]);
		}
		
		RB_Line(segstartpos, segendpos, linedirection, left, prevend, 0, e->data.electricity.stscale, e);
	}
}
#endif

/*
** VectorArrayNormalize
*
* The inputs to this routing seem to always be close to length = 1.0 (about 0.6 to 2.0)
* This means that we don't have to worry about zero length or enormously long vectors.
*/
static void VectorArrayNormalize(vec4_t *normals, unsigned int count)
{
//    assert(count);
        
#if idppc
    {
        register float half = 0.5;
        register float one  = 1.0;
        float *components = (float *)normals;
        
        // Vanilla PPC code, but since PPC has a reciprocal square root estimate instruction,
        // runs *much* faster than calling sqrt().  We'll use a single Newton-Raphson
        // refinement step to get a little more precision.  This seems to yeild results
        // that are correct to 3 decimal places and usually correct to at least 4 (sometimes 5).
        // (That is, for the given input range of about 0.6 to 2.0).
        do {
            float x, y, z;
            float B, y0, y1;
            
            x = components[0];
            y = components[1];
            z = components[2];
            components += 4;
            B = x*x + y*y + z*z;

#ifdef __GNUC__            
            asm("frsqrte %0,%1" : "=f" (y0) : "f" (B));
#else
			y0 = __frsqrte(B);
#endif
            y1 = y0 + half*y0*(one - B*y0*y0);

            x = x * y1;
            y = y * y1;
            components[-4] = x;
            z = z * y1;
            components[-3] = y;
            components[-2] = z;
        } while(count--);
    }
#else // No assembly version for this architecture, or C_ONLY defined
	// given the input, it's safe to call VectorNormalizeFast
    while (count--) {
        VectorNormalizeFast(normals[0]);
        normals++;
    }
#endif

}



/*
** LerpMeshVertexes
*/
#if idppc_altivec
static void LerpMeshVertexes_altivec(md3Surface_t *surf, float backlerp)
{
	short	*oldXyz, *newXyz, *oldNormals, *newNormals;
	float	*outXyz, *outNormal;
	float	oldXyzScale QALIGN(16);
	float   newXyzScale QALIGN(16);
	float	oldNormalScale QALIGN(16);
	float newNormalScale QALIGN(16);
	int		vertNum;
	unsigned lat, lng;
	int		numVerts;

	outXyz = tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];

	newXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
		+ (backEnd.currentEntity->e.frame * surf->numVerts * 4);
	newNormals = newXyz + 3;

	newXyzScale = MD3_XYZ_SCALE * (1.0 - backlerp);
	newNormalScale = 1.0 - backlerp;

	numVerts = surf->numVerts;

	if ( backlerp == 0 ) {
		vector signed short newNormalsVec0;
		vector signed short newNormalsVec1;
		vector signed int newNormalsIntVec;
		vector float newNormalsFloatVec;
		vector float newXyzScaleVec;
		vector unsigned char newNormalsLoadPermute;
		vector unsigned char newNormalsStorePermute;
		vector float zero;
		
		newNormalsStorePermute = vec_lvsl(0,(float *)&newXyzScaleVec);
		newXyzScaleVec = *(vector float *)&newXyzScale;
		newXyzScaleVec = vec_perm(newXyzScaleVec,newXyzScaleVec,newNormalsStorePermute);
		newXyzScaleVec = vec_splat(newXyzScaleVec,0);		
		newNormalsLoadPermute = vec_lvsl(0,newXyz);
		newNormalsStorePermute = vec_lvsr(0,outXyz);
		zero = (vector float)vec_splat_s8(0);
		//
		// just copy the vertexes
		//
		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			newXyz += 4, newNormals += 4,
			outXyz += 4, outNormal += 4) 
		{
			newNormalsLoadPermute = vec_lvsl(0,newXyz);
			newNormalsStorePermute = vec_lvsr(0,outXyz);
			newNormalsVec0 = vec_ld(0,newXyz);
			newNormalsVec1 = vec_ld(16,newXyz);
			newNormalsVec0 = vec_perm(newNormalsVec0,newNormalsVec1,newNormalsLoadPermute);
			newNormalsIntVec = vec_unpackh(newNormalsVec0);
			newNormalsFloatVec = vec_ctf(newNormalsIntVec,0);
			newNormalsFloatVec = vec_madd(newNormalsFloatVec,newXyzScaleVec,zero);
			newNormalsFloatVec = vec_perm(newNormalsFloatVec,newNormalsFloatVec,newNormalsStorePermute);
			//outXyz[0] = newXyz[0] * newXyzScale;
			//outXyz[1] = newXyz[1] * newXyzScale;
			//outXyz[2] = newXyz[2] * newXyzScale;

			lat = ( newNormals[0] >> 8 ) & 0xff;
			lng = ( newNormals[0] & 0xff );
			lat *= (FUNCTABLE_SIZE/256);
			lng *= (FUNCTABLE_SIZE/256);

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )

			outNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			outNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			vec_ste(newNormalsFloatVec,0,outXyz);
			vec_ste(newNormalsFloatVec,4,outXyz);
			vec_ste(newNormalsFloatVec,8,outXyz);
		}
	} else {
		//
		// interpolate and copy the vertex and normal
		//
		oldXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
			+ (backEnd.currentEntity->e.oldframe * surf->numVerts * 4);
		oldNormals = oldXyz + 3;

		oldXyzScale = MD3_XYZ_SCALE * backlerp;
		oldNormalScale = backlerp;

		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
			outXyz += 4, outNormal += 4) 
		{
			vec3_t uncompressedOldNormal, uncompressedNewNormal;

			// interpolate the xyz
			outXyz[0] = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
			outXyz[1] = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
			outXyz[2] = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

			// FIXME: interpolate lat/long instead?
			lat = ( newNormals[0] >> 8 ) & 0xff;
			lng = ( newNormals[0] & 0xff );
			lat *= 4;
			lng *= 4;
			uncompressedNewNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedNewNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedNewNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			lat = ( oldNormals[0] >> 8 ) & 0xff;
			lng = ( oldNormals[0] & 0xff );
			lat *= 4;
			lng *= 4;

			uncompressedOldNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedOldNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedOldNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			outNormal[0] = uncompressedOldNormal[0] * oldNormalScale + uncompressedNewNormal[0] * newNormalScale;
			outNormal[1] = uncompressedOldNormal[1] * oldNormalScale + uncompressedNewNormal[1] * newNormalScale;
			outNormal[2] = uncompressedOldNormal[2] * oldNormalScale + uncompressedNewNormal[2] * newNormalScale;

//			VectorNormalize (outNormal);
		}
    	VectorArrayNormalize((vec4_t *)tess.normal[tess.numVertexes], numVerts);
   	}
}
#endif

static void LerpMeshVertexes_scalar(md3Surface_t *surf, float backlerp)
{
	short	*oldXyz, *newXyz, *oldNormals, *newNormals;
	float	*outXyz, *outNormal;
	float	oldXyzScale, newXyzScale;
	float	oldNormalScale, newNormalScale;
	int		vertNum;
	unsigned lat, lng;
	int		numVerts;

	outXyz = tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];

	newXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
		+ (backEnd.currentEntity->e.frame * surf->numVerts * 4);
	newNormals = newXyz + 3;

	newXyzScale = MD3_XYZ_SCALE * (1.0 - backlerp);
	newNormalScale = 1.0 - backlerp;

	numVerts = surf->numVerts;

	if ( backlerp == 0 ) {
		//
		// just copy the vertexes
		//
		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			newXyz += 4, newNormals += 4,
			outXyz += 4, outNormal += 4) 
		{

			outXyz[0] = newXyz[0] * newXyzScale;
			outXyz[1] = newXyz[1] * newXyzScale;
			outXyz[2] = newXyz[2] * newXyzScale;

			lat = ( newNormals[0] >> 8 ) & 0xff;
			lng = ( newNormals[0] & 0xff );
			lat *= (FUNCTABLE_SIZE/256);
			lng *= (FUNCTABLE_SIZE/256);

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )

			outNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			outNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];
		}
	} else {
		//
		// interpolate and copy the vertex and normal
		//
		oldXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
			+ (backEnd.currentEntity->e.oldframe * surf->numVerts * 4);
		oldNormals = oldXyz + 3;

		oldXyzScale = MD3_XYZ_SCALE * backlerp;
		oldNormalScale = backlerp;

		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
			outXyz += 4, outNormal += 4) 
		{
			vec3_t uncompressedOldNormal, uncompressedNewNormal;

			// interpolate the xyz
			outXyz[0] = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
			outXyz[1] = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
			outXyz[2] = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

			// FIXME: interpolate lat/long instead?
			lat = ( newNormals[0] >> 8 ) & 0xff;
			lng = ( newNormals[0] & 0xff );
			lat *= 4;
			lng *= 4;
			uncompressedNewNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedNewNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedNewNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			lat = ( oldNormals[0] >> 8 ) & 0xff;
			lng = ( oldNormals[0] & 0xff );
			lat *= 4;
			lng *= 4;

			uncompressedOldNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedOldNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedOldNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			outNormal[0] = uncompressedOldNormal[0] * oldNormalScale + uncompressedNewNormal[0] * newNormalScale;
			outNormal[1] = uncompressedOldNormal[1] * oldNormalScale + uncompressedNewNormal[1] * newNormalScale;
			outNormal[2] = uncompressedOldNormal[2] * oldNormalScale + uncompressedNewNormal[2] * newNormalScale;

//			VectorNormalize (outNormal);
		}
    	VectorArrayNormalize((vec4_t *)tess.normal[tess.numVertexes], numVerts);
   	}
}

static void LerpMeshVertexes(md3Surface_t *surf, float backlerp)
{
#if idppc_altivec
	if (com_altivec->integer) {
		// must be in a seperate function or G3 systems will crash.
		LerpMeshVertexes_altivec( surf, backlerp );
		return;
	}
#endif // idppc_altivec
	LerpMeshVertexes_scalar( surf, backlerp );
}


/*
=============
RB_SurfaceMesh
=============
*/
static void RB_SurfaceMesh(md3Surface_t *surface) {
	int				j;
	float			backlerp;
	int				*triangles;
	float			*texCoords;
	int				indexes;
	int				Bob, Doug;
	int				numVerts;

	if (  backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame ) {
		backlerp = 0;
	} else  {
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CHECKOVERFLOW( surface->numVerts, surface->numTriangles*3 );

	LerpMeshVertexes (surface, backlerp);

	triangles = (int *) ((byte *)surface + surface->ofsTriangles);
	indexes = surface->numTriangles * 3;
	Bob = tess.numIndexes;
	Doug = tess.numVertexes;
	for (j = 0 ; j < indexes ; j++) {
		tess.indexes[Bob + j] = Doug + triangles[j];
	}
	tess.numIndexes += indexes;

	texCoords = (float *) ((byte *)surface + surface->ofsSt);

	numVerts = surface->numVerts;
	for ( j = 0; j < numVerts; j++ ) {
		tess.texCoords[Doug + j][0][0] = texCoords[j*2+0];
		tess.texCoords[Doug + j][0][1] = texCoords[j*2+1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;

}


/*
==============
RB_SurfaceFace
==============
*/
static void RB_SurfaceFace( srfSurfaceFace_t *surf ) {
	int			i;
	unsigned	*indices;
	glIndex_t	*tessIndexes;
	float		*v;
	float		*normal;
	int			ndx;
	int			Bob;
	int			numPoints;
	int			dlightBits;

	RB_CHECKOVERFLOW( surf->numPoints, surf->numIndices );

	dlightBits = surf->dlightBits;
	tess.dlightBits |= dlightBits;

	indices = ( unsigned * ) ( ( ( char  * ) surf ) + surf->ofsIndices );

	Bob = tess.numVertexes;
	tessIndexes = tess.indexes + tess.numIndexes;
	for ( i = surf->numIndices-1 ; i >= 0  ; i-- ) {
		tessIndexes[i] = indices[i] + Bob;
	}

	tess.numIndexes += surf->numIndices;

	numPoints = surf->numPoints;

	if ( tess.shader->needsNormal ) {
		normal = surf->plane.normal;
		for ( i = 0, ndx = tess.numVertexes; i < numPoints; i++, ndx++ ) {
			VectorCopy( normal, tess.normal[ndx] );
		}
	}

	for ( i = 0, v = surf->points[0], ndx = tess.numVertexes; i < numPoints; i++, v += VERTEXSIZE, ndx++ ) {
		VectorCopy( v, tess.xyz[ndx]);
		tess.texCoords[ndx][0][0] = v[3];
		tess.texCoords[ndx][0][1] = v[4];
		tess.texCoords[ndx][1][0] = v[5];
		tess.texCoords[ndx][1][1] = v[6];
		* ( unsigned int * ) &tess.vertexColors[ndx] = * ( unsigned int * ) &v[7];
		tess.vertexDlightBits[ndx] = dlightBits;
	}


	tess.numVertexes += surf->numPoints;
}


static float	LodErrorForVolume( vec3_t local, float radius ) {
	vec3_t		world;
	float		d;

	// never let it go negative
	if ( r_lodCurveError->value < 0 ) {
		return 0;
	}

	world[0] = local[0] * backEnd.or.axis[0][0] + local[1] * backEnd.or.axis[1][0] + 
		local[2] * backEnd.or.axis[2][0] + backEnd.or.origin[0];
	world[1] = local[0] * backEnd.or.axis[0][1] + local[1] * backEnd.or.axis[1][1] + 
		local[2] * backEnd.or.axis[2][1] + backEnd.or.origin[1];
	world[2] = local[0] * backEnd.or.axis[0][2] + local[1] * backEnd.or.axis[1][2] + 
		local[2] * backEnd.or.axis[2][2] + backEnd.or.origin[2];

	VectorSubtract( world, backEnd.viewParms.or.origin, world );
	d = DotProduct( world, backEnd.viewParms.or.axis[0] );

	if ( d < 0 ) {
		d = -d;
	}
	d -= radius;
	if ( d < 1 ) {
		d = 1;
	}

	return r_lodCurveError->value / d;
}

/*
=============
RB_SurfaceGrid

Just copy the grid of points and triangulate
=============
*/
static void RB_SurfaceGrid( srfGridMesh_t *cv ) {
	int		i, j;
	float	*xyz;
	float	*texCoords;
	float	*normal;
	unsigned char *color;
	drawVert_t	*dv;
	int		rows, irows, vrows;
	int		used;
	int		widthTable[MAX_GRID_SIZE];
	int		heightTable[MAX_GRID_SIZE];
	float	lodError;
	int		lodWidth, lodHeight;
	int		numVertexes;
	int		dlightBits;
	int		*vDlightBits;
	qboolean	needsNormal;

	dlightBits = cv->dlightBits;
	tess.dlightBits |= dlightBits;

	// determine the allowable discrepance
	lodError = LodErrorForVolume( cv->lodOrigin, cv->lodRadius );

	// determine which rows and columns of the subdivision
	// we are actually going to use
	widthTable[0] = 0;
	lodWidth = 1;
	for ( i = 1 ; i < cv->width-1 ; i++ ) {
		if ( cv->widthLodError[i] <= lodError ) {
			widthTable[lodWidth] = i;
			lodWidth++;
		}
	}
	widthTable[lodWidth] = cv->width-1;
	lodWidth++;

	heightTable[0] = 0;
	lodHeight = 1;
	for ( i = 1 ; i < cv->height-1 ; i++ ) {
		if ( cv->heightLodError[i] <= lodError ) {
			heightTable[lodHeight] = i;
			lodHeight++;
		}
	}
	heightTable[lodHeight] = cv->height-1;
	lodHeight++;


	// very large grids may have more points or indexes than can be fit
	// in the tess structure, so we may have to issue it in multiple passes

	used = 0;
	while ( used < lodHeight - 1 ) {
		// see how many rows of both verts and indexes we can add without overflowing
		do {
			vrows = ( SHADER_MAX_VERTEXES - tess.numVertexes ) / lodWidth;
			irows = ( SHADER_MAX_INDEXES - tess.numIndexes ) / ( lodWidth * 6 );

			// if we don't have enough space for at least one strip, flush the buffer
			if ( vrows < 2 || irows < 1 ) {
				RB_EndSurface();
				RB_BeginSurface(tess.shader, tess.fogNum );
			} else {
				break;
			}
		} while ( 1 );
		
		rows = irows;
		if ( vrows < irows + 1 ) {
			rows = vrows - 1;
		}
		if ( used + rows > lodHeight ) {
			rows = lodHeight - used;
		}

		numVertexes = tess.numVertexes;

		xyz = tess.xyz[numVertexes];
		normal = tess.normal[numVertexes];
		texCoords = tess.texCoords[numVertexes][0];
		color = ( unsigned char * ) &tess.vertexColors[numVertexes];
		vDlightBits = &tess.vertexDlightBits[numVertexes];
		needsNormal = tess.shader->needsNormal;

		for ( i = 0 ; i < rows ; i++ ) {
			for ( j = 0 ; j < lodWidth ; j++ ) {
				dv = cv->verts + heightTable[ used + i ] * cv->width
					+ widthTable[ j ];

				xyz[0] = dv->xyz[0];
				xyz[1] = dv->xyz[1];
				xyz[2] = dv->xyz[2];
				texCoords[0] = dv->st[0];
				texCoords[1] = dv->st[1];
				texCoords[2] = dv->lightmap[0];
				texCoords[3] = dv->lightmap[1];
				if ( needsNormal ) {
					normal[0] = dv->normal[0];
					normal[1] = dv->normal[1];
					normal[2] = dv->normal[2];
				}
				* ( unsigned int * ) color = * ( unsigned int * ) dv->color;
				*vDlightBits++ = dlightBits;
				xyz += 4;
				normal += 4;
				texCoords += 4;
				color += 4;
			}
		}


		// add the indexes
		{
			int		numIndexes;
			int		w, h;

			h = rows - 1;
			w = lodWidth - 1;
			numIndexes = tess.numIndexes;
			for (i = 0 ; i < h ; i++) {
				for (j = 0 ; j < w ; j++) {
					int		v1, v2, v3, v4;
			
					// vertex order to be reckognized as tristrips
					v1 = numVertexes + i*lodWidth + j + 1;
					v2 = v1 - 1;
					v3 = v2 + lodWidth;
					v4 = v3 + 1;

					tess.indexes[numIndexes] = v2;
					tess.indexes[numIndexes+1] = v3;
					tess.indexes[numIndexes+2] = v1;
					
					tess.indexes[numIndexes+3] = v1;
					tess.indexes[numIndexes+4] = v3;
					tess.indexes[numIndexes+5] = v4;
					numIndexes += 6;
				}
			}

			tess.numIndexes = numIndexes;
		}

		tess.numVertexes += rows * lodWidth;

		used += rows - 1;
	}
}


/*
===========================================================================

NULL MODEL

===========================================================================
*/

/*
===================
RB_SurfaceAxis

Draws x/y/z lines from the origin for orientation debugging
===================
*/
static void RB_SurfaceAxis( void ) {
	GL_Bind( tr.whiteImage );
	GL_State( GLS_DEFAULT );
	qglLineWidth( 3 );
	qglBegin( GL_LINES );
	qglColor3f( 1,0,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 16,0,0 );
	qglColor3f( 0,1,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,16,0 );
	qglColor3f( 0,0,1 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,0,16 );
	qglEnd();
	qglLineWidth( 1 );
}

//===========================================================================

/*
====================
RB_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
static void RB_SurfaceEntity( surfaceType_t *surfType ) {
	switch( backEnd.currentEntity->e.reType ) {
#ifdef ELITEFORCE
	case RT_ALPHAVERTPOLY:
#endif
	case RT_SPRITE:
		RB_SurfaceSprite();
		break;
	case RT_BEAM:
		RB_SurfaceBeam();
		break;
	case RT_RAIL_CORE:
		RB_SurfaceRailCore();
		break;
	case RT_RAIL_RINGS:
		RB_SurfaceRailRings();
		break;
	case RT_LIGHTNING:
		RB_SurfaceLightningBolt();
		break;
#ifdef ELITEFORCE
	case RT_ORIENTEDSPRITE:
		RB_SurfaceOrientedSprite();
		break;
	case RT_LINE:
		RB_SurfaceLine();
		break;
	case RT_ORIENTEDLINE:
		RB_SurfaceOrientedLine();
		break;
	case RT_LINE2:
		RB_SurfaceLine2();
		break;
	case RT_BEZIER:
		RB_SurfaceBezier();
		break;
	case RT_CYLINDER:
		RB_SurfaceCylinder();
		break;
	case RT_ELECTRICITY:
		RB_SurfaceElectricity();
		break;
#endif
	default:
		RB_SurfaceAxis();
		break;
	}
}

static void RB_SurfaceBad( surfaceType_t *surfType ) {
	ri.Printf( PRINT_ALL, "Bad surface tesselated.\n" );
}

static void RB_SurfaceFlare(srfFlare_t *surf)
{
	if (r_flares->integer)
		RB_AddFlare(surf, tess.fogNum, surf->origin, surf->color, surf->normal);
}

static void RB_SurfaceSkip( void *surf ) {
}


void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])( void *) = {
	(void(*)(void*))RB_SurfaceBad,			// SF_BAD, 
	(void(*)(void*))RB_SurfaceSkip,			// SF_SKIP, 
	(void(*)(void*))RB_SurfaceFace,			// SF_FACE,
	(void(*)(void*))RB_SurfaceGrid,			// SF_GRID,
	(void(*)(void*))RB_SurfaceTriangles,		// SF_TRIANGLES,
	(void(*)(void*))RB_SurfacePolychain,		// SF_POLY,
	(void(*)(void*))RB_SurfaceMesh,			// SF_MD3,
	(void(*)(void*))RB_MDRSurfaceAnim,		// SF_MDR,
	(void(*)(void*))RB_IQMSurfaceAnim,		// SF_IQM,
	(void(*)(void*))RB_SurfaceFlare,		// SF_FLARE,
	(void(*)(void*))RB_SurfaceEntity		// SF_ENTITY
};
