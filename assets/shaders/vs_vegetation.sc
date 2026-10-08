$input a_position, a_texcoord0, a_normal, a_indices, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_position, v_texcoord0, v_normal, v_color0, v_haze, v_snow, v_snowLight

#if BGFX_SHADER_LANGUAGE_HLSL == 3
#define BGFX_CONFIG_MAX_BONES 48
#else
#define BGFX_CONFIG_MAX_BONES 128
#endif

#include <bgfx_shader.sh>

#ifdef USE_HEIGHT_MAP
SAMPLER2D(s_heightmap, 1);
#endif // USE_HEIGHT_MAP
uniform vec4 u_islandExtent;
#ifdef USE_HEIGHT_MAP
#include "land_altitude.sh"
#endif // USE_HEIGHT_MAP

#include "haze.sh"
#include "land_light.sh"
#include "model_light.sh"
#include "snow.sh"

void main()
{
    uint modelIndex = uint(max(0, a_indices.x));

    mat4 model;
    model[0] = i_data0;
    model[1] = i_data1;
    model[2] = i_data2;
    model[3] = i_data3;

    // The tree's sway, or its bend away from the hand, is in its matrix (VegetationSystem)
    vec4 worldPosition = instMul(model, vec4(a_position.xyz, 1.0));

    v_position = worldPosition;

#ifdef USE_HEIGHT_MAP
    // Move the whole tree onto the land under its base. Trees are placed on the land already, so this is only for land
    // that has changed since.
    vec3 treeBasePos = vec3(model[3][0], model[3][1], model[3][2]);
    v_position.y += LandAltitude(treeBasePos.xz) - treeBasePos.y;
#endif // USE_HEIGHT_MAP

    // The game makes its trees unlit: no light shades them, they take the land's light of the cell they stand in, scaled
    // by the trees' brightness of the frame
    vec3 origin = instMul(model, vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    // A tree with a fire on it takes a grey instead, no brighter than the trees' brightness
    float brightness = i_data4.x > 0.0 ? min(i_data4.x, u_landLight.y) : u_landLight.y;
    vec3 colour = u_landLight.x > 0.0 ? min(floor(LandLightCellAt(origin.xz) * brightness), vec3_splat(255.0)) : vec3_splat(255.0);
    float hazeT = HazeT(mul(u_view, vec4(origin, 1.0)).z);
    colour = HazeDiffuse(colour, HazeFactor(hazeT));
    // The haze is added with the land's colour of the cell the tree stands in, each channel at most white
    vec3 added = HazeColour(hazeT);
    if (u_landLight.x > 0.0)
    {
        added = min(added + LandColourCellAt(origin.xz), vec3_splat(255.0));
    }
    v_haze = vec4(added / 255.0, 0.0);
    // The snow on the tree: where its texture is read, how much of it shows of 255, and its own light, which unlike the
    // tree's is shaded by the model light
    v_snow = vec4_splat(0.0);
    v_snowLight = vec3_splat(0.0);
    if (u_snow.x > 0.5)
    {
        float snowLevel = SnowObjectLevel(origin.xz, 255.0, 255.0);
        if (snowLevel > 0.0)
        {
            vec3 worldNormal = normalize(instMul(model, vec4(a_normal, 0.0)).xyz);
            vec3 localLight = ModelLightLocal(instMul(model, vec4(1.0, 0.0, 0.0, 0.0)).xyz,
                                              instMul(model, vec4(0.0, 1.0, 0.0, 0.0)).xyz,
                                              instMul(model, vec4(0.0, 0.0, 1.0, 0.0)).xyz, origin);
            v_snow = vec4(SnowUv(a_position.xyz, worldNormal), snowLevel / 255.0, 0.0);
            v_snowLight = ModelLightColour(SnowColour(colour), ModelLightFactor(a_normal, localLight));
        }
    }
    // The alpha its foliage is cut away below while it burns, 0 for its own
    v_color0 = vec4(colour / 255.0, i_data4.y);

    v_texcoord0 = vec4(a_texcoord0, 0.0, 0.0);
    v_normal = a_normal;
    gl_Position = mul(u_viewProj, v_position);
}
