#version 330 core

in vec2 UV;
in vec3 Weights;
flat in uvec3 FirstMaterialID;
flat in uvec3 SecondMaterialID;
flat in vec3 MaterialBlendCoefficient;
in float LightLevel;
in float WaterAlpha;

uniform sampler2DArray sMaterials;
uniform sampler2D sBumpMap;
uniform sampler2D sSmallBumpMap;
uniform float timeOfDay;
uniform float bumpmapStrength;
uniform float smallBumpmapStrength;

out vec4 FragColor;

void main()
{
	// do each vert with both materials
	vec4 colOne = mix(
		texture(sMaterials, vec3(UV, FirstMaterialID.r)),
		texture(sMaterials, vec3(UV, SecondMaterialID.r)),
		MaterialBlendCoefficient.r
	) * Weights.r;
	vec4 colTwo = mix(
		texture(sMaterials, vec3(UV, FirstMaterialID.g)),
		texture(sMaterials, vec3(UV, SecondMaterialID.g)),
		MaterialBlendCoefficient.g
	) * Weights.g;
	vec4 colThree = mix(
		texture(sMaterials, vec3(UV, FirstMaterialID.b)),
		texture(sMaterials, vec3(UV, SecondMaterialID.b)),
		MaterialBlendCoefficient.b
	) * Weights.b;

	// add the 3 blended textures together
	vec4 col = colOne + colTwo + colThree;

	// apply bump map (2x because it's half bright?)
	float bump = mix(1.0f, texture(sBumpMap, UV).r * 2, bumpmapStrength);
	col = col * bump;

	// apply small bump map
	float smallbump = mix(0.0f, texture(sSmallBumpMap, UV * 10).r / 2, smallBumpmapStrength);
	col = col - smallbump;

	// apply light map
	col = col * mix(.25f, LightLevel, timeOfDay);

	FragColor = vec4(col.r, col.g, col.b, WaterAlpha);

	if (WaterAlpha == 0.0) {
		discard;
	}
}