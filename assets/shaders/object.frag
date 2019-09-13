#version 150 core

in vec3 v_normal;

out vec4 o_color;

void main()
{
	vec3 col = 0.5 + 0.5 * v_normal;
	o_color = vec4(col, 1.0);
}
