$input a_position, a_texcoord0, a_color0, a_color1
$output v_texcoord0, v_color0, v_snowLight

#include <bgfx_shader.sh>

// A point of a particle surface of revolution, already in the world: its texture coordinates, its colour and alpha,
// and the specular colour the game adds after the texture
void main()
{
	gl_Position = mul(u_viewProj, vec4(a_position.xyz, 1.0));
	v_texcoord0 = vec4(a_texcoord0, 0.0, 0.0);
	v_color0 = a_color0;
	v_snowLight = a_color1;
}
