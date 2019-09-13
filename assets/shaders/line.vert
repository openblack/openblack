#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color0;

uniform mat4 u_viewProj;
uniform mat4 u_model;

out vec4 v_color;

void main()
{
	gl_Position = u_viewProj * u_model * vec4(a_position, 1.0);
	v_color = vec4(a_color0, 1.0);
}
