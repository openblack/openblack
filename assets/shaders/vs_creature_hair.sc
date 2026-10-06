$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_color0

#include <bgfx_shader.sh>

#include "haze.sh"

uniform vec4 u_islandExtent;
#include "land_light.sh"

// xyz: where the creature stands, which its hair takes the land's light and haze at
uniform vec4 u_hairOrigin;

// A creature's strands of hair, ribbons in the world. A strand's colour, 0 to 255, is its own scaled by the land's
// light under the creature, faded by the haze, then the land's colour and the haze's are added, at most white. The sun
// doesn't shade it. The texture then multiplies it.
void main()
{
	vec3 origin = u_hairOrigin.xyz;
	vec3 light = vec3_splat(255.0f);
	if (u_landLight.x > 0.0f)
	{
		light = min(floor(LandLightAt(origin.xz) * u_landLight.y), vec3_splat(255.0f));
	}
	float hazeT = HazeT(mul(u_view, vec4(origin, 1.0f)).z);
	light = HazeDiffuse(light, HazeFactor(hazeT));
	vec3 added = HazeColour(hazeT);
	if (u_landLight.x > 0.0f)
	{
		added = min(added + LandColourAt(origin.xz), vec3_splat(255.0f));
	}
	vec3 colour = floor(a_color0.rgb * 255.0f + 0.5f);
	v_color0 = vec4(min(floor(colour * light / 255.0f) + added, vec3_splat(255.0f)) / 255.0f, a_color0.a);
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	gl_Position = mul(u_viewProj, vec4(a_position.xyz, 1.0f));
}
