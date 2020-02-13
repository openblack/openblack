$input a_position, a_texcoord0, a_normal, a_indices
$output v_position, v_texcoord0, v_normal

#if BGFX_SHADER_LANGUAGE_HLSL == 3
#define BGFX_CONFIG_MAX_BONES 48
#else
#define BGFX_CONFIG_MAX_BONES 64
#endif

#include <bgfx_shader.sh>

void main()
{

#if BGFX_SHADER_LANGUAGE_HLSL > 3 || BGFX_SHADER_LANGUAGE_PSSL || BGFX_SHADER_LANGUAGE_SPIRV || BGFX_SHADER_LANGUAGE_METAL
	uint modelIndex = uint(max(0, floatBitsToInt(a_indices.x)));
#else
	uint modelIndex = uint(max(0, a_indices.x));
#endif
	v_position = mul(u_model[modelIndex], vec4(a_position.xyz, 1.0f));
	v_texcoord0 = vec4(a_texcoord0, 0, 0);
	v_normal = a_normal;
	gl_Position = mul(u_viewProj, v_position);
}
