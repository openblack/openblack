$input v_texcoord0

#include <bgfx_shader.sh>

// The luminosity as the land was laid
SAMPLER2D(s_texture, 0);
// r: the darkest of the clouds' shadows over each cell, a: the brightest light
SAMPLER2D(s_shade, 1);
// x: how much of a cell's luminosity, of 256, a light must outshine to light it
uniform vec4 u_landLuminosity;

void main()
{
	float laid = floor(texture2D(s_texture, v_texcoord0.xy).r * 255.0f + 0.5f);
	vec4 shade = texture2D(s_shade, v_texcoord0.xy);
	float value = min(laid, floor(shade.r * 255.0f + 0.5f));
	// A light takes the cell to one of the land light's 48 warm levels when it outshines the cell's light: over lit land
	// whenever it is bright enough, over a lit cell only when it is brighter
	float light = floor(shade.a * 255.0f + 0.5f);
	float threshold = floor(value * u_landLuminosity.x / 256.0f);
	if (light > threshold && (value >= 48.0f || value < light))
	{
		value = light;
	}
	gl_FragColor = vec4_splat(value / 255.0f);
}
