$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_reflection, 0);

void main()
{

	vec3 reflect_colour = texture2DProj(s_reflection, v_texcoord0).rgb;
	vec3 water_color = vec3(0.35, 0.612, 0.643);

	gl_FragColor = vec4(mix(reflect_colour, water_color, 0.8), 1.0);
}
