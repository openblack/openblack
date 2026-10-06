$input v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>

// The sea's colour and alpha textures, and what lies under the sea: the mirrored sky and land, drawn with the same
// projection as the view
SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_alpha, 1);
SAMPLER2D(s_reflection, 2);

// rgb: the sea's colour, the land's light at full luminosity
uniform vec4 u_seaColour;
// x: how long the texture repeats, y: the ripple's step, 0 to 15, zw: the ripple's direction, along the camera's view
uniform vec4 u_seaParams;
// The screen rows (see sea_rows::Rows): x the first row's screen y, y how many, z one over the first row's view depth,
// w its step from row to row
uniform vec4 u_seaRows;
// x: 1 for the still sea of the lowest detail level, y: 1 when the first row is nearly clear
uniform vec4 u_seaMode;
// xyz: the camera's position
uniform vec4 u_seaCamera;

// The point of the sea under the centre of screen pixel (px, py), py counted from the top; w is 0 above the horizon
vec4 PlanePoint(float px, float py)
{
	vec2 ndc = vec2((px + 0.5f) / u_viewRect.z * 2.0f - 1.0f, 1.0f - (py + 0.5f) / u_viewRect.w * 2.0f);
	vec4 p = mul(u_invViewProj, vec4(ndc, 0.5f, 1.0f));
	vec3 direction = p.xyz / p.w - u_seaCamera.xyz;
	if (direction.y >= 0.0f)
	{
		return vec4_splat(0.0f);
	}
	return vec4(u_seaCamera.xyz + direction * (-u_seaCamera.y / direction.y), 1.0f);
}

float ViewDepth(vec3 p)
{
	return mul(u_view, vec4(p, 1.0f)).z;
}

// The texture coordinates of row r at pixel column px: where the row meets the sea, moved along the view by its ripple.
// Each row ripples by its own step of a 16 step sine, fully beyond a depth of 70 and fading out by a depth of 30.
vec2 RowUv(float r, float px)
{
	vec3 p = PlanePoint(px, u_seaRows.x + 2.0f * r).xyz;
	float inverseDepth = u_seaRows.z + r * u_seaRows.w;
	float step = mod(u_seaParams.y + 2.0f * (r + 1.0f), 16.0f);
	float s = sin(step * 3.14159265f / 8.0f);
	float amplitude = inverseDepth < 1.0f / 70.0f ? s : (inverseDepth < 1.0f / 30.0f ? (1.0f / inverseDepth - 30.0f) * 0.025f * s : 0.0f);
	return (p.xz + u_seaParams.zw * amplitude) / u_seaParams.x;
}

// The alpha of row r: whole up to a view depth of 7000, falling to 80 of 255 at 14000. A first row that starts inside
// the screen is nearly clear.
float RowAlpha(float r)
{
	if (r < 0.5f && u_seaMode.y > 0.5f)
	{
		return 32.0f / 255.0f;
	}
	float depth = ViewDepth(PlanePoint(0.0f, u_seaRows.x + 2.0f * r).xyz);
	float alpha = depth > 14000.0f ? 80.0f : (depth < 7000.0f ? 255.0f : floor(255.0f - (depth - 7000.0f) * (175.0f / 7000.0f) + 0.5f));
	return alpha / 255.0f;
}

void main()
{
	vec2 reflectionUv = (gl_FragCoord.xy - u_viewRect.xy) / u_viewRect.zw;

	vec2 uv;
	float rowAlpha;
	if (u_seaMode.x > 0.5f)
	{
		// The still sea: the texture repeats 50 times across the 140000 unit square, wholly opaque
		uv = vec2(v_texcoord0.x + 70000.0f, 70000.0f - v_texcoord0.y) / u_seaParams.x;
		rowAlpha = 1.0f;
	}
	else
	{
		float px = floor(gl_FragCoord.x - u_viewRect.x);
#if BGFX_SHADER_LANGUAGE_GLSL
		float py = floor(u_viewRect.w - (gl_FragCoord.y - u_viewRect.y));
#else
		float py = floor(gl_FragCoord.y - u_viewRect.y);
#endif
		// Outside the rows there is no sea, only what lies under it
		float k = py - u_seaRows.x;
		if (k < 0.0f || k >= 2.0f * u_seaRows.y)
		{
			gl_FragColor = vec4(texture2D(s_reflection, reflectionUv).rgb, 1.0f);
			return;
		}
		// The rows are drawn without perspective: the texture and alpha go straight from one row to the next. Every
		// other row repeats the texture of the row before, so each line of the texture holds for two pixels, then
		// blends into the next over two more.
		float r = floor(k / 2.0f);
		float halfway = k - 2.0f * r;
		if (mod(r, 2.0f) < 0.5f)
		{
			uv = RowUv(r, px);
		}
		else
		{
			uv = halfway < 0.5f ? RowUv(r - 1.0f, px) : 0.5f * (RowUv(r - 1.0f, px) + RowUv(r + 1.0f, px));
		}
		rowAlpha = halfway < 0.5f ? RowAlpha(r) : 0.5f * (RowAlpha(r) + RowAlpha(r + 1.0f));
	}

	// The sea's texture in the sea's colour, blended by its alpha over what lies under the sea
	vec3 sea = u_seaColour.rgb * texture2D(s_diffuse, uv).rgb;
	vec3 under = texture2D(s_reflection, reflectionUv).rgb;
	float alpha = texture2D(s_alpha, uv).r * rowAlpha;
	gl_FragColor = vec4(mix(under, sea, alpha), 1.0f);
}
