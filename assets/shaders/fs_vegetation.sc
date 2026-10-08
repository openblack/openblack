$input v_position, v_texcoord0, v_normal, v_color0, v_haze, v_snow, v_snowLight

#include <bgfx_shader.sh>

#include "snow_object.sh"

SAMPLER2D(s_diffuse, 0);
uniform vec4 u_skyAlphaThreshold;

void main()
{
	// A burning tree's foliage is cut away at a higher alpha, the hotter it is
	float alphaThreshold = (u_skyAlphaThreshold.y > 0.0f && v_color0.a > 0.0f) ? v_color0.a : u_skyAlphaThreshold.y;

	vec3 light = v_color0.rgb;

	vec4 diffuseTex = texture2D(s_diffuse, v_texcoord0.xy);
	diffuseTex.rgb = diffuseTex.rgb * light;
	if (diffuseTex.a <= alphaThreshold)
	{
		discard;
	}
	// Snow covers the tree where it shows, in its own light, over its own texture
	float snowLevel = floor(v_snow.z * 255.0f + 0.5f);
	if (snowLevel > 0.0f && SnowShows(snowLevel, texture2D(s_snowAlpha, v_snow.xy).r))
	{
		diffuseTex.rgb = texture2D(s_snow, v_snow.xy).rgb * v_snowLight;
	}
	// The distance haze is added after the texture
	gl_FragColor = vec4(min(diffuseTex.rgb + v_haze.rgb, vec3_splat(1.0f)), diffuseTex.a);
}
