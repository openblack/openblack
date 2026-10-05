$input a_position, a_texcoord0
$output v_texcoord0

#include <bgfx_shader.sh>

// The sun and the moon: textured meshes placed far out in the sky
void main()
{
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	gl_Position = mul(u_viewProj, mul(u_model[0], vec4(a_position.xyz, 1.0f)));
}
