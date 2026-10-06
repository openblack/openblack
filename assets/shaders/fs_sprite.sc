$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_alpha, 1);
uniform vec4 u_tint;
// x: 1 when the sprite faces the camera (vs)
// y: 1 when s_diffuse is the colour and s_alpha its alpha, 0 when s_diffuse is a single channel of alpha
// z: 1 when the sprite adds to what is behind it by alpha, 0 when it is blended over it, premultiplied by alpha
uniform vec4 u_spriteParams;

void main()
{
	vec4 texel = texture2D(s_diffuse, v_texcoord0.xy);
	if (u_spriteParams.y > 0.5f)
	{
		float alpha = texture2D(s_alpha, v_texcoord0.xy).r;
		texel = vec4(u_spriteParams.z > 0.5f ? texel.rgb : texel.rgb * alpha, alpha);
	}
	else
	{
		texel = texel.rrrr;
	}
	gl_FragColor = texel * u_tint;
}
