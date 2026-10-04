$input a_position, a_texcoord0, a_indices, i_data0, i_data1, i_data2, i_data3
$output v_texcoord0

#if BGFX_SHADER_LANGUAGE_HLSL == 3
#define BGFX_CONFIG_MAX_BONES 48
#else
#define BGFX_CONFIG_MAX_BONES 128
#endif

#include <bgfx_shader.sh>

// The sun, from the caster's origin (ObjectShadows::k_Sun)
uniform vec4 u_shadowSun;

// Projects an object onto the land beneath it from the sun, as LH3D bakes static objects' shadows into the land's
// textures (ObjectShadows::Project): each vertex is cast from the sun onto the flat plane through the object's origin.
void main()
{
#if BGFX_SHADER_LANGUAGE_HLSL > 300 || BGFX_SHADER_LANGUAGE_PSSL || BGFX_SHADER_LANGUAGE_SPIRV
	uint modelIndex = uint(max(0, asint(a_indices.x)));
#else
	uint modelIndex = uint(max(0, a_indices.x));
#endif

	mat4 model;
	model[0] = i_data0;
	model[1] = i_data1;
	model[2] = i_data2;
	model[3] = i_data3;

	vec4 world = instMul(model, mul(u_model[modelIndex], vec4(a_position.xyz, 1.0f)));
	vec3 origin = i_data3.xyz;

	// Below the origin casts straight down
	vec3 relative = world.xyz - origin;
	float height = max(relative.y, 0.0f);
	// An offset from the point, as the sun is too far away to subtract its position precisely
	vec2 projected = relative.xz + (height / (u_shadowSun.y - height)) * (relative.xz - u_shadowSun.xz);

	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
	gl_Position = mul(u_viewProj, vec4(origin.x + projected.x, 0.0f, origin.z + projected.y, 1.0f));
	gl_Position.z = 0.0f;
}
