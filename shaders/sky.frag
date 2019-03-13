#version 150 core

in vec3 v_norm;
in vec2 TexCoord;

out vec4 outColor;

uniform sampler2D tex;

void main()
{
	vec3 col = 0.5 + 0.5 * v_norm;
    outColor = vec4(col, 1.0);

	outColor = texture(tex, TexCoord);
	outColor.a = 1.0f;
}
