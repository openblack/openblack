#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texcoord0;
layout(location = 2) in vec3 a_normal;

out vec3 v_norm;
out vec2 TexCoord;

uniform mat4 u_viewProj;

void main()
{
	v_norm = a_normal;
	TexCoord = a_texcoord0;

	gl_Position = u_viewProj * vec4(a_position, 1.0);
}
