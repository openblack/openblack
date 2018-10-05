#include "GameShaders.h"

using namespace OpenBlack::Shaders;

const char* WorldObject::VertexShader = R"(
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;

out vec3 v_norm;
out vec2 TexCoord;

uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(position, 1.0);
	v_norm = normal;
	TexCoord = tex;
}
)";

const char* WorldObject::FragmentShader = R"(
#version 150 core

in vec3 v_norm;
in vec2 TexCoord;

out vec4 outColor;

uniform sampler2D tex;

void main()
{
	//vec3 col = 0.5 + 0.5 * v_norm;
    //outColor = vec4(col, 1.0);

	outColor = texture(tex, TexCoord);
}
)";

const char* Terrain::VertexShader = R"(
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in float country;
layout(location = 4) in float altitude;

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
)";

const char* Terrain::FragmentShader = R"(
#version 330 core

precision highp int;
precision highp usampler2D;

in vec3 Color;
in vec2 Texcoord;
flat in float Country;
flat in float Altitude;
//uniform sampler2DArray sMaterials;
//uniform usampler2D sCountryLookup;
out vec4 FragColor;

void main()
{
	//uvec4 lookup = texelFetch(sCountryLookup, ivec2(Country, Altitude), 0);

	//vec4 colOne = texture(sMaterials, vec3(Texcoord, lookup.r));
	//vec4 colTwo = texture(sMaterials, vec3(Texcoord, lookup.g));
	//float coeff = float(lookup.b)/255;

	//FragColor = mix(colOne, colTwo, coeff); // lookup.b

	//FragColor = texture(sMaterials, vec3(Texcoord, 6));

    FragColor = vec4(vec3(Altitude/255), 1);
	//FragColor = vec4(1, 1, 1, 1);
	//FragColor = vec4(vec3(Country/10), 1);
	//FragColor = vec4(vec3(lookup.r/14), 1);
}
)";
