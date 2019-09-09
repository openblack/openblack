#version 330

uniform sampler2D tex;

in vec2 v_tex;
in vec3 v_norm;

out vec4 color;

float alphaThreshold = 1.0 / 255.0;

void main()
{
	vec4 diffuseTex = texture2D(tex, v_tex);
	if(diffuseTex.a <= alphaThreshold) discard;
	color = diffuseTex;
}
