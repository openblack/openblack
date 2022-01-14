$input v_position, v_texcoord0, v_normal

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
uniform vec4 u_skyAlphaThreshold;

void main()
{
	// constants
	const vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
	const vec4 lightPos = vec4(-4000.0f, 1300.0f, -1435.0f, 1.0f);
	const float ambientStrength = 0.25f;

	// unpack uniforms
	float skyType = u_skyAlphaThreshold.x;
	float alphaThreshold = u_skyAlphaThreshold.y;

	float skyBightness = skyType / 2.0f;

	// ambient
	vec3 ambient = (skyBightness * 0.25f + ambientStrength) * lightColor;

	// diffuse
	vec3 norm = normalize(v_normal);
	vec4 lightDir = normalize(lightPos - v_position);
	float diff = max(dot(v_normal, lightDir.xyz), 0.0);
	vec3 diffuse = skyBightness * diff * lightColor * ( 1.0f - ambientStrength);

	vec4 diffuseTex = texture2D(s_diffuse, v_texcoord0.xy);
	diffuseTex.rgb = diffuseTex.rgb * (ambient + diffuse);
	if (diffuseTex.a <= alphaThreshold)
	{
		discard;
	}
	gl_FragColor = diffuseTex;
}
