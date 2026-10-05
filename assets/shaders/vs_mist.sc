$input a_position, a_texcoord0, a_normal
$output v_texcoord0, v_color0, v_haze

#include <bgfx_shader.sh>

#include "haze.sh"
#include "model_light.sh"

uniform vec4 u_islandExtent;
#include "land_light.sh"

// xy: where the animation's frame is in the smoke texture
uniform vec4 u_mist;
// The mist's colour and alpha
uniform vec4 u_mistColour;

// A puff of mist: its colour taken by the land's light where it stands when u_landLight.x is set, faded by the distance
// haze, then shaded by the model light as the game shades its models
void main()
{
	v_texcoord0 = vec4(a_texcoord0 + u_mist.xy, 0.0f, 0.0f);
	vec3 origin = mul(u_model[0], vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
	vec3 colour = floor(u_mistColour.rgb * 255.0f + 0.5f);
	v_haze = vec4_splat(0.0f);
	if (u_landLight.x > 0.0f)
	{
		// The colour times the land's light, each channel rounding down
		colour = floor(colour * LandLightAt(origin.xz) / 255.0f);
		float hazeT = HazeT(mul(u_view, vec4(origin, 1.0f)).z);
		colour = HazeDiffuse(colour, HazeFactor(hazeT));
		v_haze = vec4(HazeColour(hazeT) / 255.0f, 0.0f);
	}
	vec3 localLight = ModelLightLocal(mul(u_model[0], vec4(1.0f, 0.0f, 0.0f, 0.0f)).xyz, mul(u_model[0], vec4(0.0f, 1.0f, 0.0f, 0.0f)).xyz,
	                                  mul(u_model[0], vec4(0.0f, 0.0f, 1.0f, 0.0f)).xyz, origin);
	v_color0 = vec4(ModelLightColour(colour, ModelLightFactor(a_normal, localLight)), u_mistColour.a);
	gl_Position = mul(u_viewProj, mul(u_model[0], vec4(a_position.xyz, 1.0f)));
}
