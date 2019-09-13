#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texcoord0;
layout(location = 2) in vec3 a_normal;

uniform mat4 u_viewProj;
uniform mat4 u_model;

out vec3 v_pos;
out vec3 v_norm;
out vec2 v_tex;

void main()
{
	gl_Position = u_viewProj * u_model * vec4(a_position, 1.0);

	v_pos = (u_model * vec4(a_position, 1.0)).xyz;
	v_norm = a_normal;
	v_tex = a_texcoord0;
}
