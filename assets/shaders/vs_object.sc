#if defined(USE_INSTANCING) && defined(USE_MORPH)
$input a_position, a_texcoord0, a_normal, a_indices, a_tangent, a_bitangent, a_color1, a_color2, a_color3, a_weight, i_data0, i_data1, i_data2, i_data3, i_data4
#elif defined(USE_INSTANCING) && defined(USE_LIGHTMAP)
$input a_position, a_texcoord0, a_normal, a_indices, a_texcoord3, i_data0, i_data1, i_data2, i_data3, i_data4
#elif defined(USE_INSTANCING)
$input a_position, a_texcoord0, a_normal, a_indices, i_data0, i_data1, i_data2, i_data3, i_data4
#elif defined(USE_LIGHTMAP)
$input a_position, a_texcoord0, a_normal, a_indices, a_texcoord3
#else
$input a_position, a_texcoord0, a_normal, a_indices
#endif
$output v_position, v_texcoord0, v_normal, v_color0, v_haze, v_snow, v_snowLight

// Every bone's matrix is uploaded with each draw: meshes without bones declare one
#ifndef BGFX_CONFIG_MAX_BONES
#if BGFX_SHADER_LANGUAGE_HLSL == 3
#define BGFX_CONFIG_MAX_BONES 48
#else
#define BGFX_CONFIG_MAX_BONES 128
#endif
#endif // BGFX_CONFIG_MAX_BONES

#include <bgfx_shader.sh>

#include "haze.sh"
#include "model_light.sh"
#include "window_light.sh"
#include "snow.sh"

// Pushes the mesh back by a fraction of its depth, towards the far plane at 0: the temple's rooms other than the one the
// player is in, which overlap it at the doorways
uniform vec4 u_depthBias;
// Slides the texture across the mesh, as the game does the creature's waterfall
uniform vec4 u_uvOffset;
#ifdef USE_MORPH
// How far a creature's body is pulled towards its evil or good, thin or fat and weak or strong mesh, whose vertices
// come in the second to fourth streams
uniform vec4 u_morphWeights;
#endif // USE_MORPH

#ifdef USE_HEIGHT_MAP
SAMPLER2D(s_heightmap, 1);
#endif // USE_HEIGHT_MAP
uniform vec4 u_islandExtent;
#ifdef USE_HEIGHT_MAP
#include "land_altitude.sh"
#endif // USE_HEIGHT_MAP

#ifndef USE_LIGHTMAP
#include "land_light.sh"
#endif // USE_LIGHTMAP

void main()
{
	// Unpack
#ifdef USE_HEIGHT_MAP
	vec2 extentMin = u_islandExtent.xy;
	vec2 extentMax = u_islandExtent.zw;
#endif // USE_HEIGHT_MAP

#if BGFX_SHADER_LANGUAGE_HLSL > 300 || BGFX_SHADER_LANGUAGE_PSSL || BGFX_SHADER_LANGUAGE_SPIRV
	uint modelIndex = uint(max(0, asint(a_indices.x)));
#else
	uint modelIndex = uint(max(0, a_indices.x));
#endif

#ifdef USE_INSTANCING
	mat4 model;
	model[0] = i_data0;
	model[1] = i_data1;
	model[2] = i_data2;
	model[3] = i_data3;
#define TO_WORLD(p) instMul(model, mul(u_model[modelIndex], p))
#else
#define TO_WORLD(p) mul(u_model[modelIndex], p)
#endif // USE_INSTANCING

	vec3 position = a_position.xyz;
	vec3 normal = a_normal;
#ifdef USE_MORPH
	// The body is the base moved towards each mesh, and its normal moved alike and made unit length again
	position += u_morphWeights.x * (a_tangent - a_position.xyz) + u_morphWeights.y * (a_color1 - a_position.xyz) +
	            u_morphWeights.z * (a_color3 - a_position.xyz);
	normal += u_morphWeights.x * (a_bitangent - a_normal) + u_morphWeights.y * (a_color2 - a_normal) +
	          u_morphWeights.z * (a_weight.xyz - a_normal);
	normal = normalize(normal);
#endif // USE_MORPH

	v_position = TO_WORLD(vec4(position, 1.0f));

	// The game's light, from the origin of the mesh's bone, meets the vertex's normal in the bone's own space
	vec3 origin = TO_WORLD(vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
	vec3 localLight = ModelLightLocal(TO_WORLD(vec4(1.0f, 0.0f, 0.0f, 0.0f)).xyz, TO_WORLD(vec4(0.0f, 1.0f, 0.0f, 0.0f)).xyz,
	                                  TO_WORLD(vec4(0.0f, 0.0f, 1.0f, 0.0f)).xyz, origin);
	// The object takes the colour of the land's light where it stands, then the light shades it
	vec3 colour = vec3_splat(255.0f);
#ifndef USE_LIGHTMAP
	if (u_landLight.x > 0.0f)
	{
		colour = min(floor(LandLightAt(origin.xz) * u_landLight.y), vec3_splat(255.0f));
	}
#endif // USE_LIGHTMAP
#ifdef USE_INSTANCING
	// An instance's own colour, 0xRRGGBB, multiplies the light in whole steps
	if (i_data4.y > 0.0f)
	{
		vec3 tint = vec3(floor(i_data4.y / 65536.0f), mod(floor(i_data4.y / 256.0f), 256.0f), mod(i_data4.y, 256.0f));
		colour = floor(colour * tint / 256.0f);
	}
#endif // USE_INSTANCING
	// The distance haze, once for the object at its origin: its colour fades and the haze's is added after the texture
	float hazeT = HazeT(mul(u_view, vec4(origin, 1.0f)).z);
	colour = HazeDiffuse(colour, HazeFactor(hazeT));
	// The haze is added with the land's colour where the object stands, each channel at most white
	vec3 added = HazeColour(hazeT);
#ifndef USE_LIGHTMAP
	if (u_landLight.x > 0.0f)
	{
		added = min(added + LandColourAt(origin.xz), vec3_splat(255.0f));
	}
#endif // USE_LIGHTMAP
	// w: how much snow shows on it, of 255. An instance can have its own rate and cap of 256 for it, as a field's crop has.
	v_haze = vec4(added / 255.0f, 0.0f);
	// The snow on it: where its texture is read, how much of it shows of 255, and its own light, by the object's colour
	v_snow = vec4_splat(0.0f);
	v_snowLight = vec3_splat(0.0f);
#ifdef USE_INSTANCING
	if (u_snow.x > 0.5f)
	{
		float snowCap = i_data4.w > 0.0f ? i_data4.w : 255.0f;
		float snowLevel = SnowObjectLevel(origin.xz, snowCap, snowCap);
		if (snowLevel > 0.0f)
		{
			vec3 worldNormal = normalize(TO_WORLD(vec4(normal, 0.0f)).xyz);
			v_snow = vec4(SnowUv(position, worldNormal), snowLevel / 255.0f, 0.0f);
			v_snowLight = ModelLightColour(SnowColour(colour), ModelLightFactor(normal, localLight));
		}
	}
#endif // USE_INSTANCING
	v_color0 = vec4(ModelLightColour(colour, ModelLightFactor(normal, localLight)), 1.0f);
#ifndef USE_LIGHTMAP
	if (u_landLight.z > 0.0f)
	{
		// Unlit meshes keep the land's colour, unshaded by the sun
		v_color0 = vec4(colour / 255.0f, 1.0f);
	}
#endif // USE_LIGHTMAP
	// A house's window is drawn in the grey of its light, unlit, at night while someone is home, and not at all otherwise
	bool hidden = false;
#ifdef USE_INSTANCING
	// Some instances aren't drawn at all, such as a field's crop that is yet too small to show
	hidden = i_data4.z > 0.5f;
	if (u_window.x > 0.5f)
	{
		float grey = (i_data4.x > 0.5f && u_window.z > 0.5f) ? WindowGrey(origin) : -1.0f;
		hidden = hidden || grey < 0.0f;
		v_color0 = vec4(vec3_splat(max(grey, 0.0f) / 255.0f), 1.0f);
	}
#endif // USE_INSTANCING

#ifdef USE_HEIGHT_MAP
	// The model goes with the land: each vertex moves along the model's up by how much higher the land is under it
	// than under the model's origin, and the model keeps its own height over the land
#ifdef USE_INSTANCING
	vec3 modelOrigin = i_data3.xyz;
	vec3 modelUp = i_data1.xyz;
	float modelScale = length(i_data0.xyz);
#else
	vec3 modelOrigin = u_model[modelIndex][3].xyz;
	vec3 modelUp = u_model[modelIndex][1].xyz;
	float modelScale = length(u_model[modelIndex][0].xyz);
#endif // USE_INSTANCING
	float rise = LandAltitude(v_position.xz) - LandAltitude(modelOrigin.xz);
	if (modelUp.x == 0.0f && modelUp.z == 0.0f)
	{
		v_position.y += rise;
	}
	else
	{
		v_position.xyz += modelUp * (rise / modelScale);
	}
#endif // USE_HEIGHT_MAP

#ifdef USE_LIGHTMAP
	v_texcoord0 = vec4(a_texcoord0, a_texcoord3);
#elif defined(USE_ENVIRONMENT)
	// The environment-mapped mode: the environment map's coordinates are where the normal points across and up
	// the camera's view, from 0 to 0.498
	vec3 viewNormal = normalize(mul(u_view, mul(u_model[modelIndex], vec4(normal, 0.0f))).xyz);
	v_texcoord0 = vec4(a_texcoord0, (viewNormal.xy + 1.0f) * 0.498046875f);
#else
	v_texcoord0 = vec4(a_texcoord0, 0.0f, 0.0f);
#endif // USE_LIGHTMAP
	v_texcoord0.xy += u_uvOffset.xy;
	v_normal = normal;
	gl_Position = mul(u_viewProj, v_position);
	gl_Position.z *= 1.0f - u_depthBias.x;
	if (hidden)
	{
		// Beyond the far plane, so nothing of it is drawn
		gl_Position = vec4(0.0f, 0.0f, 2.0f, 1.0f);
	}
}
