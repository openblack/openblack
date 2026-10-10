struct Vertex
{
    float3 position;
    float2 uv;
    float3 normal;
    uint boneIndices;
}

struct VOut
{
    float4 position : SV_Position;
    float4 worldPosition;
    float2 uv;
    float3 normal;
}

struct TreeCamera
{
    float4x4 viewProjection;
    float4 islandExtent;
    uint4 offsets; // model matrix offset, sway parameter offset
}

[[vk::binding(0, 0)]] Sampler2D heightMap;
[[vk::binding(1, 0)]] StructuredBuffer<float4x4> matrices;
[[vk::binding(2, 0)]] StructuredBuffer<float4> swayParameters;
[[vk::binding(0, 1)]] ConstantBuffer<TreeCamera> camera;
[[vk::binding(0, 2)]] Sampler2D diffuseTexture;
[[vk::binding(0, 3)]] ConstantBuffer<float4> skyAlphaThreshold;

[shader("vertex")]
VOut vertexMain(Vertex input, uint instanceID : SV_InstanceID)
{
    float4x4 model = matrices[camera.offsets.x + instanceID];
    float4 sway = swayParameters[camera.offsets.y + instanceID];
    float directionLength = length(sway.xy);
    float2 windDirection = directionLength > 0 ? sway.xy / directionLength : float2(0, 0);
    float4 worldPosition = mul(model, float4(input.position, 1));
    worldPosition.xz += windDirection * sin(sway.z * 0.08 * sway.w) * input.position.y;

    // Anchor every vertex to the terrain height at the tree's base.
    float3 base = mul(model, float4(0, 0, 0, 1)).xyz;
    float2 uv = (base.xz - camera.islandExtent.xy) / (camera.islandExtent.zw - camera.islandExtent.xy);
    worldPosition.y += heightMap.SampleLevel(uv, 0).r * 170.85 - base.y;

    VOut output;
    output.position = mul(camera.viewProjection, worldPosition);
    output.worldPosition = worldPosition;
    output.uv = input.uv;
    output.normal = input.normal;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    float brightness = skyAlphaThreshold.x / 2.0;
    float ambient = brightness * 0.25 + 0.75;
    float3 lightDirection = normalize(float4(-4000, 1300, -1435, 1) - input.worldPosition).xyz;
    float lighting = ambient + brightness * max(dot(input.normal, lightDirection), 0.0) * 0.25;
    float4 color = diffuseTexture.Sample(input.uv);
    if (color.a <= skyAlphaThreshold.y)
        discard;
    return float4(color.rgb * lighting, color.a);
}
