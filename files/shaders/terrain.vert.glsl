#version 330 core
in vec3 position;
in vec3 color;
in vec2 texcoord;
in float country;
in float altitude;
out vec3 Color;
out vec2 Texcoord;
flat out float Country;
flat out float Altitude;

uniform mat4 viewProj;

void main()
{
    Color = color;
	Texcoord = texcoord;
	Country = country;
	Altitude = altitude;
    //gl_Position = viewProj * vec4(position.x, altitude * 0.67, position.z, 1.0);
	//Texcoord = vec2(1/position.x, 1/position.z);
    gl_Position = viewProj * vec4(position, 1.0);
}