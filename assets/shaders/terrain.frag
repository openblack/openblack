#version 330 core

in vec2 v_texcoord0;
in vec3 v_weights;
flat in uvec3 v_FirstMaterialID;
flat in uvec3 v_SecondMaterialID;
flat in vec3 v_MaterialBlendCoefficient;
in float v_LightLevel;
in float v_WaterAlpha;

uniform sampler2DArray sMaterials;
uniform sampler2D sBumpMap;
uniform sampler2D sSmallBumpMap;
uniform float u_timeOfDay;
uniform float u_bumpmapStrength;
uniform float u_smallBumpmapStrength;

out vec4 FragColor;

void main()
{
	// do each vert with both materials
	vec4 colOne = mix(
		texture(sMaterials, vec3(v_texcoord0, v_FirstMaterialID.r)),
		texture(sMaterials, vec3(v_texcoord0, v_SecondMaterialID.r)),
		v_MaterialBlendCoefficient.r
	) * v_weights.r;
	vec4 colTwo = mix(
		texture(sMaterials, vec3(v_texcoord0, v_FirstMaterialID.g)),
		texture(sMaterials, vec3(v_texcoord0, v_SecondMaterialID.g)),
		v_MaterialBlendCoefficient.g
	) * v_weights.g;
	vec4 colThree = mix(
		texture(sMaterials, vec3(v_texcoord0, v_FirstMaterialID.b)),
		texture(sMaterials, vec3(v_texcoord0, v_SecondMaterialID.b)),
		v_MaterialBlendCoefficient.b
	) * v_weights.b;

	// add the 3 blended textures together
	vec4 col = colOne + colTwo + colThree;

	// apply bump map (2x because it's half bright?)
	float bump = mix(1.0f, texture(sBumpMap, v_texcoord0).r * 2, u_bumpmapStrength);
	col = col * bump;

	float smallbump = 1 - mix(0.0f, texture(sSmallBumpMap, v_texcoord0 * 10).r, u_smallBumpmapStrength);
	col = col * smallbump;

	// apply light map
	col = col * mix(.25f, clamp(v_LightLevel * 2, 0.5, 1), u_timeOfDay);

	FragColor = vec4(col.r, col.g, col.b, v_WaterAlpha);

	if (v_WaterAlpha == 0.0) {
		discard;
	}
}
