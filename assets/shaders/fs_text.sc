$input v_color0, v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_tex, 0);

void main()
{
	vec4 texel = texture2D(s_tex, v_texcoord0.xy);
	gl_FragColor = vec4(v_color0.rgb, texel.a * v_color0.a);
}