#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 weights;
layout(location = 2) in uvec3 firstMaterialID;
layout(location = 3) in uvec3 secondMaterialID;
layout(location = 4) in vec3 materialBlendCoefficient;
layout(location = 5) in float lightLevel;
layout(location = 6) in float waterAlpha;

out vec2 UV;
out vec3 Weights;
flat out uvec3 FirstMaterialID;
flat out uvec3 SecondMaterialID;
flat out vec3 MaterialBlendCoefficient;
out float LightLevel;
out float WaterAlpha;

uniform mat4 viewProj;
uniform vec2 blockPosition;

void main()
{
	UV = vec2(position.z / 160.0f, position.x / 160.0f);
	Weights = weights;
	FirstMaterialID = firstMaterialID;
	SecondMaterialID = secondMaterialID;
	MaterialBlendCoefficient = materialBlendCoefficient;
	LightLevel = lightLevel;
	WaterAlpha = waterAlpha;

	vec3 transformedPosition = vec3(position.x + blockPosition.x, position.y, position.z + blockPosition.y);
    gl_Position = viewProj * vec4(transformedPosition, 1.0);
}