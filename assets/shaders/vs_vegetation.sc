$input a_position, a_texcoord0, a_normal, a_indices, i_data0, i_data1, i_data2, i_data3
$output v_position, v_texcoord0, v_normal, v_color0, v_haze

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

#include "haze.sh"
#include "land_light.sh"

void main()
{
#if BGFX_SHADER_LANGUAGE_HLSL > 300 || BGFX_SHADER_LANGUAGE_PSSL || BGFX_SHADER_LANGUAGE_SPIRV
    uint modelIndex = uint(max(0, asint(a_indices.x)));
#else
    uint modelIndex = uint(max(0, a_indices.x));
#endif

    mat4 model;
    model[0] = i_data0;
    model[1] = i_data1;
    model[2] = i_data2;
    model[3] = i_data3;

    // The tree's sway, or its bend away from the hand, is in its matrix (VegetationSystem)
    vec4 worldPosition = instMul(model, vec4(a_position.xyz, 1.0));

    v_position = worldPosition;

#ifdef USE_HEIGHT_MAP
    // Move the whole tree onto the height map's land under its base. Trees are placed on the land already, so this is
    // only for land that has changed since. The height map has a texel for each corner of the land's cells, 10 units
    // apart: sample at the centre of the texel of the base's position.
    vec2 extentMin = u_islandExtent.xy;
    vec2 extentMax = u_islandExtent.zw;
    vec3 treeBasePos = vec3(model[3][0], model[3][1], model[3][2]);
    vec2 texels = (extentMax - extentMin) / 10.0 + 1.0;
    vec2 baseUv = ((treeBasePos.xz - extentMin) / 10.0 + 0.5) / texels;
    float terrain_height = texture2DLod(s_heightmap, baseUv, 0.0).r * 170.85;
    v_position.y += terrain_height - treeBasePos.y;
#endif // USE_HEIGHT_MAP

    // The game makes its trees unlit: no light shades them, they take the land's light of the cell they stand in, scaled
    // by the trees' brightness of the frame
    vec3 origin = instMul(model, vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 colour = u_landLight.x > 0.0 ? min(floor(LandLightCellAt(origin.xz) * u_landLight.y), vec3_splat(255.0)) : vec3_splat(255.0);
    float hazeT = HazeT(mul(u_view, vec4(origin, 1.0)).z);
    colour = HazeDiffuse(colour, HazeFactor(hazeT));
    // The haze is added with the land's colour of the cell the tree stands in, each channel at most white
    vec3 added = HazeColour(hazeT);
    if (u_landLight.x > 0.0)
    {
        added = min(added + LandColourCellAt(origin.xz), vec3_splat(255.0));
    }
    v_haze = vec4(added / 255.0, 0.0);
    v_color0 = vec4(colour / 255.0, 1.0);

    v_texcoord0 = vec4(a_texcoord0, 0.0, 0.0);
    v_normal = a_normal;
    gl_Position = mul(u_viewProj, v_position);
}
