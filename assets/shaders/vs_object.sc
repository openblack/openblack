#if defined(USE_INSTANCING) && defined(USE_LIGHTMAP)
$input a_position, a_texcoord0, a_normal, a_indices, a_texcoord3, i_data0, i_data1, i_data2, i_data3, i_data4
#elif defined(USE_INSTANCING)
$input a_position, a_texcoord0, a_normal, a_indices, i_data0, i_data1, i_data2, i_data3, i_data4
#elif defined(USE_LIGHTMAP)
$input a_position, a_texcoord0, a_normal, a_indices, a_texcoord3
#else
$input a_position, a_texcoord0, a_normal, a_indices
#endif
$output v_position, v_texcoord0, v_normal

// Every bone's matrix is uploaded with each draw: meshes without bones declare one
#ifndef BGFX_CONFIG_MAX_BONES
#if BGFX_SHADER_LANGUAGE_HLSL == 3
#define BGFX_CONFIG_MAX_BONES 48
#else
#define BGFX_CONFIG_MAX_BONES 128
#endif
#endif // BGFX_CONFIG_MAX_BONES

#include <bgfx_shader.sh>

// Pushes the mesh back by a fraction of its depth, towards the far plane at 0: the temple's rooms other than the one the
// player is in, which overlap it at the doorways
uniform vec4 u_depthBias;
// Slides the texture across the mesh, as LH3DObject::SetAnimatedUV does the creature's waterfall
uniform vec4 u_uvOffset;

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
	// The height map has a texel for each corner of the land's cells, 10 units apart: sample at the centre of the texel
	// of the vertex's position
	vec2 texels = (extentMax - extentMin) / 10.0f + 1.0f;
	vec2 blockUv = ((v_position.xz - extentMin) / 10.0f + 0.5f) / texels;
	float terrain_height = texture2DLod(s_heightmap, blockUv, 0.0f).r * 170.85f;
	v_position.y += terrain_height - original_height;
#endif // USE_HEIGHT_MAP

#ifdef USE_LIGHTMAP
	v_texcoord0 = vec4(a_texcoord0, a_texcoord3);
#elif defined(USE_ENVIRONMENT)
	// LH3D's render mode 2 (fn_00852C40): the environment map's coordinates are where the normal points across and up
	// the camera's view, from 0 to 0.498
	vec3 viewNormal = normalize(mul(u_view, mul(u_model[modelIndex], vec4(a_normal, 0.0f))).xyz);
	v_texcoord0 = vec4(a_texcoord0, (viewNormal.xy + 1.0f) * 0.498046875f);
#else
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
#endif // USE_LIGHTMAP
	v_texcoord0.xy += u_uvOffset.xy;
	v_normal = a_normal;
	gl_Position = mul(u_viewProj, v_position);
	gl_Position.z *= 1.0f - u_depthBias.x;
}
