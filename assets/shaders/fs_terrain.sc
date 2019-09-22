$input v_weight
//#version 330 core
//
//in vec2 v_texcoord0;
//in vec3 v_weights;
//flat in uvec3 v_FirstMaterialID;
//flat in uvec3 v_SecondMaterialID;
//flat in vec3 v_MaterialBlendCoefficient;
//in float v_LightLevel;
//in float v_WaterAlpha;

#include <bgfx_shader.sh>

//uniform sampler2DArray s_materials;
SAMPLER2DARRAY(s_materials, 0);
SAMPLER2D(s_bump, 1);
SAMPLER2D(s_smallBump, 0);
//uniform sampler2D s_bump;
//uniform sampler2D s_smallBump;
//uniform float u_timeOfDay;
//uniform float u_bumpmapStrength;
//uniform float u_smallBumpmapStrength;
//
// out vec4 o_color;

void main()
{
//	// do each vert with both materials
//	vec4 colOne = mix(
//		texture(s_materials, vec3(v_texcoord0, v_FirstMaterialID.r)),
//		texture(s_materials, vec3(v_texcoord0, v_SecondMaterialID.r)),
//		v_MaterialBlendCoefficient.r
//	) * v_weight.r;
//	vec4 colTwo = mix(
//		texture(s_materials, vec3(v_texcoord0, v_FirstMaterialID.g)),
//		texture(s_materials, vec3(v_texcoord0, v_SecondMaterialID.g)),
//		v_MaterialBlendCoefficient.g
//	) * v_weight.g;
//	vec4 colThree = mix(
//		texture(s_materials, vec3(v_texcoord0, v_FirstMaterialID.b)),
//		texture(s_materials, vec3(v_texcoord0, v_SecondMaterialID.b)),
//		v_MaterialBlendCoefficient.b
//	) * v_weight.b;
//
//	// add the 3 blended textures together
//	vec4 col = colOne + colTwo + colThree;
//
//	// apply bump map (2x because it's half bright?)
//	float bump = mix(1.0f, texture(s_bump, v_texcoord0).r * 2, u_bumpmapStrength);
//	col = col * bump;
//
//	float smallbump = 1 - mix(0.0f, texture(s_smallBump, v_texcoord0 * 10).r, u_smallBumpmapStrength);
//	col = col * smallbump;
//
//	// apply light map
//	col = col * mix(.25f, clamp(v_LightLevel * 2, 0.5, 1), u_timeOfDay);
//
//	o_color = vec4(col.r, col.g, col.b, v_WaterAlpha);
//
//	if (v_WaterAlpha == 0.0) {
//		discard;
//	}

	gl_FragColor = vec4(v_weight, 1.0f);
}
