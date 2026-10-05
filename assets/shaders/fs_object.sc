$input v_position, v_texcoord0, v_normal

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_handLight, 2);
uniform vec4 u_skyAlphaThreshold;
// The temple's controls glow under the cursor: LH3D adds the colour as the vertices' specular, after the texture stages
uniform vec4 u_glow;
// rgb: a colour the object is drawn in, as LH3DObject::SetColour gives it. w: 0 to light it as usual, otherwise to draw
// it unlit, in that colour alone, with its alpha by w
uniform vec4 u_tint;

#include "hand_light.sh"

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
	// The hand's light, where it is brighter
	vec3 light = max(ambient + diffuse, vec3_splat(HandLightAt(v_position.xyz)));
	bool tinted = u_tint.w > 0.0f;
	diffuseTex.rgb = diffuseTex.rgb * (tinted ? vec3_splat(1.0f) : light) * u_tint.rgb;
	diffuseTex.a = diffuseTex.a * (tinted ? u_tint.w : 1.0f);
	if (diffuseTex.a <= alphaThreshold)
	{
		discard;
	}
	gl_FragColor = vec4(min(diffuseTex.rgb + u_glow.rgb, vec3_splat(1.0f)), diffuseTex.a);
}
