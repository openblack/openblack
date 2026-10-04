$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_alpha, 1);
uniform vec4 u_tint;
// x: 1 when the sprite faces the camera (vs)
// y: 1 when s_diffuse is the colour and s_alpha its alpha, 0 when s_diffuse is a single channel of alpha
uniform vec4 u_spriteParams;

void main()
{
	vec4 texel = texture2D(s_diffuse, v_texcoord0.xy);
	if (u_spriteParams.y > 0.5f)
	{
		texel = vec4(texel.rgb, texture2D(s_alpha, v_texcoord0.xy).r);
	}
	else
	{
		texel = texel.rrrr;
	}
	gl_FragColor = texel * u_tint;
}
