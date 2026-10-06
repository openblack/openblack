$input v_texcoord0

#include <bgfx_shader.sh>

// A skin of a creature's base mesh and the matching skin of its evil or good mesh, both 4 bits a channel
SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_variant, 1);
// x: how much of the evil or good skin shows, 0 to 255
uniform vec4 u_skinBlend;

// A texel of a skin as its 4-bit channels
vec4 Channels(vec4 texel)
{
	return floor(texel * 15.0f + 0.5f);
}

// The creature's skin, a texel at a time: each channel of the base moved towards the variant's by the weight, in whole
// steps rounded down. Half a step is added before dividing, which leaves whole numbers where they are but keeps the
// division from falling just short of one.
void main()
{
	vec4 base = Channels(texture2DLod(s_diffuse, v_texcoord0.xy, 0.0f));
	vec4 other = Channels(texture2DLod(s_variant, v_texcoord0.xy, 0.0f));
	float weight = u_skinBlend.x;
	vec4 blended = floor((base * (255.0f - weight) + other * weight + 0.5f) / 255.0f);
	gl_FragColor = blended / 15.0f;
}
