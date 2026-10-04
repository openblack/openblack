$input v_position, v_texcoord0, v_normal

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
uniform vec4 u_skyAlphaThreshold;

void main()
{
	// constants
	const vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
	const vec4 lightPos = vec4(-4000.0f, 1300.0f, -1435.0f, 1.0f);
	const float ambientStrength = 0.75f;
	// Brightness of everything at full night, as fs_object's ambient light is then
	const float nightBrightness = 0.25f;

	// unpack uniforms
	float skyType = u_skyAlphaThreshold.x;
	float alphaThreshold = u_skyAlphaThreshold.y;

	float skyBrightness = skyType / 2.0f;

	// Daylight: the leaves are lit mostly all round, a little more on the sun's side
	vec3 norm = normalize(v_normal);
	vec4 lightDir = normalize(lightPos - v_position);
	float diff = max(dot(v_normal, lightDir.xyz), 0.0);
	vec3 daylight = (0.25f + ambientStrength + diff * (1.0f - ambientStrength)) * lightColor;

	// Trees darken towards night with the rest of the world, down to the light the buildings then have
	vec3 light = mix(vec3_splat(nightBrightness) * lightColor, daylight, skyBrightness);

	vec4 diffuseTex = texture2D(s_diffuse, v_texcoord0.xy);
	diffuseTex.rgb = diffuseTex.rgb * light;
	if (diffuseTex.a <= alphaThreshold)
	{
		discard;
	}
	gl_FragColor = diffuseTex;
}
