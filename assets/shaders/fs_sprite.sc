$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
uniform vec4 u_tint;

void main()
{
	gl_FragColor = texture2D(s_diffuse, v_texcoord0.xy).rrrr * u_tint;
}
