$input v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>

SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_alpha, 1);
SAMPLER2D(s_reflection, 2);
SAMPLER2D(s_handLight, 3);

uniform vec4 u_sky;

#include "hand_light.sh"

void main()
{	// unpack uniforms
	float skyType = u_sky.x;

	float skyBightness = skyType / 2.0f;

	// The sea at sea level, its texture repeating every 500 units, lit by the hand's light where that is brighter
	float skyLight = mix(0.25f, 1.0f, skyBightness);
	float light = max(skyLight, HandLightAt(vec3(v_texcoord0.x * 500.0f, 0.0f, v_texcoord0.y * 500.0f)));
	vec3 diffuse_colour = light * texture2D(s_diffuse, v_texcoord0.xy).rgb;
	// The reflection, lit by the sky, brightens under the light as the sea's own colour does
	vec3 reflect_colour = texture2DProj(s_reflection, v_texcoord1).rgb * (light / skyLight);

	// This was called alpha in vanilla because they rendered parts of the scene upside down in the
	// main backbuffer, then alpha blended the ocean on top to simulate a render texture fetch.
	float reflection_alpha = texture2D(s_alpha, v_texcoord0.xy).r;

	// The 3000 and formula is pretty much guessed but the results look very good.
	float horizon_alpha = saturate(1.0f / float(v_texcoord0.z / 3000.0f));

	float alpha = reflection_alpha * horizon_alpha;

	gl_FragColor = vec4(mix(reflect_colour, diffuse_colour, alpha), 1.0f);
}
