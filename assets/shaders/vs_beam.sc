$input a_position, a_color0, a_texcoord0
$output v_color0, v_texcoord0

#include <bgfx_shader.sh>

void main()
{
	v_color0 = a_color0;
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	gl_Position = mul(u_modelViewProj, vec4(a_position.xyz, 1.0f));
}
