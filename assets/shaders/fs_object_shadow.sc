$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
uniform vec4 u_skyAlphaThreshold;

// Writes where an object's shadow covers the land. The game casts the shadow through its textures' masks, so cut-out
// leaves leave gaps in a tree's shadow.

void main()
{
	if (texture2D(s_diffuse, v_texcoord0.xy).a <= u_skyAlphaThreshold.y)
	{
		discard;
	}
	gl_FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
