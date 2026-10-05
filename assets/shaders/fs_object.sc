$input v_position, v_texcoord0, v_normal, v_color0, v_haze

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_handLight, 2);
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

#include "hand_light.sh"

void main()
{
	float alphaThreshold = u_skyAlphaThreshold.y;

	vec4 diffuseTex = texture2D(s_diffuse, v_texcoord0.xy);
	// The game's model light, or the hand's light where it is brighter
	vec3 light = max(v_color0.rgb, vec3_splat(HandLightAt(v_position.xyz)));
	bool tinted = u_tint.w > 0.0f;
	diffuseTex.rgb = diffuseTex.rgb * (tinted ? vec3_splat(1.0f) : light) * u_tint.rgb;
	diffuseTex.a = diffuseTex.a * (tinted ? u_tint.w : 1.0f);
	if (diffuseTex.a <= alphaThreshold)
	{
		discard;
	}
#ifdef USE_ENVIRONMENT
	// The second texture stage adds the environment map, which saturates
	diffuseTex.rgb = min(diffuseTex.rgb + texture2D(s_environment, v_texcoord0.zw).rgb, vec3_splat(1.0f));
#endif // USE_ENVIRONMENT
	gl_FragColor = vec4(min(diffuseTex.rgb + u_glow.rgb + v_haze.rgb, vec3_splat(1.0f)), diffuseTex.a);
}
