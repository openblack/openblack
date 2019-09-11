#version 330 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_tex;
layout(location = 2) in vec3 in_norm;

uniform mat4 u_viewProjection;
uniform mat4 u_modelTransform;

out vec3 v_pos;
out vec3 v_norm;
out vec2 v_tex;

void main()
{
	gl_Position = u_viewProjection * u_modelTransform * vec4(in_pos, 1.0);

	v_pos = (u_modelTransform * vec4(in_pos, 1.0)).xyz;
	v_norm = in_norm;
	v_tex = in_tex;
}