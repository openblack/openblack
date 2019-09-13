#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texcoord0;
layout(location = 2) in vec3 a_normal;

uniform mat4 u_viewProj;
uniform mat4 u_model;

out vec3 v_position;
out vec3 v_normal;
out vec2 v_texcoord0;

void main()
{
	gl_Position = u_viewProj * u_model * vec4(a_position, 1.0);

	v_position = (u_model * vec4(a_position, 1.0)).xyz;
	v_normal = a_normal;
	v_texcoord0 = a_texcoord0;
}
