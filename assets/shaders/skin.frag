#version 330

uniform sampler2D tex;

in vec3 v_pos;
in vec3 v_norm;
in vec2 v_tex;

out vec4 color;

float alphaThreshold = 1.0 / 255.0;

void main()
{
	vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
	vec3 lightPos = vec3(-4000, 1300, -1435);

	// ambient
    float ambientStrength = 0.5;
    vec3 ambient = ambientStrength * lightColor;

    // diffuse
	vec3 norm = normalize(v_norm);
	vec3 lightDir = normalize(lightPos - v_pos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor * 0.5;

	vec4 diffuseTex = texture2D(tex, v_tex);
	diffuseTex.rgb = diffuseTex.rgb * (ambient + diffuse);
	if(diffuseTex.a <= alphaThreshold) discard;
	color = diffuseTex;
}
