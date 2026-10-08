$input v_texcoord0

#include <bgfx_shader.sh>

// The scrolling pattern's alpha, in its red
SAMPLER2D(s_diffuse, 0);
// y: the alpha the pattern must pass, of 1
uniform vec4 u_skyAlphaThreshold;

// A destroyed building's ghost lays its depth only where the pattern passes the threshold, drawing nothing of its colour
void main()
{
	if (texture2D(s_diffuse, v_texcoord0.xy).r <= u_skyAlphaThreshold.y)
	{
		discard;
	}
	gl_FragColor = vec4_splat(0.0f);
}
