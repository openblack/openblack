#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texcoord0;
layout(location = 2) in vec3 a_normal;

out vec3 v_normal;

uniform mat4 u_viewProj;
uniform mat4 u_model;

void main()
{
	v_normal = normal;
	gl_Position = u_viewProj * u_model * vec4(a_position, 1.0);
}
