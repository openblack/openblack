$input a_position, a_texcoord0
$output v_texcoord0

#include <bgfx_shader.sh>

// x, y: the cosine and sine of the moon's phase, z: 1 to take the moon's texture coordinates from the phase,
// w: 1 when the texture has an alpha of its own (fs)
uniform vec4 u_celestial;

// The sun and the moon: textured meshes placed far out in the sky
void main()
{
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	if (u_celestial.z > 0.0f)
	{
		// The moon's face is turned across its texture by the phase, so the lit part follows the real moon
		v_texcoord0.xy = vec2(a_position.x * u_celestial.x - a_position.z * u_celestial.y, a_position.y) * 0.0025f + 0.25f;
	}
	gl_Position = mul(u_viewProj, mul(u_model[0], vec4(a_position.xyz, 1.0f)));
}
