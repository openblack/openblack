$input a_position, a_weight, a_color1, a_color2, a_indices, a_color0, a_color3
$output v_texcoord0, v_weight, v_materialID0, v_materialID1, v_materialBlend, v_lightLevel, v_waterAlpha

#include <bgfx_shader.sh>

uniform vec4 u_blockPosition;

void main()
{
	v_texcoord0 = vec4(a_position.z / 160.0f, a_position.x / 160.0f, 0.0f, 0.0f);
	v_weight = a_weight;
	v_materialID0 = uvec3(a_color1);
	v_materialID1 = uvec3(a_color2);
	v_materialBlend = a_indices;
	v_lightLevel = a_color0.x;
	v_waterAlpha = a_color3;

	vec3 transformedPosition = vec3(a_position.x + u_blockPosition.x, a_position.y, a_position.z + u_blockPosition.y);
	gl_Position = mul(u_viewProj, vec4(transformedPosition, 1.0f));
}
