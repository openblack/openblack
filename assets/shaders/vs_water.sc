$input a_position, a_color0
$output v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>

vec4 ScreenSpacePosition(vec4 position)
{
    vec4 v = position;
#if !BGFX_SHADER_LANGUAGE_GLSL
    v.y *= -1.0f;
#endif
    v.xy = (v.xy + v.w) * 0.5f;
    return v;
}

void main()
{
	vec4 vertex = vec4(vec3(a_position.x, 0.0, a_position.y), 1.0);
	vec4 viewSpacePos = mul(u_view, vertex);
	vec4 position = mul(u_viewProj, vertex);
	gl_Position = position;

	v_texcoord0 = vec4(vertex.x / 500.0f, vertex.z / 500.0f, viewSpacePos.z, 0.0f);
	v_texcoord1 = ScreenSpacePosition(position);
}
