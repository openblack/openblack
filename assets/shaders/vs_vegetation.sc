$input a_position, a_texcoord0, a_normal, a_indices, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_position, v_texcoord0, v_normal

#if BGFX_SHADER_LANGUAGE_HLSL == 3
#define BGFX_CONFIG_MAX_BONES 48
#else
#define BGFX_CONFIG_MAX_BONES 128
#endif

#include <bgfx_shader.sh>

SAMPLER2D(s_heightmap, 1);
uniform vec4 u_islandExtent;

void main()
{
    vec2 extentMin = u_islandExtent.xy;
    vec2 extentMax = u_islandExtent.zw;

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

    // Get sway parameters from instance data
    // i_data4 contains: (swayX, swayZ, swayOffset, swayStrength)
    vec2 windDir = normalize(i_data4.xy); // The direction of the sway
    float swayTime = i_data4.z; // Per-instance time
    float swayStrength = i_data4.w; // Per-instance sway strength

    // Get local position
    vec3 position = a_position.xyz;

    // First transform to world space WITHOUT sway
    vec4 worldPosition = mul(u_model[modelIndex], vec4(position, 1.0));
    worldPosition = instMul(model, worldPosition);

    // Store original world position for later
    vec3 originalWorldPos = worldPosition.xyz;

    // Calculate wind sway with time, with proper phase offset
    float sway = swayTime * 0.08;

    // Calculate normalized height for vertex (0 at base, 1 at top)
    // Need to determine height in object space
    float vertexHeight = position.y;

    // Rest of the sway calculation
    float angle = sway * swayStrength;

    // Calculate displacement along the wind direction
    // sin(angle) * height gives the horizontal displacement
    float displacement = sin(angle) * vertexHeight;
    // Apply displacement in world space to ensure sway direction is correct
    worldPosition.x += windDir.x * displacement;
    worldPosition.z += windDir.y * displacement;

    // Update v_position with the swayed position
    v_position = worldPosition;

    // Get tree base position from the model matrix (translation component)
    vec3 treeBasePos = vec3(model[3][0], model[3][1], model[3][2]);
    // Sample terrain height at the tree base only
    vec2 baseBlockUv = (treeBasePos.xz - extentMin) / (extentMax - extentMin);
    float terrain_height = texture2DLod(s_heightmap, baseBlockUv, 0.0).r * 170.85;
    // Get intended base height from the model matrix
    float original_height = model[3][1]; // y-component of translation
    // Apply height adjustment uniformly to all vertices
    float height_adjustment = terrain_height - original_height;
    v_position.y += height_adjustment;

    v_texcoord0 = vec4(a_texcoord0, 0.0, 0.0);
    v_normal = a_normal;
    gl_Position = mul(u_viewProj, v_position);
}