$input a_position
$output v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_sampleRect;

void main()
{
	// Plane position to UV
	v_texcoord0.xy = vec2(a_position.x * 0.5f + 0.5f, 0.5f - a_position.y * 0.5f);
	// Zoom on section of sprite to render
	v_texcoord0.xy = v_texcoord0.xy * u_sampleRect.xy + u_sampleRect.zw;

	vec3 translation = mul(u_model[0], vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
	vec4 position = a_position;
	// Apply scaling
	position.xyz = mul(u_model[0], vec4(position.xyz, 0.0)).xyz;
	// Undo camera rotation so sprite faces camera
	position.xyz = mul(u_invView, vec4(position.xyz, 0.0)).xyz;
	// Apply translation
	position.xyz += translation;
	gl_Position = mul(u_viewProj, position);
}
