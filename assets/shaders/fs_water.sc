$input v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_reflection, 1);

void main()
{
	vec3 diffuse_colour = texture2D(s_diffuse, vec2(v_texcoord0.x, v_texcoord0.y)).rgb;
	vec3 reflect_colour = texture2DProj(s_reflection, v_texcoord1).rgb;
	// vec3 water_color = vec3(0.35, 0.612, 0.643);

	gl_FragColor = vec4(mix(reflect_colour, diffuse_colour, 0.8), 1.0);
}
