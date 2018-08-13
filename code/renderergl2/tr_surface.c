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

	RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex );
}

void RB_CheckVao(vao_t *vao)
{
	if (vao != glState.currentVao)
	{
		RB_EndSurface();
		RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);

		R_BindVao(vao);
	}

	if (vao != tess.vao)
		tess.useInternalVao = qfalse;
}


/*
==============
RB_AddQuadStampExt
==============
*/
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, float color[4], float s1, float t1, float s2, float t2 ) {
	vec3_t		normal;
	int16_t     iNormal[4];
	uint16_t    iColor[4];
	int			ndx;

	RB_CheckVao(tess.vao);

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

	R_VaoPackNormal(iNormal, normal);

	VectorCopy4(iNormal, tess.normal[ndx]);
	VectorCopy4(iNormal, tess.normal[ndx + 1]);
	VectorCopy4(iNormal, tess.normal[ndx + 2]);
	VectorCopy4(iNormal, tess.normal[ndx + 3]);

	// standard square texture coordinates
	VectorSet2(tess.texCoords[ndx], s1, t1);
	VectorSet2(tess.lightCoords[ndx], s1, t1);

	VectorSet2(tess.texCoords[ndx+1], s2, t1);
	VectorSet2(tess.lightCoords[ndx+1], s2, t1);

	VectorSet2(tess.texCoords[ndx+2], s2, t2);
	VectorSet2(tess.lightCoords[ndx+2], s2, t2);

	VectorSet2(tess.texCoords[ndx+3], s1, t2);
	VectorSet2(tess.lightCoords[ndx+3], s1, t2);

	// constant color all the way around
	// should this be identity and let the shader specify from entity?

	R_VaoPackColor(iColor, color);

	VectorCopy4(iColor, tess.color[ndx]);
	VectorCopy4(iColor, tess.color[ndx + 1]);
	VectorCopy4(iColor, tess.color[ndx + 2]);
	VectorCopy4(iColor, tess.color[ndx + 3]);

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

/*
==============
RB_AddQuadStamp
==============
*/
void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, float color[4] ) {
	RB_AddQuadStampExt( origin, left, up, color, 0, 0, 1, 1 );
}


/*
==============
RB_InstantQuad

based on Tess_InstantQuad from xreal
==============
*/
void RB_InstantQuad2(vec4_t quadVerts[4], vec2_t texCoords[4])
{
	GLimp_LogComment("--- RB_InstantQuad2 ---\n");

	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.firstIndex = 0;

	VectorCopy4(quadVerts[0], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[0], tess.texCoords[tess.numVertexes]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[1], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[1], tess.texCoords[tess.numVertexes]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[2], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[2], tess.texCoords[tess.numVertexes]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[3], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[3], tess.texCoords[tess.numVertexes]);
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	RB_UpdateTessVao(ATTR_POSITION | ATTR_TEXCOORD);

	R_DrawElements(tess.numIndexes, tess.firstIndex);

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
}


void RB_InstantQuad(vec4_t quadVerts[4])
{
	vec2_t texCoords[4];

	VectorSet2(texCoords[0], 0.0f, 0.0f);
	VectorSet2(texCoords[1], 1.0f, 0.0f);
	VectorSet2(texCoords[2], 1.0f, 1.0f);
	VectorSet2(texCoords[3], 0.0f, 1.0f);

	GLSL_BindProgram(&tr.textureColorShader);
	
	GLSL_SetUniformMat4(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

	RB_InstantQuad2(quadVerts, texCoords);
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
	float			colors[4];
	trRefEntity_t	*ent = backEnd.currentEntity;

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

	VectorScale4(ent->e.shaderRGBA, 1.0f / 255.0f, colors);

	RB_AddQuadStamp( ent->e.origin, left, up, colors );
}


/*
=============
RB_SurfacePolychain
=============
*/
static void RB_SurfacePolychain( srfPoly_t *p ) {
	int		i;
	int		numv;

	RB_CheckVao(tess.vao);

	RB_CHECKOVERFLOW( p->numVerts, 3*(p->numVerts - 2) );

	// fan triangles into the tess array
	numv = tess.numVertexes;
	for ( i = 0; i < p->numVerts; i++ ) {
		VectorCopy( p->verts[i].xyz, tess.xyz[numv] );
		tess.texCoords[numv][0] = p->verts[i].st[0];
		tess.texCoords[numv][1] = p->verts[i].st[1];
		tess.color[numv][0] = (int)p->verts[i].modulate[0] * 257;
		tess.color[numv][1] = (int)p->verts[i].modulate[1] * 257;
		tess.color[numv][2] = (int)p->verts[i].modulate[2] * 257;
		tess.color[numv][3] = (int)p->verts[i].modulate[3] * 257;

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

static void RB_SurfaceVertsAndIndexes( int numVerts, srfVert_t *verts, int numIndexes, glIndex_t *indexes, int dlightBits, int pshadowBits)
{
	int             i;
	glIndex_t      *inIndex;
	srfVert_t      *dv;
	float          *xyz, *texCoords, *lightCoords;
	int16_t        *lightdir;
	int16_t        *normal;
	int16_t        *tangent;
	glIndex_t      *outIndex;
	uint16_t       *color;

	RB_CheckVao(tess.vao);

	RB_CHECKOVERFLOW( numVerts, numIndexes );

	inIndex = indexes;
	outIndex = &tess.indexes[ tess.numIndexes ];
	for ( i = 0 ; i < numIndexes ; i++ ) {
		*outIndex++ = tess.numVertexes + *inIndex++;
	}
	tess.numIndexes += numIndexes;

	if ( tess.shader->vertexAttribs & ATTR_POSITION )
	{
		dv = verts;
		xyz = tess.xyz[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, xyz+=4 )
			VectorCopy(dv->xyz, xyz);
	}

	if ( tess.shader->vertexAttribs & ATTR_NORMAL )
	{
		dv = verts;
		normal = tess.normal[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, normal+=4 )
			VectorCopy4(dv->normal, normal);
	}

	if ( tess.shader->vertexAttribs & ATTR_TANGENT )
	{
		dv = verts;
		tangent = tess.tangent[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, tangent+=4 )
			VectorCopy4(dv->tangent, tangent);
	}

	if ( tess.shader->vertexAttribs & ATTR_TEXCOORD )
	{
		dv = verts;
		texCoords = tess.texCoords[tess.numVertexes];
		for ( i = 0 ; i < numVerts ; i++, dv++, texCoords+=2 )
			VectorCopy2(dv->st, texCoords);
	}

	if ( tess.shader->vertexAttribs & ATTR_LIGHTCOORD )
	{
		dv = verts;
		lightCoords = tess.lightCoords[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, lightCoords+=2 )
			VectorCopy2(dv->lightmap, lightCoords);
	}

	if ( tess.shader->vertexAttribs & ATTR_COLOR )
	{
		dv = verts;
		color = tess.color[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, color+=4 )
			VectorCopy4(dv->color, color);
	}

	if ( tess.shader->vertexAttribs & ATTR_LIGHTDIRECTION )
	{
		dv = verts;
		lightdir = tess.lightdir[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, lightdir+=4 )
			VectorCopy4(dv->lightdir, lightdir);
	}

#if 0  // nothing even uses vertex dlightbits
	for ( i = 0 ; i < numVerts ; i++ ) {
		tess.vertexDlightBits[ tess.numVertexes + i ] = dlightBits;
	}
#endif

	tess.dlightBits |= dlightBits;
	tess.pshadowBits |= pshadowBits;

	tess.numVertexes += numVerts;
}

static qboolean RB_SurfaceVaoCached(int numVerts, srfVert_t *verts, int numIndexes, glIndex_t *indexes, int dlightBits, int pshadowBits)
{
	qboolean recycleVertexBuffer = qfalse;
	qboolean recycleIndexBuffer = qfalse;
	qboolean endSurface = qfalse;

	if (!(!ShaderRequiresCPUDeforms(tess.shader) && !tess.shader->isSky && !tess.shader->isPortal))
		return qfalse;

	if (!numIndexes || !numVerts)
		return qfalse;

	VaoCache_BindVao();

	tess.dlightBits |= dlightBits;
	tess.pshadowBits |= pshadowBits;

	VaoCache_CheckAdd(&endSurface, &recycleVertexBuffer, &recycleIndexBuffer, numVerts, numIndexes);

	if (endSurface)
	{
		RB_EndSurface();
		RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);
	}

	if (recycleVertexBuffer)
		VaoCache_RecycleVertexBuffer();

	if (recycleIndexBuffer)
		VaoCache_RecycleIndexBuffer();

	if (!tess.numVertexes)
		VaoCache_InitQueue();

	VaoCache_AddSurface(verts, numVerts, indexes, numIndexes);

	tess.numIndexes += numIndexes;
	tess.numVertexes += numVerts;
	tess.useInternalVao = qfalse;
	tess.useCacheVao = qtrue;

	return qtrue;
}


/*
=============
RB_SurfaceTriangles
=============
*/
static void RB_SurfaceTriangles( srfBspSurface_t *srf ) {
	if (RB_SurfaceVaoCached(srf->numVerts, srf->verts, srf->numIndexes,
		srf->indexes, srf->dlightBits, srf->pshadowBits))
	{
		return;
	}

	RB_SurfaceVertsAndIndexes(srf->numVerts, srf->verts, srf->numIndexes,
			srf->indexes, srf->dlightBits, srf->pshadowBits);
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
	float			colors[4];
	trRefEntity_t	*ent = backEnd.currentEntity;

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

	VectorScale4(ent->e.shaderRGBA, 1.0f / 255.0f, colors);

	RB_AddQuadStamp( ent->e.origin, left, up, colors );
}

void RB_Line(vec3_t start, vec3_t end, vec3_t linedirection, vec3_t left,
	     vec3_t *corners, float starttex, float endtex, refEntity_t *e)
{
	int ndx, numind;

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
	tess.texCoords[ndx][0] = 0;
	tess.texCoords[ndx][1] = starttex;

	tess.texCoords[ndx+1][0] = 1;
	tess.texCoords[ndx+1][1] = starttex;
	
	tess.texCoords[ndx+2][0] = 1;
	tess.texCoords[ndx+2][1] = endtex;

	tess.texCoords[ndx+3][0] = 0;
	tess.texCoords[ndx+3][1] = endtex;

	tess.color[ndx][0] = tess.color[ndx+1][0] = tess.color[ndx+2][0] = tess.color[ndx+3][0] = e->shaderRGBA[0] * 257;
	tess.color[ndx][1] = tess.color[ndx+1][1] = tess.color[ndx+2][1] = tess.color[ndx+3][1] = e->shaderRGBA[1] * 257;
	tess.color[ndx][2] = tess.color[ndx+1][2] = tess.color[ndx+2][2] = tess.color[ndx+3][2] = e->shaderRGBA[2] * 257;
	tess.color[ndx][3] = tess.color[ndx+1][3] = tess.color[ndx+2][3] = tess.color[ndx+3][3] = e->shaderRGBA[3] * 257;

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
	tess.texCoords[ndx][0] = 0;
	tess.texCoords[ndx][1] = 0;

	tess.texCoords[ndx+1][0] = 0.5;
	tess.texCoords[ndx+1][1] = 0;
	
	tess.texCoords[ndx+2][0] = 1;
	tess.texCoords[ndx+2][1] = 0;

	tess.texCoords[ndx+3][0] = 1;
	tess.texCoords[ndx+3][1] = 1;

	tess.texCoords[ndx+4][0] = 0.5;
	tess.texCoords[ndx+4][1] = 1;

	tess.texCoords[ndx+5][0] = 0;
	tess.texCoords[ndx+5][1] = 1;

	tess.color[ndx][0] = tess.color[ndx+1][0] = tess.color[ndx+2][0] =
		tess.color[ndx+3][0] = tess.color[ndx+4][0] = tess.color[ndx+5][0] = e->shaderRGBA[0] * 257;
	tess.color[ndx][1] = tess.color[ndx+1][1] = tess.color[ndx+2][1] =
		tess.color[ndx+3][1] = tess.color[ndx+4][1] = tess.color[ndx+5][1] = e->shaderRGBA[1] * 257;
	tess.color[ndx][2] = tess.color[ndx+1][2] = tess.color[ndx+2][2] =
		tess.color[ndx+3][2] = tess.color[ndx+4][2] = tess.color[ndx+5][2] = e->shaderRGBA[2] * 257;
	tess.color[ndx][3] = tess.color[ndx+1][3] = tess.color[ndx+2][3] =
		tess.color[ndx+3][3] = tess.color[ndx+4][3] = tess.color[ndx+5][3] = e->shaderRGBA[3] * 257;

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
			tess.texCoords[ndx][0] = tcstep * (index - 1);
			tess.texCoords[ndx][1] = 0;
			
			tess.texCoords[ndx+1][0] = tcstep * (index - 1);
			tess.texCoords[ndx+1][1] = 1;

			tess.texCoords[ndx+2][0] = tcstep * index;
			tess.texCoords[ndx+2][1] = 0;
			
			tess.texCoords[ndx+3][0] = tcstep * index;
			tess.texCoords[ndx+3][1] = 1;
		}
		else
		{
			tess.texCoords[ndx][0] = 0;
			tess.texCoords[ndx][1] = 0;
			
			tess.texCoords[ndx+1][0] = 0;
			tess.texCoords[ndx+1][1] = 1;

			tess.texCoords[ndx+2][0] = e->data.cylinder.stscale;
			tess.texCoords[ndx+2][1] = 0;
			
			tess.texCoords[ndx+3][0] = e->data.cylinder.stscale;
			tess.texCoords[ndx+3][1] = 1;
		}

		tess.color[ndx][0] = tess.color[ndx+1][0] = tess.color[ndx+2][0] = tess.color[ndx+3][0] = e->shaderRGBA[0] * 257;
		tess.color[ndx][1] = tess.color[ndx+1][1] = tess.color[ndx+2][1] = tess.color[ndx+3][1] = e->shaderRGBA[1] * 257;
		tess.color[ndx][2] = tess.color[ndx+1][2] = tess.color[ndx+2][2] = tess.color[ndx+3][2] = e->shaderRGBA[2] * 257;
		tess.color[ndx][3] = tess.color[ndx+1][3] = tess.color[ndx+2][3] = tess.color[ndx+3][3] = 255 * 257;
                               
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
==============
RB_SurfaceBeam
==============
*/
static void RB_SurfaceBeam( void )
{
#define NUM_BEAM_SEGS 6
	refEntity_t *e;
	shaderProgram_t *sp = &tr.textureColorShader;
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

	GL_BindToTMU( tr.whiteImage, TB_COLORMAP );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	// FIXME: Quake3 doesn't use this, so I never tested it
	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.firstIndex = 0;

	for ( i = 0; i <= NUM_BEAM_SEGS; i++ ) {
		VectorCopy(start_points[ i % NUM_BEAM_SEGS ], tess.xyz[tess.numVertexes++]);
		VectorCopy(end_points  [ i % NUM_BEAM_SEGS ], tess.xyz[tess.numVertexes++]);
	}

	for ( i = 0; i < NUM_BEAM_SEGS; i++ ) {
		tess.indexes[tess.numIndexes++] =       i      * 2;
		tess.indexes[tess.numIndexes++] =      (i + 1) * 2;
		tess.indexes[tess.numIndexes++] = 1  +  i      * 2;

		tess.indexes[tess.numIndexes++] = 1  +  i      * 2;
		tess.indexes[tess.numIndexes++] =      (i + 1) * 2;
		tess.indexes[tess.numIndexes++] = 1  + (i + 1) * 2;
	}

	// FIXME: A lot of this can probably be removed for speed, and refactored into a more convenient function
	RB_UpdateTessVao(ATTR_POSITION);
	
	GLSL_BindProgram(sp);
		
	GLSL_SetUniformMat4(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
					
	GLSL_SetUniformVec4(sp, UNIFORM_COLOR, colorRed);

	GLSL_SetUniformInt(sp, UNIFORM_ALPHATEST, 0);

	R_DrawElements(tess.numIndexes, tess.firstIndex);

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
}

//================================================================================

static void DoRailCore( const vec3_t start, const vec3_t end, const vec3_t up, float len, float spanWidth )
{
	float		spanWidth2;
	int			vbase;
	float		t = len / 256.0f;

	RB_CheckVao(tess.vao);

	RB_CHECKOVERFLOW( 4, 6 );

	vbase = tess.numVertexes;

	spanWidth2 = -spanWidth;

	// FIXME: use quad stamp?
	VectorMA( start, spanWidth, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0] = 0;
	tess.texCoords[tess.numVertexes][1] = 0;
	tess.color[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] * 0.25f * 257.0f;
	tess.color[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] * 0.25f * 257.0f;
	tess.color[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] * 0.25f * 257.0f;
	tess.numVertexes++;

	VectorMA( start, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0] = 0;
	tess.texCoords[tess.numVertexes][1] = 1;
	tess.color[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] * 257;
	tess.color[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] * 257;
	tess.color[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] * 257;
	tess.numVertexes++;

	VectorMA( end, spanWidth, up, tess.xyz[tess.numVertexes] );

	tess.texCoords[tess.numVertexes][0] = t;
	tess.texCoords[tess.numVertexes][1] = 0;
	tess.color[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] * 257;
	tess.color[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] * 257;
	tess.color[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] * 257;
	tess.numVertexes++;

	VectorMA( end, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0] = t;
	tess.texCoords[tess.numVertexes][1] = 1;
	tess.color[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] * 257;
	tess.color[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] * 257;
	tess.color[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] * 257;
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

	RB_CheckVao(tess.vao);

	for ( i = 0; i < numSegs; i++ )
	{
		int j;

		RB_CHECKOVERFLOW( 4, 6 );

		for ( j = 0; j < 4; j++ )
		{
			VectorCopy( pos[j], tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0] = (j < 2);
			tess.texCoords[tess.numVertexes][1] = (j && j != 3);
			tess.color[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] * 257;
			tess.color[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] * 257;
			tess.color[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] * 257;
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


static void LerpMeshVertexes(mdvSurface_t *surf, float backlerp)
{
	float *outXyz;
	int16_t *outNormal, *outTangent;
	mdvVertex_t *newVerts;
	int		vertNum;

	newVerts = surf->verts + backEnd.currentEntity->e.frame * surf->numVerts;

	outXyz =     tess.xyz[tess.numVertexes];
	outNormal =  tess.normal[tess.numVertexes];
	outTangent = tess.tangent[tess.numVertexes];

	if (backlerp == 0)
	{
		//
		// just copy the vertexes
		//

		for (vertNum=0 ; vertNum < surf->numVerts ; vertNum++)
		{
			VectorCopy(newVerts->xyz,    outXyz);
			VectorCopy4(newVerts->normal, outNormal);
			VectorCopy4(newVerts->tangent, outTangent);

			newVerts++;
			outXyz += 4;
			outNormal += 4;
			outTangent += 4;
		}
	}
	else
	{
		//
		// interpolate and copy the vertex and normal
		//

		mdvVertex_t *oldVerts;

		oldVerts = surf->verts + backEnd.currentEntity->e.oldframe * surf->numVerts;

		for (vertNum=0 ; vertNum < surf->numVerts ; vertNum++)
		{
			VectorLerp(newVerts->xyz,    oldVerts->xyz,    backlerp, outXyz);

			outNormal[0] = (int16_t)(newVerts->normal[0] * (1.0f - backlerp) + oldVerts->normal[0] * backlerp);
			outNormal[1] = (int16_t)(newVerts->normal[1] * (1.0f - backlerp) + oldVerts->normal[1] * backlerp);
			outNormal[2] = (int16_t)(newVerts->normal[2] * (1.0f - backlerp) + oldVerts->normal[2] * backlerp);
			outNormal[3] = 0;

			outTangent[0] = (int16_t)(newVerts->tangent[0] * (1.0f - backlerp) + oldVerts->tangent[0] * backlerp);
			outTangent[1] = (int16_t)(newVerts->tangent[1] * (1.0f - backlerp) + oldVerts->tangent[1] * backlerp);
			outTangent[2] = (int16_t)(newVerts->tangent[2] * (1.0f - backlerp) + oldVerts->tangent[2] * backlerp);
			outTangent[3] = newVerts->tangent[3];

			newVerts++;
			oldVerts++;
			outXyz += 4;
			outNormal += 4;
			outTangent += 4;
		}
	}

}


/*
=============
RB_SurfaceMesh
=============
*/
static void RB_SurfaceMesh(mdvSurface_t *surface) {
	int				j;
	float			backlerp;
	mdvSt_t			*texCoords;
	int				Bob, Doug;
	int				numVerts;

	if (  backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame ) {
		backlerp = 0;
	} else  {
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CheckVao(tess.vao);

	RB_CHECKOVERFLOW( surface->numVerts, surface->numIndexes );

	LerpMeshVertexes (surface, backlerp);

	Bob = tess.numIndexes;
	Doug = tess.numVertexes;
	for (j = 0 ; j < surface->numIndexes ; j++) {
		tess.indexes[Bob + j] = Doug + surface->indexes[j];
	}
	tess.numIndexes += surface->numIndexes;

	texCoords = surface->st;

	numVerts = surface->numVerts;
	for ( j = 0; j < numVerts; j++ ) {
		tess.texCoords[Doug + j][0] = texCoords[j].st[0];
		tess.texCoords[Doug + j][1] = texCoords[j].st[1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;

}


/*
==============
RB_SurfaceFace
==============
*/
static void RB_SurfaceFace( srfBspSurface_t *srf ) {
	if (RB_SurfaceVaoCached(srf->numVerts, srf->verts, srf->numIndexes,
		srf->indexes, srf->dlightBits, srf->pshadowBits))
	{
		return;
	}

	RB_SurfaceVertsAndIndexes(srf->numVerts, srf->verts, srf->numIndexes,
			srf->indexes, srf->dlightBits, srf->pshadowBits);
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
static void RB_SurfaceGrid( srfBspSurface_t *srf ) {
	int		i, j;
	float	*xyz;
	float	*texCoords, *lightCoords;
	int16_t *normal;
	int16_t *tangent;
	uint16_t *color;
	int16_t *lightdir;
	srfVert_t	*dv;
	int		rows, irows, vrows;
	int		used;
	int		widthTable[MAX_GRID_SIZE];
	int		heightTable[MAX_GRID_SIZE];
	float	lodError;
	int		lodWidth, lodHeight;
	int		numVertexes;
	int		dlightBits;
	int     pshadowBits;
	//int		*vDlightBits;

	if (RB_SurfaceVaoCached(srf->numVerts, srf->verts, srf->numIndexes,
		srf->indexes, srf->dlightBits, srf->pshadowBits))
	{
		return;
	}

	RB_CheckVao(tess.vao);

	dlightBits = srf->dlightBits;
	tess.dlightBits |= dlightBits;

	pshadowBits = srf->pshadowBits;
	tess.pshadowBits |= pshadowBits;

	// determine the allowable discrepance
	lodError = LodErrorForVolume( srf->lodOrigin, srf->lodRadius );

	// determine which rows and columns of the subdivision
	// we are actually going to use
	widthTable[0] = 0;
	lodWidth = 1;
	for ( i = 1 ; i < srf->width-1 ; i++ ) {
		if ( srf->widthLodError[i] <= lodError ) {
			widthTable[lodWidth] = i;
			lodWidth++;
		}
	}
	widthTable[lodWidth] = srf->width-1;
	lodWidth++;

	heightTable[0] = 0;
	lodHeight = 1;
	for ( i = 1 ; i < srf->height-1 ; i++ ) {
		if ( srf->heightLodError[i] <= lodError ) {
			heightTable[lodHeight] = i;
			lodHeight++;
		}
	}
	heightTable[lodHeight] = srf->height-1;
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
				RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex );
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
		tangent = tess.tangent[numVertexes];
		texCoords = tess.texCoords[numVertexes];
		lightCoords = tess.lightCoords[numVertexes];
		color = tess.color[numVertexes];
		lightdir = tess.lightdir[numVertexes];
		//vDlightBits = &tess.vertexDlightBits[numVertexes];

		for ( i = 0 ; i < rows ; i++ ) {
			for ( j = 0 ; j < lodWidth ; j++ ) {
				dv = srf->verts + heightTable[ used + i ] * srf->width
					+ widthTable[ j ];

				if ( tess.shader->vertexAttribs & ATTR_POSITION )
				{
					VectorCopy(dv->xyz, xyz);
					xyz += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_NORMAL )
				{
					VectorCopy4(dv->normal, normal);
					normal += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_TANGENT )
				{
					VectorCopy4(dv->tangent, tangent);
					tangent += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_TEXCOORD )
				{
					VectorCopy2(dv->st, texCoords);
					texCoords += 2;
				}

				if ( tess.shader->vertexAttribs & ATTR_LIGHTCOORD )
				{
					VectorCopy2(dv->lightmap, lightCoords);
					lightCoords += 2;
				}

				if ( tess.shader->vertexAttribs & ATTR_COLOR )
				{
					VectorCopy4(dv->color, color);
					color += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_LIGHTDIRECTION )
				{
					VectorCopy4(dv->lightdir, lightdir);
					lightdir += 4;
				}

				//*vDlightBits++ = dlightBits;
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
	// FIXME: implement this
#if 0
	GL_BindToTMU( tr.whiteImage, TB_COLORMAP );
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
#endif
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

void RB_SurfaceVaoMdvMesh(srfVaoMdvMesh_t * surface)
{
	//mdvModel_t     *mdvModel;
	//mdvSurface_t   *mdvSurface;
	refEntity_t    *refEnt;

	GLimp_LogComment("--- RB_SurfaceVaoMdvMesh ---\n");

	if (ShaderRequiresCPUDeforms(tess.shader))
	{
		RB_SurfaceMesh(surface->mdvSurface);
		return;
	}

	if(!surface->vao)
		return;

	//RB_CheckVao(surface->vao);
	RB_EndSurface();
	RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);

	R_BindVao(surface->vao);

	tess.useInternalVao = qfalse;

	tess.numIndexes = surface->numIndexes;
	tess.numVertexes = surface->numVerts;

	//mdvModel = surface->mdvModel;
	//mdvSurface = surface->mdvSurface;

	refEnt = &backEnd.currentEntity->e;

	glState.vertexAttribsInterpolation = (refEnt->oldframe == refEnt->frame) ? 0.0f : refEnt->backlerp;

	if (surface->mdvModel->numFrames > 1)
	{
		int frameOffset, attribIndex;
		vaoAttrib_t *vAtb;

		glState.vertexAnimation = qtrue;

		if (glRefConfig.vertexArrayObject)
		{
			qglBindBuffer(GL_ARRAY_BUFFER, surface->vao->vertexesVBO);
		}

		frameOffset    = refEnt->frame * surface->vao->frameSize;

		attribIndex = ATTR_INDEX_POSITION;
		vAtb = &surface->vao->attribs[attribIndex];
		qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

		attribIndex = ATTR_INDEX_NORMAL;
		vAtb = &surface->vao->attribs[attribIndex];
		qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

		attribIndex = ATTR_INDEX_TANGENT;
		vAtb = &surface->vao->attribs[attribIndex];
		qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

		frameOffset = refEnt->oldframe * surface->vao->frameSize;

		attribIndex = ATTR_INDEX_POSITION2;
		vAtb = &surface->vao->attribs[attribIndex];
		qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

		attribIndex = ATTR_INDEX_NORMAL2;
		vAtb = &surface->vao->attribs[attribIndex];
		qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

		attribIndex = ATTR_INDEX_TANGENT2;
		vAtb = &surface->vao->attribs[attribIndex];
		qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));


		if (!glRefConfig.vertexArrayObject)
		{
			attribIndex = ATTR_INDEX_TEXCOORD;
			vAtb = &surface->vao->attribs[attribIndex];
			qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset));
		}
	}

	RB_EndSurface();

	// So we don't lerp surfaces that shouldn't be lerped
	glState.vertexAnimation = qfalse;
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
	(void(*)(void*))RB_SurfaceMesh,			// SF_MDV,
	(void(*)(void*))RB_MDRSurfaceAnim,		// SF_MDR,
	(void(*)(void*))RB_IQMSurfaceAnim,		// SF_IQM,
	(void(*)(void*))RB_SurfaceFlare,		// SF_FLARE,
	(void(*)(void*))RB_SurfaceEntity,		// SF_ENTITY
	(void(*)(void*))RB_SurfaceVaoMdvMesh,   // SF_VAO_MDVMESH
	(void(*)(void*))RB_IQMSurfaceAnimVao,   // SF_VAO_IQM
};
