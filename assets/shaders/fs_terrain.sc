$input v_texcoord0, v_weight, v_materialID0, v_materialID1, v_materialBlend, v_lightLevel, v_waterAlpha

#include <bgfx_shader.sh>

SAMPLER2DARRAY(s_materials, 0);
SAMPLER2D(s_bump, 1);
SAMPLER2D(s_smallBump, 2);

uniform vec4 u_timeOfDay;
uniform vec4 u_bumpmapStrength;
uniform vec4 u_smallBumpmapStrength;

void main()
{
	// do each vert with both materials
	vec4 colOne = mix(
		texture2DArray(s_materials, vec3(v_texcoord0.xy, v_materialID0.r)),
		texture2DArray(s_materials, vec3(v_texcoord0.xy, v_materialID1.r)),
		v_materialBlend.r
	) * v_weight.r;
	vec4 colTwo = mix(
		texture2DArray(s_materials, vec3(v_texcoord0.xy, v_materialID0.g)),
		texture2DArray(s_materials, vec3(v_texcoord0.xy, v_materialID1.g)),
		v_materialBlend.g
	) * v_weight.g;
	vec4 colThree = mix(
		texture2DArray(s_materials, vec3(v_texcoord0.xy, v_materialID0.b)),
		texture2DArray(s_materials, vec3(v_texcoord0.xy, v_materialID1.b)),
		v_materialBlend.b
	) * v_weight.b;

	// add the 3 blended textures together
	vec4 col = colOne + colTwo + colThree;

	// apply bump map (2x because it's half bright?)
	float bump = mix(1.0f, texture2D(s_bump, v_texcoord0.xy).r * 2, u_bumpmapStrength.r);
	col = col * bump;

	float smallbump = 1 - mix(0.0f, texture2D(s_smallBump, v_texcoord0.xy * 10).r, u_smallBumpmapStrength.r);
	col = col * smallbump;

	// apply light map
	col = col * mix(.25f, clamp(v_lightLevel * 2, 0.5, 1), u_timeOfDay.r);

	gl_FragColor = vec4(col.rgb, v_waterAlpha);

	if (v_waterAlpha == 0.0) {
		discard;
	}
}
