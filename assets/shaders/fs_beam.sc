$input v_color0, v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_alpha, 1);
// x: 1 when the alpha comes from s_alpha, as the atmosphere texture's does, 0 when s_diffuse has its own
uniform vec4 u_beamParams;

// Drawn additively: the texture times the vertex colour, colour and alpha, added to what is behind by alpha
void main()
{
	vec4 texel = texture2D(s_diffuse, v_texcoord0.xy);
	if (u_beamParams.x > 0.5f)
	{
		texel.a = texture2D(s_alpha, v_texcoord0.xy).r;
	}
	gl_FragColor = texel * v_color0;
}
