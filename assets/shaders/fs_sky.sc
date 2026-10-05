$input v_position, v_texcoord0, v_normal, v_color0, v_haze

#include <bgfx_shader.sh>

// The sky's dome for each alignment, blended for the time of day, one above the other from evil to good
SAMPLER2D(s_diffuse, 0);
// x, y: the alignments' domes mixed. z: how much of the second there is, of 255
uniform vec4 u_skyAlignment;

// A point of an alignment's dome, sampled as if its third of the texture stood alone, clamped to its edges
vec3 Dome(vec2 uv, float alignment)
{
	const float rows = 256.0f;
	float v = clamp(uv.y, 0.5f / rows, 1.0f - 0.5f / rows);
	return texture2D(s_diffuse, vec2(uv.x, (alignment + v) / 3.0f)).rgb;
}

// The first alignment's dome with the second drawn over it
void main()
{
	gl_FragColor = vec4(mix(Dome(v_texcoord0.xy, u_skyAlignment.x), Dome(v_texcoord0.xy, u_skyAlignment.y), u_skyAlignment.z / 255.0f), 1.0f);
}
