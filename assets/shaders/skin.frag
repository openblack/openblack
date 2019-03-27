#version 330

uniform sampler2D tex;

in vec2 v_tex;
in vec3 v_norm;

out vec4 color;

void main()
{
	//vec3 col = 0.5 + 0.5 * v_norm;
    //color = vec4(col, 1.0);

	color = texture2D(tex, v_tex);
	color.a = 1.0f;

	//vec3 col = 0.5 + 0.5 * v_norm;
	//color = vec4(col, 1.0);
}
