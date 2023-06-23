$input a_position, a_texcoord1, a_color1, a_color2, a_texcoord2, a_color0, a_color3, i_data0, i_data1, i_data2
$output v_texcoord0, v_texcoord1, v_weight, v_materialID0, v_materialID1, v_materialBlend, v_lightLevel, v_waterAlpha, v_distToCamera, v_skyABump

#include <bgfx_shader.sh>

# if BGFX_SHADER_LANGUAGE_HLSL > 300 || BGFX_SHADER_LANGUAGE_SPIRV
#   define materialIdFix(x) (floatBitsToInt(x))
# else
#   define materialIdFix(x) (ivec3(x))
# endif

SAMPLER2D(t0_heightmap, 0);

void main()
{
	// Unpack
	vec2 blockPosition = i_data0.xy;
	vec2 blockSize = i_data0.zw;
	vec2 extentMin = i_data1.xy;
	vec2 extentMax = i_data1.zw;

	v_skyABump = vec4(i_data2);

	vec2 normalizedPos = (a_position.xz + blockPosition);
	v_texcoord0 = vec4(a_position.zx / blockSize.yx, 0.0f, 0.0f);
	vec2 blockStartUv = (blockPosition + a_position.xz - extentMin) / (extentMax - extentMin);
	#if !BGFX_SHADER_LANGUAGE_GLSL
		blockStartUv.y = 1.0f - blockStartUv.y;
	#endif
	v_texcoord1 = vec4(blockStartUv, 0.0f, 0.0f);
	v_materialID0 = materialIdFix(a_color1);
	v_materialID1 = materialIdFix(a_color2);
	v_materialBlend = a_texcoord2;
	v_weight = a_texcoord1;
	v_lightLevel = a_color0.x;
	v_waterAlpha = a_color3;
	vec2 uv_pos = blockStartUv;
	uv_pos.y = 1.0 - uv_pos.y;

	vec3 transformedPosition = vec3(a_position.x + blockPosition.x, 15.0f, a_position.z + blockPosition.y);
	vec4 height = texture2DLod(t0_heightmap, uv_pos, 0);
	transformedPosition.y = height.r * 255.0;

	vec4 cs_position = mul(u_view, vec4(transformedPosition, 1.0f));
	v_distToCamera = cs_position.z;
	gl_Position = mul(u_proj, cs_position);
}
