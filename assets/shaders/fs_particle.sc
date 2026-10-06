$input v_texcoord0, v_color0

#include <bgfx_shader.sh>

// The sheet's colour and its alpha beside it, tinted by the particle's colour and alpha; blended as the particle's
// creator asks, added to what is behind or over it
SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_alpha, 1);

void main()
{
	vec3 colour = texture2D(s_diffuse, v_texcoord0.xy).rgb;
	float alpha = texture2D(s_alpha, v_texcoord0.xy).r;
	gl_FragColor = vec4(colour * v_color0.rgb, alpha * v_color0.a);
}
