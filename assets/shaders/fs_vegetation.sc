$input v_position, v_texcoord0, v_normal, v_color0, v_haze

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
uniform vec4 u_skyAlphaThreshold;

void main()
{
	float alphaThreshold = u_skyAlphaThreshold.y;

	vec3 light = v_color0.rgb;

	vec4 diffuseTex = texture2D(s_diffuse, v_texcoord0.xy);
	diffuseTex.rgb = diffuseTex.rgb * light;
	if (diffuseTex.a <= alphaThreshold)
	{
		discard;
	}
	// The distance haze is added after the texture
	gl_FragColor = vec4(min(diffuseTex.rgb + v_haze.rgb, vec3_splat(1.0f)), diffuseTex.a);
}
