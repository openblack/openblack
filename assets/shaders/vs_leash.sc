$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_color0

#include <bgfx_shader.sh>

#include "haze.sh"

uniform vec4 u_islandExtent;
#include "land_light.sh"

// A leash's rope, a ribbon in the world. Each corner takes the land's light under it, as the rope's points each do,
// faded by the haze there, then the land's colour and the haze's are added, at most white. The texture then multiplies
// it, its alpha the rope's edges.
void main()
{
	vec3 position = a_position.xyz;
	vec3 light = vec3_splat(255.0f);
	if (u_landLight.x > 0.0f)
	{
		light = min(floor(LandLightAt(position.xz) * u_landLight.y), vec3_splat(255.0f));
	}
	float hazeT = HazeT(mul(u_view, vec4(position, 1.0f)).z);
	light = HazeDiffuse(light, HazeFactor(hazeT));
	vec3 added = HazeColour(hazeT);
	if (u_landLight.x > 0.0f)
	{
		added = min(added + LandColourAt(position.xz), vec3_splat(255.0f));
	}
	vec3 colour = floor(a_color0.rgb * 255.0f + 0.5f);
	v_color0 = vec4(min(floor(colour * light / 255.0f) + added, vec3_splat(255.0f)) / 255.0f, a_color0.a);
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	gl_Position = mul(u_viewProj, vec4(position, 1.0f));
}
