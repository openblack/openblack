$input a_position, a_color0
$output v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>

vec4 ScreenSpacePosition(vec4 position)
{
	vec4 v = (position * 0.5);
    v.xy = (vec2(v.x, v.y) + v.w);
    v.zw = position.zw;

    return v;
}

void main()
{
	vec4 vertex = vec4(vec3(a_position.x * 10000, 0.0, a_position.y * 10000), 1.0);
	vec4 position = mul(u_viewProj, vertex);
	gl_Position = position;

	v_texcoord0 = vec4(vertex.z / 100.0f, vertex.x / 100.0f, 0.0f, 0.0f);
	v_texcoord1 = ScreenSpacePosition(position);
}
