$input v_texcoord0, v_color0, v_snowLight

#include <bgfx_shader.sh>

// The sheet's colour and its alpha beside it, times the surface's colour and alpha, with the specular colour added after
// as the game's vertex specular is
SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_alpha, 1);

void main()
{
	vec3 colour = texture2D(s_diffuse, v_texcoord0.xy).rgb * v_color0.rgb + v_snowLight;
	float alpha = texture2D(s_alpha, v_texcoord0.xy).r * v_color0.a;
	gl_FragColor = vec4(min(colour, vec3_splat(1.0)), alpha);
}
