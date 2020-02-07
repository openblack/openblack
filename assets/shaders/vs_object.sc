$input a_position, a_texcoord0, a_normal, a_indices
$output v_position, v_texcoord0, v_normal

#define BGFX_CONFIG_MAX_BONES 64

#include <bgfx_shader.sh>

void main()
{
	v_position = mul(u_model[uint(max(0, a_indices.x))], vec4(a_position.xyz, 1.0f));
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	v_normal = a_normal;
	gl_Position = mul(u_viewProj, v_position);
}
