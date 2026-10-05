$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);
// x: 0 to copy the land's luminosity as it was laid, 1 to cast a shadow on it; y: the shadow's alpha, of 255
uniform vec4 u_landLuminosity;

void main()
{
	float value = floor(texture2D(s_texture, v_texcoord0.xy).r * 255.0f + 0.5f);
	if (u_landLuminosity.x > 0.5f)
	{
		// A shadow darkens the land by its alpha towards the shadow's own darkness, never below 48; the land keeps the
		// darker of the two
		float shadow = floor(value);
		value = max(48.0f, 255.0f - floor((255.0f - shadow) * u_landLuminosity.y / 255.0f));
	}
	gl_FragColor = vec4_splat(value / 255.0f);
}
