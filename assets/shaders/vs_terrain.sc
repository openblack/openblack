$input a_position
$output v_texcoord0, v_texcoord1, v_lightColour, v_smallBumpFade, v_shadowCoord, v_haze

#include <bgfx_shader.sh>

#include "haze.sh"

uniform vec4 u_blockPositionAndSize;
uniform vec4 u_islandExtent;
// The land's luminosity this frame, a texel for each cell's corner
SAMPLER2D(s9_landLuminosity, 9);
// The colour of each cell corner this frame, which is added to the land's light, red and blue as the game reads them
SAMPLER2D(s10_landColour, 10);
// The land's light: a colour for each level of the cells' luminosity (LandLightTable)
SAMPLER2D(s7_landLight, 7);
// y: how bright the land's light is, a half for the land mirrored under the sea
uniform vec4 u_skyAndBump;
// World position to hand shadow texture coordinates in xy and distance past the hand along the light in z
uniform mat4 u_handShadowMatrix;
// xy: the camera's x and z, zw: the direction it faces along the ground
uniform vec4 u_smallBumpLine;
// x: how far ahead of the camera along the ground the small bump detail fades out
uniform vec4 u_smallBump;

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
	vec3 transformedPosition = vec3(a_position.x + blockPosition.x, a_position.y, a_position.z + blockPosition.y);
	// zw: where the vertex is along x and z, for the snow lying there
	v_texcoord1 = vec4(blockStartUv, transformedPosition.xz);

	// The luminosity of the vertex's cell corner this frame
	vec2 luminosityTexels = (extentMax - extentMin) / 10.0f + 1.0f;
	vec2 luminosityCell = floor((transformedPosition.xz - extentMin) / 10.0f + 0.5f);
	float luminosity =
	    floor(texture2DLod(s9_landLuminosity, (luminosityCell + 0.5f) / luminosityTexels, 0.0f).r * 255.0f + 0.5f);
	// Each vertex takes the colour of its luminosity, each channel scaled down and rounded down, and the colours are
	// blended across the land between them
	vec3 light = texture2DLod(s7_landLight, vec2((luminosity + 0.5f) / 256.0f, 0.5f), 0.0f).rgb;
	v_lightColour = floor(light * 255.0f * u_skyAndBump.y + 0.001f) / 255.0f;

	v_shadowCoord = mul(u_handShadowMatrix, vec4(transformedPosition, 1.0f));
	// The game gives the land's shadow vertices no alpha below altitude 2 (1.34 units), so shadows fade out towards the
	// water's edge
	v_shadowCoord.w = a_position.y > 1.0f ? 1.0f : 0.0f;

	vec4 cs_position = mul(u_view, vec4(transformedPosition, 1.0f));
	// The distance haze, per vertex: rgb the haze's colour added, a what the land's light is scaled by
	float hazeT = HazeT(cs_position.z);
	// The haze is added with the colour of the vertex's cell corner, each channel at most white
	vec3 cellColour =
	    floor(texture2DLod(s10_landColour, (luminosityCell + 0.5f) / luminosityTexels, 0.0f).rgb * 255.0f + 0.5f);
	v_haze = vec4(min(HazeColour(hazeT) + cellColour, vec3_splat(255.0f)) / 255.0f, HazeFactor(hazeT) / 256.0f);

	// The small bump detail is drawn over the land near the camera. It is full up to 20 units before a line across the
	// ground ahead of the camera, and gone 20 units past it. It is never drawn at the water's edge, below altitude 2,
	// and fades out towards it from the vertices around. x: the detail's colour is kept, or black; y: its alpha.
	float lineOffset = u_smallBump.x - dot(transformedPosition.xz - u_smallBumpLine.xy, u_smallBumpLine.zw);
	float aboveWater = a_position.y > 1.0f ? 1.0f : 0.0f;
	if (lineOffset >= 20.0f)
	{
		v_smallBumpFade = vec2(1.0f, aboveWater);
	}
	else if (lineOffset > -20.0f && aboveWater > 0.0f)
	{
		v_smallBumpFade = vec2(1.0f, floor(255.0f - (20.0f - lineOffset) * (255.0f / 40.0f) + 0.5f) / 255.0f);
	}
	else
	{
		v_smallBumpFade = vec2(0.0f, 0.0f);
	}
	// Land at sea level is drawn flat at height 0, in the plane of the ocean. Land is never under the sea, so win the
	// tie: pull those vertices a fraction of their distance towards the camera along their line of sight, which keeps
	// them where they are on screen and only brings their depth forward.
	if (a_position.y <= 0.0f)
	{
		cs_position.xyz *= 0.999f;
	}
	gl_Position = mul(u_proj, cs_position);
}
