$input v_texcoord0, v_color0

#include <bgfx_shader.sh>

// A model's skin, its colours and alpha in one texture
SAMPLER2D(s_diffuse, 0);

// A piece of a broken model: its skin in its vertices' colour, fading with their alpha
void main()
{
	vec4 texel = texture2D(s_diffuse, v_texcoord0.xy);
	gl_FragColor = vec4(texel.rgb * v_color0.rgb, texel.a * v_color0.a);
}
