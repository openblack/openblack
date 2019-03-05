/* OpenBlack - A reimplementation of Lionheads's Black & White engine
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

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
layout(location = 0) in vec4 color;
layout(location = 1) in vec3 position;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec3 weights;
layout(location = 4) in uvec3 firstMaterialID;
layout(location = 5) in uvec3 secondMaterialID;
layout(location = 6) in vec3 materialBlendCoefficient;
layout(location = 7) in float waterAlpha;

out vec4 Color;
out vec2 Texcoord;
out vec3 Weights;
flat out uvec3 FirstMaterialID;
flat out uvec3 SecondMaterialID;
flat out vec3 MaterialBlendCoefficient;
out float WaterAlpha;

uniform mat4 viewProj;

void main()
{
	Color = color;
	Texcoord = texcoord;
	Weights = weights;
	FirstMaterialID = firstMaterialID;
	SecondMaterialID = secondMaterialID;
	MaterialBlendCoefficient = materialBlendCoefficient;
	WaterAlpha = waterAlpha;

    gl_Position = viewProj * vec4(position, 1.0);
}
)";

const char* Terrain::FragmentShader = R"(
#version 330 core

in vec4 Color;
in vec2 Texcoord;
in vec3 Weights;
flat in uvec3 FirstMaterialID;
flat in uvec3 SecondMaterialID;
flat in vec3 MaterialBlendCoefficient;
in float WaterAlpha;

uniform sampler2DArray sMaterials;
uniform sampler2D sBumpMap;
uniform float timeOfDay;
uniform float bumpmapStrength;

out vec4 FragColor;

void main()
{
	// do each vert with both materials
	vec4 colOne = mix(
		texture(sMaterials, vec3(Texcoord, FirstMaterialID.r)),
		texture(sMaterials, vec3(Texcoord, SecondMaterialID.r)),
		MaterialBlendCoefficient.r
	) * Weights.r;
	vec4 colTwo = mix(
		texture(sMaterials, vec3(Texcoord, FirstMaterialID.g)),
		texture(sMaterials, vec3(Texcoord, SecondMaterialID.g)),
		MaterialBlendCoefficient.g
	) * Weights.g;
	vec4 colThree = mix(
		texture(sMaterials, vec3(Texcoord, FirstMaterialID.b)),
		texture(sMaterials, vec3(Texcoord, SecondMaterialID.b)),
		MaterialBlendCoefficient.b
	) * Weights.b;

	// add the 3 blended textures together
	vec4 col = colOne + colTwo + colThree;

	// apply bump map (2x because it's half bright?)
	float bump = mix(1.0f, texture(sBumpMap, Texcoord).r * 2, bumpmapStrength);
	col = col * bump;

	// apply light map
	col = col * mix(.25f, Color.a, timeOfDay);

	FragColor = vec4(col.r, col.g, col.b, WaterAlpha);
}
)";
