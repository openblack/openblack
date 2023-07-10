$input a_position, a_texcoord1, a_color1, a_color2, a_texcoord2, a_color0, a_color3
$output v_texcoord0, v_texcoord1, v_weight, v_materialID0, v_materialID1, v_materialBlend, v_lightLevel, v_waterAlpha, v_distToCamera

#include <bgfx_shader.sh>

#if BGFX_SHADER_LANGUAGE_HLSL > 300 || BGFX_SHADER_LANGUAGE_SPIRV
#   define materialIdFix(x) (floatBitsToInt(x))
#else
#   define materialIdFix(x) (ivec3(x))
#endif

uniform vec4 u_blockPositionAndSize;
uniform vec4 u_islandExtent;

void main()
{
	// Unpack
	vec2 blockPosition = u_blockPositionAndSize.xy;
	vec2 blockSize = u_blockPositionAndSize.zw;
	vec2 extentMin = u_islandExtent.xy;
	vec2 extentMax = u_islandExtent.zw;

	v_texcoord0 = vec4(a_position.zx / blockSize.yx, 0.0f, 0.0f);
	vec2 blockStartUv = (blockPosition + a_position.xz - extentMin) / (extentMax - extentMin);
	#if !BGFX_SHADER_LANGUAGE_GLSL
		blockStartUv.y = 1.0f - blockStartUv.y;
	#endif
	v_texcoord1 = vec4(blockStartUv, 0.0f, 0.0f);
	v_weight = a_texcoord1;
	v_materialID0 = materialIdFix(a_color1);
	v_materialID1 = materialIdFix(a_color2);
	v_materialBlend = a_texcoord2;
	v_lightLevel = a_color0.x;
	v_waterAlpha = a_color3;

	vec3 transformedPosition = vec3(a_position.x + blockPosition.x, a_position.y, a_position.z + blockPosition.y);

	vec4 cs_position = mul(u_view, vec4(transformedPosition, 1.0f));
	v_distToCamera = cs_position.z;
	gl_Position = mul(u_proj, cs_position);
}
