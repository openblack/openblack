$input a_position, a_texcoord1, a_color1, a_color2, a_texcoord2, a_color0, a_color3
$output v_texcoord0, v_weight, v_materialID0, v_materialID1, v_materialBlend, v_lightLevel, v_waterAlpha, v_distToCamera

#include <bgfx_shader.sh>

#if BGFX_SHADER_LANGUAGE_HLSL > 3
#   define materialIdFix(x) (floatBitsToInt(x))
#else
#   define materialIdFix(x) (ivec3(x))
#endif

uniform vec4 u_blockPosition;

void main()
{
	v_texcoord0 = vec4(a_position.z / 160.0f, a_position.x / 160.0f, 0.0f, 0.0f);
	v_weight = a_texcoord1;
	v_materialID0 = materialIdFix(a_color1);
	v_materialID1 = materialIdFix(a_color2);
	v_materialBlend = a_texcoord2;
	v_lightLevel = a_color0.x;
	v_waterAlpha = a_color3;

	vec3 transformedPosition = vec3(a_position.x + u_blockPosition.x, a_position.y, a_position.z + u_blockPosition.y);

	vec4 cs_position = mul(u_view, vec4(transformedPosition, 1.0f));
	v_distToCamera = cs_position.z;
	gl_Position = mul(u_proj, cs_position);
}
