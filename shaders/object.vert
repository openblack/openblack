#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;

out vec3 v_norm;

uniform mat4 viewProj;
uniform mat4 modelTransform;

void main()
{
	v_norm = normal;
	gl_Position = viewProj * modelTransform * vec4(position, 1.0);
}
