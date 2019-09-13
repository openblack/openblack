#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_weights;
layout(location = 2) in uvec3 a_firstMaterialID;
layout(location = 3) in uvec3 a_secondMaterialID;
layout(location = 4) in vec3 a_materialBlendCoefficient;
layout(location = 5) in float a_lightLevel;
layout(location = 6) in float a_waterAlpha;

out vec2 UV;
out vec3 Weights;
flat out uvec3 FirstMaterialID;
flat out uvec3 SecondMaterialID;
flat out vec3 MaterialBlendCoefficient;
out float LightLevel;
out float WaterAlpha;

uniform mat4 u_viewProj;
uniform vec2 u_blockPosition;

void main()
{
	UV = vec2(a_position.z / 160.0f, a_position.x / 160.0f);
	Weights = a_weights;
	FirstMaterialID = a_firstMaterialID;
	SecondMaterialID = a_secondMaterialID;
	MaterialBlendCoefficient = a_materialBlendCoefficient;
	LightLevel = a_lightLevel;
	WaterAlpha = a_waterAlpha;

	vec3 transformedPosition = vec3(a_position.x + u_blockPosition.x, a_position.y, a_position.z + u_blockPosition.y);
	gl_Position = u_viewProj * vec4(transformedPosition, 1.0);
}
