#version 330 core

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_color0;

uniform mat4 u_viewProj;
uniform mat4 u_model;

out vec4 v_color0;

void main()
{
	gl_Position = u_viewProj * u_model * a_position;
	v_color0 = a_color0;
}
