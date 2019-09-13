#version 330 core

layout(location = 0) in vec2 vertPos;

uniform mat4 u_viewProj;

varying vec4 v_texcoord0;

vec4 ScreenSpacePosition(vec4 position)
{
	vec4 v = (position * 0.5);
    v.xy = (vec2(v.x, v.y) + v.w);
    v.zw = position.zw;

    return v;
}

void main()
{
	vec4 vertex = vec4(vec3(vertPos.x * 10000, 0.0, vertPos.y * 10000), 1.0);
	vec4 position = u_viewProj * vertex;
	gl_Position = position;

	v_texcoord0 = ScreenSpacePosition(position);
}
