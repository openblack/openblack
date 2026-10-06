$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
// The texture's alpha, when it has one of its own
SAMPLER2D(s_alpha, 1);
// The colour the texture is drawn in, alpha too
uniform vec4 u_colour;
// w: 1 when s_alpha holds the texture's alpha
uniform vec4 u_celestial;

void main()
{
	vec4 texel = texture2D(s_diffuse, v_texcoord0.xy);
	if (u_celestial.w > 0.0f)
	{
		texel.a = texture2D(s_alpha, v_texcoord0.xy).r;
	}
	gl_FragColor = texel * u_colour;
}
