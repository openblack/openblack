#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_weights;
layout(location = 2) in uvec3 a_firstMaterialID;
layout(location = 3) in uvec3 a_secondMaterialID;
layout(location = 4) in vec3 a_materialBlendCoefficient;
layout(location = 5) in float a_lightLevel;
layout(location = 6) in float a_waterAlpha;

out vec2 v_texcoord0;
out vec3 v_weights;
flat out uvec3 v_FirstMaterialID;
flat out uvec3 v_SecondMaterialID;
flat out vec3 v_MaterialBlendCoefficient;
out float v_LightLevel;
out float v_WaterAlpha;

uniform mat4 u_viewProj;
uniform vec2 u_blockPosition;

void main()
{
	v_texcoord0 = vec2(a_position.z / 160.0f, a_position.x / 160.0f);
	v_weights = a_weights;
	v_FirstMaterialID = a_firstMaterialID;
	v_SecondMaterialID = a_secondMaterialID;
	v_MaterialBlendCoefficient = a_materialBlendCoefficient;
	v_LightLevel = a_lightLevel;
	v_WaterAlpha = a_waterAlpha;

	vec3 transformedPosition = vec3(a_position.x + u_blockPosition.x, a_position.y, a_position.z + u_blockPosition.y);
	gl_Position = u_viewProj * vec4(transformedPosition, 1.0);
}
