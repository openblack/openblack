$input a_position, a_texcoord0, a_normal, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_position, v_texcoord0, v_normal

#include <bgfx_shader.sh>

void main()
{
	mat4 model;
	model[0] = i_data0;
	model[1] = i_data1;
	model[2] = i_data2;
	model[3] = i_data3;

	v_position = instMul(model, vec4(a_position.xyz, 1.0f));
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	v_normal = a_normal;
	gl_Position = mul(u_viewProj, v_position);

}
