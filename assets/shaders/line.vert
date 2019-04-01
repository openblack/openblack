#version 330 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_col;

uniform mat4 u_viewProjection;

out vec4 v_color;

void main()
{
    gl_Position = u_viewProjection * vec4(in_pos, 1.0);
    v_color = vec4(in_col, 1.0);
}
