$input v_texcoord0

#include <bgfx_shader.sh>

// A river channel's footprint, whose alpha the land's alpha falls to wherever it is lower. The game reads each texel
// as it is, unfiltered, in sixteen levels.

SAMPLER2D(s_footprint, 0);
// xy: the footprint texture's size in texels
uniform vec4 u_footprintSize;

void main()
{
	vec2 texel = (floor(v_texcoord0.xy * u_footprintSize.xy) + 0.5f) / u_footprintSize.xy;
	float alpha = floor(texture2D(s_footprint, texel).a * 15.0f + 0.5f) / 15.0f;
	gl_FragColor = vec4_splat(alpha);
}
