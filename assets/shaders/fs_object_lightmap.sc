$input v_position, v_texcoord0, v_normal, v_color0, v_haze

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_lightmap, 3);
#ifdef USE_REFLECTION
SAMPLER2D(s_reflection, 4);
#endif // USE_REFLECTION
uniform vec4 u_skyAlphaThreshold;
// The temple's controls glow under the cursor: the game adds the colour as the vertices' specular, after the texture
// stages. Those that don't glow have the temple's light's colour added instead.
uniform vec4 u_glow;
// How much darker the temple's light makes each colour, the vertices' diffuse the texture is multiplied by
uniform vec4 u_darkening;

// The game draws the lit parts of the temple unlit, with the lightmap on the second texture stage: the texture times
// the lightmap, doubled, which saturates, and the texture's alpha

void main()
{
	vec4 diffuseTex = texture2D(s_diffuse, v_texcoord0.xy);
	vec3 lightmap = texture2D(s_lightmap, v_texcoord0.zw).rgb;
	vec3 colour = min(diffuseTex.rgb * (vec3_splat(1.0f) - u_darkening.rgb) * lightmap * 2.0f, vec3_splat(1.0f));
	colour = min(colour + u_glow.rgb, vec3_splat(1.0f));
#ifdef USE_REFLECTION
	// The game draws the main room mirrored through its floor, then blends the floor over it by the floor's alpha.
	// The reflection pass is drawn from the mirrored camera with the same projection, so it lines up on the screen.
	vec3 reflection = texture2D(s_reflection, gl_FragCoord.xy * u_viewTexel.xy).rgb;
	gl_FragColor = vec4(mix(reflection, colour, diffuseTex.a), 1.0f);
#else
	float alphaThreshold = u_skyAlphaThreshold.y;
	if (diffuseTex.a <= alphaThreshold)
	{
		discard;
	}
	gl_FragColor = vec4(colour, diffuseTex.a);
#endif // USE_REFLECTION
}
