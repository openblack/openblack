#ifndef LAND_ALTITUDE_SH
#define LAND_ALTITUDE_SH

// The land's height under a point, worked out as the game does: across the triangle of its cell that the point is
// in, by the cell's split, with a cell's corners of 3 or less at sea level when the cell starts at 4 or less. The
// height map holds each cell corner's altitude in red and, in green, whether its cell is split the other way. Needs
// s_heightmap and u_islandExtent.

// A cell corner's altitude and its cell's split, by its place in the height map
vec2 LandCorner(vec2 corner, vec2 texels)
{
	return floor(texture2DLod(s_heightmap, (corner + 0.5f) / texels, 0.0f).rg * 255.0f + 0.5f);
}

float LandAltitude(vec2 world)
{
	vec2 extentMin = u_islandExtent.xy;
	vec2 texels = (u_islandExtent.zw - extentMin) / 10.0f + 1.0f;
	// The point in cells of 65536 steps, as the game rounds it
	vec2 steps = floor(world * 6553.6f);
	vec2 cellOnMap = floor(steps / 65536.0f);
	vec2 frac = steps - cellOnMap * 65536.0f;
	vec2 cell = cellOnMap - extentMin / 10.0f;
	if (cell.x < 0.0f || cell.y < 0.0f || cell.x >= texels.x - 1.0f || cell.y >= texels.y - 1.0f)
	{
		return 0.0f;
	}
	vec2 corner00 = LandCorner(cell, texels);
	float a00 = corner00.x;
	float a01 = LandCorner(cell + vec2(0.0f, 1.0f), texels).x;
	float a10 = LandCorner(cell + vec2(1.0f, 0.0f), texels).x;
	float a11 = LandCorner(cell + vec2(1.0f, 1.0f), texels).x;
	// Next to the sea the low corners are at sea level
	if (a00 <= 4.0f)
	{
		a00 = a00 <= 3.0f ? 0.0f : a00;
		a01 = a01 <= 3.0f ? 0.0f : a01;
		a10 = a10 <= 3.0f ? 0.0f : a10;
		a11 = a11 <= 3.0f ? 0.0f : a11;
	}
	// The fourth corner completes the plane of the triangle the point is in
	float v00 = a00;
	float v01 = a01;
	float v10 = a10;
	float v11 = a11;
	if (corner00.y > 0.5f)
	{
		if (frac.y > 65535.0f - frac.x)
		{
			v00 = a10 - a11 + a01;
		}
		else
		{
			v11 = a10 - a00 + a01;
		}
	}
	else if (frac.x > frac.y)
	{
		v01 = a00 - a10 + a11;
	}
	else
	{
		v10 = a00 - a01 + a11;
	}
	// In whole 256ths, as the game's integer sums
	float z8 = floor(frac.y / 256.0f);
	float x8 = floor(frac.x / 256.0f);
	float edge0 = (v01 - v00) * z8 + v00 * 256.0f;
	float edge1 = (v11 - v10) * z8 + v10 * 256.0f;
	float height = floor((edge1 - edge0) * x8 / 256.0f) + edge0;
	return height * 0.67f / 256.0f;
}

#endif // LAND_ALTITUDE_SH
