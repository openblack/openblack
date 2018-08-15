#version 330 core

precision highp int;
precision highp usampler2D;

in vec3 Color;
in vec2 Texcoord;
flat in float Country;
flat in float Altitude;
uniform sampler2DArray sMaterials;
uniform usampler2D sCountryLookup;
out vec4 FragColor;

void main()
{
	uvec4 lookup = texelFetch(sCountryLookup, ivec2(Country, Altitude), 0);

	vec4 colOne = texture(sMaterials, vec3(Texcoord, lookup.r));
	vec4 colTwo = texture(sMaterials, vec3(Texcoord, lookup.g));
	float coeff = float(lookup.b)/255;

	FragColor = mix(colOne, colTwo, coeff); // lookup.b

	//FragColor = texture(sMaterials, vec3(Texcoord, 6));

    //FragColor = vec4(vec3(Altitude/255), 1);
	//FragColor = vec4(vec3(Country/10), 1);
	//FragColor = vec4(vec3(lookup.r/14), 1);
}