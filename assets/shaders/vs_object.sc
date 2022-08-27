#ifdef USE_INSTANCING
$input a_position, a_texcoord0, a_normal, a_indices, i_data0, i_data1, i_data2, i_data3, i_data4
#else
$input a_position, a_texcoord0, a_normal, a_indices
#endif // USE_INSTANCING
$output v_position, v_texcoord0, v_normal

#if BGFX_SHADER_LANGUAGE_HLSL == 3
#define BGFX_CONFIG_MAX_BONES 48
#else
#define BGFX_CONFIG_MAX_BONES 128
#endif

#include <bgfx_shader.sh>

#ifdef USE_HEIGHT_MAP
SAMPLER2D(s_heightmap, 1);
uniform vec4 u_islandExtent;
#endif // USE_HEIGHT_MAP

void main()
{
	// Unpack
#ifdef USE_HEIGHT_MAP
	vec2 extentMin = u_islandExtent.xy;
	vec2 extentMax = u_islandExtent.zw;
#endif // USE_HEIGHT_MAP

#if BGFX_SHADER_LANGUAGE_HLSL > 300 || BGFX_SHADER_LANGUAGE_PSSL || BGFX_SHADER_LANGUAGE_SPIRV
	uint modelIndex = uint(max(0, asint(a_indices.x)));
#else
	uint modelIndex = uint(max(0, a_indices.x));
#endif

	v_position = mul(u_model[modelIndex], vec4(a_position.xyz, 1.0f));

#ifdef USE_INSTANCING
	mat4 model;
	model[0] = i_data0;
	model[1] = i_data1;
	model[2] = i_data2;
	model[3] = i_data3;

	v_position = instMul(model, v_position);
#endif // USE_INSTANCING

#ifdef USE_HEIGHT_MAP
#ifdef USE_INSTANCING
	float original_height = i_data3.y;
#else
    float original_height = u_model[modelIndex][3].y;
#endif // USE_INSTANCING
	vec2 blockUv = (v_position.xz - extentMin) / (extentMax - extentMin);
	float terrain_height = texture2DLod(s_heightmap, blockUv, 0.0f).r * 170.85f;
	v_position.y += terrain_height - original_height;
#endif // USE_HEIGHT_MAP

	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	v_normal = a_normal;
	gl_Position = mul(u_viewProj, v_position);
}
