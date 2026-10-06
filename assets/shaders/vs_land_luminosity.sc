$input a_position, a_texcoord0
$output v_texcoord0

#include <bgfx_shader.sh>

// A square of the land's luminosity this frame, in cells
void main()
{
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	gl_Position = mul(u_viewProj, vec4(a_position.xy, 0.0f, 1.0f));
}
