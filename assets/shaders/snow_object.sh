#ifndef SNOW_OBJECT_SH
#define SNOW_OBJECT_SH

// The snow objects show, and its alpha that says where it shows first
SAMPLER2D(s_snow, 12);
SAMPLER2D(s_snowAlpha, 13);

// Whether a texel of an object shows snow at a level: where the snow texture's alpha reaches what the level asks for
bool SnowShows(float level, float snowAlpha)
{
	float threshold = max(250.0f - min(level, 255.0f), 0.0f);
	return level > 0.0f && floor(snowAlpha * 255.0f + 0.5f) >= threshold;
}

#endif // SNOW_OBJECT_SH
