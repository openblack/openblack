#ifndef LAND_LIGHT_SH
#define LAND_LIGHT_SH

// The land's light that models take where they stand (LandLightTable). Declare u_islandExtent before including.

// The land's light: each cell corner's luminosity, and the colour of each luminosity (LandLightTable)
SAMPLER2D(s_landLuminosity, 6);
SAMPLER2D(s_landLight, 7);
// x: 1 to colour the object by the land's light where it stands, 0 for white. y: how much brighter than the land's light
// the object is, at most white
uniform vec4 u_landLight;

// The colour of a luminosity, 0 to 255
vec3 LandLightColour(float luminosity)
{
	return floor(texture2DLod(s_landLight, vec2((luminosity + 0.5f) / 256.0f, 0.5f), 0.0f).rgb * 255.0f + 0.5f);
}

// The land's light at a point, 0 to 255: the colours of the four cells around it blended along z and then along x, in
// steps of 1/256 and whole numbers. Off the land, the colour of the brightest luminosity.
vec3 LandLightAt(vec2 xz)
{
	vec2 texels = (u_islandExtent.zw - u_islandExtent.xy) / 10.0f + 1.0f;
	vec2 position = (xz - u_islandExtent.xy) / 10.0f;
	vec2 cell = floor(position);
	bool onLand = all(greaterThanEqual(cell, vec2_splat(0.0f))) && all(lessThan(cell + 1.0f, texels));
	vec2 w = floor((position - cell) * 256.0f);
	vec3 c00 = LandLightColour(floor(texture2DLod(s_landLuminosity, (cell + 0.5f) / texels, 0.0f).r * 255.0f + 0.5f));
	vec3 c01 = LandLightColour(floor(texture2DLod(s_landLuminosity, (cell + vec2(0.5f, 1.5f)) / texels, 0.0f).r * 255.0f + 0.5f));
	vec3 c10 = LandLightColour(floor(texture2DLod(s_landLuminosity, (cell + vec2(1.5f, 0.5f)) / texels, 0.0f).r * 255.0f + 0.5f));
	vec3 c11 = LandLightColour(floor(texture2DLod(s_landLuminosity, (cell + 1.5f) / texels, 0.0f).r * 255.0f + 0.5f));
	vec3 z0 = c00 + floor((c01 - c00) * w.y / 256.0f);
	vec3 z1 = c10 + floor((c11 - c10) * w.y / 256.0f);
	return onLand ? z0 + floor((z1 - z0) * w.x / 256.0f) : LandLightColour(255.0f);
}

// The land's light of the cell a point is in, unblended, as trees take it
vec3 LandLightCellAt(vec2 xz)
{
	vec2 texels = (u_islandExtent.zw - u_islandExtent.xy) / 10.0f + 1.0f;
	vec2 cell = floor((xz - u_islandExtent.xy) / 10.0f);
	bool onLand = all(greaterThanEqual(cell, vec2_splat(0.0f))) && all(lessThan(cell, texels));
	float luminosity = floor(texture2DLod(s_landLuminosity, (cell + 0.5f) / texels, 0.0f).r * 255.0f + 0.5f);
	return LandLightColour(onLand ? luminosity : 255.0f);
}

#endif // LAND_LIGHT_SH
