#version 150 core

in vec3 v_normal;
in vec2 v_texcoord0;

uniform sampler2D s_diffuse;

void main()
{
	vec3 col = 0.5 + 0.5 * v_normal;
	vec4 outColor = vec4(col, 1.0);

	outColor = texture(s_diffuse, v_texcoord0);
	outColor.a = 1.0f;
}
