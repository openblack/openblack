$input a_position, a_color0
$output v_texcoord0, v_texcoord1, v_lightLevel, v_distToCamera, v_shadowCoord, v_haze

#include <bgfx_shader.sh>

#include "haze.sh"

uniform vec4 u_blockPositionAndSize;
uniform vec4 u_islandExtent;
// World position to hand shadow texture coordinates in xy and distance past the hand along the light in z
uniform mat4 u_handShadowMatrix;
// xy: where the first brightness of the hand's light map lies in the world's x and z (HandLight::GetOrigin)
// z: how strongly the hand lights the land
uniform vec4 u_handLight;

void main()
{
	// Unpack
	vec2 blockPosition = u_blockPositionAndSize.xy;
	vec2 blockSize = u_blockPositionAndSize.zw;
	vec2 extentMin = u_islandExtent.xy;
	vec2 extentMax = u_islandExtent.zw;

	v_texcoord0 = vec4(a_position.zx / blockSize.yx, 0.0f, 0.0f);
	vec2 blockStartUv = (blockPosition + a_position.xz - extentMin) / (extentMax - extentMin);
	#if !BGFX_SHADER_LANGUAGE_GLSL
		blockStartUv.y = 1.0f - blockStartUv.y;
	#endif
	v_texcoord1 = vec4(blockStartUv, 0.0f, 0.0f);
	v_lightLevel = a_color0.x;

	vec3 transformedPosition = vec3(a_position.x + blockPosition.x, a_position.y, a_position.z + blockPosition.y);

	// The hand's light map has a brightness for each of 12 by 12 vertices 10 units apart, its rows along x: sample
	// between those around the vertex, at the centres of their texels
	vec2 handLightCell = (transformedPosition.xz - u_handLight.xy) / 10.0f;
	v_texcoord0.zw = (handLightCell.yx + 0.5f) / 12.0f;

	v_shadowCoord = mul(u_handShadowMatrix, vec4(transformedPosition, 1.0f));
	// LH3D gives the land's shadow vertices no alpha below altitude 2 (1.34 units), so shadows fade out towards the
	// water's edge
	v_shadowCoord.w = a_position.y > 1.0f ? 1.0f : 0.0f;

	vec4 cs_position = mul(u_view, vec4(transformedPosition, 1.0f));
	// The distance haze, per vertex: rgb the haze's colour added, a what the land's light is scaled by
	float hazeT = HazeT(cs_position.z);
	v_haze = vec4(HazeColour(hazeT) / 255.0f, HazeFactor(hazeT) / 256.0f);
	v_distToCamera = cs_position.z;
	// Land at sea level is drawn flat at height 0, in the plane of the ocean. Land is never under the sea, so win the
	// tie: pull those vertices a fraction of their distance towards the camera along their line of sight, which keeps
	// them where they are on screen and only brings their depth forward.
	if (a_position.y <= 0.0f)
	{
		cs_position.xyz *= 0.999f;
	}
	gl_Position = mul(u_proj, cs_position);
}
