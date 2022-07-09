$input a_position, a_texcoord0, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_texcoord0

#include <bgfx_shader.sh>

void main()
{
	mat4 model;
	model[0] = i_data0;
	model[1] = i_data1;
	model[2] = i_data2;
	model[3] = i_data3;

	vec4 position = instMul(model, mul(u_model[0], vec4(a_position.x, 0.0f, a_position.y, 1.0f)));
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	gl_Position = mul(u_modelViewProj, position);
}
