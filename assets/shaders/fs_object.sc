$input v_position, v_texcoord0, v_normal, v_color0, v_haze, v_snow, v_snowLight

#include <bgfx_shader.sh>

#include "creature_shadow.sh"
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
// x: 1 to show only what stands above the height y: in the sea's reflection, which shows only what stands above the sea,
// and for a mesh cut by a plane. z: 1 to blend by the instance's alpha rather than dissolve the share of it gone
uniform vec4 u_seaClip;
// w: the whole object's alpha, which its texture's is multiplied by; 0 for an object drawn as its materials say
uniform vec4 u_objectLook;
// x: 1 to show only what stands below the height y: a building drawn as far as it is built
uniform vec4 u_keepBelow;
// The creature spells' looks: the ice a frozen creature is sheened with by how frozen it is (a positive v_haze.w), and
// the static an invisible one dissolves through by how far it has fizzed (a negative v_haze.w), which xy scrolls across
// its skin
SAMPLER2D(s_iceEnvironment, 10);
// The ice's alpha, which weighs how much of it is added (the stage the vertex shaders of other objects read their height
// map from)
SAMPLER2D(s_iceEnvironmentAlpha, 1);
SAMPLER2D(s_staticAlpha, 15);
uniform vec4 u_creatureSpellLook;

void main()
{
	float alphaThreshold = u_skyAlphaThreshold.y;

	vec4 diffuseTex = texture2D(s_diffuse, v_texcoord0.xy);
	vec3 light = v_color0.rgb;
	bool tinted = u_tint.w > 0.0f;
	diffuseTex.rgb = diffuseTex.rgb * (tinted ? vec3_splat(1.0f) : light) * u_tint.rgb;
	diffuseTex.a = diffuseTex.a * (tinted ? u_tint.w : 1.0f) * (u_objectLook.w > 0.0f ? u_objectLook.w : 1.0f);
	if (diffuseTex.a <= alphaThreshold || (u_seaClip.x > 0.5f && v_position.y < u_seaClip.y) ||
	    (u_keepBelow.x > 0.5f && v_position.y > u_keepBelow.y))
	{
		discard;
	}
	// Fading by its alpha, blended over what is behind it
	if (u_seaClip.z > 0.5f)
	{
		diffuseTex.a = diffuseTex.a * v_color0.a;
	}
	// Fading out of sight: the share of it gone, in specks of the world picked at random, isn't drawn
	else if (v_color0.a < 1.0f)
	{
		float pick = fract(sin(dot(floor(v_position.xyz * 4.0f), vec3(12.9898f, 78.233f, 37.719f))) * 43758.5453f);
		if (pick > v_color0.a)
		{
			discard;
		}
	}
	// A creature fizzing out of sight (a negative v_haze.w) is drawn only where the static scrolling over its skin is
	// brighter than how far it has fizzed
	if (v_haze.w < 0.0f)
	{
		float noise = texture2D(s_staticAlpha, v_texcoord0.xy + u_creatureSpellLook.xy).r;
		if (noise <= -v_haze.w)
		{
			discard;
		}
	}
	// Snow covers the object where it shows, in its own light, over its own texture
	float snowLevel = floor(v_snow.z * 255.0f + 0.5f);
	if (snowLevel > 0.0f && SnowShows(snowLevel, texture2D(s_snowAlpha, v_snow.xy).r))
	{
		diffuseTex.rgb = texture2D(s_snow, v_snow.xy).rgb * (tinted ? vec3_splat(1.0f) : v_snowLight) * u_tint.rgb;
	}
	// The creatures' shadows fall on it, under the haze
	if (u_creatureShadowInfo.x > 0.0f)
	{
		diffuseTex.rgb = diffuseTex.rgb * CreatureShadowLight(v_position.xyz);
	}
#ifdef USE_ENVIRONMENT
	// The second texture stage adds the environment map, which saturates
	diffuseTex.rgb = min(diffuseTex.rgb + texture2D(s_environment, v_texcoord0.zw).rgb, vec3_splat(1.0f));
	// Or the environment map alone, in the light, at an alpha of its own
	if (u_glow.w > 0.0f)
	{
		diffuseTex = vec4(texture2D(s_environment, v_texcoord0.zw).rgb * light, u_glow.w);
	}
#endif // USE_ENVIRONMENT
	// Frozen, a sheen of ice is added over it by how frozen it is, looked up by the way each face points across the view,
	// so it shows facet by facet
	if (v_haze.w > 0.0f)
	{
		vec3 face = normalize(cross(dFdx(v_position.xyz), dFdy(v_position.xyz)));
		vec3 eye = u_invView[3].xyz;
		face = dot(face, eye - v_position.xyz) < 0.0f ? -face : face;
		vec3 viewFace = normalize(mul(u_view, vec4(face, 0.0f)).xyz);
		vec2 iceUv = (viewFace.xy + 1.0f) * 0.498046875f;
		// Added weighed by its alpha and the freeze, over the object's colour
		vec3 ice = texture2D(s_iceEnvironment, iceUv).rgb * texture2D(s_iceEnvironmentAlpha, iceUv).r;
		diffuseTex.rgb = min(diffuseTex.rgb + ice * v_haze.w, vec3_splat(1.0f));
	}
	// A glow of its own, 0xRRGGBB, is added as the vertices' specular is, as the heal lights the people it heals
	vec3 glow = u_glow.rgb;
	if (v_snow.w > 0.5f)
	{
		float glowColour = floor(v_snow.w + 0.5f);
		glow += vec3(floor(glowColour / 65536.0f), mod(floor(glowColour / 256.0f), 256.0f), mod(glowColour, 256.0f)) / 255.0f;
	}
	gl_FragColor = vec4(min(diffuseTex.rgb + glow + v_haze.rgb, vec3_splat(1.0f)), diffuseTex.a);
}
