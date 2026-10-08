$input a_position, a_texcoord0, a_texcoord3, a_color0
$output v_texcoord0, v_color0

#include <bgfx_shader.sh>

uniform vec4 u_islandExtent;
#include "land_light.sh"

// xyz: where the piece is, whose land's light it takes
uniform vec4 u_fragmentOrigin;

// A piece of a broken model, its corners already in the world. Its colour is its particle's, times the land's light
// where it is, then shaded by the model light's factor each corner brings in a_texcoord3.x, each step in whole numbers
// of 255 as the game takes them.
void main()
{
	vec3 colour = floor(a_color0.rgb * 255.0f + 0.5f);
	colour = floor(colour * LandLightAt(u_fragmentOrigin.xz) / 256.0f);
	colour = floor(colour * a_texcoord3.x / 256.0f);
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	v_color0 = vec4(colour / 255.0f, a_color0.a);
	gl_Position = mul(u_viewProj, vec4(a_position.xyz, 1.0f));
}
