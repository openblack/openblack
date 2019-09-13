#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;

out vec3 v_norm;

uniform mat4 u_viewProj;
uniform mat4 u_model;

void main()
{
	v_norm = normal;
	gl_Position = u_viewProj * u_model * vec4(position, 1.0);
}
