$input a_position, a_texcoord0, a_normal, a_indices, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_position, v_texcoord0, v_normal

#if BGFX_SHADER_LANGUAGE_HLSL == 3
#define BGFX_CONFIG_MAX_BONES 48
#else
#define BGFX_CONFIG_MAX_BONES 128
#endif

#include <bgfx_shader.sh>

void main()
{

#if BGFX_SHADER_LANGUAGE_HLSL > 300 || BGFX_SHADER_LANGUAGE_PSSL || BGFX_SHADER_LANGUAGE_SPIRV || BGFX_SHADER_LANGUAGE_METAL
	uint modelIndex = uint(max(0, asint(a_indices.x)));
#else
	uint modelIndex = uint(max(0, a_indices.x));
#endif

	mat4 model;
	model[0] = i_data0;
	model[1] = i_data1;
	model[2] = i_data2;
	model[3] = i_data3;

	v_position = instMul(model, mul(u_model[modelIndex], vec4(a_position.xyz, 1.0f)));
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	v_normal = a_normal;
	gl_Position = mul(u_viewProj, v_position);

}
