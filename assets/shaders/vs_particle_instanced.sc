$input a_position, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_texcoord0, v_color0

#include <bgfx_shader.sh>

// One particle sprite per instance, turned into a quad here from the plane's corners (-1..1 across and up):
// i_data0: where it is, and half its width
// i_data1: half its height, its roll (or yaw when flat), and how far its corners move back by its origin, across and up
// i_data2: its sheet cell, the top left corner and the size in texture space
// i_data3: its colour and alpha
// i_data4: x is 1 when it lies flat on the ground, 0 when it faces the screen

void main()
{
	vec2 corner = a_position.xy;
	float c = cos(i_data1.y);
	float s = sin(i_data1.y);
	float x = corner.x * i_data0.w - i_data1.z;
	vec3 world;
	if (i_data4.x > 0.5)
	{
		// On the ground, its top edge away along its yaw
		float z = -corner.y * i_data1.x - i_data1.w;
		world = i_data0.xyz + vec3(c, 0.0, s) * x + vec3(-s, 0.0, c) * z;
	}
	else
	{
		// In the plane of the screen, rolled
		float y = corner.y * i_data1.x - i_data1.w;
		vec3 right = mul(u_invView, vec4(1.0, 0.0, 0.0, 0.0)).xyz;
		vec3 up = mul(u_invView, vec4(0.0, 1.0, 0.0, 0.0)).xyz;
		world = i_data0.xyz + right * (x * c + y * s) + up * (y * c - x * s);
	}
	v_texcoord0 = vec4(i_data2.xy + vec2(corner.x * 0.5 + 0.5, 0.5 - corner.y * 0.5) * i_data2.zw, 0.0, 0.0);
	v_color0 = i_data3;
	gl_Position = mul(u_viewProj, vec4(world, 1.0));
}
