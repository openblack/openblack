$input v_texcoord0, v_texcoord1, v_weight, v_materialID0, v_materialID1, v_materialBlend, v_lightLevel, v_waterAlpha, v_distToCamera, v_shadowCoord

#include <bgfx_shader.sh>

#define M_PI 3.1415926535897932384626433832795

SAMPLER2DARRAY(s0_materials, 0);
SAMPLER2D(s1_bump, 1);
SAMPLER2D(s2_smallBump, 2);
SAMPLER2D(s3_footprints, 3);
SAMPLER2D(s4_handShadow, 4);
// Where the shadows of trees, rocks and buildings cover the island, laid out as the footprints are
SAMPLER2D(s5_objectShadows, 5);
// The brightness the hand's light gives the land around it (HandLight)
SAMPLER2D(s6_handLight, 6);

uniform vec4 u_skyAndBump;
// x: darkness of the objects' shadows where they fully cover a texel, 0 without them
// yz: size of a texel of s5_objectShadows
uniform vec4 u_objectShadows;
// x: darkness of the hand's shadow where its silhouette fully covers a texel, 0 without a shadow
// y: how far before the hand along the light the shadow starts
uniform vec4 u_handShadow;
// z: how strongly the hand lights the land
uniform vec4 u_handLight;

void main()
{
	// unpack uniforms
	float skyType = u_skyAndBump.x;
	float bumpMapStrength = u_skyAndBump.y;
	float smallBumpMapStrength = u_skyAndBump.z;

	// do each vert with both materials
	vec4 colOne = mix(
		texture2DArray(s0_materials, vec3(v_texcoord0.xy, v_materialID0.r)),
		texture2DArray(s0_materials, vec3(v_texcoord0.xy, v_materialID1.r)),
		v_materialBlend.r
	) * v_weight.r;
	vec4 colTwo = mix(
		texture2DArray(s0_materials, vec3(v_texcoord0.xy, v_materialID0.g)),
		texture2DArray(s0_materials, vec3(v_texcoord0.xy, v_materialID1.g)),
		v_materialBlend.g
	) * v_weight.g;
	vec4 colThree = mix(
		texture2DArray(s0_materials, vec3(v_texcoord0.xy, v_materialID0.b)),
		texture2DArray(s0_materials, vec3(v_texcoord0.xy, v_materialID1.b)),
		v_materialBlend.b
	) * v_weight.b;

	// add the 3 blended textures together
	vec4 col = colOne + colTwo + colThree;

	// apply bump map (2x because it's half bright?)
	float bump = mix(1.0f, texture2D(s1_bump, v_texcoord0.xy).r * 2.0f, bumpMapStrength);
	col = col * bump;

	// don't apply smallbump unless we're close
	if (v_distToCamera < 200.0f) {
		float smallStrength = (1.0f - (v_distToCamera / 200.0f)) * smallBumpMapStrength;

		float smallbump = 1.0f - mix(0.0f, texture2D(s2_smallBump, v_texcoord0.xy * 10.0f).r, smallStrength);
		col = col * smallbump;
	}

	vec4 footprints = texture2D(s3_footprints, v_texcoord1.xy);
	col.rgb = mix(col.rgb, footprints.rgb, footprints.a);

	// The objects' shadows are baked into the land's textures over the footprints, under the light. LH3D counts how
	// many of eight samples in each texel are covered; four filtered taps across a texel soften the edges as much.
	vec2 texel = u_objectShadows.yz * 0.5f;
	float objectShadow = 0.25f * (
		texture2D(s5_objectShadows, v_texcoord1.xy + vec2(-texel.x, -texel.y)).r +
		texture2D(s5_objectShadows, v_texcoord1.xy + vec2(texel.x, -texel.y)).r +
		texture2D(s5_objectShadows, v_texcoord1.xy + vec2(-texel.x, texel.y)).r +
		texture2D(s5_objectShadows, v_texcoord1.xy + vec2(texel.x, texel.y)).r);
	col.rgb = col.rgb * (1.0f - objectShadow * u_objectShadows.x);

	// apply light map
	float skyBightness = skyType / 2.0f;
	float light = mix(0.25f, clamp(v_lightLevel * 2.0f, 0.5f, 1.0f), skyBightness);
	// The hand's light, where it is brighter than the land's own
	light = max(light, texture2D(s6_handLight, v_texcoord0.zw).r * u_handLight.z);
	col = col * light;

	// the hand's shadow, projected along the sunlight onto the land beyond it
	vec3 shadowCoord = v_shadowCoord.xyz;
	if (u_handShadow.x > 0.0f && shadowCoord.z > u_handShadow.y && shadowCoord.x > 0.0f && shadowCoord.x < 1.0f &&
	    shadowCoord.y > 0.0f && shadowCoord.y < 1.0f)
	{
		float coverage = texture2D(s4_handShadow, shadowCoord.xy).r;
		col.rgb = col.rgb * (1.0f - coverage * v_shadowCoord.w * u_handShadow.x);
	}

	gl_FragColor = vec4(col.rgb, v_waterAlpha);

	//gl_FragColor.r = v_distToCamera / 200.0f;

	if (v_waterAlpha == 0.0f) {
		discard;
	}
}
