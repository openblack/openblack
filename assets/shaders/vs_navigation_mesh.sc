$input a_position

#include <bgfx_shader.sh>

void main()
{
	gl_Position = mul(u_viewProj, vec4(a_position.xyz, 1.0f));
}
