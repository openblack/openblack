#version 150 core

in vec3 v_norm;
out vec4 outColor;

void main()
{
	vec3 col = 0.5 + 0.5 * v_norm;
    outColor = vec4(col, 1.0);
}
