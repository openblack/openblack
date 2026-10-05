#ifndef SNOW_SH
#define SNOW_SH

// The snow lying on the island: how deep it is in each 40 unit cell of a 128 by 128 grid, point sampled
SAMPLER2D(s_snowDepth, 11);
// x: 1 when snow is drawn on what is drawn
uniform vec4 u_snow;

// How deep the snow lies at a grid point; none off the grid
float SnowDepthCell(vec2 cell)
{
	bool onGrid = all(greaterThanEqual(cell, vec2_splat(0.0f))) && all(lessThan(cell, vec2_splat(128.0f)));
	return onGrid ? texture2DLod(s_snowDepth, (cell + 0.5f) / 128.0f, 0.0f).r : 0.0f;
}

// How deep the snow lies at a point in the world, blended between the four grid points around it
float SnowDepthAt(vec2 xz)
{
	vec2 position = xz * 0.025f;
	if (any(lessThan(position, vec2_splat(0.0f))) || any(greaterThanEqual(position, vec2_splat(127.0f))))
	{
		return 0.0f;
	}
	vec2 cell = floor(position);
	vec2 w = position - cell;
	float near = mix(SnowDepthCell(cell), SnowDepthCell(cell + vec2(1.0f, 0.0f)), w.x);
	float far = mix(SnowDepthCell(cell + vec2(0.0f, 1.0f)), SnowDepthCell(cell + 1.0f), w.x);
	return mix(near, far, w.y);
}

// How much snow shows on an object standing at a point, 0 to its cap: none below 20, then as deep as it lies beyond
// that, scaled by its rate of 256; none at all below 5
float SnowObjectLevel(vec2 origin, float rate, float cap)
{
	float beyond = floor(max(SnowDepthAt(origin) - 20.0f, 0.0f));
	float level = min(floor(beyond * rate / 256.0f), cap);
	return level < 5.0f ? 0.0f : level;
}

// The whole number part of a value, towards zero
float SnowWhole(float value)
{
	return sign(value) * floor(abs(value));
}

// Where an object's vertex reads the snow texture: across by how much the surface faces up, down by how much it faces
// along x, each less a little by where the vertex is in the mesh. The snow's alpha rises across the texture, so the
// snow lies thickest on what faces up.
vec2 SnowUv(vec3 meshPosition, vec3 worldNormal)
{
	float a = meshPosition.y * 0.3f + meshPosition.z + 20.0f;
	float b = meshPosition.z * 0.1f + meshPosition.x;
	float u = b - SnowWhole(b) - 2.5f;
	float v = a - SnowWhole(a) - 2.5f;
	return vec2(worldNormal.y + 0.2f * (u - SnowWhole(u)), worldNormal.x + 0.2f * (v - SnowWhole(v)));
}

// The colour the snow is lit in, 0 to 255, from the object's: its red and blue a quarter of the way to its green
vec3 SnowColour(vec3 colour)
{
	return vec3(floor((3.0f * colour.r + colour.g) / 4.0f), colour.g, floor((3.0f * colour.b + colour.g) / 4.0f));
}

#endif // SNOW_SH
