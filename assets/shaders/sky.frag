#version 150 core

in vec3 v_normal;
in vec2 v_texcoord0;

uniform sampler2D s_diffuse;

out vec4 o_color;

void main()
{
	vec3 col = 0.5 + 0.5 * v_normal;
	o_color = vec4(col, 1.0);

	o_color = texture(s_diffuse, v_texcoord0);
	o_color.a = 1.0f;
}
