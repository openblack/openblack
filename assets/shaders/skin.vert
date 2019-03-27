#version 330 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_tex;
layout(location = 2) in vec3 in_norm;
layout(location = 3) in int in_bone;

uniform mat4 u_viewProjection;
uniform mat4 u_modelTransform;

#define MAX_BONES 64
uniform mat4 u_boneMatrices[MAX_BONES];

out vec3 v_norm;
out vec2 v_tex;

void main()
{
	mat4 m = u_boneMatrices[in_bone];

	gl_Position = u_viewProjection * u_modelTransform * m * vec4(in_pos, 1.0);

	v_norm = in_norm;
	v_tex = in_tex;
}


/*

mat4 GetMatrix(int index)
{
    return mat4(texelFetchBuffer(boneBuffer, (index * 4) + 0),
                texelFetchBuffer(boneBuffer, (index * 4) + 1),
                texelFetchBuffer(boneBuffer, (index * 4) + 2),
                texelFetchBuffer(boneBuffer, (index * 4) + 3));
}

void main()
{
    mat4 result = GetMatrix(boneIndices[0]) * boneWeights[0];
    result += GetMatrix(boneIndices[1]) * boneWeights[1];
    result += GetMatrix(boneIndices[2]) * boneWeights[2];
result += GetMatrix(boneIndices[3]) * boneWeights[3];

*/
