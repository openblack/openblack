$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_color0

#include <bgfx_shader.sh>

// The villagers' ground blobs: quads laid over the land, in the world
void main()
{
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	v_color0 = a_color0;
	gl_Position = mul(u_viewProj, vec4(a_position.xyz, 1.0f));
}
