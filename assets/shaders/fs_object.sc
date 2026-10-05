$input v_position, v_texcoord0, v_normal, v_color0, v_haze

#include <bgfx_shader.sh>

#include "snow_object.sh"

SAMPLER2D(s_diffuse, 0);
#ifdef USE_ENVIRONMENT
SAMPLER2D(s_environment, 5);
#endif // USE_ENVIRONMENT
uniform vec4 u_skyAlphaThreshold;
// The temple's controls glow under the cursor: the game adds the colour as the vertices' specular, after the texture
// stages
uniform vec4 u_glow;
// rgb: a colour the object is drawn in, as the game gives it. w: 0 to light it as usual, otherwise to draw
// it unlit, in that colour alone, with its alpha by w
uniform vec4 u_tint;
// x: 1 in the sea's reflection, which shows only what stands above the sea
uniform vec4 u_seaClip;

void main()
{
	float alphaThreshold = u_skyAlphaThreshold.y;

	vec4 diffuseTex = texture2D(s_diffuse, v_texcoord0.xy);
	vec3 light = v_color0.rgb;
	bool tinted = u_tint.w > 0.0f;
	diffuseTex.rgb = diffuseTex.rgb * (tinted ? vec3_splat(1.0f) : light) * u_tint.rgb;
	diffuseTex.a = diffuseTex.a * (tinted ? u_tint.w : 1.0f);
	if (diffuseTex.a <= alphaThreshold || (u_seaClip.x > 0.5f && v_position.y < 0.0f))
	{
		discard;
	}
	// Snow covers the object where it shows, in the same light, over its own texture
	float snowLevel = floor(v_haze.w * 255.0f + 0.5f);
	if (snowLevel > 0.0f && SnowShows(snowLevel, texture2D(s_snowAlpha, v_texcoord0.xy).r))
	{
		diffuseTex.rgb = texture2D(s_snow, v_texcoord0.xy).rgb * (tinted ? vec3_splat(1.0f) : light) * u_tint.rgb;
	}
#ifdef USE_ENVIRONMENT
	// The second texture stage adds the environment map, which saturates
	diffuseTex.rgb = min(diffuseTex.rgb + texture2D(s_environment, v_texcoord0.zw).rgb, vec3_splat(1.0f));
#endif // USE_ENVIRONMENT
	gl_FragColor = vec4(min(diffuseTex.rgb + u_glow.rgb + v_haze.rgb, vec3_splat(1.0f)), diffuseTex.a);
}
