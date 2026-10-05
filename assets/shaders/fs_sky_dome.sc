$input v_texcoord0

#include <bgfx_shader.sh>

// The pictures of the sky, a layer for each alignment and, in each, for each time of day
SAMPLER2DARRAY(s_diffuse, 0);
// x: the alignment's first layer. y, z: the times of day blended. w: how much of the second, of 255
uniform vec4 u_skyDome;

// A texel of a picture as its 5-bit channels
vec3 Channels(vec2 uv, float time)
{
	return floor(texture2DArrayLod(s_diffuse, vec3(uv, u_skyDome.x + time), 0.0f).rgb * 31.0f + 0.5f);
}

// A row of the sky's dome for an alignment, blended for a time of day in whole steps of its channels. The two weights
// only add up to 255 of 256, so even a whole picture comes out a step darker at its brightest.
void main()
{
	vec3 lower = Channels(v_texcoord0.xy, u_skyDome.y);
	vec3 upper = Channels(v_texcoord0.xy, u_skyDome.z);
	vec3 blended = floor(lower * (255.0f - u_skyDome.w) / 256.0f) + floor(upper * u_skyDome.w / 256.0f);
	gl_FragColor = vec4(blended / 31.0f, 1.0f);
}
