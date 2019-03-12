#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;

out vec3 v_norm;
out vec2 TexCoord;

uniform mat4 viewProj;

void main()
{
	v_norm = normal;
	TexCoord = tex;

	gl_Position = viewProj * vec4(position, 1.0);
}