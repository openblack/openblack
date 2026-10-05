$input v_texcoord0, v_texcoord1, v_lightColour, v_smallBumpFade, v_shadowCoord, v_haze

#include <bgfx_shader.sh>

#define M_PI 3.1415926535897932384626433832795

// The blocks' painted textures, a layer each: the land's colour, and the coast alpha that fades it into the sea
SAMPLER2DARRAY(s0_blockTextures, 0);
// The small bump detail's colour and alpha, 12 times across a block
SAMPLER2D(s1_smallBumpAlpha, 1);
SAMPLER2D(s2_smallBump, 2);
SAMPLER2D(s3_footprints, 3);
SAMPLER2D(s4_handShadow, 4);
// Where the shadows of trees, rocks and buildings cover the island, laid out as the footprints are
SAMPLER2D(s5_objectShadows, 5);
// What of the land's alpha the rivers' channels leave, laid out as the footprints are
SAMPLER2D(s8_landAlpha, 8);

// x: the block's layer of s0_blockTextures
uniform vec4 u_block;
// y: how bright the land's light is, a half for the land mirrored under the sea
// z: the small bump detail's strength
// w: 1 to draw the land's textures alone, unlit and with no sea, as the temple's map is textured with
uniform vec4 u_skyAndBump;
// x: darkness of the objects' shadows where they fully cover a texel, 0 without them
// yz: size of a texel of s5_objectShadows
uniform vec4 u_objectShadows;
// x: darkness of the hand's shadow where its silhouette fully covers a texel, 0 without a shadow
// y: how far before the hand along the light the shadow starts
uniform vec4 u_handShadow;

void main()
{
	// unpack uniforms
	float skyType = u_skyAndBump.x;
	float smallBumpMapStrength = u_skyAndBump.z;

	// The block's texture, filtered across its texels
	vec4 block = texture2DArray(s0_blockTextures, vec3(v_texcoord0.xy, u_block.x));
	vec4 col = vec4(block.rgb, 1.0f);

	vec4 footprints = texture2D(s3_footprints, v_texcoord1.xy);
	col.rgb = mix(col.rgb, footprints.rgb, footprints.a);

	// The objects' shadows are baked into the land's textures over the footprints, under the light. The game counts
	// how many of eight samples in each texel are covered; four filtered taps across a texel soften the edges as much.
	vec2 texel = u_objectShadows.yz * 0.5f;
	float objectShadow = 0.25f * (
		texture2D(s5_objectShadows, v_texcoord1.xy + vec2(-texel.x, -texel.y)).r +
		texture2D(s5_objectShadows, v_texcoord1.xy + vec2(texel.x, -texel.y)).r +
		texture2D(s5_objectShadows, v_texcoord1.xy + vec2(-texel.x, texel.y)).r +
		texture2D(s5_objectShadows, v_texcoord1.xy + vec2(texel.x, texel.y)).r);
	col.rgb = col.rgb * (1.0f - objectShadow * u_objectShadows.x);

	if (u_skyAndBump.w > 0.0f)
	{
		gl_FragColor = vec4(col.rgb, 1.0f);
		return;
	}

	col.rgb = col.rgb * v_lightColour * v_haze.a;

	// the hand's shadow, projected along the sunlight onto the land beyond it
	vec3 shadowCoord = v_shadowCoord.xyz;
	if (u_handShadow.x > 0.0f && shadowCoord.z > u_handShadow.y && shadowCoord.x > 0.0f && shadowCoord.x < 1.0f &&
	    shadowCoord.y > 0.0f && shadowCoord.y < 1.0f)
	{
		float coverage = texture2D(s4_handShadow, shadowCoord.xy).r;
		col.rgb = col.rgb * (1.0f - coverage * v_shadowCoord.w * u_handShadow.x);
	}

	// The small bump detail is a second layer over the lit land, its colour unlit, blended by its alpha whatever the
	// coast alpha. The distance haze is added to both. The land is blended over the sea by its coast alpha, and writes
	// its depth even where it is clear. Both layers are drawn at once, premultiplied: what of the sea shows through is
	// what neither layer covers.
	vec2 smallBumpUv = v_texcoord0.xy * 12.0f;
	vec3 smallBump = texture2D(s2_smallBump, smallBumpUv).rgb * v_smallBumpFade.x;
	float bumpAlpha = texture2D(s1_smallBumpAlpha, smallBumpUv).r * v_smallBumpFade.y * smallBumpMapStrength;
	vec3 land = min(col.rgb + v_haze.rgb, vec3_splat(1.0f));
	vec3 detail = min(smallBump + v_haze.rgb, vec3_splat(1.0f));
	// The rivers' channels lower the coast alpha where they run, so the sea shows through as their water
	float landAlpha = min(block.a, texture2D(s8_landAlpha, v_texcoord1.xy).r);
	gl_FragColor = vec4(land * landAlpha * (1.0f - bumpAlpha) + detail * bumpAlpha,
	                    1.0f - (1.0f - landAlpha) * (1.0f - bumpAlpha));
}
