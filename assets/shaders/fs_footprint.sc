$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_footprint, 0);

void main()
{
	gl_FragColor = texture2D(s_footprint, v_texcoord0.xy);
}
