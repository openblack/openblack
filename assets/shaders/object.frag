#version 150 core

in vec3 v_normal;

void main()
{
	vec3 col = 0.5 + 0.5 * v_normal;
	gl_FragColor = vec4(col, 1.0);
}
