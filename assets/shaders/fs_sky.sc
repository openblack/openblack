$input v_position, v_texcoord0, v_normal

#include <bgfx_shader.sh>

SAMPLER2DARRAY(s_diffuse, 0);
uniform vec4 u_typeAlignment;

void main()
{
	// constants
	const float nightIndex = 0.0f;
	const float duskIndex = 1.0f;
	const float dayIndex = 2.0f;
	const float evilIndex = 0.0f;
	const float neutralIndex = 1.0f;
	const float goodIndex = 2.0f;

	// unpack uniform
	float textureType = clamp(u_typeAlignment.x, nightIndex, dayIndex);
	float alignment = clamp(u_typeAlignment.y, evilIndex, goodIndex);

	float alignT = mod(alignment, 1.0f);
	float alignA = alignment - alignT;
	float alignB = min(alignA + 1.0f, goodIndex);

	float typeT = mod(textureType, 1.0f);
	float typeA = textureType - typeT;
	float typeB = min(typeA + 1.0f, dayIndex);

	float indexAA = 3.0f * alignA + typeA;
	float indexAB = 3.0f * alignA + typeB;
	float indexBA = 3.0f * alignB + typeA;
	float indexBB = 3.0f * alignB + typeB;

	vec4 colorAA = texture2DArray(s_diffuse, vec3(v_texcoord0.xy, indexAA));
	vec4 colorAB = texture2DArray(s_diffuse, vec3(v_texcoord0.xy, indexAB));
	vec4 colorBA = texture2DArray(s_diffuse, vec3(v_texcoord0.xy, indexBA));
	vec4 colorBB = texture2DArray(s_diffuse, vec3(v_texcoord0.xy, indexBB));

	vec4 colorAlignA = mix(colorAA, colorBA, alignT);
	vec4 colorAlignB = mix(colorAB, colorBB, alignT);
	colorAlignA.a = 1.0f;
	colorAlignB.a = 1.0f;

	// TODO(#796): This seems to be vertex lit in vanilla, so brought in through the vertex shader and each vertex is a little different
	vec4 amospheric_fog = vec4(196.0f / 255.0f, 210.0f / 255.0f, 190.0f / 255.0f, 254.0f / 255.0f);

	gl_FragColor = amospheric_fog * mix(colorAlignA, colorAlignB, typeT);

}
