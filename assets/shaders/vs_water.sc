$input a_position, a_color0
$output v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>

void main()
{
	// The sea plane at sea level: its world x and z and its view depth
	vec4 vertex = vec4(a_position.x, 0.0f, a_position.y, 1.0f);
	gl_Position = mul(u_viewProj, vertex);
	v_texcoord0 = vec4(vertex.x, vertex.z, mul(u_view, vertex).z, 0.0f);
	v_texcoord1 = vec4_splat(0.0f);
}
