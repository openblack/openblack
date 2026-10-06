#ifndef CREATURE_SHADOW_SH
#define CREATURE_SHADOW_SH

// The creatures' shadows (src/Graphics/CreatureShadow.h): each creature's silhouette from the light, in a cell of a
// shared texture, projected onto what is drawn here.

#define CREATURE_SHADOWS 8

SAMPLER2D(s_creatureShadows, 14);
// World position to the shadow's texture coordinates in xy and distance past the creature's centre along the light in z
uniform mat4 u_creatureShadowMatrix[CREATURE_SHADOWS];
// x: how much a fully covered texel darkens, y: how far before the creature's centre along the light the shadow starts
uniform vec4 u_creatureShadow[CREATURE_SHADOWS];
// x: how many shadows there are, y: the cells across the texture, zw: half a texel of the texture
uniform vec4 u_creatureShadowInfo;

// What of the light one creature's shadow leaves at a point, 0 to 1
float CreatureShadowCellLight(int i, vec3 world)
{
	if (float(i) >= u_creatureShadowInfo.x)
	{
		return 1.0f;
	}
	vec3 coord = mul(u_creatureShadowMatrix[i], vec4(world, 1.0f)).xyz;
	float left = float(i) / u_creatureShadowInfo.y;
	float right = (float(i) + 1.0f) / u_creatureShadowInfo.y;
	if (coord.z <= u_creatureShadow[i].y || coord.x <= left || coord.x >= right || coord.y <= 0.0f || coord.y >= 1.0f)
	{
		return 1.0f;
	}
	// Four filtered taps across a texel of the game's shadow soften its edges as its box filter does
	vec2 texel = u_creatureShadowInfo.zw;
	float coverage = 0.25f * (texture2DLod(s_creatureShadows, coord.xy + vec2(-texel.x, -texel.y), 0.0f).r +
	                          texture2DLod(s_creatureShadows, coord.xy + vec2(texel.x, -texel.y), 0.0f).r +
	                          texture2DLod(s_creatureShadows, coord.xy + vec2(-texel.x, texel.y), 0.0f).r +
	                          texture2DLod(s_creatureShadows, coord.xy + vec2(texel.x, texel.y), 0.0f).r);
	return 1.0f - coverage * u_creatureShadow[i].x;
}

// What of the light the creatures' shadows leave at a point, 0 to 1.
// Each shadow is read with a constant index rather than in a loop: a loop indexes the uniform arrays by a register,
// which Direct3D 12's bgfx renderer mis-patches when it moves a fragment shader's uniforms past the vertex shader's,
// leaving a corrupt shader that fails to create a pipeline.
float CreatureShadowLight(vec3 world)
{
	return CreatureShadowCellLight(0, world) * CreatureShadowCellLight(1, world) * CreatureShadowCellLight(2, world) *
	       CreatureShadowCellLight(3, world) * CreatureShadowCellLight(4, world) * CreatureShadowCellLight(5, world) *
	       CreatureShadowCellLight(6, world) * CreatureShadowCellLight(7, world);
}

#endif // CREATURE_SHADOW_SH
