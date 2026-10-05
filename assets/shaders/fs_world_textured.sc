$input v_texcoord0, v_color0

#include <bgfx_shader.sh>

// The atmosphere texture's colours and its alpha
SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_alpha, 1);

// A world triangle textured in its vertices' colour, as opaque as both the texture's alpha and the vertices':
// rain streaks and the influence border
void main()
{
	vec3 colour = texture2D(s_diffuse, v_texcoord0.xy).rgb * v_color0.rgb;
	float alpha = texture2D(s_alpha, v_texcoord0.xy).r * v_color0.a;
	gl_FragColor = vec4(colour, alpha);
}
