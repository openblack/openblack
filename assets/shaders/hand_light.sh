// The light the god hand carries at night (HandLight). Declare SAMPLER2D(s_handLight, ...) before including.

// xy: where the first brightness of the hand's light map lies in the world's x and z (HandLight::GetOrigin)
// z: how strongly the hand lights the world
uniform vec4 u_handLight;

// The brightness the hand's light gives a point of the world, 0 to 1. The map has a brightness for each of 12 by 12
// points 10 units apart, its rows along x, and is dark around its edge.
float HandLightAt(vec3 worldPosition)
{
	vec2 cell = (worldPosition.xz - u_handLight.xy) / 10.0f;
	return texture2D(s_handLight, (cell.yx + 0.5f) / 12.0f).r * u_handLight.z;
}
