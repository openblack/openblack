#version 330

uniform sampler2D tex;

in vec2 v_tex;
in vec3 v_norm;

out vec4 color;

void main()
{
	color = texture2D(tex, v_tex);
}
